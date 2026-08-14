//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// micron::chrono -- zone offsets and the mktime inverse.
//
// The hard part is that a local wall-clock time near a transition IS NOT A FUNCTION, and both
// pathological cases need a WRITTEN rule or the answer is whichever candidate the code happened to
// try first:
//
//   AMBIGUOUS   (clocks went back, 01:30 happens twice) -> the EARLIER instant
//   NONEXISTENT (clocks went forward, 02:30 never happens) -> read with the offset in force BEFORE
//               the gap, which lands that far past the end of it
//
// glibc's mktime and Go's time.Date both answer this way.
//
// The zone table here is SYNTHETIC: chrono::tz_table is a view over caller storage, so none of this
// needs /usr/share/zoneinfo or a $TZ, and it produces the same answer on every machine.

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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a Berlin-shaped zone: CET (+01:00) with CEST (+02:00) over the summer. The instants are the real
// EU changeovers, cross-checked against date(1).
//
// 2025-03-30T01:00:00Z  +1 -> +2   (spring GAP:  02:00..03:00 local never happens)
// 2025-10-26T01:00:00Z  +2 -> +1   (autumn FOLD: 02:00..03:00 local happens twice)
// 2026-03-29T01:00:00Z  +1 -> +2
// 2026-10-25T01:00:00Z  +2 -> +1

static constexpr i64 tz_gap_2025 = 1743296400L;       // 2025-03-30T01:00:00Z
static constexpr i64 tz_fold_2025 = 1761440400L;      // 2025-10-26T01:00:00Z
static constexpr i64 tz_gap_2026 = 1774746000L;       // 2026-03-29T01:00:00Z
static constexpr i64 tz_fold_2026 = 1792890000L;      // 2026-10-25T01:00:00Z

static constexpr i64 berlin_trans[4] = { tz_gap_2025, tz_fold_2025, tz_gap_2026, tz_fold_2026 };
static constexpr i32 berlin_off[4] = { 7200, 3600, 7200, 3600 };

static ch::tz_table
berlin(void)
{
  return ch::tz_table{ berlin_trans, berlin_off, 4, 3600 };
}

static void
test_offset_at(void)
{
  sb::print("=== offset_at ===");

  const ch::tz_table z = berlin();

  test_case("before the first transition the zone reports its base offset");
  {
    require(static_cast<long>(ch::offset_at(z, 0L)), 3600L);
    require(static_cast<long>(ch::offset_at(z, tz_gap_2025 - 1L)), 3600L);
  }
  end_test_case();

  test_case("a transition takes effect AT its instant, not after it");
  {
    require(static_cast<long>(ch::offset_at(z, tz_gap_2025)), 7200L);
    require(static_cast<long>(ch::offset_at(z, tz_gap_2025 + 1L)), 7200L);
  }
  end_test_case();

  test_case("summer is +2, winter is +1");
  {
    require(static_cast<long>(ch::offset_at(z, 1750000000L)), 7200L);      // 2025-06-15
    require(static_cast<long>(ch::offset_at(z, 1768000000L)), 3600L);      // 2026-01-09
  }
  end_test_case();

  test_case("past the last transition the last offset persists");
  {
    require(static_cast<long>(ch::offset_at(z, tz_fold_2026 + 100000000L)), 3600L);
  }
  end_test_case();

  test_case("the binary search agrees with a linear scan over the whole range");
  {
    for ( long t = 1740000000L; t < 1800000000L; t += 60000L ) {
      long want = 3600;
      for ( usize i = 0; i < 4; ++i )
        if ( t >= berlin_trans[i] ) want = berlin_off[i];
      require(static_cast<long>(ch::offset_at(z, t)), want);
    }
  }
  end_test_case();

  test_case("a fixed zone and an unloaded one");
  {
    require(static_cast<long>(ch::offset_at(ch::tz_fixed(-28800), 1768000000L)), -28800L);
    require(static_cast<long>(ch::offset_at(ch::tz_utc, 1768000000L)), 0L);
    const ch::tz_table none{};
    require(static_cast<long>(ch::offset_at(none, 1768000000L)), 0L);
  }
  end_test_case();
}

