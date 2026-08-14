//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- the calendar and civil layer.
//
// The oracle is a deliberately stupid one: it walks the calendar a year and a month at a time from
// 1970 rather than sharing any algebra with Hinnant's civil_from_days. Two implementations that
// agree by construction would test nothing.
//
// The regressions this file exists for:
//   C.1  year_month_day::from_unix was off by a day for EVERY pre-1970 instant (C division
//        truncates toward zero, so -1 / 86400 == 0 and the 31st of December read as the 1st of
//        January)
//   C.9  year_month_day::ok() accepted 2026-02-30, and to_unix() NORMALISES, so an unvalidated
//        date silently answered a DIFFERENT valid date rather than failing

#include "../../src/chrono.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the oracle

static constexpr bool
o_leap(int y)
{
  return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static constexpr int
o_mdays(int y, int m)
{
  constexpr int t[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return m == 2 ? (o_leap(y) ? 29 : 28) : t[m];
}

struct odate {
  int y;
  int m;
  int d;
};

// walk day by day, year by year, from 1970-01-01. Handles negatives by walking backwards
static odate
o_from_days(long z)
{
  int y = 1970, m = 1, d = 1;
  while ( z > 0 ) {
    const long yl = o_leap(y) ? 366 : 365;
    if ( m == 1 && d == 1 && z >= yl ) {
      z -= yl;
      ++y;
      continue;
    }
    const int md = o_mdays(y, m);
    if ( d == 1 && z >= md ) {
      z -= md;
      if ( ++m > 12 ) {
        m = 1;
        ++y;
      }
      continue;
    }
    --z;
    if ( ++d > o_mdays(y, m) ) {
      d = 1;
      if ( ++m > 12 ) {
        m = 1;
        ++y;
      }
    }
  }
  while ( z < 0 ) {
    if ( m == 1 && d == 1 ) {
      const long pl = o_leap(y - 1) ? 366 : 365;
      if ( -z >= pl ) {
        z += pl;
        --y;
        continue;
      }
    }
    ++z;
    if ( --d < 1 ) {
      if ( --m < 1 ) {
        m = 12;
        --y;
      }
      d = o_mdays(y, m);
    }
  }
  return odate{ y, m, d };
}

static long
o_to_days(int y, int m, int d)
{
  long z = 0;
  if ( y >= 1970 ) {
    for ( int yy = 1970; yy < y; ++yy ) z += o_leap(yy) ? 366 : 365;
  } else {
    for ( int yy = y; yy < 1970; ++yy ) z -= o_leap(yy) ? 366 : 365;
  }
  for ( int mm = 1; mm < m; ++mm ) z += o_mdays(y, mm);
  return z + (d - 1);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

static void
test_days_roundtrip(void)
{
  sb::print("=== civil_from_days / days_from_civil vs oracle ===");

  test_case("every day from 1965-01-01 to 1985-01-01 matches the oracle (spans the epoch)");
  {
    for ( long z = -1826; z <= 5479; ++z ) {
      const auto c = ch::civil_from_days(z);
      const odate o = o_from_days(z);
      require(static_cast<long>(c.y), static_cast<long>(o.y));
      require(static_cast<long>(c.m), static_cast<long>(o.m));
      require(static_cast<long>(c.d), static_cast<long>(o.d));
      require(ch::days_from_civil(c.y, c.m, c.d), z);
    }
  }
  end_test_case();

  test_case("days_from_civil is the exact inverse over 1600..2400 (monthly)");
  {
    for ( int y = 1600; y <= 2400; ++y ) {
      for ( unsigned m = 1; m <= 12; ++m ) {
        const long z = ch::days_from_civil(y, m, 1);
        require(z, o_to_days(y, static_cast<int>(m), 1));
        const auto c = ch::civil_from_days(z);
        require(static_cast<long>(c.y), static_cast<long>(y));
        require(static_cast<long>(c.m), static_cast<long>(m));
        require(static_cast<long>(c.d), 1L);
      }
    }
  }
  end_test_case();

  test_case("the last day of every month round-trips over 1900..2200");
  {
    for ( int y = 1900; y <= 2200; ++y ) {
      for ( unsigned m = 1; m <= 12; ++m ) {
        const unsigned d = ch::days_in_month(y, m);
        require(static_cast<long>(d), static_cast<long>(o_mdays(y, static_cast<int>(m))));
        const auto c = ch::civil_from_days(ch::days_from_civil(y, m, d));
        require(static_cast<long>(c.y), static_cast<long>(y));
        require(static_cast<long>(c.m), static_cast<long>(m));
        require(static_cast<long>(c.d), static_cast<long>(d));
      }
    }
  }
  end_test_case();
}

static void
test_pre_1970(void)
{
  sb::print("=== C.1 REGRESSION: pre-1970 instants ===");

  test_case("from_unix(-1) is 1969-12-31, not 1970-01-01");
  {
    const auto ymd = micron::year_month_day::from_unix(-1);
    require(static_cast<int>(ymd.yr), 1969);
    require(static_cast<unsigned>(ymd.mo), 12u);
    require(static_cast<unsigned>(ymd.dy), 31u);
  }
  end_test_case();

  test_case("every second of 1969-12-31 maps to 1969-12-31");
  {
    for ( long s = -86400; s <= -1; ++s ) {
      const auto ymd = micron::year_month_day::from_unix(static_cast<micron::time_t>(s));
      require(static_cast<int>(ymd.yr), 1969);
      require(static_cast<unsigned>(ymd.mo), 12u);
      require(static_cast<unsigned>(ymd.dy), 31u);
    }
  }
  end_test_case();

  test_case("from_unix(-86400) is 1969-12-31 and from_unix(-86401) is 1969-12-30");
  {
    auto a = micron::year_month_day::from_unix(-86400);
    require(static_cast<unsigned>(a.dy), 31u);
    auto b = micron::year_month_day::from_unix(-86401);
    require(static_cast<unsigned>(b.dy), 30u);
  }
  end_test_case();

  test_case("negative-epoch to_civil floors: 1969-12-31T23:59:59 for -1");
  {
    const ch::civil c = ch::civil_utc(-1);
    require(c.y, 1969);
    require(c.mo, 12u);
    require(c.d, 31u);
    require(c.h, 23u);
    require(c.mi, 59u);
    require(c.s, 59u);
  }
  end_test_case();

  test_case("civil_secs round-trips every hour across the epoch boundary");
  {
    for ( long s = -172800; s <= 172800; s += 3600 ) {
      const ch::civil c = ch::civil_utc(s);
      require(ch::civil_secs(c), s);
    }
  }
  end_test_case();
}

static void
test_validation(void)
{
  sb::print("=== C.9 REGRESSION: date validation ===");

  test_case("2026-02-30 is rejected");
  {
    micron::year_month_day ymd{ micron::year{ 2026 }, micron::month{ 2 }, micron::day{ 30 } };
    require_false(ymd.ok());
    require_false(ch::civil_valid(2026, 2, 30));
  }
  end_test_case();

  test_case("2026-02-28 accepted, 2026-02-29 rejected (not a leap year)");
  {
    require_true(ch::civil_valid(2026, 2, 28));
    require_false(ch::civil_valid(2026, 2, 29));
  }
  end_test_case();

  test_case("2024-02-29 accepted (leap year)");
  {
    require_true(ch::civil_valid(2024, 2, 29));
    micron::year_month_day ymd{ micron::year{ 2024 }, micron::month{ 2 }, micron::day{ 29 } };
    require_true(ymd.ok());
  }
  end_test_case();

  test_case("the century rules: 1900 and 2100 are not leap, 2000 is");
  {
    require_false(ch::is_leap(1900));
    require_false(ch::is_leap(2100));
    require_true(ch::is_leap(2000));
    require_false(ch::civil_valid(1900, 2, 29));
    require_true(ch::civil_valid(2000, 2, 29));
  }
  end_test_case();

  test_case("every 31-day spelling of a 30-day month is rejected");
  {
    const unsigned short_months[4] = { 4, 6, 9, 11 };
    for ( unsigned i = 0; i < 4; ++i ) {
      require_false(ch::civil_valid(2026, short_months[i], 31));
      require_true(ch::civil_valid(2026, short_months[i], 30));
    }
  }
  end_test_case();

  test_case("month 0 and month 13 are rejected");
  {
    require_false(ch::civil_valid(2026, 0, 1));
    require_false(ch::civil_valid(2026, 13, 1));
    require(static_cast<long>(ch::days_in_month(2026, 0)), 0L);
    require(static_cast<long>(ch::days_in_month(2026, 13)), 0L);
  }
  end_test_case();

  test_case("day 0 is rejected");
  {
    require_false(ch::civil_valid(2026, 1, 0));
  }
  end_test_case();
}

static void
test_known_instants(void)
{
  sb::print("=== known instants ===");

  test_case("the 2038 signed-32-bit boundary");
  {
    // 2038-01-19T03:14:07Z == 2147483647
    ch::civil c{};
    c.y = 2038;
    c.mo = 1;
    c.d = 19;
    c.h = 3;
    c.mi = 14;
    c.s = 7;
    require(ch::civil_secs(c), 2147483647L);
    const ch::civil back = ch::civil_utc(2147483647L);
    require_true(back == c);
  }
  end_test_case();

  test_case("the epoch itself");
  {
    const ch::civil c = ch::civil_utc(0);
    require(c.y, 1970);
    require(c.mo, 1u);
    require(c.d, 1u);
    require(c.h, 0u);
    require(ch::civil_secs(c), 0L);
  }
  end_test_case();

  test_case("Y2K");
  {
    const ch::civil c = ch::civil_utc(946684800L);
    require(c.y, 2000);
    require(c.mo, 1u);
    require(c.d, 1u);
  }
  end_test_case();

  test_case("2026-01-09T23:06:40Z == 1768000000 (cross-checked against GNU date)");
  {
    const ch::civil c = ch::civil_utc(1768000000L);
    require(c.y, 2026);
    require(c.mo, 1u);
    require(c.d, 9u);
    require(c.h, 23u);
    require(c.mi, 6u);
    require(c.s, 40u);
  }
  end_test_case();
}

static void
test_weekday_and_ordinals(void)
{
  sb::print("=== weekday / day-of-year / ISO week ===");

  test_case("1970-01-01 was a Thursday");
  {
    require(static_cast<long>(ch::weekday_of(1970, 1, 1)), 4L);
    require(static_cast<long>(ch::iso_weekday_of(1970, 1, 1)), 4L);
  }
  end_test_case();

  test_case("2026-01-09 was a Friday, day 9, ISO week 2 (cross-checked against GNU date)");
  {
    require(static_cast<long>(ch::weekday_of(2026, 1, 9)), 5L);
    require(static_cast<long>(ch::day_of_year(2026, 1, 9)), 9L);
    const auto w = ch::iso_week_date(2026, 1, 9);
    require(static_cast<long>(w.week), 2L);
    require(static_cast<long>(w.year), 2026L);
    require(static_cast<long>(w.weekday), 5L);
  }
  end_test_case();

  test_case("weekday advances by exactly one per day over four years");
  {
    long z = ch::days_from_civil(2024, 1, 1);
    unsigned prev = ch::weekday_from_days(z);
    for ( long i = 1; i < 1461; ++i ) {
      const unsigned w = ch::weekday_from_days(z + i);
      require(static_cast<long>(w), static_cast<long>((prev + 1u) % 7u));
      prev = w;
    }
  }
  end_test_case();

  test_case("weekday is well-defined before the epoch too");
  {
    // 1969-12-31 was a Wednesday
    require(static_cast<long>(ch::weekday_of(1969, 12, 31)), 3L);
    // 1900-01-01 was a Monday
    require(static_cast<long>(ch::weekday_of(1900, 1, 1)), 1L);
  }
  end_test_case();

  test_case("day_of_year runs 1..365 / 1..366 with no gaps");
  {
    for ( int y = 1998; y <= 2006; ++y ) {
      unsigned expect = 1;
      for ( unsigned m = 1; m <= 12; ++m ) {
        for ( unsigned d = 1; d <= ch::days_in_month(y, m); ++d ) {
          require(static_cast<long>(ch::day_of_year(y, m, d)), static_cast<long>(expect));
          ++expect;
        }
      }
      require(static_cast<long>(expect - 1), static_cast<long>(ch::days_in_year(y)));
    }
  }
  end_test_case();

  test_case("ISO week: the turn of the year, where the ISO year differs from the calendar year");
  {
    // 2027-01-01 is a Friday, so it belongs to ISO week 53 of 2026
    const auto a = ch::iso_week_date(2027, 1, 1);
    require(static_cast<long>(a.year), 2026L);
    require(static_cast<long>(a.week), 53L);
    // 2029-12-31 is a Monday, which starts ISO week 1 of 2030
    const auto b = ch::iso_week_date(2029, 12, 31);
    require(static_cast<long>(b.year), 2030L);
    require(static_cast<long>(b.week), 1L);
  }
  end_test_case();

  test_case("ISO week number stays inside 1..53 across three centuries");
  {
    for ( int y = 1900; y <= 2200; ++y ) {
      for ( unsigned m = 1; m <= 12; ++m ) {
        const auto w = ch::iso_week_date(y, m, 1);
        require_true(w.week >= 1 && w.week <= 53);
        require_true(w.weekday >= 1 && w.weekday <= 7);
      }
    }
  }
  end_test_case();
}

static void
test_offsets(void)
{
  sb::print("=== utc offsets ===");

  test_case("to_civil applies a positive offset");
  {
    // 1768000000 is 2026-01-09T23:06:40Z; at +02:00 that is the 10th at 01:06:40
    const ch::civil c = ch::to_civil(1768000000L, 7200);
    require(c.d, 10u);
    require(c.h, 1u);
    require(c.mi, 6u);
    require(c.off, 7200);
  }
  end_test_case();

  test_case("to_civil applies a negative offset across midnight backwards");
  {
    const ch::civil c = ch::to_civil(1768000000L, -19800);      // -05:30
    require(c.d, 9u);
    require(c.h, 17u);
    require(c.mi, 36u);
  }
  end_test_case();

  test_case("civil_secs minus the offset recovers the instant");
  {
    for ( int off = -50400; off <= 50400; off += 1800 ) {
      const ch::civil c = ch::to_civil(1768000000L, off);
      require(ch::civil_secs(c) - static_cast<long>(c.off), 1768000000L);
    }
  }
  end_test_case();

  test_case("offset_hours / offset_minutes decompose both signs");
  {
    require(static_cast<long>(ch::offset_hours(7200)), 2L);
    require(static_cast<long>(ch::offset_minutes(7200)), 0L);
    require(static_cast<long>(ch::offset_minutes(-19800)), 30L);
    require_true(ch::offset_negative(-19800));
    require_false(ch::offset_negative(7200));
  }
  end_test_case();

  test_case("a fixed zone table answers its offset everywhere");
  {
    const ch::tz_table z = ch::tz_fixed(3600);
    require(static_cast<long>(ch::offset_at(z, 0)), 3600L);
    require(static_cast<long>(ch::offset_at(z, -1000000000L)), 3600L);
    require(static_cast<long>(ch::offset_at(ch::tz_utc, 1768000000L)), 0L);
  }
  end_test_case();
}

static void
test_epochs(void)
{
  sb::print("=== other epochs ===");

  test_case("NTP: the epoch is 2208988800 seconds after 1900");
  {
    require(ch::epoch::ntp_from_unix(0), 2208988800L);
    require(ch::epoch::unix_from_ntp(2208988800L), 0L);
    for ( long s = -1000; s <= 1000; s += 37 ) require(ch::epoch::unix_from_ntp(ch::epoch::ntp_from_unix(s)), s);
  }
  end_test_case();

  test_case("NTP fixed-point fraction round-trips to the nanosecond floor");
  {
    const u64 ts = ch::epoch::ntp_timestamp(0, 500000000u);
    require(static_cast<long>(ch::epoch::nsec_from_ntp_timestamp(ts)), 500000000L);
    require(ch::epoch::unix_sec_from_ntp_timestamp(ts), 0L);
  }
  end_test_case();

  test_case("Windows FILETIME: the epoch is 116444736000000000 ticks");
  {
    require(ch::epoch::filetime_from_unix(0), 116444736000000000L);
    require(ch::epoch::unix_from_filetime(116444736000000000L), 0L);
    require(static_cast<long>(ch::epoch::nsec_from_filetime(ch::epoch::filetime_from_unix(0, 999999900u))), 999999900L);
    for ( long s = -100000; s <= 100000; s += 7777 ) require(ch::epoch::unix_from_filetime(ch::epoch::filetime_from_unix(s)), s);
  }
  end_test_case();

  test_case("julian: MJD 40587 and JDN 2440588 at the epoch");
  {
    require(ch::epoch::mjd(0), 40587L);
    require(ch::epoch::jdn(0), 2440588L);
    // the julian day number turns over at NOON, not midnight
    require(ch::epoch::jdn(43199), 2440588L);
    require(ch::epoch::jdn(43200), 2440589L);
    require(ch::epoch::unix_from_mjd(40587L), 0L);
  }
  end_test_case();

  test_case("GPS: week/tow round-trips");
  {
    for ( long s = 400000000L; s < 400000000L + 4000000L; s += 99991L ) {
      const auto g = ch::epoch::gps_week_tow(s);
      require_true(g.tow >= 0 && g.tow < 604800);
      require(ch::epoch::unix_from_gps_week_tow(g.week, g.tow), s);
    }
  }
  end_test_case();

  test_case("TAI is unix plus the leap count");
  {
    require(ch::epoch::tai_from_unix(0, 37), 37L);
    require(ch::epoch::unix_from_tai(37, 37), 0L);
  }
  end_test_case();

  test_case("DOS date-time round-trips at two-second granularity");
  {
    ch::civil c{};
    c.y = 2026;
    c.mo = 8;
    c.d = 14;
    c.h = 14;
    c.mi = 22;
    c.s = 10;
    const auto d = ch::epoch::dos_from_civil(c);
    const ch::civil b = ch::epoch::civil_from_dos(d);
    require(b.y, 2026);
    require(b.mo, 8u);
    require(b.d, 14u);
    require(b.h, 14u);
    require(b.mi, 22u);
    require(b.s, 10u);
  }
  end_test_case();

  test_case("DOS refuses anything it cannot name");
  {
    ch::civil c{};
    c.y = 1979;
    const auto d = ch::epoch::dos_from_civil(c);
    require(static_cast<long>(d.date), 0L);
    require(static_cast<long>(d.time), 0L);
  }
  end_test_case();
}

static void
test_seeded_fuzz(void)
{
  sb::print("=== seeded fuzz ===");

  test_case("100k pseudo-random instants round-trip through civil and back");
  {
    // fixed seed, never time-based
    u64 s = 0x9E3779B97F4A7C15ull;
    for ( int i = 0; i < 100000; ++i ) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      // +/- ~300 years around the epoch
      const long inst = static_cast<long>(static_cast<i64>(s >> 20) % 19000000000LL) - 9500000000LL;
      const ch::civil c = ch::civil_utc(inst);
      require_true(ch::civil_valid(c.y, c.mo, c.d));
      require_true(c.h < 24 && c.mi < 60 && c.s < 60);
      require(ch::civil_secs(c), inst);
      const odate o = o_from_days(ch::floor_div(inst, 86400));
      require(static_cast<long>(c.y), static_cast<long>(o.y));
      require(static_cast<long>(c.mo), static_cast<long>(o.m));
      require(static_cast<long>(c.d), static_cast<long>(o.d));
    }
  }
  end_test_case();

  test_case("floor_div / floor_mod agree with the mathematical definition");
  {
    u64 s = 0xD1B54A32D192ED03ull;
    for ( int i = 0; i < 20000; ++i ) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      const long a = static_cast<long>(static_cast<i64>(s >> 16) % 2000001LL) - 1000000LL;
      const long b = 86400;
      const long q = ch::floor_div(a, b);
      const long r = ch::floor_mod(a, b);
      require(q * b + r, a);
      require_true(r >= 0 && r < b);
    }
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::chrono civil / calendar suite");
  sb::print("=====================================");
  test_days_roundtrip();
  test_pre_1970();
  test_validation();
  test_known_instants();
  test_weekday_and_ordinals();
  test_offsets();
  test_epochs();
  test_seeded_fuzz();
  sb::print("=====================================");
  sb::print("ALL CIVIL TESTS COMPLETED");
  return 1;
}
