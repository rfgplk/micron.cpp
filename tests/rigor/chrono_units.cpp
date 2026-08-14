//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- duration<Rep, Period> and the timespec arithmetic.
//
// C.10 is the regression here: __impl::normalise only ever handled tv_nsec < 0, which is right for
// the difference of two already-normalised timespecs and wrong for anything else. ts_normalise is
// the general one and has to floor in BOTH directions.

#include "../../src/chrono.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;

static void
test_duration_algebra(void)
{
  sb::print("=== duration<Rep, Period> ===");

  test_case("construction and count()");
  {
    ch::dur_ms a{ 1500 };
    require(a.count(), 1500L);
    require(ch::dur_s{}.count(), 0L);
    require(ch::dur_ns::zero().count(), 0L);
  }
  end_test_case();

  test_case("arithmetic");
  {
    ch::dur_ms a{ 1500 }, b{ 500 };
    require((a + b).count(), 2000L);
    require((a - b).count(), 1000L);
    require((a * 2L).count(), 3000L);
    require((2L * a).count(), 3000L);
    require((a / 2L).count(), 750L);
    require((a / b), 3L);
    require((a % b).count(), 0L);
    require((-a).count(), -1500L);
    require((+a).count(), 1500L);
    a += b;
    require(a.count(), 2000L);
    a -= b;
    require(a.count(), 1500L);
    ++a;
    require(a.count(), 1501L);
    --a;
    require(a.count(), 1500L);
  }
  end_test_case();

  test_case("ordering");
  {
    ch::dur_s a{ 1 }, b{ 2 }, c{ 1 };
    require_true(a < b);
    require_true(b > a);
    require_true(a <= c);
    require_true(a >= c);
    require_true(a == c);
    require_true(a != b);
    require_false(a == b);
  }
  end_test_case();

  test_case("duration_cast down is exact and truncates toward zero");
  {
    require(ch::duration_cast<ch::dur_s>(ch::dur_ms{ 1500 }).count(), 1L);
    require(ch::duration_cast<ch::dur_s>(ch::dur_ms{ 999 }).count(), 0L);
    require(ch::duration_cast<ch::dur_ms>(ch::dur_ns{ 1999999 }).count(), 1L);
    require(ch::duration_cast<ch::dur_min>(ch::dur_s{ 119 }).count(), 1L);
    require(ch::duration_cast<ch::dur_hr>(ch::dur_min{ 59 }).count(), 0L);
    require(ch::duration_cast<ch::dur_day>(ch::dur_hr{ 47 }).count(), 1L);
    require(ch::duration_cast<ch::dur_week>(ch::dur_day{ 13 }).count(), 1L);
  }
  end_test_case();

  test_case("duration_cast up is exact");
  {
    require(ch::duration_cast<ch::dur_ns>(ch::dur_s{ 1 }).count(), 1000000000L);
    require(ch::duration_cast<ch::dur_ms>(ch::dur_hr{ 1 }).count(), 3600000L);
    require(ch::duration_cast<ch::dur_s>(ch::dur_week{ 1 }).count(), 604800L);
    require(ch::duration_cast<ch::dur_us>(ch::dur_ms{ 7 }).count(), 7000L);
  }
  end_test_case();

  test_case("ns -> s does NOT overflow: the factor is gcd-reduced to a plain divide");
  {
    // a multiply-then-divide would need 9.2e18 * 1e9 first
    require(ch::duration_cast<ch::dur_s>(ch::dur_ns{ 9000000000000000000LL }).count(), 9000000000L);
  }
  end_test_case();

  test_case("a same-unit cast is the identity");
  {
    require(ch::duration_cast<ch::dur_ms>(ch::dur_ms{ 12345 }).count(), 12345L);
  }
  end_test_case();

  test_case("everything is constexpr");
  {
    static_assert(ch::dur_ms{ 1500 }.count() == 1500);
    static_assert(ch::duration_cast<ch::dur_s>(ch::dur_ms{ 2500 }).count() == 2);
    static_assert(ch::duration_cast<ch::dur_ns>(ch::dur_s{ 3 }).count() == 3000000000LL);
    require_true(true);
  }
  end_test_case();
}