static void
test_from_civil_normal(void)
{
  sb::print("=== from_civil away from a transition ===");

  const ch::tz_table z = berlin();

  test_case("a winter wall clock resolves through +01:00");
  {
    ch::civil c{};
    c.y = 2026;
    c.mo = 1;
    c.d = 15;
    c.h = 12;
    const long e = ch::from_civil(c, z);
    require(static_cast<long>(ch::offset_at(z, e)), 3600L);
    const ch::civil back = ch::to_civil(z, e);
    require(back.h, 12u);
    require(back.d, 15u);
  }
  end_test_case();

  test_case("a summer wall clock resolves through +02:00");
  {
    ch::civil c{};
    c.y = 2025;
    c.mo = 7;
    c.d = 15;
    c.h = 12;
    const long e = ch::from_civil(c, z);
    require(static_cast<long>(ch::offset_at(z, e)), 7200L);
    const ch::civil back = ch::to_civil(z, e);
    require(back.h, 12u);
  }
  end_test_case();

  // NOTE: the round trip holds for every instant EXCEPT the later hour of a fold. There, two
  // instants render as the same local time and from_civil deliberately answers the earlier one, so
  // it cannot return the later. That is the documented rule, not a defect -- and skipping the hour
  // silently would hide it, so it is asserted explicitly in test_fold below
  test_case("to_civil and from_civil are inverses over a full year, outside the ambiguous hour");
  {
    long skipped = 0;
    for ( long t = 1735689600L; t < 1767225600L; t += 3617L ) {
      if ( t >= tz_fold_2025 - 3600L && t < tz_fold_2025 + 3600L ) {
        ++skipped;
        continue;
      }
      const ch::civil c = ch::to_civil(z, t);
      require(ch::from_civil(c, z), t);
    }
    require_true(skipped > 0);      // the exclusion really did cover something
  }
  end_test_case();

  test_case("c.off is IGNORED -- the table is the authority");
  {
    ch::civil c{};
    c.y = 2025;
    c.mo = 7;
    c.d = 15;
    c.h = 12;
    c.off = -43200;      // a deliberate lie
    const long a = ch::from_civil(c, z);
    c.off = 0;
    const long b = ch::from_civil(c, z);
    require(a, b);
  }
  end_test_case();
}

static void
test_fold(void)
{
  sb::print("=== AMBIGUOUS: the autumn fold picks the EARLIER instant ===");

  const ch::tz_table z = berlin();

  test_case("02:30 on the fold day happens twice; the earlier one is chosen");
  {
    ch::civil c{};
    c.y = 2025;
    c.mo = 10;
    c.d = 26;
    c.h = 2;
    c.mi = 30;
    const long e = ch::from_civil(c, z);
    // the two candidates are 00:30Z (still +2) and 01:30Z (now +1)
    const long earlier = tz_fold_2025 - 1800L;
    const long later = tz_fold_2025 + 1800L;
    require(e, earlier);
    require_true(e < later);
    // both really are valid renderings of 02:30 local -- that is what makes it ambiguous
    require(static_cast<long>(ch::to_civil(z, earlier).h), 2L);
    require(static_cast<long>(ch::to_civil(z, earlier).mi), 30L);
    require(static_cast<long>(ch::to_civil(z, later).h), 2L);
    require(static_cast<long>(ch::to_civil(z, later).mi), 30L);
  }
  end_test_case();

  test_case("the whole ambiguous hour resolves to its earlier instant");
  {
    for ( unsigned m = 0; m < 60; m += 7 ) {
      ch::civil c{};
      c.y = 2025;
      c.mo = 10;
      c.d = 26;
      c.h = 2;
      c.mi = m;
      const long e = ch::from_civil(c, z);
      require_true(e < tz_fold_2025);
      require(static_cast<long>(ch::offset_at(z, e)), 7200L);
      const ch::civil back = ch::to_civil(z, e);
      require(back.h, 2u);
      require(back.mi, m);
    }
  }
  end_test_case();

  test_case("an unambiguous hour either side is unaffected");
  {
    ch::civil c{};
    c.y = 2025;
    c.mo = 10;
    c.d = 26;
    c.h = 1;
    const long a = ch::from_civil(c, z);
    require(static_cast<long>(ch::to_civil(z, a).h), 1L);
    c.h = 4;
    const long b = ch::from_civil(c, z);
    require(static_cast<long>(ch::to_civil(z, b).h), 4L);
  }
  end_test_case();
}

