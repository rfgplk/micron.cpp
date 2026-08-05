//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Regression: a parked timer must not be able to hold the process open.
//
// stop_coroutine_runtime()'s quiesce loop treated a non-zero pending_timers as "not quiet" with no
// grace window and no way out -- the io drain right below it had both. A task left parked on
// sleep_for(minutes) therefore blocked the loop for those minutes.
//
// That was survivable while stop() was something you called on purpose. __arm_runtime_reaper now
// runs it from a static destructor for every program that ever started the runtime, so the same
// stall became "returning from main hangs" for any program that forgot to join a sleeper.
//
// This build is deliberately CPU-only (MICRON_CORO_NO_URING): with a live ring a sleep becomes an
// IORING_OP_TIMEOUT and rides the io grace instead, so the software timer list -- which exists in
// every build -- would never be exercised.
//
// main() parks one more sleeper and returns, so the run ALSO grades the reaper: if teardown from
// the static destructor hangs, this test times out (124) instead of passing.

#define MICRON_CORO_NO_URING
#define MICRON_CORO_TIMER_DRAIN_GRACE_NS 200000000ull      // 0.2s, so the case is quick

#include "../../src/stdcoro.hpp"

#include "../snowball/snowball.hpp"

namespace coro = micron::coro;

static int FAILS = 0;

static u64
mono_ns()
{
  micron::timespec_t t{};
  micron::clock_gettime(micron::clock_monotonic, t);
  return static_cast<u64>(t.tv_sec) * 1000000000ull + static_cast<u64>(t.tv_nsec);
}

// park a sleeper that will not fire within any reasonable test lifetime
static micron::task<void>
sleeps_forever()
{
  co_await coro::sleep_for_ms(600000);      // 10 minutes
}

// returns false if the sleeper never made it onto the timer list
static bool
wait_until_parked()
{
  const u64 deadline = mono_ns() + 5000000000ull;
  for ( ;; ) {
    coro::engine *e = coro::__global_engine;
    if ( e != nullptr && e->pending_timers.get(micron::memory_order_acquire) != 0 ) return true;
    if ( mono_ns() > deadline ) return false;
    micron::yield();
  }
}

int
main()
{
  sb::check_callback([]() { ++FAILS; });
  sb::print("=== CORO TIMER TEARDOWN ===");

  sb::test_case("stop_coroutine_runtime() does not wait out a 10-minute sleep");
  {
    coro::start_coroutine_runtime(2);
    coro::detach(sleeps_forever());
    sb::require(wait_until_parked());

    const u64 t0 = mono_ns();
    coro::stop_coroutine_runtime();
    const u64 took = mono_ns() - t0;

    sb::print("teardown with a parked timer took ", took / 1000000ull, " ms (grace is ",
              MICRON_CORO_TIMER_DRAIN_GRACE_NS / 1000000ull, " ms)");
    // generous: the grace plus scheduling slop, and nowhere near the 10 minutes it used to take
    sb::check(took < 5000000000ull);
    sb::require(coro::__global_engine == nullptr);
  }
  sb::end_test_case();

  sb::test_case("a timer that DOES fire is still waited for, not cut loose");
  {
    coro::start_coroutine_runtime(2);
    static micron::atomic_token<u32> woke{ 0 };
    coro::detach([]() -> micron::task<void> {
      co_await coro::sleep_for_ms(30);
      woke.fetch_add(1, micron::memory_order_acq_rel);
    }());
    coro::stop_coroutine_runtime();
    sb::check(woke.get(micron::memory_order_acquire) == 1u);      // the grace re-arms on progress
  }
  sb::end_test_case();

  sb::test_case("the timer list does not survive into the next runtime");
  {
    // a cut-loose sleeper leaves its node in a frame belonging to the dead engine; a restart that
    // walked the old list would resume it and land in freed memory
    coro::start_coroutine_runtime(2);
    coro::detach(sleeps_forever());
    sb::require(wait_until_parked());
    coro::stop_coroutine_runtime();

    coro::start_coroutine_runtime(2);
    const i32 v = coro::sync_wait([]() -> micron::task<i32> {
      co_await coro::sleep_for_ms(5);
      co_return 7;
    }());
    coro::stop_coroutine_runtime();
    sb::check(v == 7);
  }
  sb::end_test_case();

  sb::require(FAILS == 0);

  // and now the reaper's turn: park one and walk away. If the static-destructor teardown cannot
  // give up on it, this run never exits and grades 124 rather than PASS
  coro::start_coroutine_runtime(2);
  coro::detach(sleeps_forever());
  sb::require(wait_until_parked());
  sb::print("=== CORO TIMER TEARDOWN PASSED (reaper now tears down a parked timer) ===");
  return 1;
}
