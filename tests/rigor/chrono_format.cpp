//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- the timestamp and duration writers.
//
// Every writer answers the byte count on success and 0 when the result does not fit, with a
// matching X_size() reporting the requirement. Both halves of that contract are tested here: the
// exact bytes, AND that a buffer one short refuses rather than truncating.
//
// The reference strings were cross-checked against GNU date(1) / coreutils.

#include "../../src/chrono.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;

static bool
str_eq(const char *buf, usize n, const char *want)
{
  usize k = 0;
  while ( want[k] ) ++k;
  if ( k != n ) return false;
  for ( usize i = 0; i < n; ++i )
    if ( buf[i] != want[i] ) return false;
  return true;
}

// writes into `buf`, checks the bytes, checks X_size() agrees, and checks that one byte less
// refuses outright instead of writing a short answer
#define CHECK_W(expr_write, expr_size, want)                                                                                               \
  do {                                                                                                                                     \
    char __b[256];                                                                                                                         \
    for ( usize __i = 0; __i < sizeof(__b); ++__i ) __b[__i] = '\xAB';                                                                      \
    const usize __n = (expr_write)(__b, sizeof(__b));                                                                                       \
    require_true(str_eq(__b, __n, (want)));                                                                                                 \
    require(static_cast<long>((expr_size)()), static_cast<long>(__n));                                                                      \
    if ( __n > 0 ) {                                                                                                                        \
      char __s[256];                                                                                                                       \
      for ( usize __i = 0; __i < sizeof(__s); ++__i ) __s[__i] = '\xAB';                                                                    \
      require(static_cast<long>((expr_write)(__s, __n - 1)), 0L);                                                                           \
      /* refusing means writing nothing past what it was given */                                                                           \
      require_true(__s[__n - 1] == '\xAB');                                                                                                 \
    }                                                                                                                                       \
  } while ( 0 )

static ch::civil
mk(i32 y, u32 mo, u32 d, u32 h, u32 mi, u32 s, u32 ns = 0, i32 off = 0)
{
  ch::civil c{};
  c.y = y;
  c.mo = mo;
  c.d = d;
  c.h = h;
  c.mi = mi;
  c.s = s;
  c.ns = ns;
  c.off = off;
  return c;
}

static void
test_iso(void)
{
  sb::print("=== ISO 8601 / RFC 3339 ===");

  // 1768000000 == 2026-01-09T23:06:40Z (GNU date)
  const ch::civil utc = ch::civil_utc(1768000000L);

  test_case("rfc3339 at UTC uses Z");
  {
    CHECK_W([&](char *b, usize c) { return ch::write_rfc3339(b, c, utc, 0); }, [&]() { return ch::rfc3339_size(utc, 0); },
            "2026-01-09T23:06:40Z");
  }
  end_test_case();

  test_case("rfc3339 with a positive offset uses +HH:MM");
  {
    const ch::civil c = ch::to_civil(1768000000L, 7200);
    CHECK_W([&](char *b, usize k) { return ch::write_rfc3339(b, k, c, 0); }, [&]() { return ch::rfc3339_size(c, 0); },
            "2026-01-10T01:06:40+02:00");
  }
  end_test_case();

  test_case("rfc3339 with a negative half-hour offset");
  {
    const ch::civil c = ch::to_civil(1768000000L, -19800);
    CHECK_W([&](char *b, usize k) { return ch::write_rfc3339(b, k, c, 0); }, [&]() { return ch::rfc3339_size(c, 0); },
            "2026-01-09T17:36:40-05:30");
  }
  end_test_case();

  test_case("sub-second digits TRUNCATE, they do not round");
  {
    ch::civil c = utc;
    c.ns = 123456789u;
    CHECK_W([&](char *b, usize k) { return ch::write_rfc3339(b, k, c, 3); }, [&]() { return ch::rfc3339_size(c, 3); },
            "2026-01-09T23:06:40.123Z");
    CHECK_W([&](char *b, usize k) { return ch::write_rfc3339(b, k, c, 9); }, [&]() { return ch::rfc3339_size(c, 9); },
            "2026-01-09T23:06:40.123456789Z");
    // 999999999 must stay 999 at three digits rather than rounding up to 1.000
    c.ns = 999999999u;
    CHECK_W([&](char *b, usize k) { return ch::write_rfc3339(b, k, c, 3); }, [&]() { return ch::rfc3339_size(c, 3); },
            "2026-01-09T23:06:40.999Z");
  }
  end_test_case();

  test_case("a shorter sub-second rendering is a PREFIX of a longer one");
  {
    ch::civil c = utc;
    c.ns = 507000000u;
    char a[64], b[64];
    const usize na = ch::write_rfc3339(a, sizeof(a), c, 1);
    const usize nb = ch::write_rfc3339(b, sizeof(b), c, 6);
    require_true(na > 0 && nb > 0);
    // compare up to the '.' plus one digit
    usize dot = 0;
    while ( dot < na && a[dot] != '.' ) ++dot;
    for ( usize i = 0; i <= dot + 1 && i < na; ++i ) require_true(a[i] == b[i]);
  }
  end_test_case();

  test_case("the ls --full-time shape: space separator, compact offset, always present");
  {
    ch::civil c = ch::to_civil(1768000000L, 7200);
    c.ns = 123456789u;
    CHECK_W([&](char *b, usize k) { return ch::write_iso_full(b, k, c, 9); }, [&]() { return ch::iso_full_size(c, 9); },
            "2026-01-10 01:06:40.123456789+0200");
  }
  end_test_case();

  test_case("iso_full at UTC still prints +0000, never Z");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_iso_full(b, k, utc, 0); }, [&]() { return ch::iso_full_size(utc, 0); },
            "2026-01-09 23:06:40+0000");
  }
  end_test_case();

  test_case("years outside 0..9999 take the explicitly signed expanded form");
  {
    const ch::civil a = mk(12345, 6, 7, 8, 9, 10);
    CHECK_W([&](char *b, usize k) { return ch::write_iso8601(b, k, a, 0, ch::date_sep::iso_t, ch::offset_style::none); },
            [&]() { return ch::iso8601_size(a, 0, ch::date_sep::iso_t, ch::offset_style::none); }, "+12345-06-07T08:09:10");
    const ch::civil b2 = mk(-44, 3, 15, 12, 0, 0);
    CHECK_W([&](char *b, usize k) { return ch::write_iso8601(b, k, b2, 0, ch::date_sep::iso_t, ch::offset_style::none); },
            [&]() { return ch::iso8601_size(b2, 0, ch::date_sep::iso_t, ch::offset_style::none); }, "-0044-03-15T12:00:00");
  }
  end_test_case();

  test_case("a pre-1970 instant renders correctly end to end");
  {
    const ch::civil c = ch::civil_utc(-1);
    CHECK_W([&](char *b, usize k) { return ch::write_rfc3339(b, k, c, 0); }, [&]() { return ch::rfc3339_size(c, 0); },
            "1969-12-31T23:59:59Z");
  }
  end_test_case();
}

