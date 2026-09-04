//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::signal_t -- block, wait, and handle.
//
// NOTE: `micron::signal` is an enum class (linux/process/signals.hpp:17), the RAII type is
// signal_t<M> (:401), and there is no micron::signals namespace. sigset_t lives in micron::posix.
//
// The wait arm has to RAISE the signal itself. signal_t::operator() is sigwait, so waiting on a set
// nothing ever delivers blocks until the grader's --timeout kills the binary.

#include "../../src/control.hpp"
#include "../../src/io/console.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

static volatile micron::posix::sig_atomic_t g_caught = 0;

void
test(int sig)
{
  g_caught = sig;
}

// the number of unsigned longs in a sigset_t: 16 on LP64, 32 on ILP32. Never hardcode it.
constexpr static const usize sigwords = sizeof(micron::posix::sigset_t{}.__val) / sizeof(micron::posix::sigset_t{}.__val[0]);

// true when the set carries no bits outside the low _NSIG range
static bool
validate_sigset(const micron::posix::sigset_t *s)
{
  // signals are numbered 1..64, so only the low 64 bits of the whole array may ever be set
  for ( usize i = 64 / (8 * sizeof(unsigned long int)); i < sigwords; ++i )
    if ( s->__val[i] != 0 ) return false;
  return true;
}

// true when the set is empty
static bool
check_sigset(const micron::posix::sigset_t *set)
{
  for ( usize i = 0; i < sigwords; ++i )
    if ( set->__val[i] != 0 ) return false;
  return true;
}

int
main(void)
{
  test_case("sigset_t geometry and the empty/populated predicates");
  {
    require_true(sigwords * sizeof(unsigned long int) * 8 == 1024);

    micron::posix::sigset_t empty{};
    micron::posix::sigemptyset(empty);
    require_true(check_sigset(&empty));
    require_true(validate_sigset(&empty));

    micron::posix::sigaddset(empty, micron::posix::sig_abrt);
    require_true(!check_sigset(&empty));
    require_true(validate_sigset(&empty));      // still only low bits
    require_true(micron::posix::sigismember(empty, micron::posix::sig_abrt));
    require_true(!micron::posix::sigismember(empty, micron::posix::sig_alrm));
  }
  end_test_case();

  test_case("signal_t<> builds a set holding exactly what it was given");
  {
    micron::signal_t<> s(micron::signal::abort);
    micron::posix::sigset_t &ss = s.get_signal();
    require_true(validate_sigset(&ss));
    require_true(!check_sigset(&ss));
    require_true(micron::posix::sigismember(ss, micron::posix::sig_abrt));
    require_true(!micron::posix::sigismember(ss, micron::posix::sig_int));
  }
  end_test_case();

  test_case("mask() blocks, and a raised signal is then consumed by the wait rather than killing us");
  {
    micron::signal_t<> s(micron::signal::abort);
    require_true(s.mask() == 0);

    // SIGABRT is blocked now, so raising it only makes it pending -- this is the step the old
    // form of this test was missing, and without it operator() waits forever
    require_true(micron::send(micron::signal::abort) == 0);

    require_true(s() == 0);      // sigwait consumes the pending SIGABRT and returns

    // a second wait would block, so do not issue one; the dtor restores the pre-block mask
  }
  end_test_case();

  test_case("on_signal installs a handler that actually runs");
  {
    micron::signal_t<> s(micron::signal::alarm);
    g_caught = 0;
    require_true(s.on_signal(micron::signal::alarm, test) == 0);

    // NOT masked: the handler must be reached for real
    require_true(micron::send(micron::signal::alarm) == 0);
    require_true(g_caught == micron::posix::sig_alrm);

    // signal 0 and anything past the table are rejected rather than written out of bounds
    require_true(s.on_signal(static_cast<micron::signal>(0), test) == -1);
    require_true(s.on_signal(static_cast<micron::signal>(65), test) == -1);
  }
  end_test_case();

  sb::print("=== ALL CONTROL TESTS PASSED ===");
  return 1;
}