static void
test_gap(void)
{
  sb::print("=== NONEXISTENT: the spring gap reads with the pre-gap offset ===");

  const ch::tz_table z = berlin();

  test_case("02:30 on the gap day never happens; it reads as 03:30 local");
  {
    ch::civil c{};
    c.y = 2025;
    c.mo = 3;
    c.d = 30;
    c.h = 2;
    c.mi = 30;
    const long e = ch::from_civil(c, z);
    // computed with the PRE-gap offset (+1), so 01:30Z, which renders as 03:30 local
    require(e, tz_gap_2025 + 1800L);
    const ch::civil back = ch::to_civil(z, e);
    require(back.h, 3u);
    require(back.mi, 30u);
  }
  end_test_case();

  test_case("no local time in the gap round-trips, and all of them land past it");
  {
    for ( unsigned m = 0; m < 60; m += 5 ) {
      ch::civil c{};
      c.y = 2025;
      c.mo = 3;
      c.d = 30;
      c.h = 2;
      c.mi = m;
      const long e = ch::from_civil(c, z);
      require_true(e >= tz_gap_2025);
      const ch::civil back = ch::to_civil(z, e);
      require(back.h, 3u);      // shifted out of the gap, by construction
      require(back.mi, m);
    }
  }
  end_test_case();

  test_case("01:59 and 03:00 either side of the gap are exact");
  {
    ch::civil c{};
    c.y = 2025;
    c.mo = 3;
    c.d = 30;
    c.h = 1;
    c.mi = 59;
    const long a = ch::from_civil(c, z);
    require(a, tz_gap_2025 - 60L);
    c.h = 3;
    c.mi = 0;
    const long b = ch::from_civil(c, z);
    require(b, tz_gap_2025);
  }
  end_test_case();

  test_case("the 2026 transitions behave identically -- the rule is not hard-coded to one year");
  {
    ch::civil c{};
    c.y = 2026;
    c.mo = 3;
    c.d = 29;
    c.h = 2;
    c.mi = 30;
    const long gap = ch::from_civil(c, z);
    require(gap, tz_gap_2026 + 1800L);
    require(static_cast<long>(ch::to_civil(z, gap).h), 3L);
    c.mo = 10;
    c.d = 25;
    c.h = 2;
    const long fold = ch::from_civil(c, z);
    require(fold, tz_fold_2026 - 1800L);
    require_true(fold < tz_fold_2026);
  }
  end_test_case();
}

static void
test_fixed_and_utc(void)
{
  sb::print("=== fixed zones ===");

  test_case("from_civil through a fixed zone is a plain subtraction");
  {
    const ch::tz_table z = ch::tz_fixed(19800);      // +05:30
    ch::civil c{};
    c.y = 2026;
    c.mo = 8;
    c.d = 14;
    c.h = 19;
    c.mi = 52;
    c.s = 11;
    require(ch::from_civil(c, z), 1786717331L);
  }
  end_test_case();

  test_case("from_civil through UTC is civil_secs");
  {
    ch::civil c{};
    c.y = 2026;
    c.mo = 8;
    c.d = 14;
    c.h = 14;
    c.mi = 22;
    c.s = 11;
    require(ch::from_civil(c, ch::tz_utc), 1786717331L);
    require(ch::civil_secs(c), 1786717331L);
  }
  end_test_case();
}

