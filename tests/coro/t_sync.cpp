
#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static coro::async_mutex g_mtx;
static volatile i64 g_counter = 0;

static micron::task<void>
incr(int times)
{
  for ( int i = 0; i < times; ++i ) {
    co_await g_mtx.lock();
    g_counter = g_counter + 1;
    g_mtx.unlock();
  }
}

static micron::task<void>
mtx_root(int K, int times)
{
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, incr)(times);
  co_await coro::join;
}

static int g_slots[64];
static coro::async_latch *g_latch = nullptr;
static micron::atomic_token<i64> g_consumer_sum{ -1 };

static micron::task<void>
producer(int idx)
{
  g_slots[idx] = idx + 1;
  g_latch->count_down();
  co_return;
}

static micron::task<void>
consumer(int K)
{
  co_await *g_latch;
  i64 s = 0;
  for ( int i = 0; i < K; ++i ) s += g_slots[i];
  g_consumer_sum.store(s, micron::memory_order_release);
  co_return;
}

static micron::task<void>
latch_root(int K)
{
  co_await coro::fork(coro::discard, consumer)(K);
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, producer)(k);
  co_await coro::join;
}

static coro::async_barrier *g_bar = nullptr;
static micron::atomic_token<u32> g_arrived[32];
static micron::atomic_token<u32> g_barrier_fail{ 0 };

static micron::task<void>
bar_worker(int rounds, int K)
{
  for ( int r = 0; r < rounds; ++r ) {
    g_arrived[r].fetch_add(1, micron::memory_order_acq_rel);
    co_await g_bar->arrive_and_wait();
    if ( g_arrived[r].get(micron::memory_order_acquire) != (u32)K ) g_barrier_fail.fetch_add(1, micron::memory_order_acq_rel);
  }
}

static micron::task<void>
bar_root(int K, int rounds)
{
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, bar_worker)(rounds, K);
  co_await coro::join;
}

static coro::async_mutex g_cvm;
static coro::async_condvar g_cv;
static int g_ready_val = 0;
static int g_ready = 0;
static micron::atomic_token<i64> g_cv_consumed{ 0 };

static micron::task<void>
cv_consumer()
{
  co_await g_cvm.lock();
  while ( g_ready == 0 ) co_await g_cv.wait(g_cvm);
  int v = g_ready_val;
  g_cvm.unlock();
  g_cv_consumed.fetch_add(v, micron::memory_order_acq_rel);
  co_return;
}

static micron::task<void>
cv_producer(int val)
{
  co_await g_cvm.lock();
  g_ready_val = val;
  g_ready = 1;
  g_cvm.unlock();
  g_cv.notify_all();
  co_return;
}

static micron::task<void>
cv_root(int N, int val)
{
  for ( int i = 0; i < N; ++i ) co_await coro::fork(coro::discard, cv_consumer)();
  co_await coro::fork(coro::discard, cv_producer)(val);
  co_await coro::join;
}

static coro::async_mutex g_gmtx;
static volatile i64 g_gcounter = 0;

static micron::task<void>
incr_guard(int times)
{
  for ( int i = 0; i < times; ++i ) {
    auto __g = co_await coro::scoped_lock(g_gmtx);
    g_gcounter = g_gcounter + 1;
  }
}

static micron::task<void>
guard_root(int K, int times)
{
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, incr_guard)(times);
  co_await coro::join;
}

static coro::async_rwlock g_rw;
static volatile i64 g_rwdata = 0;
static micron::atomic_token<i64> g_act_r{ 0 };
static micron::atomic_token<i64> g_act_w{ 0 };
static micron::atomic_token<i64> g_rw_fail{ 0 };

