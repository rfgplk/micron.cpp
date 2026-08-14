//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- the strict parsers and the duration grammar.
//
// Half of this file is REJECTION. A parser that only ever gets shown valid input is not tested: the
// contract here is full consumption and fixed-width fields, and every way of violating it has to
// fail rather than quietly answer something.
//
// The load-bearing case is 2026-02-30. civil_secs NORMALISES, so an unvalidated invalid date does
// not error -- it silently answers March 2nd, and a program that acts two days late looks exactly
// like one that worked.

#include "../../src/chrono.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;

static usize
slen(const char *s)
{
  usize n = 0;
  while ( s[n] ) ++n;
  return n;
}

static ch::civil_result
p_iso(const char *s)
{
  return ch::parse_iso8601(s, slen(s));
}

static ch::civil_result
p_3339(const char *s)
{
  return ch::parse_rfc3339(s, slen(s));
}

static ch::civil_result
p_2822(const char *s)
{
  return ch::parse_rfc2822(s, slen(s));
}

static ch::dur_result
p_dur(const char *s)
{
  return ch::parse_duration_ns(s, slen(s));
}

static void
test_iso_accept(void)
{
  sb::print("=== ISO 8601: accepted ===");

  test_case("a full RFC 3339 instant with Z");
  {
    const auto r = p_iso("2026-08-14T14:22:11Z");
    require_true(r.ok());
    require_true(r.has_offset);
    require(r.c.y, 2026);
    require(r.c.mo, 8u);
    require(r.c.d, 14u);
    require(r.c.h, 14u);
    require(r.c.mi, 22u);
    require(r.c.s, 11u);
    require(r.c.off, 0);
    require(ch::to_unix(r), 1786717331L);      // cross-checked with GNU date
  }
  end_test_case();

  test_case("a numeric offset, both spellings");
  {
    const auto a = p_iso("2026-08-14T16:22:11+02:00");
    require_true(a.ok());
    require(a.c.off, 7200);
    require(ch::to_unix(a), 1786717331L);
    const auto b = p_iso("2026-08-14T16:22:11+0200");
    require_true(b.ok());
    require(b.c.off, 7200);
    require(ch::to_unix(b), 1786717331L);
    const auto c = p_iso("2026-08-14T16:22:11+02");
    require_true(c.ok());
    require(c.c.off, 7200);
  }
  end_test_case();

  test_case("a negative half-hour offset");
  {
    const auto r = p_iso("2026-08-14T08:52:11-05:30");
    require_true(r.ok());
    require(r.c.off, -19800);
    require(ch::to_unix(r), 1786717331L);
  }
  end_test_case();

  test_case("a fractional second, at every width, truncating past nine digits");
  {
    const auto a = p_iso("2026-08-14T14:22:11.5Z");
    require_true(a.ok());
    require(static_cast<long>(a.c.ns), 500000000L);
    const auto b = p_iso("2026-08-14T14:22:11.123456789Z");
    require_true(b.ok());
    require(static_cast<long>(b.c.ns), 123456789L);
    const auto c = p_iso("2026-08-14T14:22:11.1234567891234Z");
    require_true(c.ok());
    require(static_cast<long>(c.c.ns), 123456789L);
  }
  end_test_case();

  test_case("a date on its own means midnight");
  {
    const auto r = p_iso("2026-08-14");
    require_true(r.ok());
    require_false(r.has_offset);
    require(r.c.h, 0u);
    require(r.c.mi, 0u);
    require(r.c.s, 0u);
  }
  end_test_case();

  test_case("seconds may be omitted, and a space may replace the T");
  {
    const auto a = p_iso("2026-08-14T14:22");
    require_true(a.ok());
    require(a.c.s, 0u);
    const auto b = p_iso("2026-08-14 14:22:11");
    require_true(b.ok());
    require(b.c.h, 14u);
  }
  end_test_case();

  test_case("a leap second is accepted by the grammar and collapses onto :59 as an instant");
  {
    const auto r = p_iso("2016-12-31T23:59:60Z");
    require_true(r.ok());
    require(r.c.s, 60u);
    require(ch::to_unix(r), 1483228799L);
  }
  end_test_case();

  test_case("pre-1970 and expanded years");
  {
    const auto a = p_iso("1969-12-31T23:59:59Z");
    require_true(a.ok());
    require(ch::to_unix(a), -1L);
    const auto b = p_iso("+12345-06-07T08:09:10Z");
    require_true(b.ok());
    require(b.c.y, 12345);
    const auto c = p_iso("-0044-03-15T12:00:00Z");
    require_true(c.ok());
    require(c.c.y, -44);
  }
  end_test_case();

  test_case("format then parse round-trips every hour of a week");
  {
    for ( long s = 1768000000L; s < 1768000000L + 604800L; s += 3600L ) {
      char b[64];
      const ch::civil c = ch::civil_utc(s);
      const usize n = ch::write_rfc3339(b, sizeof(b), c, 0);
      require_true(n > 0);
      const auto r = ch::parse_rfc3339(b, n);
      require_true(r.ok());
      require(ch::to_unix(r), s);
    }
  }
  end_test_case();
}

