

#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static micron::task<int>
fast_task(int id)
{
  co_return id;
}

static micron::task<int>
slow_task(int id)
{
  volatile i64 s = 0;
  for ( i64 i = 0; i < 20000000; ++i ) s += i;
  co_return id;
}

static micron::task<usize>
race(micron::futex_future<int> *futs, usize n)
{
  i32 r = co_await coro::when_any(futs, n);
  co_return (r < 0) ? n : static_cast<usize>(r);
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("when_any returned the fast racer's index");
  for ( int winner : { 0, 3, 7 } ) {
    micron::futex_future<int> futs[8];
    for ( int i = 0; i < 8; ++i ) futs[i] = coro::schedule((i == winner) ? fast_task(i) : slow_task(i));
    usize w = coro::sync_wait(race(futs, 8));
    sb::check(w == (usize)winner);
    for ( int i = 0; i < 8; ++i )
      if ( i != (int)w ) (void)futs[i].get();
    sb::print("when_any(winner=", winner, ") -> ", w);
  }
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO SELECT/WHEN_ANY TESTS PASSED ===");
  return 1;
}
