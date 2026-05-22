//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ABCMALLOC SOAK — class (b), single-threaded, deterministic.
//
// A very long (default 1e9 op) run with a heavy- (long-) tailed size
// distribution, designed to flush out corruption that only surfaces after an
// enormous alloc/free history. Three ingredients run together:
//
//   - long-tailed churn: random alloc/free/realloc over a large slot table,
//     sizes mostly tiny with a rare huge tail (drives tier crossings + sheet
//     pressure across the whole spectrum).
//   - a long-lived (pinned) cohort: allocated once, HELD for the entire run,
//     full-verified on every periodic sweep — catches a held block being
//     trampled by the churn around it, or a recycled slot aliasing it.
//   - fragmentation cycling: periodic fill -> strided (checkerboard) free ->
//     refill, forcing hole reuse / new sheets.
//
// Single-threaded + fixed seed => any failure reproduces at the exact op.
// (The concurrent variants live in abcmalloc_concurrent.cpp / abcmalloc_soak_mt.cpp.)
//
// Build:  duck build tests/rigor/abcmalloc_soak.cpp ; run bin/abcmalloc_soak
// Tune :  -D SOAK_OPS=<n>          total ops (default 1e9)
//         -D SOAK_SWEEP_EVERY=<n>  full-verify cadence (default 1e6)
//         -D SOAK_FRAG_EVERY=<n>   fragmentation cycle cadence (default 250000)

#include "../../src/io/console.hpp"

#include "../support/abc_rigor.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;
using abctest::rng_t;

