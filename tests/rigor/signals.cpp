//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Rigorous snowball test suite for micron POSIX signal handling.

#include "../../src/linux/process/signals.hpp"
#include "../../src/control.hpp"
#include "../../src/errno.hpp"
#include "../../src/linux/sys/signal.hpp"
#include "../../src/linux/sys/system.hpp"
#include "../../src/types.hpp"

#include "../snowball/snowball.hpp"

namespace pp = micron::posix;

static volatile int g_handler_signum = 0;
static volatile int g_handler_calls = 0;
static volatile int g_info_signum = 0;
static volatile int g_info_si_code = 0;

static void
recording_handler(int sig)
{
  g_handler_signum = sig;

  g_handler_calls = g_handler_calls + 1;
}

static void
counting_handler(int /*sig*/)
{
  g_handler_calls = g_handler_calls + 1;
}

static void
recording_info_handler(int sig, pp::siginfo_t *info, void * /*ctx*/)
{
  g_info_signum = sig;
  g_info_si_code = info ? info->si_code : -1;
}

static void
reset_counters()
{
  g_handler_signum = 0;
  g_handler_calls = 0;
  g_info_signum = 0;
  g_info_si_code = 0;
}

static void
clean_user_signals()
{
  micron::restore_default(micron::signal::user_signal_1);
  micron::restore_default(micron::signal::user_signal_2);

  pp::sigset_t empty = {};
  pp::sigemptyset(empty);
  pp::sigprocmask(pp::sig_setmask, empty, nullptr);
}

static void
test_sigaction_return_code()
{
  sb::print("=== sigaction return code (sigsetsize regression) ===");

  sb::test_case("posix::sigaction(SIGUSR1, handler) returns 0, not -EINVAL");
  {
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    int r = pp::sigaction(pp::sig_usr1, sa, nullptr);

    sb::require(r, 0);
  }
  sb::end_test_case();

  sb::test_case("posix::sigaction with old_action ptr also returns 0");
  {
    pp::sigaction_t sa = {};
    pp::sigaction_t old = {};
    sa.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    int r = pp::sigaction(pp::sig_usr2, sa, &old);
    sb::require(r, 0);
  }
  sb::end_test_case();

  sb::test_case("posix::sigprocmask(BLOCK, empty) returns 0, not -EINVAL");
  {
    pp::sigset_t empty = {};
    pp::sigemptyset(empty);
    int r = pp::sigprocmask(pp::sig_block, empty, nullptr);
    sb::require(r, 0);
  }
  sb::end_test_case();

  sb::test_case("micron::ignore(usr1) returns 0");
  sb::require(micron::ignore(micron::signal::user_signal_1), 0);
  sb::end_test_case();

  sb::test_case("micron::restore_default(usr1) returns 0");
  sb::require(micron::restore_default(micron::signal::user_signal_1), 0);
  sb::end_test_case();

  clean_user_signals();
}

static void
test_handler_delivery()
{
  sb::print("=== handler delivery via sigaction ===");

  sb::test_case("handler runs on raise(SIGUSR1)");
  {
    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, sa, nullptr), 0);
    sb::require(pp::raise(pp::sig_usr1), 0);
    sb::require(static_cast<int>(g_handler_calls), 1);
    sb::require(static_cast<int>(g_handler_signum), pp::sig_usr1);
  }
  sb::end_test_case();

  sb::test_case("handler runs again on second raise");
  {
    sb::require(pp::raise(pp::sig_usr1), 0);
    sb::require(static_cast<int>(g_handler_calls), 2);
  }
  sb::end_test_case();

  sb::test_case("handler receives correct signum for SIGUSR2");
  {
    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr2, sa, nullptr), 0);
    sb::require(pp::raise(pp::sig_usr2), 0);
    sb::require(static_cast<int>(g_handler_signum), pp::sig_usr2);
    sb::require(static_cast<int>(g_handler_calls), 1);
  }
  sb::end_test_case();

  clean_user_signals();
}

static void
test_ignore_and_default()
{
  sb::print("=== SIG_IGN / SIG_DFL ===");

  sb::test_case("ignore(usr1) suppresses delivery");
  {

    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = counting_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, sa, nullptr), 0);
    sb::require(pp::raise(pp::sig_usr1), 0);
    sb::require(static_cast<int>(g_handler_calls), 1);

    sb::require(micron::ignore(micron::signal::user_signal_1), 0);
    sb::require(pp::raise(pp::sig_usr1), 0);

    sb::require(static_cast<int>(g_handler_calls), 1);
  }
  sb::end_test_case();

  sb::test_case("restore_default(usr2) reinstates SIG_DFL");
  {

    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr2, sa, nullptr), 0);

    sb::require(micron::restore_default(micron::signal::user_signal_2), 0);

    pp::sigaction_t probe = {};
    pp::sigaction_t old = {};
    probe.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(probe.sa_mask);
    probe.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr2, probe, &old), 0);

    sb::require_true(old.sigaction_handler.sa_handler == reinterpret_cast<pp::sighandler_t>(0));
  }
  sb::end_test_case();

  clean_user_signals();
}

