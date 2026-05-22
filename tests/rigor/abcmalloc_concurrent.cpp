//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ABCMALLOC CONCURRENT STRESS — class (a).
//
// True concurrent randomized stress: 8..32 threads each running a randomized
// alloc / free / realloc mix over their own fingerprinted slot table, with NO
// phase barriers — threads run free and overlap for the bulk of the run. This
// reaches contention the 1-producer/1-consumer abcmalloc_mt.cpp cannot.
//
//   test 1  independent concurrent churn   — sweep {8, 16, 32} threads, each
//           thread owns a private live-set, frees its own blocks. Detects
//           cross-thread aliasing / corruption via location-independent
//           fingerprints (idx keyed per (thread, slot), gen bumped per realloc).
//   test 2  cross-thread donation          — a fraction of frees are handed to
//           another thread's inbox; that thread frees them, routing through
//           abc's cross-arena MPSC dealloc path under N-way producer load.
//
// Build:  duck build tests/rigor/abcmalloc_concurrent.cpp ; run bin/abcmalloc_concurrent
// Tune :  -D CONC_OPS=<n>  (per-thread ops, default 2e6)
//
// Arena budget: abc hands out one (never-recycled) arena per allocating thread
// from a pool of 64; main owns arena 0. The churn sweep uses 8+16+32 = 56
// thread lifetimes (arenas 1..56) and donation uses 7 (arenas 57..63), so every
// worker stays on its own arena — required for the donation test to actually
// exercise the *cross*-arena route.

#include "../../src/io/console.hpp"

#include "../support/abc_rigor.hpp"

#include "../snowball/snowball.hpp"
using namespace snowball;
using abctest::rng_t;

namespace
{

#ifndef CONC_OPS
#define CONC_OPS 2000000ull
#endif
constexpr u64 OPS = CONC_OPS;

constexpr u64 BASE_SEED = 0xC0FFEE12345ull;

// ── test 1: independent concurrent churn ────────────────────────────────────

constexpr usize SLOTS = 1024;

struct churn_ctx {
  abctest::live_set<SLOTS> ls;
  rng_t rng;
  abctest::counts cnt;
  u64 ops;
};

churn_ctx g_churn[abctest::ABC_MAX_WORKERS];

void
churn_worker(churn_ctx *c)
{
  // 45% alloc / 35% free / 20% realloc => ~55% steady-state occupancy, no barriers
  for ( u64 it = 0; it < c->ops; ++it ) abctest::churn_step(c->ls, c->rng, c->cnt, 45u, 35u);
  abctest::drain_all(c->ls, c->cnt);      // teardown: verify + free all still held
}

// ── test 2: cross-thread donation (N-way MPSC) ──────────────────────────────

constexpr usize DSLOTS = 512;
constexpr usize INBOX = 256;
constexpr usize DON_THREADS = 7;      // fits arenas 57..63 after the churn sweep

struct don_entry {
  byte *ptr;
  usize idx;
  u32 gen;
  usize size;
};

struct don_ctx {
  abctest::live_set<DSLOTS> ls;
  rng_t rng;
  abctest::counts cnt;
  don_entry inbox[INBOX];
  usize ihead;
  usize itail;
  micron::mutex mtx;
  don_ctx *all;
  usize tid;
  usize nthreads;
  u64 ops;
  micron::atomic_token<u32> *done;
};

don_ctx g_don[abctest::ABC_MAX_WORKERS];

// push a donated free onto victim v's inbox; false if full (caller frees itself)
bool
inbox_push(don_ctx *v, const don_entry &e)
{
  v->mtx.lock();
  bool ok = false;
  const usize nxt = (v->itail + 1u) % INBOX;
  if ( nxt != v->ihead ) {
    v->inbox[v->itail] = e;
    v->itail = nxt;
    ok = true;
  }
  v->mtx.unlock();
  return ok;
}

// drain my inbox: verify each donated block then free it. e.ptr belongs to the
// donor's arena, so abc::dealloc routes cross-arena via the owner's MPSC.
void
inbox_drain(don_ctx *c)
{
  for ( ;; ) {
    don_entry e{};
    bool got = false;
    c->mtx.lock();
    if ( c->ihead != c->itail ) {
      e = c->inbox[c->ihead];
      c->ihead = (c->ihead + 1u) % INBOX;
      got = true;
    }
    c->mtx.unlock();
    if ( !got ) break;
    ++c->cnt.verifies;
    if ( !abctest::fp_check(e.ptr, e.size, e.idx, e.gen) ) c->cnt.note_error(e.idx, e.gen, 0);
    abc::dealloc(e.ptr);
    ++c->cnt.frees;
  }
}

void
don_worker(don_ctx *c)
{
  auto &ls = c->ls;
  auto &r = c->rng;
  for ( u64 it = 0; it < c->ops; ++it ) {
    if ( (it & 0x3Fu) == 0u ) inbox_drain(c);      // service frees routed to me

    const u32 op = static_cast<u32>(r.next() % 100u);
    const usize s = static_cast<usize>(r.next() % DSLOTS);

    if ( op < 50u ) {
      if ( ls.ptr[s] == nullptr ) abctest::do_alloc(ls, s, abctest::sample_size_longtail(r), c->cnt);
    } else {
      if ( ls.ptr[s] ) {
        ++c->cnt.verifies;
        if ( !abctest::fp_check(ls.ptr[s], ls.sz[s], ls.key(s), ls.gen[s]) ) c->cnt.note_error(ls.key(s), ls.gen[s], 0);
        bool donated = false;
        if ( c->nthreads > 1 && (r.next() % 4u) == 0u ) {      // ~25% of frees donated
          const usize v = static_cast<usize>(r.next() % c->nthreads);
          if ( v != c->tid ) {
            const don_entry e{ ls.ptr[s], ls.key(s), ls.gen[s], ls.sz[s] };
            donated = inbox_push(&c->all[v], e);
          }
        }
        if ( !donated ) {
          abc::dealloc(ls.ptr[s]);
          ++c->cnt.frees;
        }
        ls.ptr[s] = nullptr;
        ls.sz[s] = 0;
        --ls.live;
      }
    }
  }

  // producers stop together; keep draining (mine + my arena's MPSC) until all done
  c->done->fetch_add(1u, micron::memory_order_acq_rel);
  while ( c->done->get(micron::memory_order_acquire) != static_cast<u32>(c->nthreads) ) {
    inbox_drain(c);
    abc::__current_arena()->__maybe_drain();
    __cpu_pause();
  }
  for ( int k = 0; k < 8; ++k ) {      // sweep stragglers donated right at the end
    inbox_drain(c);
    abc::__current_arena()->__maybe_drain();
  }

  abctest::drain_all(ls, c->cnt);      // free my own remaining live blocks
}

};      // namespace

