//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Coverage notes:
//   atomic_flag uses an atomic_token<bool>. test_and_set ignores its memorder
//   parameter (always seq_cst via swap) — documented but not asserted.
//   The wait() variant is a busy-spin; tested for behavior only, not for
//   futex-backed blocking.

// flag.hpp declares atomic_token/memory_order without including atomic.hpp;
// pull in atomic.hpp first so the file actually compiles.
#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <atomic>
#include <thread>
#include <vector>

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== ATOMIC_FLAG TESTS ===");

  test_case("default construction reads false");
  {
    atomic_flag f;
    require_false(f.test());
  }
  end_test_case();

  test_case("test_and_set returns false on first call, true on second");
  {
    atomic_flag f;
    require_false(f.test_and_set());
    require_true(f.test_and_set());
  }
  end_test_case();

  test_case("clear() resets to false");
  {
    atomic_flag f;
    f.test_and_set();
    require_true(f.test());
    f.clear();
    require_false(f.test());
  }
  end_test_case();

  test_case("test with each memory order executes");
  {
    atomic_flag f;
    f.test_and_set();
    require_true(f.test(memory_order::seq_cst));
    require_true(f.test(memory_order::acquire));
    require_true(f.test(memory_order::relaxed));
  }
  end_test_case();

  test_case("clear with each memory order executes");
  {
    atomic_flag f;
    f.test_and_set();
    f.clear(memory_order::release);
    require_false(f.test());
    f.test_and_set();
    f.clear(memory_order::seq_cst);
    require_false(f.test());
  }
  end_test_case();

  test_case("two-thread mutual exclusion via test_and_set/clear");
  {
    atomic_flag lock;
    int counter = 0;
    std::vector<std::thread> th;
    constexpr int kT = 2, kIters = 50000;
    for ( int i = 0; i < kT; ++i ) {
      th.emplace_back([&]() {
        for ( int j = 0; j < kIters; ++j ) {
          while ( lock.test_and_set() ) {
          }
          ++counter;
          lock.clear();
        }
      });
    }
    for ( auto &t : th ) t.join();
    require(counter == kT * kIters);
  }
  end_test_case();

  test_case("wait(old) returns after another thread clears");
  {
    atomic_flag f;
    f.test_and_set();
    std::atomic<bool> waiter_done{ false };
    std::thread waiter([&]() {
      f.wait(true);
      waiter_done.store(true);
    });
    // give waiter time to enter the spin
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    require_false(waiter_done.load());
    f.clear();
    waiter.join();
    require_true(waiter_done.load());
  }
  end_test_case();

  test_case("4 producers race on test_and_set; exactly one wins per epoch");
  {
    constexpr int kT = 4;
    constexpr int kEpochs = 100;
    atomic_flag gate;
    std::atomic<int> winner_count{ 0 };
    std::atomic<int> ready_count{ 0 };
    std::atomic<int> epoch{ -1 };

    std::vector<std::thread> th;
    th.reserve(kT);
    for ( int t = 0; t < kT; ++t ) {
      th.emplace_back([&]() {
        int last_seen = -1;
        for ( int e = 0; e < kEpochs; ++e ) {
          // wait for the driver to advance the epoch
          while ( epoch.load(std::memory_order_acquire) == last_seen ) {
          }
          last_seen = epoch.load(std::memory_order_acquire);
          ready_count.fetch_add(1, std::memory_order_acq_rel);
          // race
          if ( !gate.test_and_set() ) winner_count.fetch_add(1, std::memory_order_relaxed);
        }
      });
    }

    for ( int e = 0; e < kEpochs; ++e ) {
      gate.clear();
      ready_count.store(0, std::memory_order_release);
      epoch.store(e, std::memory_order_release);
      while ( ready_count.load(std::memory_order_acquire) < kT ) {
      }
    }
    for ( auto &t : th ) t.join();

    require(winner_count.load() == kEpochs);
  }
  end_test_case();

  sb::print("=== ALL ATOMIC_FLAG TESTS PASSED ===");
  return 1;
}
