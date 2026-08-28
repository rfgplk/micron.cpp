//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ABCMALLOC REALLOC SOAK (multi-threaded)
//
// Every live set rotates to a different worker after each epoch. The next
// realloc therefore consumes a block owned by another live arena while that
// owner is concurrently reallocating a third arena's blocks. This pins owner
// lookup, size discovery, caller-arena migration, prefix preservation, and
// remote release under sustained sheet churn.
//
// Build: duck build tests/rigor/abcmalloc_soak_mt_realloc.cpp
// Run:   bin/abcmalloc_soak_mt_realloc
// Tune:  -D SOAK_OPS=<n> -D SOAK_THREADS=<n> -D SOAK_SWEEP_EVERY=<n>

#define MICRON_ABC_MT 1

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

constexpr usize slots = 512;
constexpr u64 base_seed = 0xA110CA7E5EEDull;
constexpr usize tier_edges[] = { 1, 255, 257, 511, 513, 4095, 4097, 32767, 32769, 262143 };
constexpr usize tier_edge_count = sizeof(tier_edges) / sizeof(tier_edges[0]);

abctest::live_set<slots> g_sets[abctest::ABC_MAX_WORKERS];

struct soak_ctx {
  rng_t rng;
  abctest::counts cnt;
  micron::atomic_token<u32> *arrived;
  micron::atomic_token<u64> *phase;
  u64 epochs;
  u64 grows;
  u64 shrinks;
  u64 moves;
  u64 foreign;
  u64 oom;
  usize tid;
  usize nthreads;
};

soak_ctx g_ctx[abctest::ABC_MAX_WORKERS];

usize
next_size(soak_ctx *c, u64 epoch, usize slot)
{
  if ( (slot & 31u) < 2u ) {
    const usize step = static_cast<usize>(epoch + (slot >> 5u)) % tier_edge_count;
    return tier_edges[(slot & 1u) ? tier_edge_count - step - 1 : step];
  }
  return abctest::sample_size_longtail(c->rng);
}

void
barrier(soak_ctx *c, u64 epoch)
{
  const u32 prior = c->arrived->fetch_add(1, micron::memory_order_acq_rel);
  if ( prior + 1 == c->nthreads ) {
    c->arrived->store(0, micron::memory_order_relaxed);
    c->phase->store(epoch + 1, micron::memory_order_release);
    return;
  }
  while ( c->phase->get(micron::memory_order_acquire) == epoch ) __cpu_pause();
}

bool
retained_ok(const byte *ptr, usize old_size, usize retained, usize key, u32 gen)
{
  if ( old_size <= ABC_FP_FULL_LIMIT ) return abctest::fp_check(ptr, retained, key, gen);
  const auto window_ok = [&](usize begin, usize count) {
    const usize end = abctest::mn(begin + count, retained);
    for ( usize offset = begin; offset < end; ++offset )
      if ( ptr[offset] != abctest::fp_byte(key, gen, offset) ) return false;
    return true;
  };
  if ( !window_ok(0, abctest::mn(old_size, static_cast<usize>(8))) ) return false;
  if ( old_size > 16 && !window_ok(old_size / 2, 8) ) return false;
  return old_size <= 8 || window_ok(old_size - 8, 8);
}

void
realloc_foreign(soak_ctx *c, abctest::live_set<slots> &set, usize slot, usize size)
{
  byte *const old = set.ptr[slot];
  const usize old_size = set.sz[slot];
  const usize key = set.key(slot);
  const u32 old_gen = set.gen[slot];
  abc::__arena *const me = abc::__current_arena();
  abc::__arena *const owner = abc::__owner_of(old);

  ++c->cnt.verifies;
  if ( !abctest::fp_check(old, old_size, key, old_gen) ) c->cnt.note_error(key, old_gen, 0);
  if ( owner == nullptr || owner == me )
    c->cnt.note_error(key, old_gen, 1);
  else
    ++c->foreign;

  byte *const next = static_cast<byte *>(abc::realloc(old, size));
  if ( next == nullptr ) {
    ++c->oom;
    return;
  }

  if ( next == old || abc::__owner_of(next) != me )
    c->cnt.note_error(key, old_gen, 2);
  else
    ++c->moves;

  const usize retained = abctest::mn(old_size, size);
  if ( !retained_ok(next, old_size, retained, key, old_gen) ) {
    ++c->cnt.soft;
    c->cnt.note_error(key, old_gen, 3);
  }

  if ( size > old_size )
    ++c->grows;
  else if ( size < old_size )
    ++c->shrinks;

  const u32 next_gen = ++set.gen[slot];
  abctest::fp_write(next, size, key, next_gen);
  set.ptr[slot] = next;
  set.sz[slot] = size;
  ++c->cnt.reallocs;
}