int
main(void)
{
  sb::print("=== ABCMALLOC CONCURRENT STRESS ===");
  sb::print("    per-thread ops: ", static_cast<usize>(OPS));

  // ── test 1: independent concurrent churn, swept over thread counts ─────────
  const usize sweep[] = { 8u, 16u, 32u };
  for ( usize k = 0; k < 3; ++k ) {
    const usize n = sweep[k];
    test_case("concurrent independent churn (alloc/free/realloc, no barriers)");
    sb::print("  -- threads: ", n);

    for ( usize i = 0; i < n; ++i ) {
      g_churn[i].ls.init(i);
      g_churn[i].rng = rng_t::from_seed(BASE_SEED ^ (0x9E3779B97F4A7C15ull * (i + 1)) ^ (static_cast<u64>(n) << 40));
      g_churn[i].cnt = abctest::counts{};
      g_churn[i].ops = OPS;
    }

    const usize base_usage = abc::musage();
    abctest::run_workers(churn_worker, g_churn, n);
    const usize after_usage = abc::musage();

    const abctest::counts t = abctest::sum_counts(g_churn, n);
    sb::print("     allocs=", static_cast<usize>(t.allocs), " frees=", static_cast<usize>(t.frees),
              " reallocs=", static_cast<usize>(t.reallocs), " verifies=", static_cast<usize>(t.verifies));
    sb::print("     hard_errors=", static_cast<usize>(t.hard_errors), " soft(realloc-prefix)=", static_cast<usize>(t.soft));
    sb::print("     musage base=", base_usage, " after=", after_usage);

    require(t.hard_errors, static_cast<u64>(0));      // zero corruption
    require(t.allocs, t.frees);                       // every block freed exactly once
    require_true(after_usage > 0);                    // sane (sticky sheets => no return-to-zero)
    end_test_case();
  }

  // ── test 2: cross-thread donation (N-way MPSC) ────────────────────────────
  test_case("cross-thread donation: free across arenas via MPSC under N-way load");
  {
    const usize n = DON_THREADS;
    micron::atomic_token<u32> done{ 0 };
    for ( usize i = 0; i < n; ++i ) {
      g_don[i].ls.init(i);
      g_don[i].rng = rng_t::from_seed(BASE_SEED ^ 0xD07A7104ull ^ (0xBF58476D1CE4E5B9ull * (i + 1)));
      g_don[i].cnt = abctest::counts{};
      g_don[i].ihead = 0;
      g_don[i].itail = 0;
      g_don[i].all = g_don;
      g_don[i].tid = i;
      g_don[i].nthreads = n;
      g_don[i].ops = OPS;
      g_don[i].done = &done;
    }
    sb::print("  -- donation threads: ", n);

    const usize base_usage = abc::musage();
    abctest::run_workers(don_worker, g_don, n);
    abc::__current_arena()->__maybe_drain();      // reclaim anything routed to main's arena
    const usize after_usage = abc::musage();

    const abctest::counts t = abctest::sum_counts(g_don, n);
    sb::print("     allocs=", static_cast<usize>(t.allocs), " frees=", static_cast<usize>(t.frees),
              " verifies=", static_cast<usize>(t.verifies));
    sb::print("     hard_errors=", static_cast<usize>(t.hard_errors), " soft=", static_cast<usize>(t.soft));
    sb::print("     musage base=", base_usage, " after=", after_usage);

    require(t.hard_errors, static_cast<u64>(0));
    require(t.allocs, t.frees);      // aggregate balance (each block freed by whoever owns the dealloc)
    require_true(after_usage > 0);
  }
  end_test_case();

  sb::print("=== ALL ABCMALLOC CONCURRENT TESTS PASSED ===");
  return 1;
}