static void
test_old_action_roundtrip()
{
  sb::print("=== old_action round-trip ===");

  sb::test_case("install A then B; B's old_action surfaces A's handler");
  {
    pp::sigaction_t a = {};
    a.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(a.sa_mask);
    a.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, a, nullptr), 0);

    pp::sigaction_t b = {};
    pp::sigaction_t old = {};
    b.sigaction_handler.sa_handler = counting_handler;
    pp::sigemptyset(b.sa_mask);
    b.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, b, &old), 0);
    sb::require_true(old.sigaction_handler.sa_handler == recording_handler);
  }
  sb::end_test_case();

  sb::test_case("old_action.sa_flags carries the SA_RESTART bit we installed");
  {

    pp::sigaction_t a = {};
    a.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(a.sa_mask);
    a.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, a, nullptr), 0);

    pp::sigaction_t probe = {};
    pp::sigaction_t old = {};
    probe.sigaction_handler.sa_handler = counting_handler;
    pp::sigemptyset(probe.sa_mask);
    probe.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, probe, &old), 0);
    sb::require_true((old.sa_flags & pp::sa_restart) != 0);
  }
  sb::end_test_case();

  clean_user_signals();
}

static void
test_siginfo_handler()
{
  sb::print("=== SA_SIGINFO three-argument handler ===");

  sb::test_case("SA_SIGINFO handler fires and receives siginfo_t");
  {
    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_sigaction = recording_info_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart | pp::sa_siginfo;
    sb::require(pp::sigaction(pp::sig_usr1, sa, nullptr), 0);
    sb::require(pp::raise(pp::sig_usr1), 0);
    sb::require(static_cast<int>(g_info_signum), pp::sig_usr1);

    sb::require_true(static_cast<int>(g_info_si_code) != -1);
  }
  sb::end_test_case();

  clean_user_signals();
}

static void
test_sigprocmask_block_unblock()
{
  sb::print("=== sigprocmask block / unblock ===");

  sb::test_case("blocked signal is deferred; unblock delivers it");
  {
    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, sa, nullptr), 0);

    pp::sigset_t mask = {}, old = {};
    pp::sigemptyset(mask);
    pp::sigaddset(mask, pp::sig_usr1);
    sb::require(pp::sigprocmask(pp::sig_block, mask, &old), 0);

    sb::require(pp::raise(pp::sig_usr1), 0);

    sb::require(static_cast<int>(g_handler_calls), 0);

    sb::require(pp::sigprocmask(pp::sig_setmask, old, nullptr), 0);
    sb::require(static_cast<int>(g_handler_calls), 1);
    sb::require(static_cast<int>(g_handler_signum), pp::sig_usr1);
  }
  sb::end_test_case();

  sb::test_case("sigprocmask reports old mask correctly");
  {
    pp::sigset_t before = {}, after = {}, queried = {};
    pp::sigemptyset(before);
    pp::sigaddset(before, pp::sig_usr1);
    pp::sigaddset(before, pp::sig_usr2);
    sb::require(pp::sigprocmask(pp::sig_setmask, before, nullptr), 0);

    pp::sigemptyset(after);
    sb::require(pp::sigprocmask(pp::sig_setmask, after, &queried), 0);

    sb::require(pp::sigismember(queried, pp::sig_usr1), 1);
    sb::require(pp::sigismember(queried, pp::sig_usr2), 1);
    sb::require(pp::sigismember(queried, pp::sig_int), 0);
  }
  sb::end_test_case();

  clean_user_signals();
}

static void
test_signal_mask_guard()
{
  sb::print("=== signal_mask_guard RAII ===");

  sb::test_case("guard blocks usr1 inside scope, restores on exit");
  {
    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = recording_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, sa, nullptr), 0);

    sb::require_false(micron::is_blocked(micron::signal::user_signal_1));

    {
      micron::signal_mask_guard g(micron::signal::user_signal_1);
      sb::require_true(micron::is_blocked(micron::signal::user_signal_1));
      sb::require(pp::raise(pp::sig_usr1), 0);

      sb::require(static_cast<int>(g_handler_calls), 0);
    }

    sb::require_false(micron::is_blocked(micron::signal::user_signal_1));
    sb::require(static_cast<int>(g_handler_calls), 1);
  }
  sb::end_test_case();

  sb::test_case("guard with explicit release() also restores");
  {
    micron::signal_mask_guard g(micron::signal::user_signal_2);
    sb::require_true(micron::is_blocked(micron::signal::user_signal_2));
    g.release();
    sb::require_false(micron::is_blocked(micron::signal::user_signal_2));
  }
  sb::end_test_case();

  clean_user_signals();
}

