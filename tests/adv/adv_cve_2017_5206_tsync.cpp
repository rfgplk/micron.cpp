//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// CVE-2017-5206   --  Firejail, CVSS 9.0, CWE-693
// CVE-2019-12589  --  Firejail, CVSS 8.8, CWE-693
//
// 5206:  "Seccomp sandbox bypass affecting Firejail on affected pre-4.8 Linux kernels."
//        affected: Firejail < 0.9.44.4
// 12589: "Seccomp filters could be modified from within a jail, weakening filtering for processes
//        subsequently joining that jail."                            fixed: Firejail 0.9.60
//
// THE SHAPE
//
// Both are the same claim from two directions: THE FILTER THE PROCESS ENDS UP RUNNING UNDER IS NOT
// THE FILTER THE POLICY DESCRIBED.
//
// In 12589 the filter came from a file inside the jail, so the jail could rewrite it before the
// next process picked it up. In 5206 the filter was correct but did not cover everything it was
// meant to. The durable form of the second is the one that outlives any particular kernel version:
//
//     seccomp(2) with SECCOMP_SET_MODE_FILTER applies to the CALLING THREAD ONLY.
//
// A multithreaded process that installs a filter from one thread has confined one thread. Its
// siblings share the address space and the file descriptor table, so an unconfined sibling is not a
// partial bypass, it is a total one -- the confined thread can hand it work. SECCOMP_FILTER_FLAG_TSYNC
// exists for exactly this and makes the install atomic across the thread group.
//
// MICRON'S ANALOGUE
//
// sandbox is fine and for a good reason: it forks, and a forked child has one thread, so
// load_raw(prog, true, 0) at sandbox.hpp:408 is correct as written.
//
// The in-process face is not. src/sec/policy.hpp:180-184:
//
//     static int apply(void) noexcept {
//       auto fb = build(seccomp::act_errno(static_cast<u16>(error::permissions)));
//       return seccomp::load(fb, true);              // <-- extra_flags = 0, no TSYNC
//     }
//
// and identically in seccomp_strict_policy_n::apply (policy.hpp:203-208) and in the functional
// terminal fp.hpp:230-254 (`install()`). These are the entry points a program uses to confine
// ITSELF -- and micron is a threading library. Any program that has started a worker pool, a
// coroutine runtime (tasks/coroutine/cl_sched.hpp spawns workers), or an io_uring reactor before
// calling Policy::apply() has confined the calling thread and nothing else.
//
// load_tsync() exists (seccomp.hpp:712-717) and install_tsync() exists (fp.hpp:245-254). Neither is
// the default, and nothing at the call site says which one you want.
//
// The second-order defect: TSYNC can PARTIALLY fail. If a sibling thread cannot be synchronised the
// kernel returns its tid rather than 0, and without SECCOMP_FILTER_FLAG_TSYNC_ESRCH the caller
// cannot tell that apart from an ordinary error. micron defines the flag
// (linux/sys/seccomp.hpp:34) and never uses it, so a partial sync is reported as a positive return
// value that `r < 0` treats as success.
//
// WHAT THIS PINS
//   1  Policy::apply() confines EVERY thread, not just the caller     -- run live, with a sibling
//   2  ... demonstrated: the non-TSYNC install leaves the sibling running  (negative control)
//   3  the strict-policy face is wired to the same loader, and its default really is a KILL
//   4  a TSYNC install that only partially succeeded is an error, not a positive return
//   5  sandbox's own install stays correct                            (regression guard)
//
// POLARITY: inverted. Contracts 1 and 4 FAIL on the tree as it stands. Contract 2 passes today and
// forever -- it is the demonstration, driven through load() explicitly so it keeps working after
// apply() is fixed. Contracts 3 and 5 pass today.
//
// A NOTE ON CONTRACT 3, because it is where an easy lie would have gone. The obvious version is the
// two-thread experiment again under a kill default, and it CANNOT be staged: micron's thread
// teardown issues a syscall no allowlist here enumerates, so the child dies with SIGSYS (observed:
// sig 31) before the sibling can report, and widening the groups until it survives would be tuning
// the test against this machine's runtime. `sb::skip` was the tempting answer and is the wrong one --
// the contract would read as covered forever while observing nothing. Both faces share ONE loader,
// so contract 1 already proves TSYNC for both; what contract 3 adds is that the strict face reaches
// that loader at all, and that its default is a kill rather than an errno.
//
// NEGATIVE CONTROL: contract 2. The same probe, the same syscall, the same sibling thread, through
// the loader that is documented NOT to synchronise -- and the sibling must survive. If it ever
// stops surviving, contract 1 has stopped observing TSYNC and is passing because of something else.
//
// CONTROL (ungated): a TSYNC install must still let the calling thread run the syscalls the policy
// allows, and must not kill the process outright. "Nothing runs any more" satisfies contract 1
// without being a fix.
//
// Build:
//   duck test tests/adv/adv_cve_2017_5206_tsync.cpp -o bin/adv --timeout 120 -f

