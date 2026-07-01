#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static micron::atomic_token<i64> g_fg{ 0 };

static micron::task<void>
fg_work(i64 v)
{
  g_fg.fetch_add(v, micron::memory_order_acq_rel);
  co_return;
}

static micron::task<void>
fg_driver(coro::fork_group *g, int N)
{
  for ( int i = 0; i < N; ++i ) g->spawn(fg_work, (i64)1);
  co_await g->join();
  for ( int i = 0; i < N; ++i ) g->spawn(fg_work, (i64)2);
  co_await g->join();
}

static micron::atomic_token<i64> g_fg_iters{ 0 };

static micron::task<void>
fg_spin(coro::cancellation_token tok)
{
  for ( i64 i = 0; i < 1000000000LL; ++i ) {
    if ( tok.cancelled() ) co_return;
    g_fg_iters.fetch_add(1, micron::memory_order_relaxed);
  }
}

static micron::task<void>
fg_cancel_driver(coro::fork_group *g, int N)
{
  for ( int i = 0; i < N; ++i ) g->spawn(fg_spin, g->token());
  g->request_cancel();
  co_await g->join();
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("fork_group: incremental spawn/join total");
  {
    coro::fork_group g;
    g_fg.store(0, micron::memory_order_relaxed);
    coro::sync_wait(fg_driver(&g, 100));
    sb::check(g_fg.get(micron::memory_order_acquire) == 100 * 1 + 100 * 2);
  }
  sb::end_test_case();

  sb::test_case("fork_group: cancel stopped members early");
  {
    coro::fork_group g;
    g_fg_iters.store(0, micron::memory_order_relaxed);
    coro::sync_wait(fg_cancel_driver(&g, 8));
    sb::check(g_fg_iters.get(micron::memory_order_acquire) < 8LL * 1000000000LL);
    sb::print("fork_group cancel: ", g_fg_iters.get(micron::memory_order_acquire), " iters before stop (join completed)");
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO FORK_GROUP TESTS PASSED ===");
  return 1;
}
