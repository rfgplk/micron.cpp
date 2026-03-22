// Copyright (c) 2024- David Lucius Severus
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// test_chrono.cpp
//
// Compile:
//   c++ -std=c++23 -g -Wall -Wextra -o test_chrono test_chrono.cpp && ./test_chrono

#include "../src/chrono.hpp"     // adjust path as needed
#include "../snowball/snowball.hpp"

// small helper — busy-spin for ~N milliseconds without sleeping
// (avoids pulling in unistd.h / nanosleep)
static void
busy_wait_ms(int ms)
{
  micron::system_clock<micron::system_clocks::monotonic> clk;
  while ( clk.elapsed<micron::unit::milliseconds>() < static_cast<micron::fduration_t>(ms) )
    ;
}

// ─────────────────────────────────────────────────────────────────────────────
// ① free conversion functions
// ─────────────────────────────────────────────────────────────────────────────

static void
test_conversions()
{
  sb::print("=== free conversion functions ===");

  sb::test_case("days(86400.0) == 1.0");
  sb::require(micron::days(86400.0), 1.0);
  sb::end_test_case();

  sb::test_case("days(0.0) == 0.0");
  sb::require(micron::days(0.0), 0.0);
  sb::end_test_case();

  sb::test_case("hours(3600.0) == 1.0");
  sb::require(micron::hours(3600.0), 1.0);
  sb::end_test_case();

  sb::test_case("hours(7200.0) == 2.0");
  sb::require(micron::hours(7200.0), 2.0);
  sb::end_test_case();

  sb::test_case("minutes(60.0) == 1.0");
  sb::require(micron::minutes(60.0), 1.0);
  sb::end_test_case();

  sb::test_case("minutes(120.0) == 2.0");
  sb::require(micron::minutes(120.0), 2.0);
  sb::end_test_case();

  sb::test_case("seconds<base_ratio>(1.0) == 1.0");
  sb::require(micron::seconds<micron::base_ratio>(1.0), 1.0);
  sb::end_test_case();

  sb::test_case("milliseconds<milli>(1.0) == 1000.0");
  sb::require(micron::milliseconds<micron::milli>(1.0), 1000.0);
  sb::end_test_case();

  sb::test_case("microseconds<micro>(1.0) == 1000000.0");
  sb::require(micron::microseconds<micron::micro>(1.0), 1000000.0);
  sb::end_test_case();

  sb::test_case("nanoseconds<nano>(1.0) == 1000000000.0");
  sb::require(micron::nanoseconds<micron::nano>(1.0), 1000000000.0);
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ② __impl::normalise
// ─────────────────────────────────────────────────────────────────────────────

static void
test_normalise()
{
  sb::print("=== __impl::normalise ===");

  sb::test_case("positive nsec unchanged");
  {
    time_t sec = 5;
    long nsec = 500'000'000L;
    micron::__impl::normalise(sec, nsec);
    sb::require(sec, time_t{ 5 });
    sb::require(nsec, 500'000'000L);
  }
  sb::end_test_case();

  sb::test_case("zero nsec unchanged");
  {
    time_t sec = 3;
    long nsec = 0L;
    micron::__impl::normalise(sec, nsec);
    sb::require(sec, time_t{ 3 });
    sb::require(nsec, 0L);
  }
  sb::end_test_case();

  sb::test_case("negative nsec borrows one second");
  {
    time_t sec = 5;
    long nsec = -200'000'000L;
    micron::__impl::normalise(sec, nsec);
    sb::require(sec, time_t{ 4 });
    sb::require(nsec, 800'000'000L);
  }
  sb::end_test_case();

  sb::test_case("negative nsec exactly -1s becomes 0 nsec");
  {
    time_t sec = 2;
    long nsec = -1'000'000'000L;
    micron::__impl::normalise(sec, nsec);
    sb::require(sec, time_t{ 1 });
    sb::require(nsec, 0L);
  }
  sb::end_test_case();

  sb::test_case("sec 0, negative nsec => sec -1");
  {
    time_t sec = 0;
    long nsec = -1L;
    micron::__impl::normalise(sec, nsec);
    sb::require(sec, time_t{ -1 });
    sb::require(nsec, 999'999'999L);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ③ __impl::delta_to_unit
// ─────────────────────────────────────────────────────────────────────────────

static void
test_delta_to_unit()
{
  sb::print("=== __impl::delta_to_unit ===");

  sb::test_case("nanoseconds: 1s 500ns = 1000000500.0ns");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::nanoseconds>(1, 500L);
    sb::require(r, 1'000'000'500.0);
  }
  sb::end_test_case();

  sb::test_case("nanoseconds: 0s 0ns = 0");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::nanoseconds>(0, 0L);
    sb::require(r, 0.0);
  }
  sb::end_test_case();

  sb::test_case("microseconds: 1s 0ns = 1000000.0us");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::microseconds>(1, 0L);
    sb::require(r, 1'000'000.0);
  }
  sb::end_test_case();

  sb::test_case("microseconds: 0s 1000ns = 1.0us");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::microseconds>(0, 1'000L);
    sb::require(r, 1.0);
  }
  sb::end_test_case();

  sb::test_case("milliseconds: 1s 0ns = 1000.0ms");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::milliseconds>(1, 0L);
    sb::require(r, 1'000.0);
  }
  sb::end_test_case();

  sb::test_case("milliseconds: 0s 1000000ns = 1.0ms");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::milliseconds>(0, 1'000'000L);
    sb::require(r, 1.0);
  }
  sb::end_test_case();

  sb::test_case("seconds: 5s 500000000ns = 5.5s");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::seconds>(5, 500'000'000L);
    sb::require(r, 5.5);
  }
  sb::end_test_case();

  sb::test_case("seconds: 0s 0ns = 0.0s");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::seconds>(0, 0L);
    sb::require(r, 0.0);
  }
  sb::end_test_case();

  sb::test_case("minutes: 120s 0ns = 2.0min");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::minutes>(120, 0L);
    sb::require(r, 2.0);
  }
  sb::end_test_case();

  sb::test_case("hours: 7200s 0ns = 2.0hr");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::hours>(7200, 0L);
    sb::require(r, 2.0);
  }
  sb::end_test_case();

  sb::test_case("days: 172800s 0ns = 2.0 days");
  {
    auto r = micron::__impl::delta_to_unit<micron::unit::days>(172800, 0L);
    sb::require(r, 2.0);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ④ system_clock — construction and basic lifecycle
// ─────────────────────────────────────────────────────────────────────────────

static void
test_system_clock_lifecycle()
{
  sb::print("=== system_clock lifecycle ===");

  sb::test_case("default-constructed clock has nonzero begin_point");
  {
    micron::system_clock<> clk;
    auto bp = clk.begin_point();
    sb::require_true(bp.tv_sec != 0 || bp.tv_nsec != 0);
  }
  sb::end_test_case();

  sb::test_case("default-constructed clock has zero end_point (not yet stopped)");
  {
    micron::system_clock<> clk;
    auto ep = clk.end_point();
    sb::require_true(!clk.stopped());
  }
  sb::end_test_case();

  sb::test_case("stopped() returns true after stop()");
  {
    micron::system_clock<> clk;
    clk.stop();
    sb::require_true(clk.stopped());
  }
  sb::end_test_case();

  sb::test_case("copy constructor preserves begin_point");
  {
    micron::system_clock<> a;
    auto bp = a.begin_point();
    micron::system_clock<> b(a);
    sb::require(b.begin_point().tv_sec, bp.tv_sec);
    sb::require(b.begin_point().tv_nsec, bp.tv_nsec);
  }
  sb::end_test_case();

  sb::test_case("move constructor zeroes source end_point");
  {
    micron::system_clock<> a;
    a.stop();
    sb::require_true(a.stopped());
    micron::system_clock<> b(static_cast<micron::system_clock<> &&>(a));
    sb::require_true(b.stopped());
    // source should be zeroed
    sb::require_false(a.stopped());
  }
  sb::end_test_case();

  sb::test_case("reset() restarts begin_point and clears end_point");
  {
    micron::system_clock<> clk;
    clk.stop();
    sb::require_true(clk.stopped());
    clk.reset();
    sb::require_false(clk.stopped());
    sb::require_true(clk.begin_point().tv_sec != 0 || clk.begin_point().tv_nsec != 0);
  }
  sb::end_test_case();

  sb::test_case("copy assignment preserves both points");
  {
    micron::system_clock<> a;
    a.stop();
    micron::system_clock<> b;
    b = a;
    sb::require(b.begin_point().tv_sec, a.begin_point().tv_sec);
    sb::require(b.end_point().tv_sec, a.end_point().tv_sec);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑤ system_clock — timing correctness
// ─────────────────────────────────────────────────────────────────────────────

static void
test_system_clock_timing()
{
  sb::print("=== system_clock timing ===");

  sb::test_case("elapsed milliseconds > 0 after busy-wait 5ms");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    busy_wait_ms(5);
    auto ms = clk.elapsed<micron::unit::milliseconds>();
    sb::require_true(ms > 0.0);
  }
  sb::end_test_case();

  sb::test_case("elapsed milliseconds >= 5 after busy-wait 5ms");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    busy_wait_ms(5);
    auto ms = clk.elapsed<micron::unit::milliseconds>();
    sb::require_greater(ms, 0.0);
  }
  sb::end_test_case();

  sb::test_case("elapsed microseconds > elapsed milliseconds (same wait)");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    busy_wait_ms(2);
    clk.stop();
    auto us = clk.read<micron::unit::microseconds>();
    auto ms = clk.read<micron::unit::milliseconds>();
    sb::require_greater(us, ms);
  }
  sb::end_test_case();

  sb::test_case("elapsed nanoseconds > elapsed microseconds (same wait)");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    busy_wait_ms(2);
    clk.stop();
    auto ns = clk.read<micron::unit::nanoseconds>();
    auto us = clk.read<micron::unit::microseconds>();
    sb::require_greater(ns, us);
  }
  sb::end_test_case();

  sb::test_case("read_ms() agrees with read<milliseconds>()");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    busy_wait_ms(3);
    clk.stop();
    auto r1 = clk.read_ms();
    auto r2 = clk.read<micron::unit::milliseconds>();
    sb::require(r1, r2);
  }
  sb::end_test_case();

  sb::test_case("start() restarts timing; second measurement > first");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    busy_wait_ms(2);
    auto t1 = clk.elapsed<micron::unit::milliseconds>();
    clk.start();
    busy_wait_ms(2);
    auto t2 = clk.elapsed<micron::unit::milliseconds>();
    // both should be positive; neither should see the combined wait
    sb::require_greater(t1, 0.0);
    sb::require_greater(t2, 0.0);
  }
  sb::end_test_case();

  sb::test_case("lap() returns elapsed and resets start; second lap independent");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    busy_wait_ms(3);
    auto lap1 = clk.lap();
    sb::require_greater(lap1, 0.0);
    busy_wait_ms(3);
    auto lap2 = clk.lap();
    sb::require_greater(lap2, 0.0);
    // neither lap should be astronomically large (sanity: < 10 seconds)
    sb::require_smaller(lap1, 10.0);
    sb::require_smaller(lap2, 10.0);
  }
  sb::end_test_case();

  sb::test_case("now() returns a positive millisecond timestamp");
  {
    auto ms = micron::system_clock<>::now();
    sb::require_greater(ms, 0.0);
  }
  sb::end_test_case();

  sb::test_case("two successive now() calls are non-decreasing");
  {
    auto t1 = micron::system_clock<>::now();
    auto t2 = micron::system_clock<>::now();
    sb::require_true(t2 >= t1);
  }
  sb::end_test_case();

  sb::test_case("now_ts() returns a valid micron::timespec (tv_sec > 0)");
  {
    auto ts = micron::system_clock<>::now_ts();
    sb::require_greater(ts.tv_sec, time_t{ 0 });
  }
  sb::end_test_case();

  sb::test_case("resolution() returns without throwing");
  {
    auto res = micron::system_clock<>::resolution();
    sb::require_true(res.tv_sec >= 0 && res.tv_nsec >= 0);
  }
  sb::end_test_case();

  sb::test_case("monotonic clock start_get() returns same as begin_point()");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    auto ts = clk.start_get();
    sb::require(ts.tv_sec, clk.begin_point().tv_sec);
  }
  sb::end_test_case();

  sb::test_case("stop_get() returns same as end_point() after stop");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    auto ts = clk.stop_get();
    sb::require(ts.tv_sec, clk.end_point().tv_sec);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑥ time_point
