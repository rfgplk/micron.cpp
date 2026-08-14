//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::posix -- the libc time layer.
//
// micron had none of this: no struct tm, no gmtime, no mktime, no strftime, no strptime, no
// sysconf. And difftime was DECLARED at linux/sys/time.hpp:207 with no definition anywhere, so any
// ODR-use of it was a link error -- this file using it is the regression test.

#include "../../src/chrono.hpp"
#include "../../src/chrono/tz.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace ch = micron::chrono;
namespace pp = micron::posix;

static usize
slen(const char *s)
{
  usize n = 0;
  while ( s[n] ) ++n;
  return n;
}

static bool
str_eq(const char *a, const char *b)
{
  usize i = 0;
  for ( ; a[i] && b[i]; ++i )
    if ( a[i] != b[i] ) return false;
  return a[i] == b[i];
}

static void
test_broken_down(void)
{
  sb::print("=== gmtime_r / timegm ===");

  test_case("gmtime_r fills every field, with the POSIX offsets");
  {
    micron::time_t t = 1768000000;      // 2026-01-09T23:06:40Z, a Friday, day 9
    pp::tm_t tm{};
    require_true(pp::gmtime_r(&t, &tm) == &tm);
    require(tm.tm_year, 126);      // years since 1900
    require(tm.tm_mon, 0);         // JANUARY IS ZERO
    require(tm.tm_mday, 9);
    require(tm.tm_hour, 23);
    require(tm.tm_min, 6);
    require(tm.tm_sec, 40);
    require(tm.tm_wday, 5);        // Friday
    require(tm.tm_yday, 8);        // Jan 1 is ZERO
    require(tm.tm_isdst, 0);
    require(tm.tm_gmtoff, 0L);
  }
  end_test_case();

  test_case("timegm is the exact inverse of gmtime_r");
  {
    for ( long s = -100000000L; s < 2000000000L; s += 7919993L ) {
      pp::tm_t tm{};
      micron::time_t t = static_cast<micron::time_t>(s);
      pp::gmtime_r(&t, &tm);
      require(static_cast<long>(pp::timegm(&tm)), s);
    }
  }
  end_test_case();

  test_case("timegm normalises out-of-range fields the way glibc does");
  {
    pp::tm_t tm{};
    tm.tm_year = 126;      // 2026
    tm.tm_mon = 0;
    tm.tm_mday = 32;       // the 32nd of January is the 1st of February
    const micron::time_t t = pp::timegm(&tm);
    require(tm.tm_mon, 1);
    require(tm.tm_mday, 1);
    pp::tm_t check{};
    pp::gmtime_r(&t, &check);
    require(check.tm_mon, 1);
    require(check.tm_mday, 1);
  }
  end_test_case();

  test_case("gmtime_r handles pre-1970");
  {
    micron::time_t t = -1;
    pp::tm_t tm{};
    pp::gmtime_r(&t, &tm);
    require(tm.tm_year, 69);
    require(tm.tm_mon, 11);
    require(tm.tm_mday, 31);
    require(tm.tm_hour, 23);
    require(tm.tm_sec, 59);
  }
  end_test_case();

  test_case("null arguments are refused, not dereferenced");
  {
    pp::tm_t tm{};
    micron::time_t t = 0;
    require_true(pp::gmtime_r(nullptr, &tm) == nullptr);
    require_true(pp::gmtime_r(&t, nullptr) == nullptr);
    require(static_cast<long>(pp::timegm(nullptr)), -1L);
  }
  end_test_case();

  test_case("civil <-> tm conversion round-trips");
  {
    const ch::civil c = ch::civil_utc(1768000000L);
    pp::tm_t tm{};
    pp::tm_of_civil(c, tm);
    const ch::civil back = pp::civil_of_tm(tm);
    require(back.y, c.y);
    require(back.mo, c.mo);
    require(back.d, c.d);
    require(back.h, c.h);
    require(back.mi, c.mi);
    require(back.s, c.s);
  }
  end_test_case();
}

