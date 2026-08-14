//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- the C.2 regression, and the optimisation-level gate for it.
//
// time_of_day(fduration_t) took `hours = (total/3600) % 24` and then subtracted only hours*3600
// from the running total, so the whole-day part was never removed: time_of_day(90000.0) answered
// 1 hour and 1440 MINUTES. It is decomposed on integers now.
//
// WHY THE VOLATILE ROUND TRIP: every value here goes through a noinline opaque() first. Feeding
// literals lets the compiler fold the whole decomposition at compile time, where the arithmetic is
// exact by construction and the test sees nothing -- which is exactly why tests/rigor/math.cpp
// could not see the -Ofast float-classification bugs that float_classify.cpp exists to catch.
//
// BUILD THIS AT -O0 AND AT -Ofast. duck defaults to -Ofast, which turns on -freciprocal-math and
// -ffinite-math-only; a division ladder is precisely the shape those transform.

#include "../../src/chrono.hpp"
// NOTE: the tick<->ns conversion lives in the measurement tier, which the umbrella deliberately
// does not pull in -- ordinary code should not pay for inline asm and a cpuid probe
#include "../../src/chrono/calibrate.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;

// the compiler cannot see through this, so every decomposition below happens at RUNTIME
[[gnu::noinline]] static micron::fduration_t
opaque(micron::fduration_t v)
{
  volatile micron::fduration_t s = v;
  return s;
}

[[gnu::noinline]] static long
opaque_l(long v)
{
  volatile long s = v;
  return s;
}

static void
test_day_overflow(void)
{
  sb::print("=== C.2 REGRESSION: whole days are removed before the split ===");

  test_case("time_of_day(90000) is 01:00:00, not 1h 1440m");
  {
    micron::time_of_day t(opaque(90000.0));
    require(static_cast<long>(t.hours()), 1L);
    require(static_cast<long>(t.minutes()), 0L);
    require(static_cast<long>(t.seconds()), 0L);
  }
  end_test_case();

  test_case("minutes and seconds stay in range for any input at all");
  {
    for ( long s = 0; s < 400000; s += 37 ) {
      micron::time_of_day t(opaque(static_cast<micron::fduration_t>(s)));
      require_true(t.hours() >= 0.0 && t.hours() < 24.0);
      require_true(t.minutes() >= 0.0 && t.minutes() < 60.0);
      require_true(t.seconds() >= 0.0 && t.seconds() < 60.0);
    }
  }
  end_test_case();

  test_case("input at or past a day wraps like a wall clock");
  {
    const long cases[6][2] = { { 86400, 0 }, { 86401, 1 }, { 90000, 3600 }, { 172800, 0 }, { 172801, 1 }, { 1000000, 1000000 % 86400 } };
    for ( int i = 0; i < 6; ++i ) {
      micron::time_of_day t(opaque(static_cast<micron::fduration_t>(cases[i][0])));
      const long want = cases[i][1];
      require(static_cast<long>(t.hours()), want / 3600);
      require(static_cast<long>(t.minutes()), (want / 60) % 60);
      require(static_cast<long>(t.seconds()), want % 60);
    }
  }
  end_test_case();

  test_case("negative input floors into [0, 86400) rather than producing garbage");
  {
    micron::time_of_day a(opaque(-1.0));
    require(static_cast<long>(a.hours()), 23L);
    require(static_cast<long>(a.minutes()), 59L);
    require(static_cast<long>(a.seconds()), 59L);
    micron::time_of_day b(opaque(-0.5));
    require(static_cast<long>(b.hours()), 23L);
    require(static_cast<long>(b.seconds()), 59L);
    require_true(b.subseconds() > 0.49 && b.subseconds() < 0.51);
    micron::time_of_day c(opaque(-86400.0));
    require(static_cast<long>(c.hours()), 0L);
    require(static_cast<long>(c.minutes()), 0L);
    require(static_cast<long>(c.seconds()), 0L);
  }
  end_test_case();
}

