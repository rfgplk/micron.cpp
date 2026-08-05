//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// The coroutine synchronisation primitives under real contention.
//
// tests/coro/t_sync.cpp touches async_mutex, async_latch, async_barrier, async_condvar and
// scoped_lock at low width. Three of the primitives have NO test anywhere in the tree:
//
//   manual_reset_event   set / reset / is_set and the awaiter
//   async_semaphore      acquire / try_acquire / try_acquire_many / release / abort
//   async_lock_guard     the RAII form (only the scoped_lock factory is touched)
//
// and async_rwlock is constructed but never checked for the property that makes it an rwlock.

#define MICRON_ABC_MT 1
#define MICRON_CORO_URING

#include "../../src/stdcoro.hpp"

#include "../snowball/snowball.hpp"
#include "../support/lifetime.hpp"

using namespace snowball;
namespace coro = micron::coro;

namespace
{

constexpr u64 ROUNDS = 24;
constexpr usize CONTENDERS = 64;
constexpr u64 PER = 64;

u64 g_plain = 0;
coro::async_mutex g_mtx;
micron::atomic_token<u64> g_critical_now{ 0 };
micron::atomic_token<u64> g_overlap{ 0 };

micron::task<i64>
mutex_worker(u64 n)
{
  for ( u64 i = 0; i < n; ++i ) {
    co_await g_mtx.lock();
    if ( g_critical_now.add_fetch(1, micron::memory_order_acq_rel) != 1 ) g_overlap.fetch_add(1, micron::memory_order_relaxed);
    ++g_plain;
    g_critical_now.sub_fetch(1, micron::memory_order_acq_rel);
    g_mtx.unlock();
  }
  co_return static_cast<i64>(n);
}

micron::task<i64>
guard_worker(u64 n)
{
  for ( u64 i = 0; i < n; ++i ) {
    auto g = co_await coro::scoped_lock(g_mtx);
    if ( g_critical_now.add_fetch(1, micron::memory_order_acq_rel) != 1 ) g_overlap.fetch_add(1, micron::memory_order_relaxed);
    ++g_plain;
    g_critical_now.sub_fetch(1, micron::memory_order_acq_rel);
  }
  co_return static_cast<i64>(n);
}

micron::task<i64>
trylock_worker(u64 n)
{
  i64 got = 0;
  for ( u64 i = 0; i < n; ++i ) {
    if ( g_mtx.try_lock() ) {
      if ( g_critical_now.add_fetch(1, micron::memory_order_acq_rel) != 1 ) g_overlap.fetch_add(1, micron::memory_order_relaxed);
      ++g_plain;
      g_critical_now.sub_fetch(1, micron::memory_order_acq_rel);
      g_mtx.unlock();
      ++got;
    }
    co_await coro::reschedule();
  }
  co_return got;
}

coro::manual_reset_event g_ev;
micron::atomic_token<u64> g_passed{ 0 };

micron::task<i64>
event_waiter(void)
{
  co_await g_ev;
  g_passed.fetch_add(1, micron::memory_order_relaxed);
  co_return 1;
}

micron::task<i64>
event_round(usize n)
{
  g_ev.reset();
  g_passed.store(0, micron::memory_order_release);
  if ( g_ev.is_set() ) co_return -5000;

  coro::eventual<micron::vector<i64>> e;
  co_await coro::fork(
      &e, +[](usize k) -> micron::task<micron::vector<i64>> {
        co_return co_await coro::spawn_many(k, [](usize) -> micron::task<i64> { co_return co_await event_waiter(); });
      })(n);

  for ( int s = 0; s < 64; ++s ) co_await coro::reschedule();
  g_ev.set();
  if ( !g_ev.is_set() ) co_return -5001;

  co_await coro::join;
  auto got = micron::move(e).operator*();
  if ( got.size() != n ) co_return -5002;
  if ( g_passed.get(micron::memory_order_acquire) != n ) co_return -5003;

  co_await g_ev;
  co_return 0;
}

coro::async_semaphore g_sem{ 0 };
micron::atomic_token<u64> g_inside{ 0 };
micron::atomic_token<u64> g_over_cap{ 0 };

micron::task<i64>
sem_worker(u64 n, u64 cap)
{
  for ( u64 i = 0; i < n; ++i ) {
    co_await g_sem.acquire();

    if ( g_inside.add_fetch(1, micron::memory_order_acq_rel) > cap ) g_over_cap.fetch_add(1, micron::memory_order_relaxed);
    co_await coro::reschedule();
    g_inside.sub_fetch(1, micron::memory_order_acq_rel);
    g_sem.release();
  }
  co_return static_cast<i64>(n);
}

coro::async_rwlock g_rw;
micron::atomic_token<i64> g_readers{ 0 };
micron::atomic_token<i64> g_writers{ 0 };
micron::atomic_token<u64> g_rw_violation{ 0 };
u64 g_rw_plain = 0;

void
rw_sample(void) noexcept
{
  const i64 w = g_writers.get(micron::memory_order_acquire);
  const i64 r = g_readers.get(micron::memory_order_acquire);
  if ( w > 1 || (w > 0 && r > 0) ) g_rw_violation.fetch_add(1, micron::memory_order_relaxed);
}

micron::task<i64>
rw_reader(u64 n)
{
  u64 seen = 0;
  for ( u64 i = 0; i < n; ++i ) {
    co_await g_rw.lock_shared();
    g_readers.fetch_add(1, micron::memory_order_acq_rel);
    rw_sample();
    seen += g_rw_plain;
    g_readers.fetch_sub(1, micron::memory_order_acq_rel);
    g_rw.unlock_shared();
    co_await coro::reschedule();
  }
  co_return static_cast<i64>(seen == 0 ? 0 : 1);
}

micron::task<i64>
rw_writer(u64 n)
{
  for ( u64 i = 0; i < n; ++i ) {
    co_await g_rw.lock();
    g_writers.fetch_add(1, micron::memory_order_acq_rel);
    rw_sample();
    ++g_rw_plain;
    g_writers.fetch_sub(1, micron::memory_order_acq_rel);
    g_rw.unlock();
    co_await coro::reschedule();
  }
  co_return static_cast<i64>(n);
}

micron::task<i64>
latch_round(usize n)
{

  coro::async_latch latch{ static_cast<i64>(n / 2u) };
  micron::atomic_token<u64> after{ 0 };

  coro::eventual<micron::vector<i64>> e;
  co_await coro::fork(
      &e, +[](coro::async_latch *l, micron::atomic_token<u64> *a, usize k) -> micron::task<micron::vector<i64>> {
        co_return co_await coro::spawn_many(k, [l, a](usize i) -> micron::task<i64> {
          if ( i % 2u ) {
            co_await *l;
            a->fetch_add(1, micron::memory_order_relaxed);
          } else {
            l->count_down();
          }
          co_return 1;
        });
      })(&latch, &after, n);

  co_await coro::join;
  auto got = micron::move(e).operator*();
  if ( got.size() != n ) co_return -5100;
  if ( !latch.try_wait() ) co_return -5101;
  if ( after.get(micron::memory_order_acquire) != n / 2u ) co_return -5102;
  co_return 0;
}

micron::task<i64>
barrier_round(u32 parties, u32 gens)
{
  coro::async_barrier bar{ parties };
  micron::atomic_token<u64> arrived{ 0 };

  coro::eventual<micron::vector<i64>> e;
  co_await coro::fork(
      &e, +[](coro::async_barrier *b, micron::atomic_token<u64> *a, usize p, u32 g) -> micron::task<micron::vector<i64>> {
        co_return co_await coro::spawn_many(p, [b, a, g](usize) -> micron::task<i64> {
          for ( u32 k = 0; k < g; ++k ) {
            a->fetch_add(1, micron::memory_order_relaxed);
            co_await b->arrive_and_wait();
          }
          co_return 1;
        });
      })(&bar, &arrived, static_cast<usize>(parties), gens);

  co_await coro::join;
  auto got = micron::move(e).operator*();
  if ( got.size() != parties ) co_return -5200;
  if ( arrived.get(micron::memory_order_acquire) != static_cast<u64>(parties) * gens ) co_return -5201;
  co_return 0;
}

}      // namespace