#include "../../src/std.hpp"

#include "../../src/control.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/sec/groups.hpp"
#include "../../src/sec/policy.hpp"
#include "../../src/sec/seccomp.hpp"
#include "../../src/thread/thread.hpp"

#include "../snowball/snowball.hpp"
#include "../support/adv_kit.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;
namespace s = micron::sec;

namespace
{

constexpr u16 eperm = static_cast<u16>(mc::error::permissions);

// getpgid for the same reason sec_seccomp_live.cpp:44 picks it: micron's runtime never issues it,
// so nothing but the probe can trip the rule.
constexpr i32 probe_nr = SYS_getpgid;

constexpr i32 sibling_unconfined = 91;
constexpr i32 sibling_confined = 92;
constexpr i32 install_failed = 93;
constexpr i32 partial_sync_hidden = 94;

// A filter that denies exactly the probe and allows the runtime. Deliberately SECCOMP_RET_ERRNO and
// not a kill: the sibling has to survive to report what it saw, and a killed thread reports nothing.
template<usize N>
void
build_probe_filter(sc::filter_builder<N> &fb)
{
  fb.require_native_arch();
  fb.deny_errno(probe_nr, static_cast<u16>(mc::error::no_entry));      // ENOENT: distinctive
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  for ( usize i = 0; i < g::signal::count; ++i ) fb.allow(g::signal::calls[i]);
  for ( usize i = 0; i < g::process::count; ++i ) fb.allow(g::process::calls[i]);
  for ( usize i = 0; i < g::memory::count; ++i ) fb.allow(g::memory::calls[i]);
  for ( usize i = 0; i < g::time::count; ++i ) fb.allow(g::time::calls[i]);
  fb.default_allow();
}

// what the sibling thread reports back
volatile i32 g_sibling_saw = 0;
volatile i32 g_sibling_ready = 0;
volatile i32 g_filter_installed = 0;

void
sibling_body(void)
{
  g_sibling_ready = 1;
  // spin until the main thread has installed the filter. A futex would be cleaner but the filter
  // under test may or may not permit it, and the point is to observe the filter rather than to
  // negotiate with it.
  for ( u64 i = 0; i < 200000000ull && g_filter_installed == 0; ++i ) __asm__ __volatile__("" ::: "memory");
  // ENOENT is the filter speaking. Anything else -- a real pgid, or a different errno -- means the
  // rule is not in force on this thread.
  const long r = mc::syscall(probe_nr, 0);
  g_sibling_saw = (r == -static_cast<long>(mc::error::no_entry)) ? sibling_confined : sibling_unconfined;
}

// Run the whole experiment in a child, because installing a filter and NO_NEW_PRIVS is irreversible
// and would poison every later case in this binary (sec_seccomp_live.cpp:9-15 makes the same call).
// `use_tsync` picks the loader.
template<bool UseTsync>
i32
sibling_experiment(void)
{
  g_sibling_saw = 0;
  g_sibling_ready = 0;
  g_filter_installed = 0;

  mc::thread t(sibling_body);
  for ( u64 i = 0; i < 200000000ull && g_sibling_ready == 0; ++i ) __asm__ __volatile__("" ::: "memory");
  if ( g_sibling_ready == 0 ) return adv::setup_failed;

  sc::filter_builder<512> fb;
  build_probe_filter(fb);
  if ( !fb.valid() ) return adv::setup_failed;

  const int r = UseTsync ? sc::load_tsync(fb, true) : sc::load(fb, true);
  if ( r < 0 ) return install_failed;
  g_filter_installed = 1;

  t.join();

  // the calling thread must always be confined, either way -- if it is not, the experiment says
  // nothing about the sibling
  if ( mc::syscall(probe_nr, 0) != -static_cast<long>(mc::error::no_entry) ) return adv::setup_failed;

  return g_sibling_saw;
}

}      // namespace

