//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// async_barrier / fork_group seal-and-drain.
//
// the last arriver used to bump the generation and THEN drain the waiter list as two separate
// steps, while push_unless evaluates its predicate under the list lock. a waiter that refused to
// park for generation G (correct) could re-arrive, park for G+1, and be drained by the stale
// generation-G waker -- it then passes G+1 on its own and the barrier stays desynchronised for
// every round after. t_sync caught this ~3 times in 10 on arm64-qemu and never on x64; this test
// leans on the window directly so it is catchable natively too.
//
// the small K values matter: with K=2 a single refuse-then-rearm is enough to desync, so the
// window does not need eight participants to line up.

#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;

static int FAILS = 0;

static constexpr int MAX_K = 16;
static constexpr int MAX_R = 400;

static coro::async_barrier *g_bar = nullptr;
static micron::atomic_token<u32> g_arrived[MAX_R];
static micron::atomic_token<u32> g_fail{ 0 };
static micron::atomic_token<u32> g_passed{ 0 };

static micron::task<void>
cyc_worker(int rounds, int K, int skew)
{
  for ( int r = 0; r < rounds; ++r ) {
    // uneven arrival: some participants burn a little before arriving, which widens the gap
    // between the last arriver's generation bump and its drain
    for ( int s = 0; s < skew * (r & 3); ++s ) __cpu_pause();
    g_arrived[r].fetch_add(1, micron::memory_order_acq_rel);
    co_await g_bar->arrive_and_wait();
    if ( g_arrived[r].get(micron::memory_order_acquire) != (u32)K ) g_fail.fetch_add(1, micron::memory_order_acq_rel);
    g_passed.fetch_add(1, micron::memory_order_acq_rel);
  }
}

static micron::task<void>
cyc_root(int K, int rounds, int skew)
{
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, cyc_worker)(rounds, K, skew * k);
  co_await coro::join;
}

static void
cycle(int K, int R, int skew)
{
  coro::async_barrier bar((u32)K);
  g_bar = &bar;
  g_fail.store(0, micron::memory_order_relaxed);
  g_passed.store(0, micron::memory_order_relaxed);
  for ( int r = 0; r < R; ++r ) g_arrived[r].store(0, micron::memory_order_relaxed);
  coro::sync_wait(cyc_root(K, R, skew));
  // no participant may observe a partial arrival count...
  sb::check(g_fail.get(micron::memory_order_acquire) == 0);
  // ...and every participant must clear every round (a desync also loses passes)
  sb::check(g_passed.get(micron::memory_order_acquire) == (u32)(K * R));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fork_group has the same shape: __outstanding is re-armable via spawn(), so a spawn+join landing
// between sub_fetch==0 and the drain would wake a joiner that must still wait

static micron::atomic_token<i64> g_fg_done{ 0 };

static micron::task<void>
fg_leaf()
{
  g_fg_done.fetch_add(1, micron::memory_order_acq_rel);
  co_return;
}

static micron::task<void>
fg_churn(coro::fork_group *g, int rounds, int per)
{
  for ( int r = 0; r < rounds; ++r ) {
    for ( int i = 0; i < per; ++i ) g->spawn(fg_leaf);
    co_await g->join();
    // join must not return while work is still outstanding
    sb::check(g->outstanding() <= 0);
  }
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("async_barrier: cyclic reuse, K=2 (narrowest desync window)");
  cycle(2, MAX_R, 0);
  sb::end_test_case();

  sb::test_case("async_barrier: cyclic reuse, K=2 with skewed arrival");
  cycle(2, MAX_R, 24);
  sb::end_test_case();

  sb::test_case("async_barrier: cyclic reuse, K=3");
  cycle(3, MAX_R, 8);
  sb::end_test_case();

  sb::test_case("async_barrier: cyclic reuse, K=8 skewed");
  cycle(8, 200, 16);
  sb::end_test_case();

  sb::test_case("async_barrier: cyclic reuse, K=16");
  cycle(MAX_K, 120, 4);
  sb::end_test_case();

  sb::test_case("fork_group: join never returns with work outstanding");
  {
    coro::fork_group g;
    g_fg_done.store(0, micron::memory_order_relaxed);
    coro::sync_wait(fg_churn(&g, 60, 8));
    sb::check(g_fg_done.get(micron::memory_order_acquire) == 60 * 8);
    sb::check(g.outstanding() <= 0);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  if ( FAILS != 0 ) return 0;
  sb::print("=== ALL BARRIER CYCLE TESTS PASSED ===");
  return 1;
}
