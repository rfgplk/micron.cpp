//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ABCMALLOC ADVERSARIAL CONCURRENCY PATTERNS.
//
// Where abcmalloc_concurrent.cpp does statistically-random per-thread churn,
// this suite drives the *deterministic contention shapes* and *reuse races*
// where allocator bugs concentrate. It layers a global live-address registry
// over every alloc/free so cross-thread reuse-while-live is caught structurally,
// in addition to the payload fingerprints.
//
// Mapping to the requested adversarial modes:
//   (1) cross-thread reuse-after-free race + (4) ABA / freelist structural
//       corruption  -> abctest::live_registry: note_alloc() AFTER abc::alloc,
//       note_free() BEFORE abc::dealloc. Any address handed to two owners whose
//       live windows overlap is a collision. (A literal "poison freed memory and
//       re-verify it stays poisoned" check races with honest reuse and
//       false-positives; the registry pins the same invariant race-free.)
//   (2) per-thread arena migration / TLS-reuse pressure -> test C spawns repeated
//       *waves of short-lived* workers: each wave is a fresh batch of OS threads
//       (fresh TLS, fresh arena claim) that runs a burst and exits, so the next
//       wave reuses TLS slots and claims new arenas.
//   (3a) size-lockstep contention -> test A: all threads allocate the SAME size
//        class simultaneously (barrier-synchronised bursts), maximising
//        concurrent same-tier sheet creation / VA carving.
//   (3b) phase-shifted size waves -> test B: thread i runs class (phase+i)%K, so
//        every tier is under concurrent pressure from a rotating set of threads.
//
// Arena budget (abc: 1 never-recycled arena/allocating-thread, cap 64; main owns
// arena 0): A(ADV_THREADS) + B(ADV_THREADS) + C(BURST_WAVES*BURST_WAVE_THREADS)
// is kept <= 63 so every worker gets its own arena.
//
// Build: duck build tests/rigor/abcmalloc_adversarial.cpp ; run bin/abcmalloc_adversarial

#include "../../src/io/console.hpp"

#include "../support/abc_rigor.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;
using abctest::rng_t;