static void
test_localtime_mktime(void)
{
  sb::print("=== localtime_r / mktime, against an explicit zone ===");

  // the same Berlin-shaped table the tz suite uses
  static constexpr i64 tr[2] = { 1743296400L, 1761440400L };
  static constexpr i32 of[2] = { 7200, 3600 };
  const ch::tz_table z{ tr, of, 2, 3600 };

  test_case("localtime_r applies the zone offset");
  {
    micron::time_t t = 1750000000;      // 2025-06-15T15:06:40Z, inside summer time
    pp::tm_t tm{};
    require_true(pp::localtime_r(&t, &tm, z) == &tm);
    require(tm.tm_gmtoff, 7200L);
    require(tm.tm_hour, 17);
  }
  end_test_case();

  test_case("mktime is the inverse of localtime_r away from a transition");
  {
    for ( long s = 1746000000L; s < 1760000000L; s += 999983L ) {
      pp::tm_t tm{};
      micron::time_t t = static_cast<micron::time_t>(s);
      pp::localtime_r(&t, &tm, z);
      require(static_cast<long>(pp::mktime(&tm, z)), s);
    }
  }
  end_test_case();

  test_case("mktime through UTC agrees with timegm");
  {
    pp::tm_t a{};
    a.tm_year = 126;
    a.tm_mon = 7;
    a.tm_mday = 14;
    a.tm_hour = 14;
    a.tm_min = 22;
    a.tm_sec = 11;
    pp::tm_t b = a;
    require(static_cast<long>(pp::mktime(&a, ch::tz_utc)), static_cast<long>(pp::timegm(&b)));
    require(static_cast<long>(pp::timegm(&b)), 1786717331L);
  }
  end_test_case();
}

static void
test_render(void)
{
  sb::print("=== asctime_r / ctime_r / strftime ===");

  test_case("asctime_r writes 26 bytes ending in newline and NUL");
  {
    micron::time_t t = 1768000000;
    pp::tm_t tm{};
    pp::gmtime_r(&t, &tm);
    char buf[pp::asctime_buf];
    for ( usize i = 0; i < sizeof(buf); ++i ) buf[i] = '\x7F';
    require_true(pp::asctime_r(&tm, buf) == buf);
    require_true(str_eq(buf, "Fri Jan  9 23:06:40 2026\n"));
    require(static_cast<long>(slen(buf)), 25L);
    require_true(buf[24] == '\n');
    require_true(buf[25] == '\0');
  }
  end_test_case();

  test_case("ctime_r goes through the zone");
  {
    micron::time_t t = 1768000000;
    char buf[pp::asctime_buf];
    require_true(pp::ctime_r(&t, buf, ch::tz_utc) == buf);
    require_true(str_eq(buf, "Fri Jan  9 23:06:40 2026\n"));
    require_true(pp::ctime_r(&t, buf, ch::tz_fixed(7200)) == buf);
    require_true(str_eq(buf, "Sat Jan 10 01:06:40 2026\n"));
  }
  end_test_case();

  // REGRESSION: a year wider than four digits does not fit the 24-byte budget. asctime_r used to
  // let write_asctime clobber all 24 bytes on the way to reporting 0, then return NULL without
  // ever planting a terminator -- the caller was left holding a plausible-looking timestamp that
  // ran off the end of its own char[26]. strptime's six-digit %Y reaches this from input
  test_case("asctime_r terminates the buffer on every failure path");
  {
    char buf[pp::asctime_buf];
    for ( usize i = 0; i < sizeof(buf); ++i ) buf[i] = '\x7F';

    pp::tm_t wide{};
    wide.tm_year = 10126;      // 12026 -- five digits
    wide.tm_mon = 7;
    wide.tm_mday = 14;
    require_true(pp::asctime_r(&wide, buf) == nullptr);
    require(static_cast<long>(slen(buf)), 0L);

    // and the same through the six-digit %Y strptime accepts, which is how untrusted input gets here
    for ( usize i = 0; i < sizeof(buf); ++i ) buf[i] = '\x7F';
    pp::tm_t parsed{};
    require_true(pp::strptime("999999-08-14 14:22:11", "%Y-%m-%d %H:%M:%S", &parsed) != nullptr);
    require_true(pp::asctime_r(&parsed, buf) == nullptr);
    require(static_cast<long>(slen(buf)), 0L);

    // a sane year still renders, so the size gate did not cost the success path
    for ( usize i = 0; i < sizeof(buf); ++i ) buf[i] = '\x7F';
    micron::time_t t = 1768000000;
    pp::tm_t ok{};
    pp::gmtime_r(&t, &ok);
    require_true(pp::asctime_r(&ok, buf) == buf);
    require(static_cast<long>(slen(buf)), 25L);
  }
  end_test_case();

  test_case("strftime NUL-terminates and returns the count without it");
  {
    micron::time_t t = 1768000000;
    pp::tm_t tm{};
    pp::gmtime_r(&t, &tm);
    char buf[64];
    const usize n = pp::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    require(static_cast<long>(n), 19L);
    require_true(buf[n] == '\0');
    require_true(str_eq(buf, "2026-01-09 23:06:40"));
  }
  end_test_case();

  test_case("strftime answers 0 when the result plus its NUL does not fit");
  {
    micron::time_t t = 1768000000;
    pp::tm_t tm{};
    pp::gmtime_r(&t, &tm);
    char buf[20];
    // exactly 19 bytes of output needs 20 with the NUL, so 20 works and 19 does not
    require(static_cast<long>(pp::strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &tm)), 19L);
    require(static_cast<long>(pp::strftime(buf, 19, "%Y-%m-%d %H:%M:%S", &tm)), 0L);
    require(static_cast<long>(pp::strftime(buf, 0, "%Y", &tm)), 0L);
    require(static_cast<long>(pp::strftime(nullptr, 20, "%Y", &tm)), 0L);
  }
  end_test_case();

  test_case("posix::strftime agrees with chrono::write_strftime");
  {
    const char *fmts[6] = { "%F %T", "%a %b %e", "%j %V %u", "%I %p", "%Y%m%dT%H%M%S", "%s" };
    micron::time_t t = 1768000000;
    pp::tm_t tm{};
    pp::gmtime_r(&t, &tm);
    const ch::civil c = ch::civil_utc(1768000000L);
    for ( int i = 0; i < 6; ++i ) {
      char a[128], b[128];
      const usize na = pp::strftime(a, sizeof(a), fmts[i], &tm);
      const usize nb = ch::write_strftime(b, sizeof(b), fmts[i], c);
      require(static_cast<long>(na), static_cast<long>(nb));
      for ( usize k = 0; k < na; ++k ) require_true(a[k] == b[k]);
    }
  }
  end_test_case();
}

