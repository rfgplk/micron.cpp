//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- the clock tier.
//
// Two regressions live here:
//
//   C.5  system_clock and micron::now() exc<>() on a failed clock_gettime, and with -fno-exceptions
//        exc<> degenerates to a write to fd 2 followed by abort(). The chrono::*_ns() readers must
//        never do that: they answer -errno.
//
//   D.1  micron::sleep(ms) called the ONE-ARGUMENT nanosleep, which passes nullptr for `rem`, and
//        then re-issued the FULL original interval on every EINTR. Under a periodic signal it never
//        terminated. The test for it is below: interrupt a sleep repeatedly and assert the total
//        elapsed time is roughly what was asked for, not a multiple of it.

#include "../../src/chrono.hpp"
#include "../../src/linux/process/signals.hpp"
#include "../../src/std.hpp"
#include "../../src/sync/pause.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;

static void
test_readers(void)
{
  sb::print("=== the never-throwing readers ===");

  test_case("every reader answers a positive value");
  {
    require_true(ch::mono_ns() > 0);
    require_true(ch::real_ns() > 0);
    require_true(ch::raw_ns() > 0);
    require_true(ch::boot_ns() > 0);
    require_true(ch::cpu_ns() >= 0);
    require_true(ch::thread_cpu_ns() >= 0);
    require_true(ch::now_ns() > 0);
    require_true(ch::now_us() > 0);
    require_true(ch::now_ms() > 0);
    require_true(ch::now_s() > 0);
  }
  end_test_case();

  test_case("the realtime clock is somewhere sane -- past 2024, before 2100");
  {
    const i64 s = ch::now_s();
    require_true(s > 1704067200L);       // 2024-01-01
    require_true(s < 4102444800L);       // 2100-01-01
  }
  end_test_case();

  test_case("the unit ladder is self-consistent");
  {
    const i64 ns = ch::now_ns();
    const i64 us = ch::now_us();
    const i64 ms = ch::now_ms();
    // taken microseconds apart, so allow a second of slack rather than demanding equality
    require_true(us - ns / 1000 < 1000000L && ns / 1000 - us < 1000000L);
    require_true(ms - us / 1000 < 1000L && us / 1000 - ms < 1000L);
  }
  end_test_case();

  test_case("C.5: a bad clockid answers -errno and does NOT abort");
  {
    // reaching the next line at all is the assertion: under -k, exc<> would have aborted the process
    const i64 r = ch::clock_ns(static_cast<micron::clockid_t>(0x7FFFFFFF));
    require_true(r < 0);
    const i64 res = ch::clock_resolution_ns(static_cast<micron::clockid_t>(0x7FFFFFFF));
    require_true(res < 0);
  }
  end_test_case();

  test_case("the monotonic clock never goes backwards");
  {
    i64 prev = ch::mono_ns();
    for ( int i = 0; i < 50000; ++i ) {
      const i64 now = ch::mono_ns();
      require_true(now >= prev);
      prev = now;
    }
  }
  end_test_case();

  test_case("clock_resolution_ns is positive and no coarser than a tick");
  {
    const i64 r = ch::clock_resolution_ns(micron::clock_monotonic);
    require_true(r > 0);
    require_true(r <= 1000000000L);
    const i64 c = ch::clock_resolution_ns(micron::clock_monotonic_coarse);
    require_true(c > 0);
    require_true(c >= r);      // "coarse" has to actually be coarser, or at least not finer
  }
  end_test_case();
}

