
#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static i64
now_ms()
{
  micron::timespec_t t{};
  micron::clock_gettime(micron::clock_monotonic, t);
  return (i64)t.tv_sec * 1000 + (i64)t.tv_nsec / 1000000;
}

static micron::task<i64>
sleep_and_return(u64 ms)
{
  co_await coro::sleep_for_ms(ms);
  co_return (i64) ms;
}

static micron::atomic_token<i64> g_woke{ 0 };

static micron::task<void>
sleeper(u64 ms)
{
  co_await coro::sleep_for_ms(ms);
  g_woke.fetch_add(1, micron::memory_order_acq_rel);
}

static micron::task<void>
many_sleeps(int n)
{
  for ( int i = 0; i < n; ++i ) co_await coro::fork(coro::discard, sleeper)((u64)(5 + i));
  co_await coro::join;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("sleep_for ~50ms accuracy");
  {
    i64 t0 = now_ms();
    i64 r = coro::sync_wait(sleep_and_return(50));
    i64 dt = now_ms() - t0;
    sb::check(r == 50);
    sb::check(dt >= 40 && dt <= 250);
    sb::print("sleep_for(50ms) measured ", dt, " ms");
  }
  sb::end_test_case();

  sb::test_case("all concurrent sleeps fired");
  {
    const int N = 20;
    g_woke.store(0, micron::memory_order_relaxed);
    coro::sync_wait(many_sleeps(N));
    sb::check(g_woke.get(micron::memory_order_acquire) == N);
  }
  sb::end_test_case();

  sb::test_case("detached sleeper fired on the timer");
  {
    g_woke.store(0, micron::memory_order_relaxed);
    coro::detach(sleeper(30));
    coro::sync_wait(sleep_and_return(80));
    sb::check(g_woke.get(micron::memory_order_acquire) == 1);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO TIMER TESTS PASSED ===");
  return 1;
}