static void
test_ts_normalise(void)
{
  sb::print("=== C.10 REGRESSION: ts_normalise, both directions ===");

  test_case("a nsec overflow carries into seconds");
  {
    micron::timespec_t t{};
    t.tv_sec = 5;
    t.tv_nsec = 1500000000;
    ch::ts_normalise(t);
    require(static_cast<long>(t.tv_sec), 6L);
    require(static_cast<long>(t.tv_nsec), 500000000L);
  }
  end_test_case();

  test_case("a large nsec overflow carries many seconds");
  {
    micron::timespec_t t{};
    t.tv_sec = 0;
    t.tv_nsec = 9500000000;
    ch::ts_normalise(t);
    require(static_cast<long>(t.tv_sec), 9L);
    require(static_cast<long>(t.tv_nsec), 500000000L);
  }
  end_test_case();

  test_case("a nsec borrow FLOORS rather than truncating toward zero");
  {
    micron::timespec_t t{};
    t.tv_sec = 5;
    t.tv_nsec = -1;
    ch::ts_normalise(t);
    require(static_cast<long>(t.tv_sec), 4L);
    require(static_cast<long>(t.tv_nsec), 999999999L);
  }
  end_test_case();

  test_case("a multi-second borrow");
  {
    micron::timespec_t t{};
    t.tv_sec = 0;
    t.tv_nsec = -2500000000;
    ch::ts_normalise(t);
    require(static_cast<long>(t.tv_sec), -3L);
    require(static_cast<long>(t.tv_nsec), 500000000L);
  }
  end_test_case();

  test_case("an already-normalised value is untouched");
  {
    for ( long ns = 0; ns < 1000000000L; ns += 7919L ) {
      micron::timespec_t t{};
      t.tv_sec = 3;
      t.tv_nsec = ns;
      ch::ts_normalise(t);
      require(static_cast<long>(t.tv_sec), 3L);
      require(static_cast<long>(t.tv_nsec), ns);
    }
  }
  end_test_case();

  test_case("tv_nsec always ends in [0, 1e9) whatever it started as");
  {
    u64 s = 0x2545F4914F6CDD1Dull;      // fixed seed
    for ( int i = 0; i < 50000; ++i ) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      micron::timespec_t t{};
      t.tv_sec = static_cast<time64_t>(static_cast<i64>(s >> 40) % 100000LL - 50000LL);
      t.tv_nsec = static_cast<decltype(t.tv_nsec)>(static_cast<i64>(s) % 20000000000LL - 10000000000LL);
      const i64 before = ch::ns_of_ts(t);
      ch::ts_normalise(t);
      require_true(t.tv_nsec >= 0 && t.tv_nsec < 1000000000);
      require(ch::ns_of_ts(t), before);      // normalising must not change the VALUE
    }
  }
  end_test_case();
}