static void
test_strptime(void)
{
  sb::print("=== strptime ===");

  test_case("the ISO shape");
  {
    pp::tm_t tm{};
    const char *r = pp::strptime("2026-08-14 14:22:11", "%Y-%m-%d %H:%M:%S", &tm);
    require_true(r != nullptr);
    require_true(*r == '\0');
    require(tm.tm_year, 126);
    require(tm.tm_mon, 7);
    require(tm.tm_mday, 14);
    require(tm.tm_hour, 14);
    require(tm.tm_min, 22);
    require(tm.tm_sec, 11);
    require(static_cast<long>(pp::timegm(&tm)), 1786717331L);
  }
  end_test_case();

  test_case("%F and %T composites");
  {
    pp::tm_t tm{};
    const char *r = pp::strptime("2026-08-14T14:22:11", "%FT%T", &tm);
    require_true(r != nullptr);
    require(static_cast<long>(pp::timegm(&tm)), 1786717331L);
  }
  end_test_case();

  test_case("month and weekday names, abbreviated and full");
  {
    pp::tm_t a{};
    require_true(pp::strptime("14 Aug 2026", "%d %b %Y", &a) != nullptr);
    require(a.tm_mon, 7);
    pp::tm_t b{};
    require_true(pp::strptime("14 August 2026", "%d %B %Y", &b) != nullptr);
    require(b.tm_mon, 7);
    pp::tm_t c{};
    require_true(pp::strptime("Friday", "%A", &c) != nullptr);
    require(c.tm_wday, 5);
  }
  end_test_case();

  test_case("12-hour input with am/pm");
  {
    pp::tm_t a{};
    require_true(pp::strptime("11:06:40 PM", "%I:%M:%S %p", &a) != nullptr);
    require(a.tm_hour, 23);
    pp::tm_t b{};
    require_true(pp::strptime("12:00:00 AM", "%I:%M:%S %p", &b) != nullptr);
    require(b.tm_hour, 0);
    pp::tm_t c{};
    require_true(pp::strptime("12:00:00 PM", "%I:%M:%S %p", &c) != nullptr);
    require(c.tm_hour, 12);
  }
  end_test_case();

  test_case("%s and %z");
  {
    pp::tm_t a{};
    require_true(pp::strptime("1768000000", "%s", &a) != nullptr);
    require(a.tm_year, 126);
    require(a.tm_mday, 9);
    pp::tm_t b{};
    require_true(pp::strptime("+0200", "%z", &b) != nullptr);
    require(b.tm_gmtoff, 7200L);
  }
  end_test_case();

  test_case("the returned pointer is the first UNCONSUMED character");
  {
    pp::tm_t tm{};
    const char *r = pp::strptime("2026-08-14 leftover", "%Y-%m-%d", &tm);
    require_true(r != nullptr);
    require_true(str_eq(r, " leftover"));
  }
  end_test_case();

  test_case("wday and yday are recomputed from the parsed date");
  {
    pp::tm_t tm{};
    require_true(pp::strptime("2026-01-09", "%Y-%m-%d", &tm) != nullptr);
    require(tm.tm_wday, 5);      // Friday
    require(tm.tm_yday, 8);
  }
  end_test_case();

  test_case("mismatches are rejected");
  {
    pp::tm_t tm{};
    require_true(pp::strptime("2026/08/14", "%Y-%m-%d", &tm) == nullptr);
    require_true(pp::strptime("2026-13-14", "%Y-%m-%d", &tm) == nullptr);
    require_true(pp::strptime("2026-08-14", "%Y-%m-%dT%H", &tm) == nullptr);
    require_true(pp::strptime("xx", "%Y", &tm) == nullptr);
    require_true(pp::strptime(nullptr, "%Y", &tm) == nullptr);
    require_true(pp::strptime("2026", nullptr, &tm) == nullptr);
    require_true(pp::strptime("2026", "%Y", nullptr) == nullptr);
  }
  end_test_case();

  test_case("strftime then strptime round-trips");
  {
    for ( long s = 1735689600L; s < 1767225600L; s += 999983L ) {
      micron::time_t t = static_cast<micron::time_t>(s);
      pp::tm_t out{};
      pp::gmtime_r(&t, &out);
      char buf[64];
      const usize n = pp::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &out);
      require_true(n > 0);
      pp::tm_t back{};
      require_true(pp::strptime(buf, "%Y-%m-%d %H:%M:%S", &back) != nullptr);
      require(static_cast<long>(pp::timegm(&back)), s);
    }
  }
  end_test_case();
}

