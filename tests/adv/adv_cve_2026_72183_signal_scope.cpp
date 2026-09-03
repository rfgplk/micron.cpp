//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2026-72183  --  Linux Landlock, CVSS 8.4, CWE-693 (protection mechanism failure)
//
// "A LANDLOCK_SCOPE_SIGNAL bypass involving asynchronous SIGIO delivery could cross a Landlock
//  domain and signal processes outside the sandbox."
//     fixed: 6.12.101, 6.18.40, 7.1.5, 7.2
//
// THE SHAPE
//
// Landlock ABI 6 added LANDLOCK_SCOPE_SIGNAL: a sandboxed process may not signal a process outside
// its domain. The check sits on the synchronous signal paths -- kill, tgkill, pidfd_send_signal.
//
// SIGIO is not a synchronous signal path. A process arms it once:
//
//     fcntl(fd, F_SETOWN, target_pid);
//     fcntl(fd, F_SETSIG, SIGUSR1);
//     fcntl(fd, F_SETFL, O_ASYNC);
//
// and thereafter the KERNEL delivers the signal on the descriptor's behalf, from softirq context,
// with no scope check on the path. The sandbox never calls kill(2) at all; it asks the kernel to do
// it later. Same effect, different door.
//
// The lesson generalises past this one bug: LANDLOCK SCOPE IS NOT A PROCESS BOUNDARY. It is a check
// on some paths to one. A PID namespace is a boundary -- a process that cannot NAME a pid cannot
// signal it, asynchronously or otherwise, because there is nothing to put in F_SETOWN.
//
// MICRON'S ANALOGUE
//
// (a) sandbox cannot express scope at all. The plumbing exists everywhere else:
//     landlock_scope_signal is defined (linux/sys/landlock.hpp:50), ll::scope::signal is an enum
//     value (sec/landlock.hpp:61), try_ruleset takes a third parameter, and fp.hpp:301-305
//     (`confine()`) exposes all three axes. But sandbox.hpp:385 is
//
//         landlock::ruleset rs = landlock::try_ruleset(__ll_handled);
//
//     -- one argument. There is no sandbox::landlock_scope(), no __ll_scope member, and the second
//     and third parameters default to none. So the main entry point of the whole layer cannot ask
//     for the defence this CVE is about.
//
// (b) The ABI narrowing is silent. try_ruleset (landlock.hpp:306-308) intersects the request with
//     what the running kernel supports and errors only if NOTHING survives -- so on a pre-6.12
//     kernel, asking for scope::signal returns a valid ruleset with no scoping and no diagnostic.
//
// (c) groups::io grants SYS_fcntl unfiltered (groups.hpp:119), which is F_SETOWN and F_SETSIG --
//     the exact primitive. Not wrong on its own; worth knowing it is there.
//
// WHAT THIS PINS
//   1  sandbox can express landlock scope
//   2  a scoped ruleset actually stops kill() across the domain     (the synchronous path)
//   3  a PID namespace stops it regardless of landlock              (the boundary that always holds)
//   4  ... and it is a real boundary: the sandbox cannot even see the outside pid
//   5  an ABI that cannot express the requested scope is reported, not silently dropped
//   6  environment: the SIGIO path itself is closed on this kernel
//
// POLARITY: mixed and stated per contract. 1 and 5 FAIL on the tree as it stands. 2, 3, 4 pass and
// are the guards -- 3 and 4 in particular are the reason a fix for 1 is defence in depth rather than
// the only thing holding. 6 is a check on the KERNEL, not on micron: this box runs 7.1.10, past the
// 7.1.5 fix, so it is expected to pass and is reported as an environment result.
//
// NEGATIVE CONTROL: contract 2 requires the unscoped ruleset to ALLOW the signal that the scoped one
// denies, in the same run. Without that half, "the signal failed" could mean the target was gone.
//
// CONTROL (ungated): a scoped sandbox must still be able to signal ITSELF and its own children.
// Scope is about crossing the domain, not about signals; a sandbox that cannot raise() cannot run a
// signal handler, and killing that would break far more than it fixed.
//
// Build:
//   duck test tests/adv/adv_cve_2026_72183_signal_scope.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/linux/io/sys.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"
#include "../../src/sec/landlock.hpp"
#include "../../src/sec/sandbox.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace s = micron::sec;
namespace ll = micron::sec::landlock;
namespace ns = micron::sec::ns;