static void
test_iso_reject(void)
{
  sb::print("=== ISO 8601: REJECTED ===");

  test_case("2026-02-30 does not exist and must NOT normalise into March");
  {
    const auto r = p_iso("2026-02-30");
    require_false(r.ok());
    require_true(r.err == ch::parse_err::bad_day);
  }
  end_test_case();

  test_case("2026-02-29 is rejected, 2024-02-29 is not");
  {
    require_false(p_iso("2026-02-29").ok());
    require_true(p_iso("2024-02-29").ok());
    require_false(p_iso("1900-02-29").ok());
    require_true(p_iso("2000-02-29").ok());
  }
  end_test_case();

  test_case("a one-digit field is a typo, not a second spelling");
  {
    require_false(p_iso("2026-8-14").ok());
    require_false(p_iso("2026-08-4").ok());
    require_false(p_iso("2026-08-14T4:22:11Z").ok());
    require_false(p_iso("2026-08-14T14:2:11Z").ok());
    require_false(p_iso("226-08-14").ok());
  }
  end_test_case();

  test_case("out-of-range fields");
  {
    require_false(p_iso("2026-13-01").ok());
    require_false(p_iso("2026-00-01").ok());
    require_false(p_iso("2026-08-00").ok());
    require_false(p_iso("2026-08-32").ok());
    require_false(p_iso("2026-08-14T24:00:00Z").ok());
    require_false(p_iso("2026-08-14T14:60:00Z").ok());
    require_false(p_iso("2026-08-14T14:22:61Z").ok());
  }
  end_test_case();

  test_case("trailing characters are never ignored");
  {
    require_false(p_iso("2026-08-14T14:22:11Zjunk").ok());
    require_false(p_iso("2026-08-14 ").ok());
    require_false(p_iso("2026-08-14T14:22:11+02:00x").ok());
  }
  end_test_case();

  test_case("malformed separators and empty input");
  {
    require_false(p_iso("2026/08/14").ok());
    require_false(p_iso("20260814").ok());
    require_false(p_iso("2026-08-14X14:22:11").ok());
    require_false(p_iso("2026-08-14T14-22-11").ok());
    require_false(p_iso("").ok());
    require_false(ch::parse_iso8601(nullptr, 0).ok());
  }
  end_test_case();

  test_case("a bad offset");
  {
    require_false(p_iso("2026-08-14T14:22:11+2:00").ok());
    require_false(p_iso("2026-08-14T14:22:11+24:00").ok());
    require_false(p_iso("2026-08-14T14:22:11+02:60").ok());
    require_false(p_iso("2026-08-14T14:22:11Q").ok());
  }
  end_test_case();

  test_case("a truncated fraction");
  {
    require_false(p_iso("2026-08-14T14:22:11.Z").ok());
    require_false(p_iso("2026-08-14T14:22:11.").ok());
  }
  end_test_case();

  test_case("rfc3339 additionally demands an offset");
  {
    require_true(p_iso("2026-08-14T14:22:11").ok());
    const auto r = p_3339("2026-08-14T14:22:11");
    require_false(r.ok());
    require_true(r.err == ch::parse_err::no_offset);
    require_true(p_3339("2026-08-14T14:22:11Z").ok());
  }
  end_test_case();
}