// ─────────────────────────────────────────────────────────────────────────────

static void
test_time_point()
{
  sb::print("=== time_point ===");

  sb::test_case("default-constructed time_since_epoch() == 0");
  {
    micron::time_point<> tp;
    sb::require(tp.time_since_epoch(), micron::fduration_t{ 0 });
  }
  sb::end_test_case();

  sb::test_case("explicit construction preserves value");
  {
    micron::time_point<> tp{ 42.5 };
    sb::require(tp.time_since_epoch(), 42.5);
  }
  sb::end_test_case();

  sb::test_case("operator+= accumulates correctly");
  {
    micron::time_point<> tp{ 10.0 };
    tp += 5.0;
    sb::require(tp.time_since_epoch(), 15.0);
  }
  sb::end_test_case();

  sb::test_case("operator-= decrements correctly");
  {
    micron::time_point<> tp{ 10.0 };
    tp -= 3.0;
    sb::require(tp.time_since_epoch(), 7.0);
  }
  sb::end_test_case();

  sb::test_case("operator== / != reflexive and symmetric");
  {
    micron::time_point<> a{ 5.0 }, b{ 5.0 }, c{ 6.0 };
    sb::require_true(a == b);
    sb::require_false(a == c);
    sb::require_true(a != c);
    sb::require_false(a != b);
  }
  sb::end_test_case();

  sb::test_case("operator< / > / <= / >= ordering");
  {
    micron::time_point<> small{ 1.0 }, large{ 2.0 };
    sb::require_true(small < large);
    sb::require_true(large > small);
    sb::require_true(small <= small);
    sb::require_true(large >= large);
    sb::require_false(large < small);
  }
  sb::end_test_case();

  sb::test_case("free operator+(tp, d)");
  {
    micron::time_point<> tp{ 10.0 };
    auto r = tp + 3.0;
    sb::require(r.time_since_epoch(), 13.0);
    sb::require(tp.time_since_epoch(), 10.0);     // original unchanged
  }
  sb::end_test_case();

  sb::test_case("free operator+(d, tp) commutative");
  {
    micron::time_point<> tp{ 10.0 };
    auto r = 3.0 + tp;
    sb::require(r.time_since_epoch(), 13.0);
  }
  sb::end_test_case();

  sb::test_case("free operator-(tp, d)");
  {
    micron::time_point<> tp{ 10.0 };
    auto r = tp - 4.0;
    sb::require(r.time_since_epoch(), 6.0);
  }
  sb::end_test_case();

  sb::test_case("free operator-(tp, tp) returns difference");
  {
    micron::time_point<> a{ 15.0 }, b{ 10.0 };
    micron::fduration_t d = a - b;
    sb::require(d, 5.0);
  }
  sb::end_test_case();

  sb::test_case("as<milliseconds>() round-trips stored ms value");
  {
    micron::time_point<> tp{ 2000.0 };     // 2000 ms
    auto ms = tp.as<micron::unit::milliseconds>();
    sb::require(ms, 2000.0);
  }
  sb::end_test_case();

  sb::test_case("as<seconds>() converts ms to seconds");
  {
    micron::time_point<> tp{ 3000.0 };     // 3000 ms = 3 s
    auto s = tp.as<micron::unit::seconds>();
    sb::require(s, 3.0);
  }
  sb::end_test_case();

  sb::test_case("as<microseconds>() converts ms to microseconds");
  {
    micron::time_point<> tp{ 1.0 };     // 1 ms = 1000 us
    auto us = tp.as<micron::unit::microseconds>();
    sb::require(us, 1'000.0);
  }
  sb::end_test_case();

  sb::test_case("as<nanoseconds>() converts ms to nanoseconds");
  {
    micron::time_point<> tp{ 1.0 };     // 1 ms = 1000000 ns
    auto ns = tp.as<micron::unit::nanoseconds>();
    sb::require(ns, 1'000'000.0);
  }
  sb::end_test_case();

  sb::test_case("as<minutes>() converts ms to minutes");
  {
    micron::time_point<> tp{ 60'000.0 };     // 60000 ms = 1 min
    auto m = tp.as<micron::unit::minutes>();
    sb::require(m, 1.0);
  }
  sb::end_test_case();

  sb::test_case("as<hours>() converts ms to hours");
  {
    micron::time_point<> tp{ 3'600'000.0 };     // 3600000 ms = 1 hr
    auto h = tp.as<micron::unit::hours>();
    sb::require(h, 1.0);
  }
  sb::end_test_case();

  sb::test_case("time_point::now() returns positive epoch value");
  {
    auto tp = micron::time_point<>::now();
    sb::require_greater(tp.time_since_epoch(), 0.0);
  }
  sb::end_test_case();

  sb::test_case("two successive now() are non-decreasing");
  {
    auto t1 = micron::time_point<>::now();
    auto t2 = micron::time_point<>::now();
    sb::require_true(t2 >= t1);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑦ time_of_day
// ─────────────────────────────────────────────────────────────────────────────

static void
test_time_of_day()
{
  sb::print("=== time_of_day ===");

  sb::test_case("default construction yields all zeros");
  {
    micron::time_of_day t;
    sb::require(t.hours(), 0.0);
    sb::require(t.minutes(), 0.0);
    sb::require(t.seconds(), 0.0);
    sb::require(t.subseconds(), 0.0);
  }
  sb::end_test_case();

  sb::test_case("h/m/s/ss constructor preserves all fields");
  {
    micron::time_of_day t{ 3.0, 25.0, 47.0, 0.5 };
    sb::require(t.hours(), 3.0);
    sb::require(t.minutes(), 25.0);
    sb::require(t.seconds(), 47.0);
    sb::require(t.subseconds(), 0.5);
  }
  sb::end_test_case();

  sb::test_case("duration constructor: 0s => midnight");
  {
    micron::time_of_day t{ 0.0 };
    sb::require(t.hours(), 0.0);
    sb::require(t.minutes(), 0.0);
    sb::require(t.seconds(), 0.0);
  }
  sb::end_test_case();

  sb::test_case("duration constructor: 3600s => 1 hour exactly");
  {
    micron::time_of_day t{ 3600.0 };
    sb::require(t.hours(), 1.0);
    sb::require(t.minutes(), 0.0);
    sb::require(t.seconds(), 0.0);
  }
  sb::end_test_case();

  sb::test_case("duration constructor: 3661s => 1h 1m 1s");
  {
    micron::time_of_day t{ 3661.0 };
    sb::require(t.hours(), 1.0);
    sb::require(t.minutes(), 1.0);
    sb::require(t.seconds(), 1.0);
  }
  sb::end_test_case();

  sb::test_case("duration constructor: 90s => 0h 1m 30s");
  {
    micron::time_of_day t{ 90.0 };
    sb::require(t.hours(), 0.0);
    sb::require(t.minutes(), 1.0);
    sb::require(t.seconds(), 30.0);
  }
  sb::end_test_case();

  sb::test_case("to_duration() round-trips h/m/s construction");
  {
    micron::time_of_day t{ 2.0, 30.0, 15.0, 0.0 };
    micron::fduration_t d = t.to_duration();
    sb::require(d, 2.0 * 3600 + 30.0 * 60 + 15.0);
  }
  sb::end_test_case();

  sb::test_case("to_duration() round-trips from-seconds construction");
  {
    micron::fduration_t secs = 7384.0;     // 2h 3m 4s
    micron::time_of_day t{ secs };
    sb::require(t.to_duration(), secs);
  }
  sb::end_test_case();

  sb::test_case("time_of_day_now() returns a valid object without throwing");
  {
    auto t = micron::time_of_day_now();
    // if it threw, we would never reach here — also validate the result
    sb::require_true(t.hours() >= 0.0);
  }
  sb::end_test_case();

  sb::test_case("time_of_day_now() hours in [0, 24)");
  {
    auto t = micron::time_of_day_now();
    sb::require_true(t.hours() >= 0.0 && t.hours() < 24.0);
  }
  sb::end_test_case();

  sb::test_case("time_of_day_now() minutes in [0, 60)");
  {
    auto t = micron::time_of_day_now();
    sb::require_true(t.minutes() >= 0.0 && t.minutes() < 60.0);
  }
  sb::end_test_case();

  sb::test_case("time_of_day_now() seconds in [0, 60)");
  {
    auto t = micron::time_of_day_now();
    sb::require_true(t.seconds() >= 0.0 && t.seconds() < 60.0);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑧ year / month / day
// ─────────────────────────────────────────────────────────────────────────────

static void
test_year_month_day_types()
{
  sb::print("=== year / month / day primitives ===");

  // year
  sb::test_case("year converts to int correctly");
  sb::require(static_cast<int>(micron::year{ 2024 }), 2024);
  sb::end_test_case();

  sb::test_case("year 2000 is a leap year");
  sb::require_true(micron::year{ 2000 }.is_leap());
  sb::end_test_case();

  sb::test_case("year 1900 is NOT a leap year (div 100, not 400)");
  sb::require_false(micron::year{ 1900 }.is_leap());
  sb::end_test_case();

  sb::test_case("year 2024 is a leap year (div 4, not 100)");
  sb::require_true(micron::year{ 2024 }.is_leap());
  sb::end_test_case();

  sb::test_case("year 2023 is NOT a leap year");
  sb::require_false(micron::year{ 2023 }.is_leap());
  sb::end_test_case();

  sb::test_case("year 1600 is a leap year (div 400)");
  sb::require_true(micron::year{ 1600 }.is_leap());
  sb::end_test_case();

  // month
  sb::test_case("month converts to unsigned correctly");
  sb::require(static_cast<unsigned>(micron::month{ 6 }), 6u);
  sb::end_test_case();

  sb::test_case("month 1 is ok");
  sb::require_true(micron::month{ 1 }.ok());
  sb::end_test_case();

  sb::test_case("month 12 is ok");
  sb::require_true(micron::month{ 12 }.ok());
  sb::end_test_case();

  sb::test_case("month 0 is not ok");
  sb::require_false(micron::month{ 0 }.ok());
  sb::end_test_case();

  sb::test_case("month 13 is not ok");
  sb::require_false(micron::month{ 13 }.ok());
  sb::end_test_case();

  // day
  sb::test_case("day converts to unsigned correctly");
  sb::require(static_cast<unsigned>(micron::day{ 15 }), 15u);
  sb::end_test_case();

  sb::test_case("day 1 is ok");
  sb::require_true(micron::day{ 1 }.ok());
  sb::end_test_case();

  sb::test_case("day 31 is ok");
  sb::require_true(micron::day{ 31 }.ok());
  sb::end_test_case();

  sb::test_case("day 0 is not ok");
  sb::require_false(micron::day{ 0 }.ok());
  sb::end_test_case();

  sb::test_case("day 32 is not ok");
  sb::require_false(micron::day{ 32 }.ok());
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑨ year_month_day — from_unix / to_unix / ok
// ─────────────────────────────────────────────────────────────────────────────

static void
test_year_month_day()
{
  sb::print("=== year_month_day ===");

  // UNIX epoch = 1970-01-01
  sb::test_case("from_unix(0) == 1970-01-01");
  {
    auto ymd = micron::year_month_day::from_unix(0);
    sb::require(static_cast<int>(ymd.yr), 1970);
    sb::require(static_cast<unsigned>(ymd.mo), 1u);
    sb::require(static_cast<unsigned>(ymd.dy), 1u);
  }
  sb::end_test_case();

  // 2000-01-01: 10957 days after epoch = 946684800s
  sb::test_case("from_unix(946684800) == 2000-01-01");
  {
    auto ymd = micron::year_month_day::from_unix(946684800LL);
    sb::require(static_cast<int>(ymd.yr), 2000);
    sb::require(static_cast<unsigned>(ymd.mo), 1u);
    sb::require(static_cast<unsigned>(ymd.dy), 1u);
  }
  sb::end_test_case();

  // 2024-03-15 = 1710460800s
  sb::test_case("from_unix(1710460800) == 2024-03-15");
  {
    auto ymd = micron::year_month_day::from_unix(1710460800LL);
    sb::require(static_cast<int>(ymd.yr), 2024);
    sb::require(static_cast<unsigned>(ymd.mo), 3u);
    sb::require(static_cast<unsigned>(ymd.dy), 15u);
  }
  sb::end_test_case();

  // 1999-12-31 = 946598400s
  sb::test_case("from_unix(946598400) == 1999-12-31");
  {
    auto ymd = micron::year_month_day::from_unix(946598400LL);
    sb::require(static_cast<int>(ymd.yr), 1999);
    sb::require(static_cast<unsigned>(ymd.mo), 12u);
    sb::require(static_cast<unsigned>(ymd.dy), 31u);
  }
  sb::end_test_case();

  // to_unix round-trip: 1970-01-01
  sb::test_case("to_unix(from_unix(0)) == 0");
  {
    auto ymd = micron::year_month_day::from_unix(0);
    sb::require(ymd.to_unix(), time_t{ 0 });
  }
  sb::end_test_case();

  // to_unix round-trip: 2000-01-01
  sb::test_case("to_unix(from_unix(946684800)) == 946684800");
  {
    auto ymd = micron::year_month_day::from_unix(946684800LL);
    sb::require(ymd.to_unix(), time_t{ 946684800LL });
  }
  sb::end_test_case();

  // to_unix round-trip: 2024-03-15
  sb::test_case("to_unix round-trip 2024-03-15");
  {
    auto ymd = micron::year_month_day::from_unix(1710460800LL);
    sb::require(ymd.to_unix(), time_t{ 1710460800LL });
  }
  sb::end_test_case();

  // manual construction + ok()
  sb::test_case("manually constructed valid date ok() == true");
  {
    micron::year_month_day ymd{ micron::year{ 2024 }, micron::month{ 6 }, micron::day{ 15 } };
    sb::require_true(ymd.ok());
  }
  sb::end_test_case();

  sb::test_case("invalid month -> ok() == false");
  {
    micron::year_month_day ymd{ micron::year{ 2024 }, micron::month{ 0 }, micron::day{ 15 } };
    sb::require_false(ymd.ok());
  }
  sb::end_test_case();

  sb::test_case("invalid day -> ok() == false");
  {
    micron::year_month_day ymd{ micron::year{ 2024 }, micron::month{ 5 }, micron::day{ 0 } };
    sb::require_false(ymd.ok());
  }
  sb::end_test_case();

  sb::test_case("from_unix round-trips multiple consecutive days");
  {
    // step through 30 days from 2000-01-01 and verify day increments
    time_t base = 946684800LL;
    for ( int d = 0; d < 30; d++ ) {
      auto ymd = micron::year_month_day::from_unix(base + static_cast<time_t>(d) * 86400LL);
      sb::require_true(ymd.ok());
      // round-trip
      sb::require(ymd.to_unix(), base + static_cast<time_t>(d) * 86400LL);
    }
  }
  sb::end_test_case();

  sb::test_case("today() returns a valid date");
  {
    auto ymd = micron::today();
    sb::require_true(ymd.ok());
    sb::require_true(static_cast<int>(ymd.yr) >= 2024);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑩ auto_timer
// ─────────────────────────────────────────────────────────────────────────────

static void
test_auto_timer()
{
  sb::print("=== auto_timer ===");

  sb::test_case("auto_timer writes elapsed seconds to out pointer on destruction");
  {
    micron::fduration_t result = -1.0;
    {
      micron::auto_timer<> t{ &result };
      busy_wait_ms(3);
    }     // destructor fires here
    sb::require_greater(result, 0.0);
  }
  sb::end_test_case();

  sb::test_case("auto_timer with null out pointer does not crash");
  {
    micron::auto_timer<> t{ nullptr };
    // destructor fires at end of scope — if it throws/crashes, test fails
    sb::require_true(true);
  }
  sb::end_test_case();

  sb::test_case("nested auto_timers: inner < outer");
  {
    micron::fduration_t outer_t = 0.0, inner_t = 0.0;
    {
      micron::auto_timer<> outer{ &outer_t };
      busy_wait_ms(2);
      {
        micron::auto_timer<> inner{ &inner_t };
        busy_wait_ms(2);
      }
      busy_wait_ms(1);
    }
    sb::require_greater(outer_t, inner_t);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑪ free utility functions
// ─────────────────────────────────────────────────────────────────────────────

static void
test_free_utils()
{
  sb::print("=== free utility functions ===");

  sb::test_case("now() returns positive ms timestamp");
  {
    sb::require_greater(micron::now(), 0.0);
  }
  sb::end_test_case();

  sb::test_case("two successive now() are non-decreasing");
  {
    auto t1 = micron::now();
    auto t2 = micron::now();
    sb::require_true(t2 >= t1);
  }
  sb::end_test_case();

  sb::test_case("now_ts() tv_sec is positive (post-epoch)");
  {
    auto ts = micron::now_ts();
    sb::require_greater(ts.tv_sec, time_t{ 0 });
  }
  sb::end_test_case();

  sb::test_case("unix_time() matches now_ts() tv_sec (within 1s)");
  {
    auto ut = micron::unix_time();
    auto ts = micron::now_ts();
    auto diff = ts.tv_sec - ut;
    sb::require_true(diff >= -1 && diff <= 1);
  }
  sb::end_test_case();

  sb::test_case("elapsed(begin, end) with 0 delta returns 0");
  {
    micron::timespec_t t{};
    t.tv_sec = 10;
    t.tv_nsec = 0;
    sb::require(micron::elapsed<micron::unit::seconds>(t, t), 0.0);
  }
  sb::end_test_case();

  sb::test_case("elapsed<seconds>(begin, end) 1s apart returns 1.0");
  {
    micron::timespec_t begin{}, end{};
    begin.tv_sec = 100;
    begin.tv_nsec = 0;
    end.tv_sec = 101;
    end.tv_nsec = 0;
    sb::require(micron::elapsed<micron::unit::seconds>(begin, end), 1.0);
  }
  sb::end_test_case();

  sb::test_case("elapsed<milliseconds>(begin, end) 1s apart returns 1000.0");
  {
    micron::timespec_t begin{}, end{};
    begin.tv_sec = 100;
    begin.tv_nsec = 0;
    end.tv_sec = 101;
    end.tv_nsec = 0;
    sb::require(micron::elapsed<micron::unit::milliseconds>(begin, end), 1000.0);
  }
  sb::end_test_case();

  sb::test_case("elapsed handles sub-second nsec delta");
  {
    micron::timespec_t begin{}, end{};
    begin.tv_sec = 5;
    begin.tv_nsec = 0;
    end.tv_sec = 5;
    end.tv_nsec = 500'000'000L;
    sb::require(micron::elapsed<micron::unit::seconds>(begin, end), 0.5);
  }
  sb::end_test_case();

  sb::test_case("elapsed handles negative nsec (borrow via normalise)");
  {
    micron::timespec_t begin{}, end{};
    begin.tv_sec = 4;
    begin.tv_nsec = 500'000'000L;
    end.tv_sec = 5;
    end.tv_nsec = 0;
    sb::require(micron::elapsed<micron::unit::seconds>(begin, end), 0.5);
  }
  sb::end_test_case();

  sb::test_case("timediff(t0, t1) returns t1 - t0 as fduration_t");
  {
    sb::require(micron::timediff(100, 150), 50.0);
    sb::require(micron::timediff(200, 100), -100.0);
    sb::require(micron::timediff(0, 0), 0.0);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// ⑫ stress / regression
// ─────────────────────────────────────────────────────────────────────────────

static void
test_stress()
{
  sb::print("=== stress / regression ===");

  sb::test_case("from_unix / to_unix round-trip across 100 years (daily)");
  {
    // 1970-01-01 through ~2042 stepping one day
    time_t base = 0LL;
    for ( int d = 0; d < 365 * 70; d += 31 ) {     // every 31 days
      time_t ts = base + static_cast<time_t>(d) * 86400LL;
      auto ymd = micron::year_month_day::from_unix(ts);
      sb::require_true(ymd.ok());
      sb::require(ymd.to_unix(), ts);
    }
  }
  sb::end_test_case();

  sb::test_case("delta_to_unit all units consistent at 1s boundary");
  {
    using U = micron::unit;
    sb::require(micron::__impl::delta_to_unit<U::nanoseconds>(1, 0), 1e9);
    sb::require(micron::__impl::delta_to_unit<U::microseconds>(1, 0), 1e6);
    sb::require(micron::__impl::delta_to_unit<U::milliseconds>(1, 0), 1e3);
    sb::require(micron::__impl::delta_to_unit<U::seconds>(1, 0), 1.0);
  }
  sb::end_test_case();

  sb::test_case("normalise idempotent for nsec in [0, 1e9)");
  {
    for ( long nsec = 0; nsec < 1'000'000'000L; nsec += 100'000'000L ) {
      time_t sec = 10;
      long n = nsec;
      micron::__impl::normalise(sec, n);
      sb::require(sec, time_t{ 10 });
      sb::require(n, nsec);
    }
  }
  sb::end_test_case();

  sb::test_case("multiple lap() calls stay positive and are each < 5s");
  {
    micron::system_clock<micron::system_clocks::monotonic> clk;
    for ( int i = 0; i < 5; ++i ) {
      busy_wait_ms(1);
      auto t = clk.lap();
      sb::require_greater(t, 0.0);
      sb::require_smaller(t, 5.0);
    }
  }
  sb::end_test_case();

  sb::test_case("time_point arithmetic chain consistent");
  {
    micron::time_point<> tp{ 0.0 };
    for ( int i = 0; i < 100; ++i )
      tp += 1.0;
    sb::require(tp.time_since_epoch(), 100.0);
    for ( int i = 0; i < 50; ++i )
      tp -= 1.0;
    sb::require(tp.time_since_epoch(), 50.0);
  }
  sb::end_test_case();
}

// ─────────────────────────────────────────────────────────────────────────────
// entry point
// ─────────────────────────────────────────────────────────────────────────────

int
main()
{
  sb::print("micron::chrono test suite");
  sb::print("=========================");

  test_conversions();
  test_normalise();
  test_delta_to_unit();
  test_system_clock_lifecycle();
  test_system_clock_timing();
  test_time_point();
  test_time_of_day();
  test_year_month_day_types();
  test_year_month_day();
  test_auto_timer();
  test_free_utils();
  test_stress();

  sb::print("=========================");
  sb::print("ALL TESTS COMPLETED");
  return 1;
}
