#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static micron::atomic_token<i64> g_work{ 0 };

static micron::task<void>
canceller(coro::cancellation_source *src, i64 spin)
{
  volatile i64 s = 0;
  for ( i64 i = 0; i < spin; ++i ) s += i;
  (void)s;
  src->cancel();
  co_return;
}

static micron::task<void>
worker_tok(coro::cancellation_token tok, i64 iters)
{
  for ( i64 i = 0; i < iters; ++i ) {
    if ( tok.cancelled() ) co_return;
    g_work.fetch_add(1, micron::memory_order_relaxed);
  }
}

static micron::task<i64>
root_tok(coro::cancellation_token tok, coro::cancellation_source *src, int K, i64 iters)
{
  co_await coro::fork(coro::discard, canceller)(src, (i64)100000);
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, worker_tok)(tok, iters);
  co_await coro::join;
  co_return g_work.get(micron::memory_order_relaxed);
}

static micron::task<void>
worker_cp(i64 iters)
{
  for ( i64 i = 0; i < iters; ++i ) {
    if ( co_await coro::cancelpoint() ) co_return;
    g_work.fetch_add(1, micron::memory_order_relaxed);
  }
}

static micron::task<i64>
root_cp(coro::cancellation_source *src, int K, i64 iters)
{
  co_await coro::fork(coro::discard, canceller)(src, (i64)100000);
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, worker_cp)(iters);
  co_await coro::join;
  co_return g_work.get(micron::memory_order_relaxed);
}

static micron::task<i64>
root_nocancel(coro::cancellation_token tok, int K, i64 iters)
{
  for ( int k = 0; k < K; ++k ) co_await coro::fork(coro::discard, worker_tok)(tok, iters);
  co_await coro::join;
  co_return g_work.get(micron::memory_order_relaxed);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("cancellation_token: stopped before completing all work");
  {
    g_work.store(0, micron::memory_order_relaxed);
    coro::cancellation_source src;
    const int K = 8;
    const i64 ITERS = 500000000LL;
    i64 done = coro::sync_wait(root_tok(src.token(), &src, K, ITERS));
    sb::check(done < (i64)K * ITERS);
    sb::print("token path: ", done, " of ", (i64)K * ITERS, " iters before cancel");
  }
  sb::end_test_case();

  sb::test_case("cancelpoint: stopped before completing all work");
  {
    g_work.store(0, micron::memory_order_relaxed);
    coro::cancellation_source src;
    const int K = 8;
    const i64 ITERS = 50000000LL;
    auto t = root_cp(&src, K, ITERS);
    src.bind(t);
    i64 done = coro::sync_wait(micron::move(t));
    sb::check(done < (i64)K * ITERS);
    sb::print("cancelpoint path: ", done, " of ", (i64)K * ITERS, " iters before cancel");
  }
  sb::end_test_case();

  sb::test_case("no-cancel: all work completed");
  {
    g_work.store(0, micron::memory_order_relaxed);
    coro::cancellation_source src;
    const int K = 4;
    const i64 ITERS = 100000LL;
    i64 done = coro::sync_wait(root_nocancel(src.token(), K, ITERS));
    sb::check(done == (i64)K * ITERS);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO CANCELLATION TESTS PASSED ===");
  return 1;
}
