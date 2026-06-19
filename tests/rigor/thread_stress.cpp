//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/pause.hpp"
#include "../../src/sync/until.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mt.hpp"

#ifndef RIGOR_ITERS
#define RIGOR_ITERS 2000
#endif

using namespace snowball;

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== THREAD STRESS RIGOR ===");

  test_case("high-contention shared counter across 16 threads");
  {
    atomic_token<u64> c{ 0 };
    constexpr int kT = 16;
    const u64 iters = (u64)RIGOR_ITERS;
    mtest::parallel(kT, [&c, iters](int) {
      for ( u64 i = 0; i < iters; ++i ) c.fetch_add(1, memory_order_acq_rel);
    });
    require(c.get(memory_order_acquire), (u64)((u64)kT * iters));
  }
  end_test_case();

  test_case("spawn/join churn: many short-lived threads run exactly once");
  {
    atomic_token<u64> total{ 0 };
    const int rounds = (RIGOR_ITERS / 4) < 100 ? 100 : (RIGOR_ITERS / 4);
    for ( int r = 0; r < rounds; ++r ) {
      auto t = solo::spawn<auto_thread<>>([&total]() { total.fetch_add(1, memory_order_acq_rel); });
      solo::join(t);
    }
    require(total.get(memory_order_acquire), (u64)rounds);
  }
  end_test_case();

  test_case("terminate-storm: spawn runaway -> hard-stop -> reap, repeated");
  {
    const int storm = (RIGOR_ITERS / 10) < 50 ? 50 : ((RIGOR_ITERS / 10) > 400 ? 400 : (RIGOR_ITERS / 10));
    for ( int i = 0; i < storm; ++i ) {
      atomic_token<bool> ready{ false };
      atomic_token<bool> never{ false };
      auto t = solo::spawn<auto_thread<>>([&ready, &never]() {
        ready.store(true, memory_order_release);
        while ( !never.get(memory_order_acquire) ) micron::sleep(1);
      });
      micron::until_timeout(2000.0, [&ready]() { return ready.get(memory_order_acquire); });
      solo::terminate(t);
      solo::dismiss(t);
      require(!is_alive_ptr(t), true);
    }
    require(true);
  }
  end_test_case();

  test_case("repeated per-thread suspend/resume cycles stay correct");
  {
    atomic_token<bool> stop{ false };
    atomic_token<u64> ticks{ 0 };
    atomic_token<bool> ready{ false };
    auto t = solo::spawn<auto_thread<>>([&stop, &ticks, &ready]() {
      ready.store(true, memory_order_release);
      while ( !stop.get(memory_order_acquire) ) {
        ticks.fetch_add(1, memory_order_acq_rel);
        micron::sleep(1);
      }
    });
    micron::until_timeout(2000.0, [&ready]() { return ready.get(memory_order_acquire); });

    const int cycles = (RIGOR_ITERS / 50) < 20 ? 20 : ((RIGOR_ITERS / 50) > 40 ? 40 : (RIGOR_ITERS / 50));
    for ( int i = 0; i < cycles; ++i ) {
      solo::sleep(t);
      micron::sleep(20);
      u64 a = ticks.get(memory_order_acquire);
      micron::sleep(20);
      require(a == ticks.get(memory_order_acquire), true);
      solo::awaken(t);
      u64 b = ticks.get(memory_order_acquire);
      micron::until_timeout(2000.0, [&ticks, b]() { return ticks.get(memory_order_acquire) > b; });
      require(ticks.get(memory_order_acquire) > b, true);
    }
    stop.store(true, memory_order_release);
    solo::join(t);
  }
  end_test_case();

  sb::print("=== ALL THREAD STRESS TESTS PASSED ===");
  return 1;
}