static void
test_other_shapes(void)
{
  sb::print("=== rfc2822 / http / asctime / compact / log ===");

  const ch::civil utc = ch::civil_utc(1768000000L);

  test_case("rfc2822 matches date -R");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_rfc2822(b, k, utc); }, [&]() { return ch::rfc2822_size(utc); },
            "Fri, 09 Jan 2026 23:06:40 +0000");
  }
  end_test_case();

  test_case("rfc2822 carries a non-zero offset compactly");
  {
    const ch::civil c = ch::to_civil(1768000000L, 7200);
    CHECK_W([&](char *b, usize k) { return ch::write_rfc2822(b, k, c); }, [&]() { return ch::rfc2822_size(c); },
            "Sat, 10 Jan 2026 01:06:40 +0200");
  }
  end_test_case();

  test_case("the HTTP Date: header spells the zone GMT");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_http_date(b, k, utc); }, [&]() { return ch::http_date_size(utc); },
            "Fri, 09 Jan 2026 23:06:40 GMT");
  }
  end_test_case();

  test_case("asctime is 24 bytes with a SPACE-padded day");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_asctime(b, k, utc); }, [&]() { return ch::asctime_size(utc); },
            "Fri Jan  9 23:06:40 2026");
    char b[64];
    require(static_cast<long>(ch::write_asctime(b, sizeof(b), utc)), 24L);
  }
  end_test_case();

  test_case("compact is filename-safe and sorts lexicographically");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_compact(b, k, utc); }, [&]() { return ch::compact_size(utc, true); },
            "20260109T230640Z");
    // lexicographic order must follow chronological order
    char a[32], c[32];
    const usize na = ch::write_compact(a, sizeof(a), ch::civil_utc(1768000000L));
    const usize nc = ch::write_compact(c, sizeof(c), ch::civil_utc(1768000001L));
    require(static_cast<long>(na), static_cast<long>(nc));
    bool less = false;
    for ( usize i = 0; i < na; ++i ) {
      if ( a[i] != c[i] ) {
        less = a[i] < c[i];
        break;
      }
    }
    require_true(less);
  }
  end_test_case();

  test_case("the log stamp has no offset and three sub-second digits");
  {
    ch::civil c = utc;
    c.ns = 42000000u;
    CHECK_W([&](char *b, usize k) { return ch::write_log_stamp(b, k, c, 3); }, [&]() { return ch::log_stamp_size(c, 3); },
            "2026-01-09 23:06:40.042");
  }
  end_test_case();
}