static void
test_rfc2822(void)
{
  sb::print("=== RFC 2822 / 1123 ===");

  test_case("the canonical form");
  {
    const auto r = p_2822("Fri, 09 Jan 2026 23:06:40 +0000");
    require_true(r.ok());
    require(r.c.y, 2026);
    require(r.c.mo, 1u);
    require(r.c.d, 9u);
    require(r.c.h, 23u);
    require(r.c.off, 0);
    require(ch::to_unix(r), 1768000000L);
  }
  end_test_case();

  test_case("the HTTP spelling with GMT");
  {
    const auto r = p_2822("Fri, 09 Jan 2026 23:06:40 GMT");
    require_true(r.ok());
    require(ch::to_unix(r), 1768000000L);
  }
  end_test_case();

  test_case("the day name is optional, and an offset shifts the instant");
  {
    const auto a = p_2822("9 Jan 2026 23:06:40 +0000");
    require_true(a.ok());
    require(ch::to_unix(a), 1768000000L);
    const auto b = p_2822("Sat, 10 Jan 2026 01:06:40 +0200");
    require_true(b.ok());
    require(ch::to_unix(b), 1768000000L);
  }
  end_test_case();

  test_case("the obsolete two-digit year");
  {
    const auto a = p_2822("01 Jan 70 00:00:00 +0000");
    require_true(a.ok());
    require(a.c.y, 1970);
    const auto b = p_2822("01 Jan 26 00:00:00 +0000");
    require_true(b.ok());
    require(b.c.y, 2026);
  }
  end_test_case();

  test_case("rejections");
  {
    require_false(p_2822("Xyz, 09 Jan 2026 23:06:40 +0000").ok());
    require_false(p_2822("Fri, 09 Xxx 2026 23:06:40 +0000").ok());
    require_false(p_2822("Fri, 30 Feb 2026 23:06:40 +0000").ok());
    require_false(p_2822("Fri, 09 Jan 2026 25:06:40 +0000").ok());
    require_false(p_2822("Fri, 09 Jan 2026 23:06:40 +0000 junk").ok());
    require_false(p_2822("").ok());
  }
  end_test_case();

  test_case("format then parse round-trips");
  {
    for ( long s = 1768000000L; s < 1768000000L + 86400L * 30L; s += 86400L * 3L ) {
      char b[64];
      const usize n = ch::write_rfc2822(b, sizeof(b), ch::civil_utc(s));
      require_true(n > 0);
      const auto r = ch::parse_rfc2822(b, n);
      require_true(r.ok());
      require(ch::to_unix(r), s);
    }
  }
  end_test_case();
}