static micron::task<void>
rw_writer(int iters)
{
  for ( int i = 0; i < iters; ++i ) {
    co_await g_rw.lock();
    i64 w = g_act_w.add_fetch(1, micron::memory_order_acq_rel);
    i64 r = g_act_r.get(micron::memory_order_acquire);
    if ( w != 1 || r != 0 ) g_rw_fail.fetch_add(1, micron::memory_order_acq_rel);
    g_rwdata = g_rwdata + 1;
    g_act_w.sub_fetch(1, micron::memory_order_acq_rel);
    g_rw.unlock();
  }
}

static micron::task<void>
rw_reader(int iters)
{
  for ( int i = 0; i < iters; ++i ) {
    co_await g_rw.lock_shared();
    g_act_r.add_fetch(1, micron::memory_order_acq_rel);
    if ( g_act_w.get(micron::memory_order_acquire) != 0 ) g_rw_fail.fetch_add(1, micron::memory_order_acq_rel);
    g_act_r.sub_fetch(1, micron::memory_order_acq_rel);
    g_rw.unlock_shared();
  }
}

static micron::task<void>
rw_root(int W, int wi, int R, int ri)
{
  for ( int k = 0; k < W; ++k ) co_await coro::fork(coro::discard, rw_writer)(wi);
  for ( int k = 0; k < R; ++k ) co_await coro::fork(coro::discard, rw_reader)(ri);
  co_await coro::join;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("async_mutex 8x10000 == 80000");
  g_counter = 0;
  coro::sync_wait(mtx_root(8, 10000));
  sb::check(g_counter == 80000);
  sb::print("async_mutex counter = ", (i64)g_counter, " (expect 80000)");
  sb::end_test_case();

  sb::test_case("async_latch: consumer saw all producers");
  {
    const int K = 16;
    coro::async_latch latch(K);
    g_latch = &latch;
    g_consumer_sum.store(-1, micron::memory_order_relaxed);
    for ( int i = 0; i < K; ++i ) g_slots[i] = 0;
    coro::sync_wait(latch_root(K));
    i64 expect = (i64)K * (K + 1) / 2;
    sb::check(g_consumer_sum.get(micron::memory_order_acquire) == expect);
  }
  sb::end_test_case();

  sb::test_case("async_barrier: no task passed before all arrived");
  {
    const int K = 8, R = 20;
    coro::async_barrier bar((u32)K);
    g_bar = &bar;
    g_barrier_fail.store(0, micron::memory_order_relaxed);
    for ( int r = 0; r < R; ++r ) g_arrived[r].store(0, micron::memory_order_relaxed);
    coro::sync_wait(bar_root(K, R));
    sb::check(g_barrier_fail.get(micron::memory_order_acquire) == 0);
  }
  sb::end_test_case();

  sb::test_case("async_condvar: all consumers woke + read");
  {
    const int N = 12, VAL = 99;
    g_ready = 0;
    g_ready_val = 0;
    g_cv_consumed.store(0, micron::memory_order_relaxed);
    coro::sync_wait(cv_root(N, VAL));
    sb::check(g_cv_consumed.get(micron::memory_order_acquire) == (i64)N * VAL);
  }
  sb::end_test_case();

  sb::test_case("async scoped_lock guard 8x10000 == 80000");
  {
    g_gcounter = 0;
    coro::sync_wait(guard_root(8, 10000));
    sb::check(g_gcounter == 80000);
  }
  sb::end_test_case();

  sb::test_case("async_rwlock: exclusion invariant + increment count");
  {
    const int W = 4, WI = 5000, R = 8, RI = 5000;
    g_rwdata = 0;
    g_act_r.store(0, micron::memory_order_relaxed);
    g_act_w.store(0, micron::memory_order_relaxed);
    g_rw_fail.store(0, micron::memory_order_relaxed);
    coro::sync_wait(rw_root(W, WI, R, RI));
    sb::check(g_rwdata == (i64)W * WI);
    sb::check(g_rw_fail.get(micron::memory_order_acquire) == 0);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO SYNC PRIMITIVE TESTS PASSED ===");
  return 1;
}