static void
test_strftime(void)
{
  sb::print("=== strftime ===");

  const ch::civil utc = ch::civil_utc(1768000000L);

  test_case("%F %T is the ISO date and time");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%F %T", utc); },
            [&]() { return ch::strftime_size("%F %T", utc); }, "2026-01-09 23:06:40");
  }
  end_test_case();

  test_case("names, ordinals and week numbers");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%a %A %b %B %j %V %u %w", utc); },
            [&]() { return ch::strftime_size("%a %A %b %B %j %V %u %w", utc); }, "Fri Friday Jan January 009 02 5 5");
  }
  end_test_case();

  test_case("12-hour clock and the am/pm markers");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%I %p %P %r", utc); },
            [&]() { return ch::strftime_size("%I %p %P %r", utc); }, "11 PM pm 11:06:40 PM");
    const ch::civil noon = mk(2026, 1, 9, 12, 0, 0);
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%I %p", noon); },
            [&]() { return ch::strftime_size("%I %p", noon); }, "12 PM");
    const ch::civil midnight = mk(2026, 1, 9, 0, 0, 0);
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%I %p", midnight); },
            [&]() { return ch::strftime_size("%I %p", midnight); }, "12 AM");
  }
  end_test_case();

  test_case("%s reproduces the epoch second from the fields and their offset");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%s", utc); }, [&]() { return ch::strftime_size("%s", utc); },
            "1768000000");
    const ch::civil off = ch::to_civil(1768000000L, 7200);
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%s", off); }, [&]() { return ch::strftime_size("%s", off); },
            "1768000000");
  }
  end_test_case();

  test_case("%z and %Z");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%z %Z", utc); },
            [&]() { return ch::strftime_size("%z %Z", utc); }, "+0000 UTC");
    const ch::civil off = ch::to_civil(1768000000L, -19800);
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%z %Z", off); },
            [&]() { return ch::strftime_size("%z %Z", off); }, "-0530 -0530");
  }
  end_test_case();

  test_case("literals, %% and an unknown specifier passing through");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "x%%y%Qz", utc); },
            [&]() { return ch::strftime_size("x%%y%Qz", utc); }, "x%y%Qz");
  }
  end_test_case();

  test_case("%e and %k are space-padded, %d and %H are zero-padded");
  {
    const ch::civil c = mk(2026, 3, 5, 7, 8, 9);
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "[%e][%d][%k][%H]", c); },
            [&]() { return ch::strftime_size("[%e][%d][%k][%H]", c); }, "[ 5][05][ 7][07]");
  }
  end_test_case();

  test_case("%c has asctime's shape");
  {
    CHECK_W([&](char *b, usize k) { return ch::write_strftime(b, k, "%c", utc); }, [&]() { return ch::strftime_size("%c", utc); },
            "Fri Jan  9 23:06:40 2026");
  }
  end_test_case();

  test_case("an empty format writes nothing");
  {
    char b[8];
    require(static_cast<long>(ch::write_strftime(b, sizeof(b), "", utc)), 0L);
    require(static_cast<long>(ch::strftime_size("", utc)), 0L);
  }
  end_test_case();
}

