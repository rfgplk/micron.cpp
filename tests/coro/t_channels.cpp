#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static micron::async_channel<int, 256> g_bch;
static int g_src[5000];
static int g_dst[5000];
static micron::atomic_token<i64> g_bulk_pulled{ 0 };

static micron::task<void>
bulk_producer(int n)
{
  usize off = 0;
  while ( off < (usize)n ) {
    usize got = co_await g_bch.push_bulk(g_src + off, (usize)n - off);
    off += got;
    if ( got == 0 ) break;
  }
}

static micron::task<void>
bulk_consumer(int n)
{
  usize off = 0;
  while ( off < (usize)n ) {
    usize got = co_await g_bch.pull_bulk(g_dst + off, (usize)n - off);
    if ( got == 0 ) break;
    off += got;
  }
  g_bulk_pulled.store((i64)off, micron::memory_order_release);
}

static micron::task<void>
bulk_root(int n)
{
  co_await coro::fork(coro::discard, bulk_consumer)(n);
  co_await coro::call(bulk_producer, n);
  co_await coro::join;
}

static micron::unbounded_channel<int> g_uch;
static micron::atomic_token<i64> g_u_sum{ 0 };
static micron::atomic_token<i64> g_u_cnt{ 0 };

static micron::task<void>
u_producer(int base, int n)
{
  for ( int i = 0; i < n; ++i ) g_uch.push(base + i);
  co_return;
}

static micron::task<void>
u_consumer()
{
  for ( ;; ) {
    micron::chan_pull<int> r = co_await g_uch.pull();
    if ( !r.ok ) co_return;
    g_u_sum.fetch_add(r.value, micron::memory_order_acq_rel);
    g_u_cnt.fetch_add(1, micron::memory_order_acq_rel);
  }
}

static micron::task<void>
u_run_producers(int P, int N)
{
  for ( int p = 0; p < P; ++p ) co_await coro::fork(coro::discard, u_producer)(p * N, N);
  co_await coro::join;
}

static micron::task<void>
u_root(int P, int C, int N)
{
  for ( int c = 0; c < C; ++c ) co_await coro::fork(coro::discard, u_consumer)();
  co_await coro::call(u_run_producers, P, N);
  g_uch.close();
  co_await coro::join;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("async_channel bulk push/pull FIFO 1P/1C");
  {
    const int N = 5000;
    for ( int i = 0; i < N; ++i ) {
      g_src[i] = i;
      g_dst[i] = -1;
    }
    g_bulk_pulled.store(0, micron::memory_order_relaxed);
    coro::sync_wait(bulk_root(N));
    bool ok = (g_bulk_pulled.get(micron::memory_order_acquire) == N);
    for ( int i = 0; ok && i < N; ++i )
      if ( g_dst[i] != i ) ok = false;
    sb::check(ok);
  }
  sb::end_test_case();

  sb::test_case("unbounded_channel: exactly-once count + sum");
  {
    const int P = 4, C = 4, N = 10000;
    g_u_sum.store(0, micron::memory_order_relaxed);
    g_u_cnt.store(0, micron::memory_order_relaxed);
    coro::sync_wait(u_root(P, C, N));
    i64 total = (i64)P * N;
    i64 expect_sum = (total - 1) * total / 2;
    sb::check(g_u_cnt.get(micron::memory_order_acquire) == total);
    sb::check(g_u_sum.get(micron::memory_order_acquire) == expect_sum);
  }
  sb::end_test_case();

  sb::test_case("sync_channel: FIFO + close semantics");
  {
    micron::sync_channel<int, 64> sch;
    for ( int i = 0; i < 50; ++i ) sb::check(sch.try_push(i));
    bool ok = true;
    for ( int i = 0; i < 50; ++i ) {
      int v = -1;
      if ( !sch.try_pull(v) || v != i ) ok = false;
    }
    sb::check(ok);
    int v;
    sb::check(!sch.try_pull(v));
    sch.close();
    sb::check(!sch.push(7));
    sb::check(!sch.pull(v));
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO CHANNEL TESTS PASSED ===");
  return 1;
}