void
soak_worker(soak_ctx *c)
{
  u64 completed = 0;
  u64 next_sweep = SOAK_SWEEP_EVERY;
  for ( u64 epoch = 0; epoch < c->epochs; ++epoch ) {
    const usize set_index = (c->tid + static_cast<usize>(epoch % c->nthreads)) % c->nthreads;
    auto &set = g_sets[set_index];
    for ( usize slot = 0; slot < set.cap(); ++slot ) {
      realloc_foreign(c, set, slot, next_size(c, epoch, slot));
      ++completed;
    }
    if ( completed >= next_sweep ) {
      abctest::verify_all(set, c->cnt);
      next_sweep += SOAK_SWEEP_EVERY;
    }
    barrier(c, epoch);
  }

  const usize final_set = (c->tid + static_cast<usize>((c->epochs - 1) % c->nthreads)) % c->nthreads;
  abctest::verify_all(g_sets[final_set], c->cnt);
  abctest::drain_all(g_sets[final_set], c->cnt);
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC REALLOC SOAK (multi-threaded) ===");

  usize n = SOAK_THREADS;
  if ( n < 2 ) n = 2;
  if ( n > 32 ) n = 32;
  const u64 requested_per = static_cast<u64>(SOAK_OPS) / n;
  const u64 epochs = requested_per == 0 ? 1 : (requested_per + slots - 1) / slots;
  const u64 per = epochs * slots;
  sb::print("    threads=", n, " epochs=", static_cast<usize>(epochs), " reallocs/thread=", static_cast<usize>(per), " total~",
            static_cast<usize>(per * n));
  sb::print("    sweep_every=", static_cast<usize>(SOAK_SWEEP_EVERY), " rotating_slots=", slots);

  micron::atomic_token<u32> arrived{ 0 };
  micron::atomic_token<u64> phase{ 0 };
  const usize base_usage = abc::musage();
  for ( usize worker = 0; worker < n; ++worker ) {
    g_sets[worker].init(worker);
    g_ctx[worker].rng = rng_t::from_seed(base_seed ^ (0x9E3779B97F4A7C15ull * (worker + 1)));
    g_ctx[worker].cnt = abctest::counts{};
    g_ctx[worker].arrived = &arrived;
    g_ctx[worker].phase = &phase;
    g_ctx[worker].epochs = epochs;
    g_ctx[worker].grows = 0;
    g_ctx[worker].shrinks = 0;
    g_ctx[worker].moves = 0;
    g_ctx[worker].foreign = 0;
    g_ctx[worker].oom = 0;
    g_ctx[worker].tid = worker;
    g_ctx[worker].nthreads = n;
    for ( usize slot = 0; slot < g_sets[worker].cap(); ++slot )
      require_true(abctest::do_alloc(g_sets[worker], slot, abctest::sample_size_longtail(g_ctx[worker].rng), g_ctx[worker].cnt));
  }
  const usize warm_usage = abc::musage();

  test_case("mt realloc soak: every epoch migrates every block across live arenas");
  abctest::run_workers(soak_worker, g_ctx, n);
  end_test_case();
  const usize after_usage = abc::musage();

  const abctest::counts total = abctest::sum_counts(g_ctx, n);
  u64 grows = 0, shrinks = 0, moves = 0, foreign = 0, oom = 0;
  for ( usize worker = 0; worker < n; ++worker ) {
    grows += g_ctx[worker].grows;
    shrinks += g_ctx[worker].shrinks;
    moves += g_ctx[worker].moves;
    foreign += g_ctx[worker].foreign;
    oom += g_ctx[worker].oom;
  }

  sb::print("    allocs=", static_cast<usize>(total.allocs), " frees=", static_cast<usize>(total.frees),
            " reallocs=", static_cast<usize>(total.reallocs), " verifies=", static_cast<usize>(total.verifies));
  sb::print("    foreign=", static_cast<usize>(foreign), " moved=", static_cast<usize>(moves), " grows=", static_cast<usize>(grows),
            " shrinks=", static_cast<usize>(shrinks), " oom=", static_cast<usize>(oom));
  sb::print("    hard_errors=", static_cast<usize>(total.hard_errors), " prefix_errors=", static_cast<usize>(total.soft));
  sb::print("    musage base=", base_usage, " warm=", warm_usage, " after=", after_usage);

  require(total.hard_errors, static_cast<u64>(0));
  require(total.soft, static_cast<u64>(0));
  require(total.allocs, total.frees);
  require(foreign, total.reallocs + oom);
  require(moves, total.reallocs);
  require_true(grows != 0 && shrinks != 0);
  require_true(after_usage > 0);
  require_true(after_usage <= warm_usage + n * (384ull << 20));

  sb::print("=== ABCMALLOC REALLOC SOAK (multi-threaded) PASSED ===");
  return 1;
}
