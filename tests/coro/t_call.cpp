#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static micron::task<u64>
fib_bound(u64 n)
{
  if ( n < 2 ) co_return n;
  u64 a = 0, b = 0;
  co_await coro::fork[&a, fib_bound](n - 1);
  co_await coro::call[&b, fib_bound](n - 2);
  co_await coro::join;
  co_return a + b;
}

static micron::task<u64>
fib_legacy(u64 n)
{
  if ( n < 2 ) co_return n;
  u64 a = 0;
  co_await coro::fork[&a, fib_legacy](n - 1);
  u64 b = co_await coro::call(fib_legacy, n - 2);
  co_await coro::join;
  co_return a + b;
}

static micron::atomic_token<i64> g_side{ 0 };

static micron::task<void>
side_effect(i64 v)
{
  g_side.fetch_add(v, micron::memory_order_relaxed);
  co_return;
}

static micron::task<void>
do_discard()
{
  co_await coro::call(coro::discard, side_effect)((i64)7);
  co_return;
}

static micron::task<u64>
do_eventual(u64 n)
{
  coro::eventual<u64> e;
  co_await coro::call(&e, fib_bound)(n);
  co_await coro::join;
  co_return micron::move(e).operator*();
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  coro::start_coroutine_runtime();

  sb::test_case("fib_bound matches reference table 0..10");
  {
    const u64 expect[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55 };
    for ( u64 n = 0; n <= 10; ++n ) {
      u64 r = coro::sync_wait(fib_bound(n));
      sb::check(r == expect[n]);
    }
  }
  sb::end_test_case();

  sb::test_case("fib_bound large inputs");
  sb::check(coro::sync_wait(fib_bound(20)) == 6765);
  sb::check(coro::sync_wait(fib_bound(28)) == 317811);
  sb::end_test_case();

  sb::test_case("fib via legacy call()");
  sb::check(coro::sync_wait(fib_legacy(20)) == 6765);
  sb::end_test_case();

  sb::test_case("call(discard, fn) ran the child");
  g_side.store(0, micron::memory_order_relaxed);
  coro::sync_wait(do_discard());
  sb::check(g_side.get(micron::memory_order_relaxed) == 7);
  sb::end_test_case();

  sb::test_case("call(&eventual, fn)");
  sb::check(coro::sync_wait(do_eventual(15)) == 610);
  sb::end_test_case();

  coro::stop_coroutine_runtime();
  sb::require(FAILS == 0);
  sb::print("=== ALL CORO CALL/FORK/JOIN TESTS PASSED ===");
  return 1;
}