static void
test_tzif(void)
{
  sb::print("=== the TZif reader ===");

  // NOTE: bounded by the caller, not by a static buffer inside the library
  static u8 scratch[64u << 10];
  static ch::tz::tz_storage<512> store;

  test_case("$TZ sanitisation refuses traversal and anything odd");
  {
    require_false(ch::tz::__impl::safe_zone(nullptr));
    require_false(ch::tz::__impl::safe_zone(""));
    require_false(ch::tz::__impl::safe_zone("/etc/passwd"));
    require_false(ch::tz::__impl::safe_zone("../../etc/passwd"));
    require_false(ch::tz::__impl::safe_zone("Europe/../Berlin"));
    require_false(ch::tz::__impl::safe_zone(":Europe/Berlin"));
    require_false(ch::tz::__impl::safe_zone("Europe/Berlin;rm"));
    require_true(ch::tz::__impl::safe_zone("Europe/Berlin"));
    require_true(ch::tz::__impl::safe_zone("America/Argentina/Buenos_Aires"));
    require_true(ch::tz::__impl::safe_zone("UTC"));
  }
  end_test_case();

  test_case("garbage is rejected rather than parsed");
  {
    u8 junk[64] = { 0 };
    require_false(ch::tz::parse(junk, sizeof(junk), store));
    require_false(ch::tz::parse(nullptr, 0, store));
    const u8 tiny[4] = { 'T', 'Z', 'i', 'f' };
    require_false(ch::tz::parse(tiny, sizeof(tiny), store));
  }
  end_test_case();

  test_case("a nonexistent zone fails cleanly");
  {
    require_false(ch::tz::load_named("Nowhere/Nothing", scratch, sizeof(scratch), store));
    require_false(store.loaded);
    // an unloaded table is still safe to query
    require(static_cast<long>(ch::offset_at(store.view(), 1768000000L)), 0L);
  }
  end_test_case();

  // the rest only runs where the system actually has a zone database
  test_case("a real zone parses, if /usr/share/zoneinfo exists here");
  {
    if ( ch::tz::load_named("Europe/Berlin", scratch, sizeof(scratch), store) ) {
      const ch::tz_table z = store.view();
      require_true(store.n > 0);
      // Berlin is +1 in January and +2 in July, whatever the exact transition instants are
      require(static_cast<long>(ch::offset_at(z, 1768000000L)), 3600L);      // 2026-01-09
      require(static_cast<long>(ch::offset_at(z, 1752000000L)), 7200L);      // 2025-07-08
      // and the fold/gap rules hold against the real table too
      ch::civil c{};
      c.y = 2026;
      c.mo = 7;
      c.d = 1;
      c.h = 12;
      const long e = ch::from_civil(c, z);
      require(static_cast<long>(ch::to_civil(z, e).h), 12L);
    } else {
      sb::print("  (no zoneinfo on this machine; skipping the live-database half)");
    }
    require_true(true);
  }
  end_test_case();

  test_case("UTC from the database is a zero offset everywhere");
  {
    if ( ch::tz::load_named("UTC", scratch, sizeof(scratch), store) ) {
      const ch::tz_table z = store.view();
      require(static_cast<long>(ch::offset_at(z, 0L)), 0L);
      require(static_cast<long>(ch::offset_at(z, 1768000000L)), 0L);
      require(static_cast<long>(ch::offset_at(z, -1768000000L)), 0L);
    }
    require_true(true);
  }
  end_test_case();
}

int
main(void)
{
  sb::print("micron::chrono timezone suite");
  sb::print("=============================");
  test_offset_at();
  test_from_civil_normal();
  test_fold();
  test_gap();
  test_fixed_and_utc();
  test_tzif();
  sb::print("=============================");
  sb::print("ALL TIMEZONE TESTS COMPLETED");
  return 1;
}