int
main(void)
{
  sb::print("=== ADV CVE-2017-5206 / 2019-12589 (a filter that covers one thread of many) ===");

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 2  NEGATIVE CONTROL FIRST -- prove the sibling really does escape a non-TSYNC install

  {
    sb::test_case("negative control: a non-TSYNC install leaves a sibling thread unconfined");
    const adv::child_result r = adv::run_child([]() -> i32 { return sibling_experiment<false>(); });
    if ( r.code == install_failed || r.g == adv::grade::setup ) {
      sb::skip("could not stage the two-thread experiment here (thread spawn or filter install failed)");
    } else {
      sb::print("  load() (no TSYNC)      -> sibling ", r.code == sibling_unconfined ? "UNCONFINED, as it must be" : "confined");
      sb::require(r.code, sibling_unconfined);
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 1  and now the claim

  {
    sb::test_case("load_tsync() confines every thread in the group");
    const adv::child_result r = adv::run_child([]() -> i32 { return sibling_experiment<true>(); });
    if ( r.code == install_failed || r.g == adv::grade::setup ) {
      sb::skip("could not stage the two-thread experiment here");
    } else {
      sb::print("  load_tsync()           -> sibling ", r.code == sibling_confined ? "confined" : "UNCONFINED");
      sb::require(r.code, sibling_confined);
    }
  }

  {
    sb::test_case("Policy::apply() must confine every thread, not just the caller");
    // This is the finding: apply() is what a program calls to confine itself, and a program that
    // has started any worker before calling it gets exactly one thread confined.
    const adv::child_result r = adv::run_child([]() -> i32 {
      g_sibling_saw = 0;
      g_sibling_ready = 0;
      g_filter_installed = 0;

      mc::thread t(sibling_body);
      for ( u64 i = 0; i < 200000000ull && g_sibling_ready == 0; ++i ) __asm__ __volatile__("" ::: "memory");
      if ( g_sibling_ready == 0 ) return adv::setup_failed;

      using self_policy = s::seccomp_policy_n<512, s::errno_call<probe_nr, static_cast<u16>(mc::error::no_entry)>,
                                              s::allow<g::baseline, g::signal, g::process, g::memory, g::time>>;
      if ( self_policy::apply() < 0 ) return install_failed;
      g_filter_installed = 1;
      t.join();

      if ( mc::syscall(probe_nr, 0) != -static_cast<long>(mc::error::no_entry) ) return adv::setup_failed;
      return g_sibling_saw;
    });
    if ( r.code == install_failed || r.g == adv::grade::setup ) {
      sb::skip("could not stage the two-thread experiment here");
    } else {
      if ( r.code == sibling_unconfined )
        sb::print("  Policy::apply() confined only the calling thread "
                  "(policy.hpp:183 calls load(fb, true) with extra_flags = 0)");
      sb::require(r.code, sibling_confined);
    }
  }

  {
    sb::test_case("the strict-policy face installs and confines, on the same loader");
    //
    // NOT the two-thread experiment, and the reason is worth stating rather than skipping over.
    //
    // I tried it and it cannot be staged here: a kill-default policy leaves no room for a syscall
    // nobody enumerated, and micron's thread TEARDOWN issues one -- the child dies with SIGSYS
    // (observed: grade=signalled, sig=31) before the sibling can report, no matter how many groups
    // the allowlist names. Widening the list until it survives would be tuning the test against this
    // machine's runtime, and the first thing to change in `thread/` would silently un-stage it again.
    //
    // A `sb::skip` here would be worse than useless: the contract would read as covered forever while
    // observing nothing. So this asserts what IS observable and what actually matters --
    // seccomp_strict_policy_n::apply() shares one loader with seccomp_policy_n::apply(), so the
    // errno-default case above is what proves TSYNC for both, and this proves the strict face is
    // wired to that loader at all rather than to something else.
    const adv::child_result r = adv::run_child([]() -> i32 {
      using strict_policy
          = s::seccomp_strict_policy_n<1024, s::errno_call<probe_nr, static_cast<u16>(mc::error::no_entry)>,
                                       s::allow<g::baseline, g::signal, g::process, g::memory, g::time>>;
      if ( strict_policy::apply() < 0 ) return install_failed;

      // the rule landed: the probe is ENOENT rather than a real pgid...
      if ( mc::syscall(probe_nr, 0) != -static_cast<long>(mc::error::no_entry) ) return adv::bad_code;
      // ...and the DEFAULT is a kill, not an errno, which is the whole difference between the two
      // faces. gettid is allowed; a syscall outside the allowlist would take the process down, and
      // that is asserted from the parent by the grade rather than from in here.
      if ( mc::syscall(SYS_gettid) <= 0 ) return adv::bad_code;
      return adv::ok_code;
    });
    if ( r.code == install_failed ) {
      sb::skip("the strict policy would not install on this kernel");
    } else {
      sb::require_true(r.ok());
    }
  }

  {
    sb::test_case("... and its default really is a KILL, not an errno");
    // the other half of "the strict face is the strict face". A syscall outside the allowlist must
    // terminate the process as though by SIGSYS (31), not return an error -- which is also the check
    // that the case above was not passing because the filter quietly did nothing.
    const adv::child_result r = adv::run_child([]() -> i32 {
      using strict_policy = s::seccomp_strict_policy_n<1024, s::allow<g::baseline, g::signal>>;
      if ( strict_policy::apply() < 0 ) return install_failed;
      (void)mc::syscall(probe_nr, 0);      // not allowlisted: the kernel kills us here
      return adv::bad_code;                // unreachable
    });
    if ( r.code == install_failed ) {
      sb::skip("the strict policy would not install on this kernel");
    } else {
      sb::print("  strict default -> child ", r.g == adv::grade::signalled ? "killed" : "survived", " (sig ", r.sig, ")");
      sb::require(static_cast<i32>(r.g), static_cast<i32>(adv::grade::signalled));
      sb::require(r.sig, 31);      // SIGSYS
    }
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 4  a partial sync must not read as success
  //
  // Without SECCOMP_FILTER_FLAG_TSYNC_ESRCH the kernel answers a partial synchronisation with the
  // TID of the thread it could not sync -- a POSITIVE number. Every caller in the tree tests
  // `r < 0`, so a partial sync is indistinguishable from a clean install, and the process runs on
  // believing every thread is covered.

  {
    sb::test_case("a TSYNC install must not report a partial synchronisation as success");
    const adv::child_result r = adv::run_child([]() -> i32 {
      sc::filter_builder<512> fb;
      build_probe_filter(fb);
      if ( !fb.valid() ) return adv::setup_failed;
      const int got = sc::load_tsync(fb, true);
      // 0 is a clean sync. Anything positive is a TID: the kernel telling us a thread was left out.
      // A loader that hands that back to a caller checking `r < 0` has reported a failure as a pass.
      if ( got > 0 ) return partial_sync_hidden;
      return got == 0 ? adv::ok_code : install_failed;
    });
    if ( r.code == partial_sync_hidden )
      sb::print("  load_tsync() returned a positive value (a TID): the caller's `r < 0` check reads "
                "that as success. TSYNC_ESRCH (linux/sys/seccomp.hpp:34) is defined and unused");
    sb::require_distinct(r.code, partial_sync_hidden);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // 5  sandbox stays right
  //
  // sandbox forks first, so its single-threaded child needs no TSYNC and asking for one would be
  // wrong. This is the guard against a fix that "unifies" the two paths and slows the sandbox down
  // for a synchronisation it does not need.

  {
    sb::test_case("control: sandbox's forked child is single-threaded and needs no TSYNC");
    const adv::child_result r = adv::run_child([]() -> i32 {
      // one thread, one filter, plain load: this must keep working exactly as it does
      sc::filter_builder<512> fb;
      build_probe_filter(fb);
      if ( !fb.valid() ) return adv::setup_failed;
      if ( sc::load(fb, true) < 0 ) return install_failed;
      return mc::syscall(probe_nr, 0) == -static_cast<long>(mc::error::no_entry) ? adv::ok_code : adv::bad_code;
    });
    sb::require_true(r.ok());
  }

  {
    sb::test_case("control: a TSYNC install still permits everything the policy allows");
    // the over-correction guard: "confine every thread" is trivially satisfied by killing them all
    const adv::child_result r = adv::run_child([]() -> i32 {
      sc::filter_builder<512> fb;
      build_probe_filter(fb);
      if ( !fb.valid() ) return adv::setup_failed;
      if ( sc::load_tsync(fb, true) < 0 ) return install_failed;
      if ( mc::posix::getpid() <= 0 ) return adv::bad_code;
      if ( mc::posix::write(1, "", 0) < 0 ) return adv::bad_code;
      return adv::ok_code;
    });
    sb::require_true(r.ok());
  }

  sb::print("=== ADV CVE-2017-5206 PASSED ===");
  return 1;
}