static void
test_ts_convert(void)
{
  sb::print("=== timespec <-> scalar ===");

  test_case("ts_of_ns / ns_of_ts round-trip, both signs");
  {
    const i64 v[10] = { 0, 1, 999999999, 1000000000, 1000000001, -1, -999999999, -1000000000, 1768000000000000000LL, -86400000000000LL };
    for ( int i = 0; i < 10; ++i ) {
      const micron::timespec_t t = ch::ts_of_ns(v[i]);
      require_true(t.tv_nsec >= 0 && t.tv_nsec < 1000000000);
      require(ch::ns_of_ts(t), v[i]);
    }
  }
  end_test_case();

  test_case("ts_of_ms and ts_of_us");
  {
    const micron::timespec_t a = ch::ts_of_ms(1500);
    require(static_cast<long>(a.tv_sec), 1L);
    require(static_cast<long>(a.tv_nsec), 500000000L);
    const micron::timespec_t b = ch::ts_of_us(2500000);
    require(static_cast<long>(b.tv_sec), 2L);
    require(static_cast<long>(b.tv_nsec), 500000000L);
  }
  end_test_case();

  test_case("ms_of_ts / us_of_ts truncate");
  {
    const micron::timespec_t t = ch::ts_of_ns(1999999999);
    require(ch::ms_of_ts(t), 1999L);
    require(ch::us_of_ts(t), 1999999L);
  }
  end_test_case();

  test_case("ts_add / ts_sub carry and borrow");
  {
    const micron::timespec_t a = ch::ts_of_ns(1500000000);
    const micron::timespec_t b = ch::ts_of_ns(900000000);
    require(ch::ns_of_ts(ch::ts_add(a, b)), 2400000000LL);
    require(ch::ns_of_ts(ch::ts_sub(a, b)), 600000000LL);
    require(ch::ns_of_ts(ch::ts_sub(b, a)), -600000000LL);
  }
  end_test_case();

  test_case("ts_cmp orders correctly, including on the nanosecond tail alone");
  {
    const micron::timespec_t a = ch::ts_of_ns(1000000001);
    const micron::timespec_t b = ch::ts_of_ns(1000000002);
    require(static_cast<long>(ch::ts_cmp(a, b)), -1L);
    require(static_cast<long>(ch::ts_cmp(b, a)), 1L);
    require(static_cast<long>(ch::ts_cmp(a, a)), 0L);
    require_true(ch::ts_is_zero(ch::ts_of_ns(0)));
    require_false(ch::ts_is_zero(a));
  }
  end_test_case();

  test_case("ts_of / dur_of_ts bridge the duration type");
  {
    const ch::dur_ns d{ 1234567890 };
    require(ch::dur_of_ts(ch::ts_of(d)).count(), d.count());
  }
  end_test_case();

  test_case("random ns values survive the full round trip");
  {
    u64 s = 0xCBF29CE484222325ull;
    for ( int i = 0; i < 50000; ++i ) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      const i64 ns = static_cast<i64>(s >> 8) % 4000000000000000LL - 2000000000000000LL;
      require(ch::ns_of_ts(ch::ts_of_ns(ns)), ns);
    }
  }
  end_test_case();
}

static void
test_legacy_surface(void)
{
  sb::print("=== the f64 surface is unchanged ===");

  test_case("days/hours/minutes DIVIDE, milli/micro/nanoseconds MULTIPLY (both pinned)");
  {
    require(micron::days(86400.0), 1.0);
    require(micron::hours(3600.0), 1.0);
    require(micron::minutes(60.0), 1.0);
    require(micron::seconds(1.0), 1.0);
    require(micron::milliseconds(1.0), 1000.0);
    require(micron::microseconds(1.0), 1000000.0);
    require(micron::nanoseconds(1.0), 1000000000.0);
  }
  end_test_case();

  test_case("the ns constants agree with the f64 converters");
  {
    require(static_cast<long long>(ch::ns_per_s), 1000000000LL);
    require(static_cast<long long>(ch::ns_per_min), 60000000000LL);
    require(static_cast<long long>(ch::ns_per_hour), 3600000000000LL);
    require(static_cast<long long>(ch::ns_per_day), 86400000000000LL);
    require(static_cast<long long>(ch::ns_per_week), 604800000000000LL);
    require(ch::sec_per_week, 604800L);
  }
  end_test_case();

  test_case("__impl::normalise still borrows exactly as it did");
  {
    micron::time_t sec = 5;
    long nsec = -1;
    micron::__impl::normalise(sec, nsec);
    require(static_cast<long>(sec), 4L);
    require(nsec, 999999999L);
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::chrono units suite");
  sb::print("==========================");
  test_duration_algebra();
  test_ts_normalise();
  test_ts_convert();
  test_legacy_surface();
  sb::print("==========================");
  sb::print("ALL UNITS TESTS COMPLETED");
  return 1;
}