static void
test_sigwait_pending()
{
  sb::print("=== sigwait on pending signal ===");

  sb::test_case("blocked + pending signal is consumed by sigwait, not handler");
  {
    reset_counters();
    pp::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = counting_handler;
    pp::sigemptyset(sa.sa_mask);
    sa.sa_flags = pp::sa_restart;
    sb::require(pp::sigaction(pp::sig_usr1, sa, nullptr), 0);

    pp::sigset_t mask = {}, old = {};
    pp::sigemptyset(mask);
    pp::sigaddset(mask, pp::sig_usr1);
    sb::require(pp::sigprocmask(pp::sig_block, mask, &old), 0);

    sb::require(pp::raise(pp::sig_usr1), 0);

    sb::require(static_cast<int>(g_handler_calls), 0);

    int got = -1;
    int err = pp::sigwait(mask, got);
    sb::require(err, 0);
    sb::require(got, pp::sig_usr1);

    sb::require(static_cast<int>(g_handler_calls), 0);

    sb::require(pp::sigprocmask(pp::sig_setmask, old, nullptr), 0);
  }
  sb::end_test_case();

  clean_user_signals();
}

static void
test_sigset_algebra()
{
  sb::print("=== sigset_t algebra ===");

  sb::test_case("sigemptyset zeros, sigaddset adds, sigismember reports");
  {
    pp::sigset_t s = {};
    pp::sigemptyset(s);
    sb::require(pp::sigismember(s, pp::sig_usr1), 0);
    pp::sigaddset(s, pp::sig_usr1);
    sb::require(pp::sigismember(s, pp::sig_usr1), 1);
    sb::require(pp::sigismember(s, pp::sig_usr2), 0);
  }
  sb::end_test_case();

  sb::test_case("sigdelset removes a previously added signal");
  {
    pp::sigset_t s = {};
    pp::sigemptyset(s);
    pp::sigaddset(s, pp::sig_int);
    pp::sigaddset(s, pp::sig_term);
    sb::require(pp::sigismember(s, pp::sig_int), 1);
    pp::sigdelset(s, pp::sig_int);
    sb::require(pp::sigismember(s, pp::sig_int), 0);
    sb::require(pp::sigismember(s, pp::sig_term), 1);
  }
  sb::end_test_case();

  sb::test_case("sigfillset includes every catchable signal");
  {
    pp::sigset_t s = {};
    pp::sigemptyset(s);
    pp::sigfillset(s);
    sb::require(pp::sigismember(s, pp::sig_usr1), 1);
    sb::require(pp::sigismember(s, pp::sig_usr2), 1);
    sb::require(pp::sigismember(s, pp::sig_int), 1);
    sb::require(pp::sigismember(s, pp::sig_term), 1);
  }
  sb::end_test_case();

  sb::test_case("sigisemptyset distinguishes empty from non-empty");
  {
    pp::sigset_t s = {};
    pp::sigemptyset(s);
    sb::require_true(pp::sigisemptyset(s) != 0);
    pp::sigaddset(s, pp::sig_usr1);
    sb::require_false(pp::sigisemptyset(s) != 0);
  }
  sb::end_test_case();

  sb::test_case("sigandset returns intersection");
  {
    pp::sigset_t a = {}, b = {};
    pp::sigemptyset(a);
    pp::sigemptyset(b);
    pp::sigaddset(a, pp::sig_usr1);
    pp::sigaddset(a, pp::sig_usr2);
    pp::sigaddset(b, pp::sig_usr2);
    pp::sigaddset(b, pp::sig_term);
    pp::sigset_t r = pp::sigandset(a, b);
    sb::require(pp::sigismember(r, pp::sig_usr1), 0);
    sb::require(pp::sigismember(r, pp::sig_usr2), 1);
    sb::require(pp::sigismember(r, pp::sig_term), 0);
  }
  sb::end_test_case();

  sb::test_case("sigorset returns union");
  {
    pp::sigset_t a = {}, b = {};
    pp::sigemptyset(a);
    pp::sigemptyset(b);
    pp::sigaddset(a, pp::sig_usr1);
    pp::sigaddset(b, pp::sig_usr2);
    pp::sigset_t r = pp::sigorset(a, b);
    sb::require(pp::sigismember(r, pp::sig_usr1), 1);
    sb::require(pp::sigismember(r, pp::sig_usr2), 1);
    sb::require(pp::sigismember(r, pp::sig_term), 0);
  }
  sb::end_test_case();
}

static_assert(pp::__sig_syscall_size == 8, "rt_sig* syscalls require sigsetsize == sizeof(kernel sigset_t) "
                                           "== _NSIG/8 == 8 bytes on Linux. If this assertion fails, the "
                                           "kernel will -EINVAL every signal-related syscall.");
static_assert(pp::__sig_syscall_size != sizeof(pp::sigset_t), "Userspace posix::sigset_t is padded to 1024 bits for ABI "
                                                              "compat with glibc; sizeof must NOT be what we pass to rt_sig*.");

int
main()
{
  sb::print("micron::posix signal test suite");
  sb::print("===============================");

  test_sigaction_return_code();
  test_handler_delivery();
  test_ignore_and_default();
  test_old_action_roundtrip();
  test_siginfo_handler();
  test_sigprocmask_block_unblock();
  test_signal_mask_guard();
  test_sigwait_pending();
  test_sigset_algebra();

  sb::print("===============================");
  sb::print("ALL SIGNAL TESTS PASSED");
  return 1;
}