static void
test_system_clock(void)
{
  sb::print("=== system_clock, including the overloads bbench needs ===");

  test_case("steady_clock is the monotonic one");
  {
    const micron::fduration_t a = micron::steady_clock::now();
    const micron::fduration_t b = micron::steady_clock::now();
    require_true(b >= a);
    // and it is NOT the realtime clock: milliseconds-since-boot is far smaller than since-epoch
    require_true(micron::steady_clock::now() < micron::system_clock<>::now());
  }
  end_test_case();

  test_case("read_ds / read_us / read_ns exist and are consistent -- bbench calls all of them");
  {
    micron::steady_clock c;
    c.start();
    micron::posix::usleep(3000);
    c.stop();
    const micron::fduration_t s = c.read<micron::unit::seconds>();
    require_true(c.read_ds() > s * 9.0 && c.read_ds() < s * 11.0);
    require_true(c.read_ms() > s * 900.0 && c.read_ms() < s * 1100.0);
    require_true(c.read_us() > s * 900000.0 && c.read_us() < s * 1100000.0);
    require_true(c.read_ns() > s * 900000000.0 && c.read_ns() < s * 1100000000.0);
  }
  end_test_case();

  test_case("the two-argument read overloads difference their arguments");
  {
    const micron::timespec_t a = ch::ts_of_ns(1000000000L);
    const micron::timespec_t b = ch::ts_of_ns(1500000000L);
    require(micron::steady_clock::read<micron::unit::milliseconds>(b, a), 500.0);
    require(micron::steady_clock::read_ms(b, a), 500.0);
    require(micron::steady_clock::read_us(b, a), 500000.0);
    require(micron::steady_clock::read_ns(b, a), 500000000.0);
    require(micron::steady_clock::read_ms(a, b), -500.0);
  }
  end_test_case();

  test_case("delta_ns is the exact integer form of the same measurement");
  {
    micron::steady_clock c;
    c.start();
    micron::posix::usleep(2000);
    c.stop();
    const i64 d = c.delta_ns();
    require_true(d > 1000000L);
    require_true(d < 100000000L);
    // and it agrees with the f64 path
    const micron::fduration_t f = c.read<micron::unit::nanoseconds>();
    require_true(f > static_cast<micron::fduration_t>(d) * 0.99 && f < static_cast<micron::fduration_t>(d) * 1.01);
  }
  end_test_case();

  test_case("C.12: assigning a timespec clears the stale end point");
  {
    micron::steady_clock c;
    c.start();
    c.stop();
    require_true(c.stopped());
    c = ch::ts_of_ns(ch::mono_ns());
    require_false(c.stopped());
  }
  end_test_case();

  test_case("system_clock<>::now_ns never throws and tracks now()");
  {
    const i64 a = micron::steady_clock::now_ns();
    require_true(a > 0);
    const micron::fduration_t b = micron::steady_clock::now();
    require_true(b > 0.0);
    // now() is milliseconds, now_ns() is nanoseconds
    require_true(static_cast<micron::fduration_t>(a) / 1e6 < b + 1000.0);
  }
  end_test_case();
}

static void
test_auto_timer(void)
{
  sb::print("=== C.4 REGRESSION: auto_timer's move ===");

  test_case("a moved-from auto_timer does NOT write over the result");
  {
    micron::fduration_t out = -1.0;
    {
      micron::auto_timer<micron::system_clocks::monotonic> a(&out);
      micron::auto_timer<micron::system_clocks::monotonic> b(static_cast<micron::auto_timer<micron::system_clocks::monotonic> &&>(a));
      micron::posix::usleep(2000);
      // `a` is destroyed here too. Before the fix its `out` still pointed at our variable and its
      // clock had a zeroed begin, so it wrote roughly seconds-since-epoch over the real answer
    }
    require_true(out >= 0.0);
    require_true(out < 1.0);      // ~2ms; the bug produced ~1.7e9
  }
  end_test_case();

  test_case("an ordinary auto_timer still measures");
  {
    micron::fduration_t out = -1.0;
    {
      micron::auto_timer<micron::system_clocks::monotonic> t(&out);
      micron::posix::usleep(5000);
    }
    require_true(out > 0.001);
    require_true(out < 1.0);
  }
  end_test_case();

  test_case("a null-out auto_timer is harmless");
  {
    {
      micron::auto_timer<micron::system_clocks::monotonic> t(nullptr);
      micron::posix::usleep(1000);
    }
    require_true(true);
  }
  end_test_case();

  test_case("scoped_timer measures in integer nanoseconds and never raises");
  {
    i64 out = -1;
    {
      micron::scoped_timer t(&out);
      micron::posix::usleep(5000);
    }
    require_true(out > 1000000L);
    require_true(out < 1000000000L);
  }
  end_test_case();
}