static void
test_misc(void)
{
  sb::print("=== difftime / usleep / sysconf ===");

  test_case("D.1: difftime is DEFINED, and follows POSIX's argument order");
  {
    // this used to be a bare declaration -- linking this test at all is the regression
    require(micron::difftime(10, 4), 6.0);
    require(micron::difftime(4, 10), -6.0);
    require(micron::difftime(0, 0), 0.0);
    // and micron::timediff is the same value with the arguments swapped
    require(micron::timediff(4, 10), 6.0);
  }
  end_test_case();

  test_case("usleep waits about as long as asked");
  {
    const i64 t0 = ch::mono_ns();
    require(static_cast<long>(pp::usleep(20000)), 0L);      // 20ms
    const i64 t1 = ch::mono_ns();
    require_true(t1 - t0 >= 19000000L);
    require_true(t1 - t0 < 500000000L);
  }
  end_test_case();

  test_case("nsleep too");
  {
    const i64 t0 = ch::mono_ns();
    require(static_cast<long>(pp::nsleep(15000000ull)), 0L);
    const i64 t1 = ch::mono_ns();
    require_true(t1 - t0 >= 14000000L);
  }
  end_test_case();

  test_case("clk_tck comes from the auxv, falling back to 100");
  {
    const long h = pp::clk_tck();
    require_true(h > 0);
    require_true(h <= 10000);
    require(pp::sysconf(pp::_sc_clk_tck), h);
  }
  end_test_case();

  test_case("sysconf answers the page size and refuses what it cannot know");
  {
    const long ps = pp::sysconf(pp::_sc_pagesize);
    require_true(ps >= 4096);
    require_true((ps & (ps - 1)) == 0);      // a power of two
    require(pp::sysconf(9999), -1L);
    require(pp::sysconf(pp::_sc_arg_max), -1L);
  }
  end_test_case();

  test_case("the auxv reader itself");
  {
    // AT_PAGESZ is published by every kernel micron builds for
    require_true(micron::getauxval(micron::at_pagesz) >= 4096);
    require_true(micron::auxval_has(micron::at_pagesz));
    require_false(micron::auxval_has(0xDEADBEEFul));
    require(static_cast<long>(micron::getauxval(0xDEADBEEFul)), 0L);
  }
  end_test_case();

  test_case("adjtimex answers a time_* status code, not a byte count");
  {
    micron::timex_t tx{};
    const i32 r = micron::adjtimex(tx);
    // a NON-NEGATIVE return is one of the time_* codes; time_error just means "clock unsynchronised"
    require_true(r >= 0);
    require_true(r <= micron::time_error);
    // tai_offset returns that field, or < 0
    const i32 tai = micron::tai_offset();
    require_true(tai >= 0);
  }
  end_test_case();

  test_case("sched_rr_get_interval reports a timeslice for this thread");
  {
    micron::timespec_t ts{};
    const i32 r = micron::sched_rr_get_interval(0, ts);
    require(static_cast<long>(r), 0L);
    require_true(ch::ns_of_ts(ts) >= 0);
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::posix time-layer suite");
  sb::print("==============================");
  test_broken_down();
  test_localtime_mktime();
  test_render();
  test_strptime();
  test_misc();
  sb::print("==============================");
  sb::print("ALL POSIX TIME TESTS COMPLETED");
  return 1;
}