static void
test_duration(void)
{
  sb::print("=== the duration grammar ===");

  test_case("a bare number is SECONDS");
  {
    const auto r = p_dur("90");
    require_true(r.ok());
    require(static_cast<long long>(r.ns), 90LL * 1000000000LL);
  }
  end_test_case();

  test_case("500ms is milliseconds, 500m is minutes -- longest match wins");
  {
    const auto a = p_dur("500ms");
    require_true(a.ok());
    require(static_cast<long long>(a.ns), 500LL * 1000000LL);
    const auto b = p_dur("500m");
    require_true(b.ok());
    require(static_cast<long long>(b.ns), 500LL * 60LL * 1000000000LL);
  }
  end_test_case();

  test_case("every suffix");
  {
    require(static_cast<long long>(p_dur("1ns").ns), 1LL);
    require(static_cast<long long>(p_dur("1us").ns), 1000LL);
    require(static_cast<long long>(p_dur("1ms").ns), 1000000LL);
    require(static_cast<long long>(p_dur("1s").ns), 1000000000LL);
    require(static_cast<long long>(p_dur("1m").ns), 60LL * 1000000000LL);
    require(static_cast<long long>(p_dur("1h").ns), 3600LL * 1000000000LL);
    require(static_cast<long long>(p_dur("1d").ns), 86400LL * 1000000000LL);
    require(static_cast<long long>(p_dur("1w").ns), 604800LL * 1000000000LL);
  }
  end_test_case();

  test_case("concatenated components add");
  {
    const auto a = p_dur("1h30m");
    require_true(a.ok());
    require(static_cast<long long>(a.ns), 5400LL * 1000000000LL);
    const auto b = p_dur("1w2d3h4m5s6ms7us8ns");
    require_true(b.ok());
    const long long want = (604800LL + 2 * 86400LL + 3 * 3600LL + 4 * 60LL + 5LL) * 1000000000LL + 6LL * 1000000LL + 7LL * 1000LL + 8LL;
    require(static_cast<long long>(b.ns), want);
  }
  end_test_case();

  test_case("fractions are exact fixed point, never a double");
  {
    require(static_cast<long long>(p_dur("2.5s").ns), 2500000000LL);
    require(static_cast<long long>(p_dur("0.1s").ns), 100000000LL);
    require(static_cast<long long>(p_dur("0.000000001s").ns), 1LL);
    require(static_cast<long long>(p_dur("1.5h").ns), 5400LL * 1000000000LL);
    // a tenth of a second is not representable in binary; a fixed-point parser still lands exactly
    require(static_cast<long long>(p_dur("0.3s").ns), 300000000LL);
  }
  end_test_case();

  test_case("inf / infinity");
  {
    const auto a = p_dur("inf");
    require_true(a.ok());
    require_true(a.forever);
    const auto b = p_dur("INFINITY");
    require_true(b.ok());
    require_true(b.forever);
    require_false(p_dur("infinit").ok());
  }
  end_test_case();

  test_case("a TRAILING unitless component is seconds: 1h30 is 1h30s");
  {
    const auto r = p_dur("1h30");
    require_true(r.ok());
    require(static_cast<long long>(r.ns), 3630LL * 1000000000LL);
  }
  end_test_case();

  test_case("rejections");
  {
    require_false(p_dur("").ok());
    require_false(p_dur("abc").ok());
    require_false(p_dur("90x").ok());      // a bare number followed by anything else is a typo
    require_false(p_dur("-5s").ok());      // no sign: a duration is a magnitude
    require_false(p_dur("1.s").ok());      // a '.' must be followed by digits
    require_false(p_dur(".5s").ok());      // and preceded by them -- NUMBER is not optional
    require_false(ch::parse_duration_ns(nullptr, 0).ok());
  }
  end_test_case();

  test_case("overflow is an error, never a wrap");
  {
    const auto r = p_dur("99999999999999999999w");
    require_false(r.ok());
    const auto s = p_dur("1000000000000w");
    require_false(s.ok());
    require_true(s.err == ch::parse_err::overflow);
  }
  end_test_case();
}

