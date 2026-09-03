//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// micron::sec::seccomp against a live kernel.
//
// EVERY case forks. Installing a filter, and setting NO_NEW_PRIVS, are irreversible for the life of
// the process -- so a single in-process install would poison every test_case after it, and the
// first one to deny a syscall the harness needs would take the whole binary down. The child does
// the work and reports through its exit status; the parent asserts on that.
//
// Nothing here needs privilege: an unprivileged task may install a filter as long as it sets
// NO_NEW_PRIVS first, which is exactly what sec::seccomp::load() does.

#include "../../src/std.hpp"

#include "../../src/exit.hpp"
#include "../../src/linux/process/fork.hpp"
#include "../../src/linux/process/wait.hpp"
#include "../../src/linux/sys/prctl.hpp"
#include "../../src/sec/groups.hpp"
#include "../../src/sec/seccomp.hpp"

#include "../snowball/snowball.hpp"

namespace mc = micron;
namespace sc = micron::sec::seccomp;
namespace g = micron::sec::groups;

namespace
{

// child exit codes. deliberately not 0/1 so a crash or an accidental fallthrough cannot be
// mistaken for a pass
constexpr i32 ok_code = 21;
constexpr i32 bad_code = 22;
constexpr i32 setup_failed = 23;

// the syscall the filters below discriminate on. getpgid is a pure read of process state: it is
// safe to call at any point, and micron's runtime never issues it on its own, so nothing but the
// test can trip the rule
constexpr i32 probe_nr = SYS_getpgid;

template<usize N>
void
allow_runtime(sc::filter_builder<N> &fb)
{
  for ( usize i = 0; i < g::baseline::count; ++i ) fb.allow(g::baseline::calls[i]);
  for ( usize i = 0; i < g::signal::count; ++i ) fb.allow(g::signal::calls[i]);
}

// run `fn` in a forked child and hand back its raw wait status
i32
in_child(void (*fn)(void))
{
  const int pid = mc::try_fork();
  if ( pid < 0 ) return -1;
  if ( pid == 0 ) {
    fn();
    mc::sys_group_exit(bad_code);      // fn must exit itself; reaching here is a failure
  }
  int status = 0;
  (void)mc::waitpid(pid, &status, 0);
  return status;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the child bodies

void
child_strict(void)
{
  // strict mode permits exactly read, write, _exit and sigreturn. getpgid is not among them, so
  // the kernel SIGKILLs us the moment we ask
  if ( sc::strict() < 0 ) mc::sys_group_exit(setup_failed);
  (void)mc::syscall(probe_nr, 0);
  mc::sys_group_exit(bad_code);      // unreachable: the kernel kills us above
}

void
child_default_errno(void)
{
  sc::filter_builder<512> fb;
  fb.require_native_arch();
  allow_runtime(fb);
  fb.default_errno(1);      // EPERM
  if ( !fb.valid() ) mc::sys_group_exit(setup_failed);
  if ( sc::load(fb) < 0 ) mc::sys_group_exit(setup_failed);

  // probe_nr is in no allowed group, so the default action answers for it
  const long r = mc::syscall(probe_nr, 0);
  mc::sys_group_exit(r == -1 ? ok_code : bad_code);
}

void
child_deny_errno_specific(void)
{
  sc::filter_builder<512> fb;
  fb.require_native_arch();
  fb.deny_errno(probe_nr, 30);      // EROFS
  allow_runtime(fb);
  fb.default_allow();
  if ( sc::load(fb) < 0 ) mc::sys_group_exit(setup_failed);

  const long denied = mc::syscall(probe_nr, 0);
  // a syscall that is NOT the denied one must still work: getpid is in baseline and allowed
  const long allowed = mc::syscall(SYS_getpid);
  mc::sys_group_exit((denied == -30 && allowed > 0) ? ok_code : bad_code);
}

void
child_arg_discrimination(void)
{
  sc::filter_builder<512> fb;
  fb.require_native_arch();
  fb.allow_if(probe_nr, sc::arg_eq(0, 0));      // getpgid(0) only
  allow_runtime(fb);
  fb.default_errno(1);
  if ( sc::load(fb) < 0 ) mc::sys_group_exit(setup_failed);

  const long with_zero = mc::syscall(probe_nr, 0);       // matches the predicate -> allowed
  const long with_one = mc::syscall(probe_nr, 12345);    // same syscall, wrong arg -> default
  mc::sys_group_exit((with_zero >= 0 && with_one == -1) ? ok_code : bad_code);
}

void
child_nnp_required(void)
{
  // an unprivileged task may not SET_MODE_FILTER without NO_NEW_PRIVS; the kernel says EACCES.
  // sec::seccomp::load() sets NNP for you, so this reaches past it to the raw call to prove the
  // requirement is real and that load()'s ordering is what makes it work
  sc::filter_builder<512> fb;
  fb.require_native_arch();
  allow_runtime(fb);
  fb.default_allow();
  auto prog = fb.prog();

  const int without = mc::posix::seccomp_load_filter(prog, 0);
  if ( without != -13 ) mc::sys_group_exit(bad_code);      // -EACCES

  if ( mc::prctl(mc::PR_SET_NO_NEW_PRIVS, 1UL) < 0 ) mc::sys_group_exit(setup_failed);
  const int with = mc::posix::seccomp_load_filter(prog, 0);
  mc::sys_group_exit(with == 0 ? ok_code : bad_code);
}

void
child_reports_filter_mode(void)
{
  if ( mc::prctl(mc::PR_GET_SECCOMP) != 0 ) mc::sys_group_exit(bad_code);      // disabled to begin with

  sc::filter_builder<512> fb;
  fb.require_native_arch();
  allow_runtime(fb);
  fb.allow(SYS_prctl);
  fb.default_allow();
  if ( sc::load(fb) < 0 ) mc::sys_group_exit(setup_failed);

  // 2 == SECCOMP_MODE_FILTER
  const i32 mode = mc::prctl(mc::PR_GET_SECCOMP);
  mc::sys_group_exit(mode == static_cast<i32>(mc::posix::seccomp_mode_filter) ? ok_code : bad_code);
}

void
child_overflow_refused(void)
{
  // the builder truncates silently once full. load() must refuse the result rather than confine
  // this process by a policy nobody wrote
  sc::filter_builder<16> tiny;
  tiny.require_native_arch();
  for ( i32 nr = 0; nr < 500; ++nr ) tiny.allow(nr);
  tiny.default_kill();

  const int r = sc::load(tiny);
  if ( r >= 0 ) mc::sys_group_exit(bad_code);      // it installed a truncated filter

  // and refusing must have left this process unconfined
  mc::sys_group_exit(mc::prctl(mc::PR_GET_SECCOMP) == 0 ? ok_code : bad_code);
}

void
child_residual_truncation_refused(void)
{
  // the residual-1 shape: the rules that did not fit stopped one slot short of the end, so the
  // seal still landed and the program is perfectly well formed. The KERNEL WILL TAKE THIS ONE --
  // there is no malformed jump for it to reject -- which is why the builder has to refuse it.
  // With a default of ALLOW the truncation is fail-open: syscalls the author denied are permitted
  sc::filter_builder<20> fb;
  fb.require_native_arch();
  for ( usize i = 0; i < g::network::count; ++i ) fb.deny_errno(g::network::calls[i], 1);
  fb.default_allow();

  if ( !fb.overflowed ) mc::sys_group_exit(bad_code);      // rules were dropped and nothing said so
  if ( sc::load(fb) >= 0 ) mc::sys_group_exit(bad_code);   // it installed a policy nobody wrote

  // prove the program really would have been accepted, so the refusal is the only thing that
  // stopped it: hand the same instructions to the kernel through the unchecked raw path
  mc::bpf::fprog_t prog{ static_cast<u16>(fb.count), fb.insns };
  if ( mc::prctl(mc::PR_SET_NO_NEW_PRIVS, 1UL) < 0 ) mc::sys_group_exit(setup_failed);
  const int forced = mc::posix::seccomp_load_filter(prog, 0);
  mc::sys_group_exit(forced == 0 ? ok_code : bad_code);
}

void
child_load_notif_refuses_ungated(void)
{
  // load_notif() used to skip the valid() gate its sibling load() enforces. USER_NOTIF is where
  // that hurts most: an ungated filter answers SECCOMP_RET_ALLOW to a compat-ABI syscall straight
  // out of the BPF, so the supervisor never sees the call and cannot compensate
  sc::filter_builder<512> fb;
  allow_runtime(fb);      // no require_native_arch()
  fb.default_notify();
  if ( fb.valid() ) mc::sys_group_exit(bad_code);

  const int r = sc::load_notif(fb);
  if ( r >= 0 ) mc::sys_group_exit(bad_code);      // it installed, and handed back a listener fd
  if ( r != -22 ) mc::sys_group_exit(bad_code);    // -EINVAL, the same answer load() gives

  // refusing must have left this process unconfined
  mc::sys_group_exit(mc::prctl(mc::PR_GET_SECCOMP) == 0 ? ok_code : bad_code);
}

void
child_load_notif_accepts_gated(void)
{
  // the positive control: the gate is not simply refusing every notif filter
  if ( !sc::action_avail_notify() ) mc::sys_group_exit(ok_code);      // kernel has no USER_NOTIF

  sc::filter_builder<512> fb;
  fb.require_native_arch();
  allow_runtime(fb);
  fb.allow(SYS_prctl);
  fb.deny(probe_nr, mc::posix::seccomp_ret_user_notif);
  fb.default_allow();
  if ( !fb.valid() ) mc::sys_group_exit(setup_failed);

  const int listener = sc::load_notif(fb);
  if ( listener < 0 ) mc::sys_group_exit(setup_failed);
  mc::sys_group_exit(mc::prctl(mc::PR_GET_SECCOMP) == static_cast<i32>(mc::posix::seccomp_mode_filter) ? ok_code : bad_code);
}

void
child_load_raw_refuses_ungated(void)
{
  // load_raw() takes a program that never came from a builder, so valid() cannot be asked. It gets
  // the structural half of the same check, and the escape hatch has to be named
  sc::filter_builder<512> fb;
  allow_runtime(fb);      // no arch gate
  fb.default_allow();
  mc::bpf::fprog_t prog{ static_cast<u16>(fb.count), fb.insns };

  if ( sc::prog_is_arch_gated(prog) ) mc::sys_group_exit(bad_code);
  if ( sc::load_raw(prog) >= 0 ) mc::sys_group_exit(bad_code);
  if ( mc::prctl(mc::PR_GET_SECCOMP) != 0 ) mc::sys_group_exit(bad_code);

  // a gated program goes through the same call untouched
  sc::filter_builder<512> gated;
  gated.require_native_arch();
  allow_runtime(gated);
  gated.allow(SYS_prctl);
  gated.default_allow();
  mc::bpf::fprog_t gp{ static_cast<u16>(gated.count), gated.insns };
  if ( !sc::prog_is_arch_gated(gp) ) mc::sys_group_exit(bad_code);
  if ( sc::load_raw(gp) < 0 ) mc::sys_group_exit(bad_code);
  mc::sys_group_exit(mc::prctl(mc::PR_GET_SECCOMP) == static_cast<i32>(mc::posix::seccomp_mode_filter) ? ok_code : bad_code);
}

void
child_ungated_filter_is_really_bypassable(void)
{
  // the reason the gates above exist, demonstrated rather than asserted. Install an UNGATED filter
  // through the named escape hatch, denying getpgid by its native number, then reach the kernel
  // through the ia32 compat entry where that number means something else. On amd64 the call goes
  // through -- the ladder was written for a syscall table this entry does not use
#if defined(__micron_arch_amd64)
  sc::filter_builder<512> fb;
  allow_runtime(fb);
  fb.deny_errno(probe_nr, 30);      // EROFS on the native number
  fb.default_allow();
  mc::bpf::fprog_t prog{ static_cast<u16>(fb.count), fb.insns };

  if ( sc::prog_is_arch_gated(prog) ) mc::sys_group_exit(bad_code);
  if ( mc::prctl(mc::PR_SET_NO_NEW_PRIVS, 1UL) < 0 ) mc::sys_group_exit(setup_failed);
  if ( sc::load_raw(prog, false, 0, /*unchecked=*/true) < 0 ) mc::sys_group_exit(setup_failed);

  // the native entry is filtered as written
  if ( mc::syscall(probe_nr, 0) != -30 ) mc::sys_group_exit(bad_code);

  // and the x32 entry carries the same number with the ABI bit set. Without a gate the compare
  // never matches, so the deny does not apply and the default answers instead
  const long via_x32 = mc::syscall(static_cast<long>(0x40000000 | probe_nr), 0);
  mc::sys_group_exit(via_x32 != -30 ? ok_code : bad_code);
#else
  mc::sys_group_exit(ok_code);
#endif
}

void
child_x32_gate(void)
{
  sc::filter_builder<512> fb;
  fb.require_native_arch();
  allow_runtime(fb);
  fb.default_allow();
  if ( sc::load(fb) < 0 ) mc::sys_group_exit(setup_failed);

  // getpid is allowed; the same number with the x32 bit set is a different syscall entirely and
  // must not inherit that permission
  if ( mc::syscall(SYS_getpid) <= 0 ) mc::sys_group_exit(bad_code);
  (void)mc::syscall(static_cast<long>(0x40000000 | SYS_getpid));
#if defined(__micron_arch_amd64)
  mc::sys_group_exit(bad_code);      // unreachable: the range deny kills the process
#else
  mc::sys_group_exit(ok_code);
#endif
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void
require_child_ok(void (*fn)(void))
{
  const i32 status = in_child(fn);
  sb::require_true(status >= 0);
  sb::require_true(mc::wifexited(status));
  sb::require(mc::wexitstatus(status), ok_code);
}

// NOTE: the two kill paths do NOT use the same signal, and asserting the wrong one is how a
// "did it die?" test passes for the wrong reason. strict mode SIGKILLs (9); a filter returning
// SECCOMP_RET_KILL_PROCESS / _KILL_THREAD terminates as though by SIGSYS (31)
constexpr i32 sigkill = 9;
constexpr i32 sigsys = 31;

void
require_child_killed(void (*fn)(void), i32 expect_sig)
{
  const i32 status = in_child(fn);
  sb::require_true(status >= 0);
  sb::require_true(mc::wifsignaled(status));
  sb::require(mc::wtermsig(status), expect_sig);
}

};      // namespace

int
main(void)
{
  sb::print("=== SEC SECCOMP LIVE ===");

  // ---------------------------------------------------------------- //
  sb::test_case("strict mode kills the process on a syscall outside its four");
  {
    require_child_killed(child_strict, sigkill);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("default_errno turns an unlisted syscall into a real -EPERM at the call site");
  {
    require_child_ok(child_default_errno);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("deny_errno reaches only its own syscall, and its errno is the one asked for");
  {
    require_child_ok(child_deny_errno_specific);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("allow_if discriminates two calls to the SAME syscall by argument value");
  {
    require_child_ok(child_arg_discrimination);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("SET_MODE_FILTER is EACCES without NO_NEW_PRIVS and succeeds with it");
  {
    require_child_ok(child_nnp_required);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("the kernel reports SECCOMP_MODE_FILTER once load() returns");
  {
    require_child_ok(child_reports_filter_mode);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("load() refuses an overflowed builder and leaves the process unconfined");
  {
    require_child_ok(child_overflow_refused);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("a truncation the KERNEL would have accepted is refused by the builder");
  {
    require_child_ok(child_residual_truncation_refused);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("load_notif() enforces the same gate as load(), and still installs a gated filter");
  {
    require_child_ok(child_load_notif_refuses_ungated);
    require_child_ok(child_load_notif_accepts_gated);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("load_raw() refuses a program with no arch gate unless told it is deliberate");
  {
    require_child_ok(child_load_raw_refuses_ungated);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("an ungated filter really is bypassable, which is why the gates are not pedantry");
  {
    require_child_ok(child_ungated_filter_is_really_bypassable);
  }
  sb::end_test_case();

  // ---------------------------------------------------------------- //
  sb::test_case("an x32-tagged syscall number does not inherit a native syscall's permission");
  {
#if defined(__micron_arch_amd64)
    require_child_killed(child_x32_gate, sigsys);
#else
    require_child_ok(child_x32_gate);
#endif
  }
  sb::end_test_case();

  sb::print("=== SEC SECCOMP LIVE PASSED ===");
  return 1;
}