static void
test_durations(void)
{
  sb::print("=== duration writers ===");

  test_case("hms: under a day is HH:MM:SS, past one is Dd HH:MM");
  {
    const u64 a = 2 * 3600 + 11 * 60 + 33;
    CHECK_W([&](char *b, usize k) { return ch::write_duration_hms(b, k, a); }, [&]() { return ch::duration_hms_size(a); },
            "02:11:33");
    const u64 c = 4 * 86400 + 2 * 3600 + 11 * 60;
    CHECK_W([&](char *b, usize k) { return ch::write_duration_hms(b, k, c); }, [&]() { return ch::duration_hms_size(c); },
            "4d 02:11");
    CHECK_W([&](char *b, usize k) { return ch::write_duration_hms(b, k, 0); }, [&]() { return ch::duration_hms_size(0); },
            "00:00:00");
  }
  end_test_case();

  test_case("clock: M:SS.cc under an hour, H:MM:SS under a day, Dd HH:MM past one");
  {
    // ticks at 100 Hz, the USER_HZ convention
    const u64 hz = 100;
    CHECK_W([&](char *b, usize k) { return ch::write_duration_clock(b, k, 125u * hz + 50u, hz); },
            [&]() { return ch::duration_clock_size(125u * hz + 50u, hz); }, "2:05.50");
    CHECK_W([&](char *b, usize k) { return ch::write_duration_clock(b, k, 7384u * hz, hz); },
            [&]() { return ch::duration_clock_size(7384u * hz, hz); }, "2:03:04");
    CHECK_W([&](char *b, usize k) { return ch::write_duration_clock(b, k, 100000u * hz, hz); },
            [&]() { return ch::duration_clock_size(100000u * hz, hz); }, "1d 03:46");
  }
  end_test_case();

  test_case("units: whole components only, largest first");
  {
    const u64 v = 3600ull * ch::ns_per_s + 23ull * ch::ns_per_min + 4ull * ch::ns_per_s;
    CHECK_W([&](char *b, usize k) { return ch::write_duration_units(b, k, v); }, [&]() { return ch::duration_units_size(v); },
            "1h23m4s");
    CHECK_W([&](char *b, usize k) { return ch::write_duration_units(b, k, 0); }, [&]() { return ch::duration_units_size(0); },
            "0s");
    CHECK_W([&](char *b, usize k) { return ch::write_duration_units(b, k, 1); }, [&]() { return ch::duration_units_size(1); },
            "1ns");
    CHECK_W([&](char *b, usize k) { return ch::write_duration_units(b, k, 1500ull * ch::ns_per_ms); },
            [&]() { return ch::duration_units_size(1500ull * ch::ns_per_ms); }, "1s500ms");
  }
  end_test_case();

  test_case("units output feeds straight back through parse_duration_ns");
  {
    const u64 vals[9] = { 0ull,
                          1ull,
                          999ull,
                          ch::ns_per_us + 1ull,
                          ch::ns_per_ms * 1500ull,
                          ch::ns_per_s * 90ull,
                          ch::ns_per_hour + ch::ns_per_min * 23ull + ch::ns_per_s * 4ull,
                          ch::ns_per_week * 3ull + ch::ns_per_day * 2ull,
                          ch::ns_per_day * 400ull + 7ull };
    for ( usize i = 0; i < 9; ++i ) {
      char b[128];
      const usize n = ch::write_duration_units(b, sizeof(b), vals[i]);
      require_true(n > 0);
      const auto r = ch::parse_duration_ns(b, n);
      require_true(r.ok());
      require(static_cast<long long>(r.ns), static_cast<long long>(vals[i]));
    }
  }
  end_test_case();
}

static void
test_capacity_contract(void)
{
  sb::print("=== the buffer contract ===");

  const ch::civil utc = ch::civil_utc(1768000000L);

  test_case("a zero-capacity buffer answers 0 and writes nothing");
  {
    char guard[4] = { '\x7F', '\x7F', '\x7F', '\x7F' };
    require(static_cast<long>(ch::write_rfc3339(guard, 0, utc, 0)), 0L);
    require(static_cast<long>(ch::write_rfc2822(guard, 0, utc)), 0L);
    require(static_cast<long>(ch::write_duration_units(guard, 0, 12345)), 0L);
    for ( int i = 0; i < 4; ++i ) require_true(guard[i] == '\x7F');
  }
  end_test_case();

  test_case("X_size() never depends on the buffer it is given");
  {
    require(static_cast<long>(ch::rfc3339_size(utc, 0)), 20L);
    require(static_cast<long>(ch::rfc3339_size(utc, 3)), 24L);
    require(static_cast<long>(ch::rfc3339_size(utc, 9)), 30L);
    require(static_cast<long>(ch::asctime_size(utc)), 24L);
    require(static_cast<long>(ch::compact_size(utc, true)), 16L);
    require(static_cast<long>(ch::compact_size(utc, false)), 15L);
    // and unlike the old hardcoded 16/15, it tracks a year that does not fit in four digits
    require(static_cast<long>(ch::compact_size(ch::civil{ .y = 12026 }, true)), 17L);
  }
  end_test_case();

  test_case("every advertised max is actually large enough");
  {
    ch::civil c = utc;
    c.ns = 999999999u;
    c.off = -50400;
    require_true(ch::iso8601_size(c, 9, ch::date_sep::space, ch::offset_style::colon) <= ch::iso8601_max);
    require_true(ch::rfc2822_size(c) <= ch::rfc2822_max);
    require_true(ch::asctime_size(c) <= ch::asctime_max);
    require_true(ch::duration_units_size(~0ull) <= ch::duration_max);
    require_true(ch::duration_hms_size(~0ull >> 1) <= ch::duration_max);
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::chrono format suite");
  sb::print("===========================");
  test_iso();
  test_other_shapes();
  test_strftime();
  test_durations();
  test_capacity_contract();
  sb::print("===========================");
  sb::print("ALL FORMAT TESTS COMPLETED");
  return 1;
}