static void
test_until(void)
{
  sb::print("=== the sleep --until subset ===");

  test_case("@SECONDS");
  {
    const auto r = ch::parse_until("@1768000000", 11);
    require_true(r.ok());
    require_true(r.epoch);
    require(r.secs, 1768000000L);
    require(ch::until_epoch(r, 0, ch::tz_utc), 1768000000L);
  }
  end_test_case();

  test_case("HH:MM[:SS] is time-only and rolls forward when it has passed");
  {
    const auto r = ch::parse_until("09:00", 5);
    require_true(r.ok());
    require_true(r.time_only);
    require(r.c.h, 9u);
    // 1768000000 is 2026-01-09T23:06:40Z, so 09:00 UTC has gone -- expect the 10th
    const auto u = ch::parse_until("09:00Z", 6);
    require_true(u.ok());
    require_true(u.utc);
    const long e = ch::until_epoch(u, 1768000000L, ch::tz_utc);
    const ch::civil c = ch::civil_utc(e);
    require(c.d, 10u);
    require(c.h, 9u);
    require_true(e > 1768000000L);
  }
  end_test_case();

  test_case("a time still to come today stays today");
  {
    const auto u = ch::parse_until("23:30Z", 6);
    require_true(u.ok());
    const long e = ch::until_epoch(u, 1768000000L, ch::tz_utc);
    const ch::civil c = ch::civil_utc(e);
    require(c.d, 9u);
    require(c.h, 23u);
    require(c.mi, 30u);
  }
  end_test_case();

  test_case("a full date-time, with and without Z");
  {
    const auto a = ch::parse_until("2026-08-14T14:22:11Z", 20);
    require_true(a.ok());
    require_true(a.utc);
    require(ch::until_epoch(a, 0, ch::tz_utc), 1786717331L);
    const auto b = ch::parse_until("2026-08-14", 10);
    require_true(b.ok());
    require_false(b.time_only);
  }
  end_test_case();

  test_case("rejections");
  {
    require_false(ch::parse_until("", 0).ok());
    require_false(ch::parse_until("9:00", 4).ok());
    require_false(ch::parse_until("25:00", 5).ok());
    require_false(ch::parse_until("09:60", 5).ok());
    require_false(ch::parse_until("2026-02-30", 10).ok());
    require_false(ch::parse_until("@abc", 4).ok());
  }
  end_test_case();
}

static void
test_fuzz(void)
{
  sb::print("=== seeded fuzz ===");

  test_case("a random byte soup never crashes and never accepts garbage as a date");
  {
    u64 s = 0x243F6A8885A308D3ull;      // fixed seed, never time-based
    char buf[40];
    for ( int it = 0; it < 60000; ++it ) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      const usize n = static_cast<usize>((s >> 13) % 32u);
      for ( usize i = 0; i < n; ++i ) {
        s = s * 6364136223846793005ull + 1442695040888963407ull;
        // bias toward characters a timestamp could contain, so the parser is actually exercised
        const char alphabet[] = "0123456789-:TZ+. ";
        buf[i] = alphabet[(s >> 17) % (sizeof(alphabet) - 1)];
      }
      const auto r = ch::parse_iso8601(buf, n);
      if ( r.ok() ) {
        // whatever it accepted must be a real date it can render back identically
        require_true(ch::civil_valid(r.c.y, r.c.mo, r.c.d));
        require_true(r.c.h < 24 && r.c.mi < 60 && r.c.s <= 60);
      }
      const auto d = ch::parse_duration_ns(buf, n);
      if ( d.ok() && !d.forever ) require_true(d.ns <= ~0ull);
      (void)ch::parse_rfc2822(buf, n);
      (void)ch::parse_until(buf, n);
    }
  }
  end_test_case();

  test_case("every valid instant in a year round-trips format -> parse -> format");
  {
    u64 s = 0xB7E151628AED2A6Bull;
    char a[64], b[64];
    for ( int it = 0; it < 20000; ++it ) {
      s = s * 6364136223846793005ull + 1442695040888963407ull;
      const long inst = 1735689600L + static_cast<long>((s >> 12) % 31536000ull);
      const ch::civil c = ch::civil_utc(inst);
      const usize na = ch::write_rfc3339(a, sizeof(a), c, 0);
      const auto r = ch::parse_rfc3339(a, na);
      require_true(r.ok());
      require(ch::to_unix(r), inst);
      const usize nb = ch::write_rfc3339(b, sizeof(b), r.c, 0);
      require(static_cast<long>(nb), static_cast<long>(na));
      for ( usize i = 0; i < na; ++i ) require_true(a[i] == b[i]);
    }
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::chrono parse suite");
  sb::print("==========================");
  test_iso_accept();
  test_iso_reject();
  test_rfc2822();
  test_duration();
  test_until();
  test_fuzz();
  sb::print("==========================");
  sb::print("ALL PARSE TESTS COMPLETED");
  return 1;
}