static void
test_exhaustive_day(void)
{
  sb::print("=== every second of a day, at RUNTIME ===");

  test_case("all 86400 decompositions match integer arithmetic");
  {
    long bad = 0;
    for ( long s = 0; s < 86400; ++s ) {
      micron::time_of_day t(opaque(static_cast<micron::fduration_t>(s)));
      if ( static_cast<long>(t.hours()) != s / 3600 || static_cast<long>(t.minutes()) != (s / 60) % 60
           || static_cast<long>(t.seconds()) != s % 60 )
        ++bad;
    }
    require(bad, 0L);
  }
  end_test_case();

  test_case("to_duration() round-trips every second of a day");
  {
    for ( long s = 0; s < 86400; s += 7 ) {
      micron::time_of_day t(opaque(static_cast<micron::fduration_t>(s)));
      require(static_cast<long>(t.to_duration()), s);
    }
  }
  end_test_case();

  test_case("sub-second values survive the decomposition");
  {
    micron::time_of_day t(opaque(7384.25));
    require(static_cast<long>(t.hours()), 2L);
    require(static_cast<long>(t.minutes()), 3L);
    require(static_cast<long>(t.seconds()), 4L);
    require_true(t.subseconds() > 0.24 && t.subseconds() < 0.26);
    require_true(t.to_duration() > 7384.24 && t.to_duration() < 7384.26);
  }
  end_test_case();
}

static void
test_civil_runtime(void)
{
  sb::print("=== the integer civil layer, at RUNTIME ===");

  test_case("every hour of a year decomposes and recomposes exactly");
  {
    for ( long t = 1735689600L; t < 1767225600L; t += 3600L ) {
      const ch::civil c = ch::civil_utc(opaque_l(t));
      require(ch::civil_secs(c), t);
      require_true(c.h < 24 && c.mi < 60 && c.s < 60);
    }
  }
  end_test_case();

  test_case("the ns <-> timespec ladder is exact at runtime too");
  {
    for ( long ns = -2000000000L; ns <= 2000000000L; ns += 7919L ) {
      const micron::timespec_t ts = ch::ts_of_ns(opaque_l(ns));
      require(ch::ns_of_ts(ts), ns);
    }
  }
  end_test_case();

  test_case("cycles_to_ns / ns_to_cycles round-trip within the floor's own bound");
  {
    // NOTE: both directions FLOOR, so the round trip loses ceil(r / 1e9) ticks where r < hz is the
    // first floor's remainder. Above a gigahertz that is more than one tick -- four at 3.2 GHz --
    // and asserting "within 1" would be asserting something arithmetically false
    const u64 hz[5] = { 1000000000ull, 2300000000ull, 3200000000ull, 24000000ull, 100ull };
    for ( int i = 0; i < 5; ++i ) {
      const u64 tol = (hz[i] + 999999999ull) / 1000000000ull;
      for ( u64 c = 0; c < 10000000ull; c += 999983ull ) {
        const u64 ns = ch::cycles_to_ns(c, hz[i]);
        const u64 back = ch::ns_to_cycles(ns, hz[i]);
        require_true(back <= c);
        require_true(c - back <= tol);
      }
    }
  }
  end_test_case();

  test_case("cycles_to_ns is an EXACT floor, checked against a 128-bit oracle");
  {
    const u64 hz[4] = { 1000000000ull, 2300000000ull, 3200000000ull, 24000000ull };
    u64 s = 0x9E3779B97F4A7C15ull;
    for ( int i = 0; i < 4; ++i ) {
      for ( int k = 0; k < 20000; ++k ) {
        s = s * 6364136223846793005ull + 1442695040888963407ull;
        const u64 c = s >> 20;      // up to ~1.7e13 ticks, well past what f64 keeps exact
        const uint128_t want = (static_cast<uint128_t>(c) * 1000000000ull) / hz[i];
        require(ch::cycles_to_ns(c, hz[i]), static_cast<u64>(want));
      }
    }
  }
  end_test_case();

  test_case("a zero frequency answers 0 rather than dividing by it");
  {
    require(ch::cycles_to_ns(12345, 0), 0ull);
    require(ch::cycles_to_us(12345, 0), 0ull);
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::chrono -Ofast regression suite");
  sb::print("======================================");
  test_day_overflow();
  test_exhaustive_day();
  test_civil_runtime();
  sb::print("======================================");
  sb::print("ALL -Ofast REGRESSION TESTS COMPLETED");
  return 1;
}
