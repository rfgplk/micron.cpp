//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ABCMALLOC SOAK (multi-threaded) — class (b), concurrent endurance.
//
// The single-threaded soak rules (heavy long-tailed sizes, a long-lived "pinned"
// cohort held the whole run, and fragmentation cycling) run concurrently on
// SOAK_THREADS worker threads, with the total op budget SOAK_OPS divided across
// them. Each thread owns its own slot tables, RNG stream, and pinned cohort, so
// the only shared subsystem under test is the allocator itself. This isolates
// *endurance under concurrency* (cross-thread free is covered separately by
// abcmalloc_concurrent.cpp's donation test).
//
// Build:  duck build tests/rigor/abcmalloc_soak_mt.cpp ; run bin/abcmalloc_soak_mt
// Tune :  -D SOAK_OPS=<n>      total ops across all threads (default 1e9)
//         -D SOAK_THREADS=<n>  worker count 8..32 (default 16)
//         -D SOAK_SWEEP_EVERY=<n>  per-thread full-verify cadence (default 1e6)
//         -D SOAK_FRAG_EVERY=<n>   per-thread fragmentation cadence (default 250000)

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
#ifndef SOAK_THREADS
#define SOAK_THREADS 16u
#endif
#ifndef SOAK_SWEEP_EVERY
#define SOAK_SWEEP_EVERY 1000000ull
#endif
#ifndef SOAK_FRAG_EVERY
#define SOAK_FRAG_EVERY 250000ull
#endif

constexpr usize MT_CHURN = 2048;      // per-thread churn slots
constexpr usize MT_PINNED = 64;       // per-thread long-lived cohort
constexpr u64 BASE_SEED = 0x50A11BEEFull;

struct soak_ctx {
  abctest::live_set<MT_CHURN> churn;
  abctest::live_set<MT_PINNED> pin;
  rng_t rng;
  abctest::counts cnt;
  u64 ops;
  usize tid;
};

soak_ctx g_ctx[abctest::ABC_MAX_WORKERS];

void
mt_frag(soak_ctx *c, usize cycle)
{
  auto &ch = c->churn;
  for ( usize i = 0; i < MT_CHURN; ++i )
    if ( ch.ptr[i] == nullptr ) abctest::do_alloc(ch, i, abctest::sample_size_longtail(c->rng), c->cnt);

  const usize stride = 2u + (cycle % 4u);
  for ( usize i = 0; i < MT_CHURN; i += stride )
    if ( ch.ptr[i] ) abctest::do_free(ch, i, c->cnt);

  for ( usize i = 0; i < MT_CHURN; i += stride )
    if ( ch.ptr[i] == nullptr ) abctest::do_alloc(ch, i, abctest::sample_size_longtail(c->rng), c->cnt);
}

void
soak_worker(soak_ctx *c)
{
  // per-thread long-lived cohort, held for this thread's entire run
  for ( usize i = 0; i < MT_PINNED; ++i ) abctest::do_alloc(c->pin, i, abctest::sample_size_pinned(c->rng), c->cnt);

  for ( u64 it = 0; it < c->ops; ++it ) {
    if ( (it % static_cast<u64>(SOAK_SWEEP_EVERY)) == 0u ) {
      abctest::verify_all(c->pin, c->cnt);
      abctest::verify_all(c->churn, c->cnt);
    }
    if ( it > 0 && (it % static_cast<u64>(SOAK_FRAG_EVERY)) == 0u ) mt_frag(c, static_cast<usize>(it / static_cast<u64>(SOAK_FRAG_EVERY)));
    abctest::churn_step(c->churn, c->rng, c->cnt, 45u, 35u);
  }

  abctest::verify_all(c->pin, c->cnt);
  abctest::drain_all(c->pin, c->cnt);
  abctest::drain_all(c->churn, c->cnt);
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC SOAK (multi-threaded) ===");

  usize n = SOAK_THREADS;
  if ( n < 1 ) n = 1;
  if ( n > abctest::ABC_MAX_WORKERS ) n = abctest::ABC_MAX_WORKERS;
  const u64 per = static_cast<u64>(SOAK_OPS) / n;
  sb::print("    threads=", n, " ops/thread=", static_cast<usize>(per), " total~", static_cast<usize>(SOAK_OPS));
  sb::print("    sweep_every=", static_cast<usize>(SOAK_SWEEP_EVERY), " frag_every=", static_cast<usize>(SOAK_FRAG_EVERY));

  for ( usize i = 0; i < n; ++i ) {
    // churn keys: [tid*MT_CHURN, ...); pinned keys high+disjoint: [(1024+tid)*MT_PINNED, ...)
    g_ctx[i].churn.init(i);
    g_ctx[i].pin.init(1024 + i);
    g_ctx[i].rng = rng_t::from_seed(BASE_SEED ^ (0x9E3779B97F4A7C15ull * (i + 1)));
    g_ctx[i].cnt = abctest::counts{};
    g_ctx[i].ops = per;
    g_ctx[i].tid = i;
  }

  const usize base_usage = abc::musage();
  test_case("mt soak: per-thread long-tail churn + held cohort + fragmentation cycling");
  abctest::run_workers(soak_worker, g_ctx, n);
  end_test_case();
  const usize after_usage = abc::musage();

  const abctest::counts t = abctest::sum_counts(g_ctx, n);
  sb::print("    allocs=", static_cast<usize>(t.allocs), " frees=", static_cast<usize>(t.frees),
            " reallocs=", static_cast<usize>(t.reallocs), " verifies=", static_cast<usize>(t.verifies));
  sb::print("    hard_errors=", static_cast<usize>(t.hard_errors), " soft(realloc-prefix)=", static_cast<usize>(t.soft));
  sb::print("    musage base=", base_usage, " after=", after_usage);

  require(t.hard_errors, static_cast<u64>(0));
  require(t.allocs, t.frees);
  require_true(after_usage > 0);
  // sticky sheets never unmap, and each thread brings up its own arena (eager hot
  // tiers + working-set sheets), so the floor scales with thread count, not ops.
  // A real per-op leak over SOAK_OPS would dwarf this thread-proportional bound.
  require_true(after_usage <= base_usage + n * (384ull << 20));

  sb::print("=== ABCMALLOC SOAK (multi-threaded) PASSED ===");
  return 1;
}