int
main(void)
{
  sb::print("=== CORO ASYNC PRIMITIVES UNDER CONTENTION ===");
  sb::print("    rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  contenders: ", static_cast<usize>(CONTENDERS),
            "  scale: ", static_cast<usize>(ltest::stress_scale));

  const i32 wm0 = ltest::fd_watermark();
  coro::start_coroutine_runtime();

  test_case("async_mutex: lock / scoped_lock / try_lock never overlap, no increment lost");
  {
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      g_plain = 0;
      g_critical_now.store(0, micron::memory_order_release);
      g_overlap.store(0, micron::memory_order_release);

      auto out = coro::sync_wait(coro::spawn_many(CONTENDERS, [](usize i) -> micron::task<i64> {
        if ( i % 3u == 0 )
          co_return co_await mutex_worker(PER);
        else if ( i % 3u == 1 )
          co_return co_await guard_worker(PER);
        else
          co_return co_await trylock_worker(PER);
      }));
      require(out.size(), CONTENDERS);

      u64 expect = 0;
      for ( usize i = 0; i < out.size(); ++i ) expect += static_cast<u64>(out[i]);
      require(g_plain, expect);
      require(g_overlap.get(micron::memory_order_acquire), static_cast<u64>(0));
      require(g_critical_now.get(micron::memory_order_acquire), static_cast<u64>(0));
    }
    sb::print("     mutex rounds=", static_cast<usize>(n), " x ", static_cast<usize>(CONTENDERS),
              " contenders, no overlap, no lost increment");
  }
  end_test_case();

  test_case("manual_reset_event: all waiters park, one set() releases every one");
  {
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      const i32 rc = static_cast<i32>(coro::sync_wait(event_round(48)));
      if ( rc != 0 ) sb::print("     event round ", static_cast<usize>(r), " rc=", rc);
      require(rc, 0);
    }
    sb::print("     event rounds=", static_cast<usize>(n), " x 48 waiters");
  }
  end_test_case();

  test_case("async_semaphore: permits conserved, never more holders than permits");
  {
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      constexpr u64 CAP = 4;
      g_inside.store(0, micron::memory_order_release);
      g_over_cap.store(0, micron::memory_order_release);
      g_sem.release(static_cast<i64>(CAP));

      auto out = coro::sync_wait(coro::spawn_many(32u, [](usize) -> micron::task<i64> { co_return co_await sem_worker(16, CAP); }));
      require(out.size(), static_cast<usize>(32));
      require(g_over_cap.get(micron::memory_order_acquire), static_cast<u64>(0));
      require(g_inside.get(micron::memory_order_acquire), static_cast<u64>(0));

      const i64 back = g_sem.try_acquire_many(static_cast<i64>(CAP) + 4);
      require(back, static_cast<i64>(CAP));
      require_false(g_sem.try_acquire());
    }
    sb::print("     semaphore rounds=", static_cast<usize>(n), " permits conserved");
  }
  end_test_case();

  test_case("async_rwlock: writers exclusive, readers never overlap a writer");
  {
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      g_readers.store(0, micron::memory_order_release);
      g_writers.store(0, micron::memory_order_release);
      g_rw_violation.store(0, micron::memory_order_release);
      g_rw_plain = 0;

      auto out = coro::sync_wait(coro::spawn_many(48u, [](usize i) -> micron::task<i64> {
        if ( i % 4u == 0 )
          co_return co_await rw_writer(24);
        else
          co_return co_await rw_reader(24);
      }));
      require(out.size(), static_cast<usize>(48));
      require(g_rw_violation.get(micron::memory_order_acquire), static_cast<u64>(0));
      require(g_readers.get(micron::memory_order_acquire), static_cast<i64>(0));
      require(g_writers.get(micron::memory_order_acquire), static_cast<i64>(0));

      require(g_rw_plain, static_cast<u64>(12 * 24));
    }
    sb::print("     rwlock rounds=", static_cast<usize>(n), " no reader/writer overlap");
  }
  end_test_case();

  test_case("async_latch counts down once; async_barrier cycles across generations");
  {
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      require(static_cast<i32>(coro::sync_wait(latch_round(32))), 0);
      require(static_cast<i32>(coro::sync_wait(barrier_round(8, 12))), 0);
      require(static_cast<i32>(coro::sync_wait(barrier_round(2, 32))), 0);
    }
    sb::print("     latch/barrier rounds=", static_cast<usize>(n));
  }
  end_test_case();

  require(coro::io_pending(), static_cast<u64>(0));
  coro::stop_coroutine_runtime();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  sb::print("=== ALL CORO ASYNC PRIMITIVE TESTS PASSED ===");
  return 1;
}