static void
test_deadlines(void)
{
  sb::print("=== deadlines and sleeping ===");

  test_case("remaining_ns saturates at zero rather than going negative");
  {
    require(ch::remaining_ns(100, 50), 50L);
    require(ch::remaining_ns(100, 100), 0L);
    require(ch::remaining_ns(100, 200), 0L);
  }
  end_test_case();

  test_case("remaining_ms clamps a far deadline instead of overflowing into a negative");
  {
    require(static_cast<long>(ch::remaining_ms(1000000000L, 0)), 1000L);
    require(static_cast<long>(ch::remaining_ms(0, 1000000000L)), 0L);
    // an int-millisecond poll timeout reads a negative as "block forever"
    const i32 v = ch::remaining_ms(0x7FFFFFFFFFFFFFFFLL, 0);
    require_true(v >= 0);
    require(static_cast<long>(v), 3600000L);
  }
  end_test_case();

  test_case("deadline::in_ms is in the future and armed");
  {
    const ch::deadline d = ch::deadline::in_ms(50);
    require_true(d.armed());
    require_true(d.at_ns > ch::mono_ns());
    require_true(ch::remaining_ns(d.at_ns, ch::mono_ns()) <= 50000000L);
  }
  end_test_case();

  test_case("sleep_ns sleeps about as long as asked -- the overflow guard must not fire");
  {
    const i64 t0 = ch::mono_ns();
    const i32 r = ch::sleep_ns(30000000ll);
    const i64 t1 = ch::mono_ns();
    require(static_cast<long>(r), 0L);
    require_true(t1 - t0 >= 29000000ll);
    require_true(t1 - t0 < 200000000ll);
  }
  end_test_case();

  test_case("sleep_ns(0) and a negative are no-ops");
  {
    require(static_cast<long>(ch::sleep_ns(0)), 0L);
    require(static_cast<long>(ch::sleep_ns(-1)), 0L);
  }
  end_test_case();

  test_case("sleep_until on an already-past deadline returns immediately");
  {
    const i64 t0 = ch::mono_ns();
    const i32 r = ch::sleep_until(micron::clock_monotonic, t0 - 1000000000ll);
    const i64 t1 = ch::mono_ns();
    require(static_cast<long>(r), 0L);
    require_true(t1 - t0 < 100000000ll);
  }
  end_test_case();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// D.1: interrupt a sleep and check it RESUMES rather than RESTARTS

static volatile int g_signals = 0;

static void
__chrono_test_handler(int)
{
  ++g_signals;
}

static void
test_eintr_resume(void)
{
  sb::print("=== D.1 REGRESSION: an interrupted sleep resumes, it does not restart ===");

  test_case("an itimer firing every 10ms does not extend a 100ms sleep");
  {
    // SIGALRM every 10ms for the duration of a 100ms sleep: ten interruptions. A sleep that
    // restarted its full interval on each one would take a second or never finish at all
    // NOTE: sa_flags is 0 on purpose. SA_RESTART would have the kernel resume the syscall for us,
    // which is precisely the behaviour under test -- with it set, a sleep that restarts its full
    // interval would be indistinguishable from one that resumes
    micron::posix::sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = &__chrono_test_handler;
    micron::posix::sigemptyset(sa.sa_mask);
    sa.sa_flags = 0;
    micron::posix::sigaction(micron::posix::sig_alrm, sa, nullptr);

    micron::itimerval_t it{};
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 10000;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 10000;
    micron::setitimer(micron::itimer_real, it);

    g_signals = 0;
    const i64 t0 = ch::mono_ns();
    const i32 r = ch::sleep_until(micron::clock_monotonic, t0 + 100000000ll);
    const i64 t1 = ch::mono_ns();

    micron::itimerval_t off{};
    micron::setitimer(micron::itimer_real, off);

    require(static_cast<long>(r), 0L);
    require_true(g_signals > 2);                  // it really was interrupted, repeatedly
    require_true(t1 - t0 >= 95000000ll);          // and it really did wait
    require_true(t1 - t0 < 400000000ll);          // but nothing like ten times over
  }
  end_test_case();

  test_case("micron::sleep(ms) terminates under the same signal storm");
  {
    micron::itimerval_t it{};
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 10000;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 10000;
    micron::setitimer(micron::itimer_real, it);

    g_signals = 0;
    const i64 t0 = ch::mono_ns();
    micron::sleep(100);      // milliseconds
    const i64 t1 = ch::mono_ns();

    micron::itimerval_t off{};
    micron::setitimer(micron::itimer_real, off);

    require_true(g_signals > 2);
    require_true(t1 - t0 >= 95000000ll);
    // before the fix each EINTR re-issued the whole 100ms, so this ran for as long as the timer did
    require_true(t1 - t0 < 400000000ll);
  }
  end_test_case();

  test_case("sleep_for and sleep_nano behave the same way");
  {
    micron::itimerval_t it{};
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 10000;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 10000;
    micron::setitimer(micron::itimer_real, it);

    const i64 t0 = ch::mono_ns();
    micron::sleep_for(60);
    const i64 t1 = ch::mono_ns();
    micron::sleep_nano(60000000ull);
    const i64 t2 = ch::mono_ns();

    micron::itimerval_t off{};
    micron::setitimer(micron::itimer_real, off);

    require_true(t1 - t0 >= 55000000ll && t1 - t0 < 300000000ll);
    require_true(t2 - t1 >= 55000000ll && t2 - t1 < 300000000ll);
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::chrono clock suite");
  sb::print("==========================");
  test_readers();
  test_system_clock();
  test_auto_timer();
  test_deadlines();
  test_eintr_resume();
  sb::print("==========================");
  sb::print("ALL CLOCK TESTS COMPLETED");
  return 1;
}