namespace
{

#ifndef ADV_THREADS
#define ADV_THREADS 8u
#endif
#ifndef ADV_PHASES
#define ADV_PHASES 160u
#endif
#ifndef BURST_WAVES
#define BURST_WAVES 10u
#endif
#ifndef BURST_WAVE_THREADS
#define BURST_WAVE_THREADS 4u
#endif
#ifndef BURST_OPS
#define BURST_OPS 200000ull
#endif

constexpr u64 BASE_SEED = 0xADE12A11ull;

abctest::live_registry g_reg;      // 8 MiB, shared across all threads

// representative size per tier: precise / small / medium / large / huge
constexpr usize kClassSizes[] = { 200u, 480u, 3200u, 24000u, 200000u };
constexpr usize kNClass = sizeof(kClassSizes) / sizeof(kClassSizes[0]);

// ── registry + fingerprint aware alloc/free ─────────────────────────────────

template<usize S>
inline void
radv_alloc(abctest::live_set<S> &ls, usize s, usize n, abctest::counts &c)
{
  byte *p = abc::alloc(n);
  if ( !p ) [[unlikely]]
    return;
  if ( g_reg.note_alloc(p) )      // address already live elsewhere -> double alloc
    c.note_error(ls.key(s), ls.gen[s], 0);
  const u32 g = ++ls.gen[s];
  abctest::fp_write(p, n, ls.key(s), g);
  ls.ptr[s] = p;
  ls.sz[s] = n;
  ++ls.live;
  ++c.allocs;
}

template<usize S>
inline void
radv_free(abctest::live_set<S> &ls, usize s, abctest::counts &c)
{
  ++c.verifies;
  if ( !abctest::fp_check(ls.ptr[s], ls.sz[s], ls.key(s), ls.gen[s]) ) c.note_error(ls.key(s), ls.gen[s], 0);
  g_reg.note_free(ls.ptr[s]);      // remove BEFORE dealloc: closes the free->reuse window
  abc::dealloc(ls.ptr[s]);
  ls.ptr[s] = nullptr;
  ls.sz[s] = 0;
  --ls.live;
  ++c.frees;
}

// ── sense-reversing barrier (reusable across bursts) ────────────────────────

struct barrier_t {
  micron::atomic_token<u32> count;
  micron::atomic_token<u32> sense;
  u32 n;
};

inline void
barrier_wait(barrier_t &b, u32 &my_sense)
{
  my_sense ^= 1u;
  if ( b.count.add_fetch(1u, micron::memory_order_acq_rel) == b.n ) {
    b.count.store(0u, micron::memory_order_release);
    b.sense.store(my_sense, micron::memory_order_release);
  } else {
    while ( b.sense.get(micron::memory_order_acquire) != my_sense ) __cpu_pause();
  }
}

barrier_t g_bar;

// ── tests A & B: lockstep / phase-shifted size bursts ───────────────────────

constexpr usize WAVE_SLOTS = 128;

struct wave_ctx {
  abctest::live_set<WAVE_SLOTS> ls;
  rng_t rng;
  abctest::counts cnt;
  usize tid;
  usize phases;
  bool shifted;      // false: lockstep (3a); true: phase-shifted waves (3b)
};

wave_ctx g_wave[abctest::ABC_MAX_WORKERS];

void
wave_worker(wave_ctx *c)
{
  u32 sense = 0;
  for ( usize ph = 0; ph < c->phases; ++ph ) {
    const usize cls = (c->shifted ? (ph + c->tid) : ph) % kNClass;
    const usize n = kClassSizes[cls];

    barrier_wait(g_bar, sense);      // all threads enter the burst together
    for ( usize i = 0; i < WAVE_SLOTS; ++i ) radv_alloc(c->ls, i, n, c->cnt);

    barrier_wait(g_bar, sense);      // all threads free together
    for ( usize i = 0; i < WAVE_SLOTS; ++i )
      if ( c->ls.ptr[i] ) radv_free(c->ls, i, c->cnt);
  }
}

void
run_wave_test(const char *name, usize n, bool shifted)
{
  test_case(name);
  sb::print("  -- threads: ", n, " phases: ", static_cast<usize>(ADV_PHASES), shifted ? " (phase-shifted)" : " (lockstep)");
  g_bar.count.store(0u);
  g_bar.sense.store(0u);
  g_bar.n = static_cast<u32>(n);
  for ( usize i = 0; i < n; ++i ) {
    g_wave[i].ls.init(i);
    g_wave[i].rng = rng_t::from_seed(BASE_SEED ^ (0x9E3779B97F4A7C15ull * (i + 1)) ^ (shifted ? 0xBBu : 0xAAu));
    g_wave[i].cnt = abctest::counts{};
    g_wave[i].tid = i;
    g_wave[i].phases = ADV_PHASES;
    g_wave[i].shifted = shifted;
  }
  const u64 col0 = g_reg.collisions.get();
  abctest::run_workers(wave_worker, g_wave, n);
  const abctest::counts t = abctest::sum_counts(g_wave, n);
  sb::print("     allocs=", static_cast<usize>(t.allocs), " frees=", static_cast<usize>(t.frees),
            " verifies=", static_cast<usize>(t.verifies), " hard_errors=", static_cast<usize>(t.hard_errors));
  sb::print("     registry collisions(+)=", static_cast<usize>(g_reg.collisions.get() - col0),
            " tracked-now=", static_cast<usize>(g_reg.tracked.get()));
  require(t.hard_errors, static_cast<u64>(0));
  require(t.allocs, t.frees);
  require(g_reg.collisions.get(), col0);                  // no double-allocations this test
  require(g_reg.tracked.get(), static_cast<u64>(0));      // everything freed -> registry empty
  end_test_case();
}

// ── test C: worker-burst churn (arena migration / TLS-reuse pressure) ───────

constexpr usize BURST_SLOTS = 512;

struct burst_ctx {
  abctest::live_set<BURST_SLOTS> ls;
  rng_t rng;
  abctest::counts cnt;
  u64 ops;
};

burst_ctx g_burst[abctest::ABC_MAX_WORKERS];

void
burst_worker(burst_ctx *c)
{
  for ( u64 it = 0; it < c->ops; ++it ) {
    const u32 op = static_cast<u32>(c->rng.next() % 100u);
    const usize s = static_cast<usize>(c->rng.next() % BURST_SLOTS);
    if ( op < 55u ) {
      if ( c->ls.ptr[s] == nullptr ) radv_alloc(c->ls, s, abctest::sample_size_longtail(c->rng), c->cnt);
    } else {
      if ( c->ls.ptr[s] ) radv_free(c->ls, s, c->cnt);
    }
  }
  for ( usize i = 0; i < BURST_SLOTS; ++i )
    if ( c->ls.ptr[i] ) radv_free(c->ls, i, c->cnt);
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC ADVERSARIAL CONCURRENCY ===");

  // (3a) size-lockstep: all threads hammer one size class at once
  run_wave_test("size-lockstep bursts: all threads allocate one size class in unison", ADV_THREADS, false);

  // (3b) phase-shifted waves: every tier under concurrent rotating pressure
  run_wave_test("phase-shifted waves: thread i allocates class (phase+i)%K", ADV_THREADS, true);

  // (2) arena migration / TLS-reuse: repeated waves of short-lived workers
  test_case("worker-burst churn: short-lived thread waves (arena claim / TLS reuse)");
  {
    const usize m = BURST_WAVE_THREADS;
    abctest::counts total;
    const u64 col0 = g_reg.collisions.get();
    for ( usize wave = 0; wave < BURST_WAVES; ++wave ) {
      for ( usize i = 0; i < m; ++i ) {
        g_burst[i].ls.init(wave * m + i);      // globally-unique owner across waves
        g_burst[i].rng = rng_t::from_seed(BASE_SEED ^ (static_cast<u64>(wave) << 24) ^ (0xBF58476D1CE4E5B9ull * (i + 1)));
        g_burst[i].cnt = abctest::counts{};
        g_burst[i].ops = BURST_OPS;
      }
      abctest::run_workers(burst_worker, g_burst, m);
      const abctest::counts w = abctest::sum_counts(g_burst, m);
      total.allocs += w.allocs;
      total.frees += w.frees;
      total.verifies += w.verifies;
      total.hard_errors += w.hard_errors;
      require(w.hard_errors, static_cast<u64>(0));            // fail fast per wave
      require(g_reg.tracked.get(), static_cast<u64>(0));      // each wave fully drains
    }
    sb::print("  -- waves: ", static_cast<usize>(BURST_WAVES), " x ", m, " short-lived threads");
    sb::print("     allocs=", static_cast<usize>(total.allocs), " frees=", static_cast<usize>(total.frees),
              " verifies=", static_cast<usize>(total.verifies), " hard_errors=", static_cast<usize>(total.hard_errors));
    sb::print("     registry collisions(+)=", static_cast<usize>(g_reg.collisions.get() - col0));
    require(total.hard_errors, static_cast<u64>(0));
    require(total.allocs, total.frees);
    require(g_reg.collisions.get(), col0);
  }
  end_test_case();

  sb::print("    total registry collisions: ", static_cast<usize>(g_reg.collisions.get()));
  require(g_reg.collisions.get(), static_cast<u64>(0));
  sb::print("=== ALL ABCMALLOC ADVERSARIAL TESTS PASSED ===");
  return 1;
}