namespace
{

#ifndef SOAK_OPS
#define SOAK_OPS 1000000000ull
#endif
#ifndef SOAK_SWEEP_EVERY
#define SOAK_SWEEP_EVERY 1000000ull
#endif
#ifndef SOAK_FRAG_EVERY
#define SOAK_FRAG_EVERY 250000ull
#endif

constexpr usize CHURN = 8192;      // churn slots
constexpr usize PINNED = 256;      // long-lived cohort

abctest::live_set<CHURN> g_churn;
abctest::live_set<PINNED> g_pin;

// fill all free churn slots, free a rotating checkerboard subset, then refill
// the holes — a deliberate fragmentation wave.
void
frag_cycle(rng_t &r, abctest::counts &c, usize cycle)
{
  for ( usize i = 0; i < CHURN; ++i )
    if ( g_churn.ptr[i] == nullptr ) abctest::do_alloc(g_churn, i, abctest::sample_size_longtail(r), c);

  const usize stride = 2u + (cycle % 4u);      // 2,3,4,5 — varies the hole pattern
  for ( usize i = 0; i < CHURN; i += stride )
    if ( g_churn.ptr[i] ) abctest::do_free(g_churn, i, c);

  for ( usize i = 0; i < CHURN; i += stride )
    if ( g_churn.ptr[i] == nullptr ) abctest::do_alloc(g_churn, i, abctest::sample_size_longtail(r), c);
}

// dump the first corrupt live churn block: its identity, what abc thinks of the
// pointer, where the fingerprint first diverges, and whether any *other* live
// slot's address range overlaps it (which would prove the allocator handed out
// two aliasing live blocks).
void
diagnose(abctest::live_set<CHURN> &ls)
{
  for ( usize s = 0; s < CHURN; ++s ) {
    if ( !ls.ptr[s] ) continue;
    const byte *p = ls.ptr[s];
    const usize n = ls.sz[s];
    const usize key = ls.key(s);
    const u32 g = ls.gen[s];
    if ( abctest::fp_check(p, n, key, g) ) continue;      // this one is fine

    sb::print("  DIAG corrupt churn slot=", s, " key=", key, " gen=", static_cast<usize>(g), " sz=", n);
    sb::print("       ptr=", reinterpret_cast<const void *>(p), " aligned16=", abctest::ptr_aligned(p, 16),
              " within=", abc::within(const_cast<byte *>(p)), " present=", abc::is_present(const_cast<byte *>(p)),
              " qsize=", abc::query_size(const_cast<byte *>(p)));

    // first divergent byte, scanning only the regions we actually wrote
    auto report_off = [&](usize i) {
      const byte e = abctest::fp_byte(key, g, i);
      sb::print("       first mismatch off=", i, " expected=", static_cast<usize>(e), " got=", static_cast<usize>(p[i]));
    };
    bool found = false;
    if ( n <= ABC_FP_FULL_LIMIT ) {
      for ( usize i = 0; i < n && !found; ++i )
        if ( p[i] != abctest::fp_byte(key, g, i) ) {
          report_off(i);
          found = true;
        }
    } else {      // canary windows: head / mid / tail
      const usize wins[3] = { 0u, n / 2u, n - 8u };
      for ( usize w = 0; w < 3 && !found; ++w )
        for ( usize k = 0; k < 8 && !found; ++k ) {
          const usize i = wins[w] + k;
          if ( p[i] != abctest::fp_byte(key, g, i) ) {
            report_off(i);
            found = true;
          }
        }
    }

    // overlap scan against every other live churn slot
    usize overlaps = 0;
    for ( usize t = 0; t < CHURN; ++t ) {
      if ( t == s || !ls.ptr[t] ) continue;
      const byte *q = ls.ptr[t];
      const usize m = ls.sz[t];
      const bool disjoint = (q + m <= p) || (p + n <= q);
      if ( !disjoint ) {
        ++overlaps;
        if ( overlaps <= 4 )
          sb::print("       OVERLAPS live slot=", t, " ptr=", reinterpret_cast<const void *>(q), " sz=", m,
                    " gen=", static_cast<usize>(ls.gen[t]));
      }
    }
    sb::print("       total overlapping live slots=", overlaps);
    return;
  }
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC SOAK (single-threaded, deterministic) ===");
  sb::print("    SOAK_OPS=", static_cast<usize>(SOAK_OPS), " sweep_every=", static_cast<usize>(SOAK_SWEEP_EVERY),
            " frag_every=", static_cast<usize>(SOAK_FRAG_EVERY));

  rng_t r = rng_t::from_seed(0x5040A11ull);
  abctest::counts cnt;
  g_churn.init(0);       // churn keys: [0, CHURN)
  g_pin.init(1000);      // pinned keys: [1000*PINNED, ...) — disjoint from churn

  // long-lived cohort: allocated once, held for the entire run
  for ( usize i = 0; i < PINNED; ++i ) require_true(abctest::do_alloc(g_pin, i, abctest::sample_size_pinned(r), cnt));

  const usize warm_usage = abc::musage();
  usize peak_usage = warm_usage;
  sb::print("    pinned cohort live=", g_pin.live, " warm musage=", warm_usage);

  test_case("soak: long-tail churn + held cohort + fragmentation cycling");
  for ( u64 it = 0; it < static_cast<u64>(SOAK_OPS); ++it ) {
    // periodic: full-verify the held cohort + all live churn, leak tripwire, progress
    if ( (it % static_cast<u64>(SOAK_SWEEP_EVERY)) == 0u ) {
      abctest::verify_all(g_pin, cnt);
      abctest::verify_all(g_churn, cnt);
      const usize u = abc::musage();
      if ( u > peak_usage ) peak_usage = u;
      if ( cnt.hard_errors ) {
        sb::print("   CORRUPTION at op=", static_cast<usize>(it), " first key=", static_cast<usize>(cnt.first_idx),
                  " gen=", static_cast<usize>(cnt.first_gen));
        diagnose(g_churn);
      }
      require_true(cnt.hard_errors == 0);
      // sticky sheets => musage plateaus; a real per-op leak over 1e9 ops blows
      // far past warm+1GiB. (allocs==frees at the end catches accounting leaks.)
      require_true(u <= warm_usage + (1ull << 30));
      if ( (it % (static_cast<u64>(SOAK_SWEEP_EVERY) * 50u)) == 0u )
        sb::print("   op=", static_cast<usize>(it), " musage=", u, " churn_live=", g_churn.live, " allocs=", static_cast<usize>(cnt.allocs),
                  " soft=", static_cast<usize>(cnt.soft));
    }

    if ( it > 0 && (it % static_cast<u64>(SOAK_FRAG_EVERY)) == 0u )
      frag_cycle(r, cnt, static_cast<usize>(it / static_cast<u64>(SOAK_FRAG_EVERY)));

    abctest::churn_step(g_churn, r, cnt, 45u, 35u);
  }
  end_test_case();

  // final verification + teardown
  abctest::verify_all(g_pin, cnt);
  abctest::drain_all(g_pin, cnt);
  abctest::drain_all(g_churn, cnt);
  const usize end_usage = abc::musage();

  sb::print("    done: allocs=", static_cast<usize>(cnt.allocs), " frees=", static_cast<usize>(cnt.frees),
            " reallocs=", static_cast<usize>(cnt.reallocs), " verifies=", static_cast<usize>(cnt.verifies));
  sb::print("    hard_errors=", static_cast<usize>(cnt.hard_errors), " soft(realloc-prefix)=", static_cast<usize>(cnt.soft));
  sb::print("    warm musage=", warm_usage, " peak=", peak_usage, " end=", end_usage);

  require(cnt.hard_errors, static_cast<u64>(0));
  require(cnt.allocs, cnt.frees);
  require_true(end_usage > 0);

  sb::print("=== ABCMALLOC SOAK PASSED ===");
  return 1;
}
