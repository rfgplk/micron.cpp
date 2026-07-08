#include "../../src/tasks/tasks.hpp"
#include "../snowball/snowball.hpp"

namespace coro = micron::coro;
static int FAILS = 0;

static micron::task<u64>
resched_loop(u64 n)
{
  u64 acc = 0;
  for ( u64 i = 0; i < n; ++i ) {
    co_await coro::reschedule();
    acc += i;
  }
  co_return acc;
}

static micron::task<u64>
resched_then_fork(u64 n)
{
  co_await coro::reschedule();
  u64 a = 0, b = 0;
  co_await coro::fork[&a, resched_loop](n);
  co_await coro::call[&b, resched_loop](n);
  co_await coro::join;
  co_return a + b;
}

static micron::task<void>
sleepy_child(u64 ms)
{
  co_await coro::sleep_for_ms(ms);
  co_return;
}

static micron::task<u64>
fork_sleepy(u64 kids)
{
  for ( u64 i = 0; i < kids; ++i ) co_await coro::fork(coro::discard, sleepy_child)((u64)1);
  co_await coro::join;
  co_return kids;
}

static micron::task<u64>
fork_resched_join(u64 ms)
{
  co_await coro::fork(coro::discard, sleepy_child)(ms);
  co_await coro::reschedule();
  co_await coro::join;
  co_return 1;
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });

  sb::test_case("single-worker reschedule completes (deadlock regression)");
  coro::start_coroutine_runtime(1);
  sb::check(coro::sync_wait(resched_loop(10000)) == (10000ull * 9999ull) / 2ull);
  sb::end_test_case();

  sb::test_case("single-worker reschedule-then-balanced-fork joins");
  for ( u64 r = 0; r < 50; ++r ) sb::check(coro::sync_wait(resched_then_fork(20)) == 2ull * ((20ull * 19ull) / 2ull));
  sb::end_test_case();

  sb::test_case("single-worker suspended fork children join");
  sb::check(coro::sync_wait(fork_sleepy(16)) == 16);
  sb::end_test_case();

  sb::test_case("single-worker fork -> reschedule -> join (balanced-pop kind canary)");
  {
    u64 ok = 0;
    for ( u64 r = 0; r < 200; ++r ) ok += coro::sync_wait(fork_resched_join((u64)1));
    sb::check(ok == 200);
  }
  coro::stop_coroutine_runtime();
  sb::end_test_case();

  sb::test_case("multi-worker reschedule + fork storm");
  coro::start_coroutine_runtime();
  {
    micron::vector<u64> got = coro::sync_wait(coro::spawn_many(64, [](usize) -> micron::task<u64> {
      u64 v = co_await coro::call(resched_then_fork, (u64)16);
      co_return v;
    }));
    for ( usize i = 0; i < got.size(); ++i ) sb::check(got[i] == 2ull * ((16ull * 15ull) / 2ull));
  }
  sb::end_test_case();

  sb::test_case("multi-worker suspended fork children join (repeat)");
  for ( u64 r = 0; r < 20; ++r ) sb::check(coro::sync_wait(fork_sleepy(32)) == 32);
  coro::stop_coroutine_runtime();
  sb::end_test_case();

  sb::require(FAILS == 0);
  sb::print("=== ALL SCHED STRESS TESTS PASSED ===");
  return 1;
}