namespace
{

constexpr i32 signalled_out = 131;
constexpr i32 saw_outside_pid = 132;
constexpr i32 scope_not_applied = 133;
constexpr i32 sigio_crossed = 134;

// the ABI that first carries LANDLOCK_SCOPE_SIGNAL
constexpr i32 abi_scope = 6;

// a pid we can legally probe with signal 0 and that is guaranteed to exist and to be outside any
// sandbox we build: our own parent, the test binary itself.
volatile i32 g_outside_pid = 0;

// set by the handler below when a cross-domain SIGIO actually ARRIVES. `volatile sig_atomic_t` in
// spirit -- the only thing the handler does is store, which is all a handler may safely do.
volatile i32 g_sigio_seen = 0;

void
__note_sigio(int)
{
  g_sigio_seen = 1;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2026-72183 (landlock signal scope is not a process boundary) ===");

  g_outside_pid = static_cast<i32>(mc::posix::getpid());
  sb::require_true(mc::posix::geteuid() != 0);

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  sandbox must be able to ask for it

  {
    sb::test_case("sandbox must be able to express landlock scope");
#if defined(__micron_sec_sandbox_has_landlock_scope)
    s::sandbox box;
    box.namespaces(ns::ns_kind::user);
    box.landlock_scope(ll::scope::signal | ll::scope::abstract_unix_socket);
    sb::require_true(box.configured());
    sb::require_true(any(box.landlock_scoped() & ll::scope::signal));
#else
    sb::print("  sandbox has no way to set landlock scope: sandbox.hpp:385 calls try_ruleset with one "
              "argument, so access_net and scope default to none. The plumbing exists everywhere else "
              "(fp.hpp:301 exposes all three axes) -- the main entry point simply cannot reach it");
    sb::require_true(false);
#endif
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2  the synchronous path, with its negative control

  {
    sb::test_case("a scoped ruleset stops kill() across the domain");
    if ( !adv::have_landlock(abi_scope) ) {
      sb::skip("landlock abi < 6 on this kernel: LANDLOCK_SCOPE_SIGNAL does not exist here");
    } else {
      // NEGATIVE CONTROL: without the scope, the signal must land. If it does not, the target is
      // wrong and the scoped case below would pass for the wrong reason.
      const adv::child_result unscoped = adv::run_child([]() -> i32 {
        ll::ruleset rs = ll::try_ruleset(ll::read_only, ll::access_net::none, ll::scope::none);
        if ( !rs.valid() ) return adv::setup_failed;
        if ( rs.restrict_self() < 0 ) return adv::setup_failed;
        // signal 0: existence check, no delivery. Enough to see the scope decision.
        return mc::posix::kill(g_outside_pid, 0) >= 0 ? signalled_out : adv::ok_code;
      });
      sb::print("  unscoped ruleset -> kill(parent, 0) ", unscoped.code == signalled_out ? "SUCCEEDS, as it must" : "failed");
      sb::require(unscoped.code, signalled_out);

      const adv::child_result scoped = adv::run_child([]() -> i32 {
        ll::ruleset rs = ll::try_ruleset(ll::read_only, ll::access_net::none, ll::scope::signal);
        if ( !rs.valid() ) return adv::setup_failed;
        if ( !any(rs.scoped() & ll::scope::signal) ) return scope_not_applied;
        if ( rs.restrict_self() < 0 ) return adv::setup_failed;
        return mc::posix::kill(g_outside_pid, 0) >= 0 ? signalled_out : adv::ok_code;
      });
      if ( scoped.code == scope_not_applied ) sb::print("  the ruleset reports no signal scope despite abi >= 6");
      if ( scoped.code == signalled_out ) sb::print("  kill() crossed a LANDLOCK_SCOPE_SIGNAL domain");
      sb::require_distinct(scoped.code, scope_not_applied);
      sb::require_true(scoped.ok());
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 3 + 4  the boundary that does not depend on an LSM getting every path right
  //
  // This is the substantive claim of the file. A PID namespace does not CHECK whether a signal may
  // cross; it removes the ability to name the target. There is no asynchronous path around that,
  // because F_SETOWN needs a pid and there is no pid to give it.

  if ( !adv::have_userns() ) {
    sb::test_case("pid namespace cases");
    sb::skip("this kernel refuses an unprivileged user namespace; a pid namespace cannot be created here");
    sb::print("=== ADV CVE-2026-72183 PASSED (pidns half skipped) ===");
    return 1;
  }

  {
    sb::test_case("a pid namespace stops the signal regardless of landlock");
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::pid | ns::ns_kind::mount);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // inside a fresh pid ns we are pid 1 and the outside pid does not exist as a name
      if ( mc::posix::getpid() != 1 ) return adv::setup_failed;
      if ( mc::posix::kill(g_outside_pid, 0) >= 0 ) return signalled_out;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == signalled_out ) sb::print("  the outside pid was signallable from inside a pid namespace");
    sb::require(code, adv::ok_code);
  }

  {
    sb::test_case("... and the outside pid is not even nameable");
    // the difference between a check and a boundary: the async path needs a pid to arm F_SETOWN
    // with, and there is not one.
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::pid | ns::ns_kind::mount);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      // arm SIGIO at the outside pid -- the CVE's exact primitive
      i32 fds[2] = { -1, -1 };
      if ( mc::posix::pipe2(fds, 0) < 0 ) return adv::setup_failed;
      const i32 owned = mc::posix::fcntl(fds[0], mc::posix::f_setown, g_outside_pid);
      (void)mc::posix::close(fds[0]);
      (void)mc::posix::close(fds[1]);
      // F_SETOWN on a pid that does not exist in this namespace must fail. If it succeeded, the
      // kernel resolved a pid we should not have been able to name.
      if ( owned >= 0 ) return saw_outside_pid;
      return adv::ok_code;
    });
    sb::require_true(r.is_first());
    const i32 code = r.cast<s::sandbox::exit_status>().code();
    if ( code == saw_outside_pid ) sb::print("  F_SETOWN resolved a pid from outside the pid namespace");
    sb::require(code, adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // CONTROL -- ungated

  {
    sb::test_case("control: a confined process can still signal itself and its own children");
    // scope is about crossing the domain. A sandbox that cannot raise() cannot run a signal handler,
    // and cannot reap a child it started -- so "no signals at all" is not a fix.
    s::sandbox box;
    box.namespaces(ns::ns_kind::user | ns::ns_kind::pid | ns::ns_kind::mount);
    sb::require_true(box.configured());

    const auto r = box.run_to_completion([]() -> i32 {
      if ( mc::posix::kill(mc::posix::getpid(), 0) < 0 ) return adv::bad_code;
      const int kid = mc::try_fork();
      if ( kid < 0 ) return adv::setup_failed;
      if ( kid == 0 ) mc::sys_group_exit(0);
      const i32 ok = mc::posix::kill(kid, 0);
      int st = 0;
      (void)mc::waitpid(kid, &st, 0);
      return ok >= 0 ? adv::ok_code : adv::bad_code;
    });
    sb::require_true(r.is_first());
    sb::require(r.cast<s::sandbox::exit_status>().code(), adv::ok_code);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  silent ABI narrowing, in the specific shape this CVE cares about

  {
    sb::test_case("a scope the running ABI cannot express must be reported");
    ll::ruleset rs = ll::try_ruleset(ll::read_only, ll::access_net::none, ll::scope::signal);
    if ( !rs.valid() ) {
      sb::skip("this kernel refuses the ruleset outright; there is no silent-narrowing case to observe");
    } else {
      const bool asked = true;
      const bool got = any(rs.scoped() & ll::scope::signal);
      if ( asked && !got )
        sb::print("  scope::signal was requested and silently dropped: the ruleset is VALID and does "
                  "not scope signals, and the only way to notice is to re-read rs.scoped() "
                  "(landlock.hpp:306-308)");
      sb::require_true(got);
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 6  the kernel's own half
  //
  // About THIS BOX, not about micron: 7.1.5 carries the fix and anything running this suite is past
  // it. Reported as an environment result so a pass here is never mistaken for a micron result.

  {
    sb::test_case("environment: a SIGIO armed inside the domain is not DELIVERED outside it");
    if ( !adv::have_landlock(abi_scope) ) {
      sb::skip("landlock abi < 6; LANDLOCK_SCOPE_SIGNAL does not exist on this kernel");
    } else {
      // ARMING IS NOT THE CROSSING. An earlier draft of this case required F_SETOWN to fail, which
      // was wrong: the kernel records the owner without consulting the domain, and the scope check
      // -- when it happens at all -- happens at DELIVERY, from softirq context. So the only thing
      // worth asserting is whether the signal actually arrives, which means this process has to be
      // listening for it.
      g_sigio_seen = 0;
      mc::posix::sigaction_t sa{};
      sa.sigaction_handler.sa_handler = &__note_sigio;
      sa.sa_flags = mc::posix::sa_restart;      // micron's sigaction() installs the restorer itself
      mc::posix::sigaction_t old{};
      if ( mc::posix::sigaction(mc::posix::sig_usr1, sa, &old) < 0 ) {
        sb::skip("cannot install a SIGUSR1 handler here; delivery cannot be observed");
      } else {
        const adv::child_result r = adv::run_child([]() -> i32 {
          i32 fds[2] = { -1, -1 };
          if ( mc::posix::pipe2(fds, 0) < 0 ) return adv::setup_failed;

          ll::ruleset rs = ll::try_ruleset(ll::read_only, ll::access_net::none, ll::scope::signal);
          if ( !rs.valid() ) return adv::setup_failed;
          if ( !any(rs.scoped() & ll::scope::signal) ) return scope_not_applied;
          if ( rs.restrict_self() < 0 ) return adv::setup_failed;

          // the CVE's door, fully armed: owner outside the domain, a specific signal, async delivery
          (void)mc::posix::fcntl(fds[1], mc::posix::f_setown, g_outside_pid);
          (void)mc::posix::fcntl(fds[1], mc::posix::f_setsig, mc::posix::sig_usr1);
          const i32 fl = mc::posix::fcntl(fds[1], mc::posix::f_getfl);
          (void)mc::posix::fcntl(fds[1], mc::posix::f_setfl, fl | mc::posix::o_async);

          // make the descriptor readable, which is what triggers the kernel-side delivery
          (void)mc::posix::write(fds[1], "x", 1);
          (void)mc::posix::close(fds[0]);
          (void)mc::posix::close(fds[1]);
          return adv::ok_code;
        });

        // give any queued delivery a chance to land before we look
        for ( u64 i = 0; i < 50000000ull && g_sigio_seen == 0; ++i ) __asm__ __volatile__("" ::: "memory");

        (void)mc::posix::sigaction(mc::posix::sig_usr1, old, nullptr);

        sb::require_distinct(r.code, scope_not_applied);
        if ( g_sigio_seen != 0 )
          sb::print("  a SIGIO armed inside a LANDLOCK_SCOPE_SIGNAL domain was DELIVERED to a process "
                    "outside it (CVE-2026-72183; fixed in 6.12.101 / 6.18.40 / 7.1.5 / 7.2)");
        else
          sb::print("  no cross-domain SIGIO delivery observed; this kernel carries the fix");
        sb::require(static_cast<i32>(g_sigio_seen), 0);
      }
    }
  }

  sb::print("=== ADV CVE-2026-72183 PASSED ===");
  return 1;
}
