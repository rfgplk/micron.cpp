//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// REGRESSION: queuing_inlet<T> was unsafe under any contention at all.
//
// sync/inlet.hpp's queuing_mutex_adapter held ONE mcs_node as a member and handed that same node to
// every thread that locked. an MCS node is per-ACQUIRER -- it IS the waiter's slot in the queue --
// so two concurrent lockers wrote the same next/waiting pair: lost wakeups, two holders inside at
// once, or a permanent park, depending on the interleaving. it now aliases mcs_lock, which draws a
// node from a per-thread slot table.
//
// tests/rigor/sync_inlet.cpp exercises the inlet API but only through mutex_inlet, so none of this
// was ever reached.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/sync/inlet.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

#include "../support/lockcheck.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{
constexpr u64 SEED_IQ = 0x1D1E7C0FFEEULL;
}

int
main(void)
{
  using namespace micron;
  sb::print("=== QUEUING INLET TESTS ===");

  test_case("the adapter is mcs_lock now, so it satisfies is_mutex");
  {
    static_assert(is_same_v<queuing_mutex_adapter, mcs_lock>, "queuing_mutex_adapter must be the per-thread-node lock");
    static_assert(is_mutex<queuing_mutex_adapter>);
    require_true(true);
  }
  end_test_case();

  test_case("single-threaded store / load / apply");
  {
    queuing_inlet<u64> in;
    in.store(7);
    require(in.load() == 7ull);
    in.apply([](u64 &v) { v += 5; });
    require(in.load() == 12ull);
    require_false(in.locked());
  }
  end_test_case();

  test_case("access() hands out a handle that unlocks on scope exit");
  {
    queuing_inlet<u64> in;
    in.store(1);
    {
      auto h = in.access();
      require_true(in.locked());
    }
    require_false(in.locked());
  }
  end_test_case();

  // ---- the regression proper ----
  test_case("contended apply(): every increment lands, exactly once");
  {
    queuing_inlet<u64> in;
    in.store(0);
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000;
    lcheck::start_gate gate(static_cast<u32>(kT));
    lcheck::watchdog wd;

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(10000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        in.apply([](u64 &v) { ++v; });
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    require(in.load() == static_cast<u64>(kT) * kIters);
    require_false(in.locked());
  }
  end_test_case();

  test_case("contended access(): no two handles alive at the same instant");
  {
    // the shared-node bug shows up here rather than in a counter total: it admits a second holder
    // whose increment may still not collide
    queuing_inlet<u64> in;
    in.store(0);
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 2500;
    lcheck::start_gate gate(static_cast<u32>(kT));
    lcheck::watchdog wd;

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(10000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        {
          auto h = in.access();
          pr.enter();
          pr.dwell(12);
          ++(*h);
          pr.leave();
        }
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());

    require(pr.entries.get(memory_order::acquire) == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
    require(in.load() == static_cast<u64>(kT) * kIters);
  }
  end_test_case();

  test_case("try_access under contention never hands out a second live handle");
  {
    queuing_inlet<u64> in;
    in.store(0);
    lcheck::exclusion_probe pr;
    atomic_token<u64> got(0);
    atomic_token<u64> missed(0);
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000;
    lcheck::start_gate gate(static_cast<u32>(kT));

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        auto h = in.try_access();
        if ( h.is_first() ) {
          pr.enter();
          pr.dwell(8);
          pr.leave();
          got.fetch_add(1, memory_order::acq_rel);
        } else {
          missed.fetch_add(1, memory_order::acq_rel);
        }
      }
    });

    sb::print("     try_access got=", static_cast<usize>(got.get(memory_order::acquire)),
              " missed=", static_cast<usize>(missed.get(memory_order::acquire)));
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require((got.get(memory_order::acquire) + missed.get(memory_order::acquire)) == static_cast<u64>(kT) * kIters);
    require_false(in.locked());
  }
  end_test_case();

  test_case("mixed apply / load / store across threads keeps the value coherent");
  {
    queuing_inlet<u64> in;
    in.store(0);
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 2000;
    lcheck::start_gate gate(static_cast<u32>(kT));
    atomic_token<u64> adds(0);

    mtest::parallel(kT, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      u64 s = SEED_IQ + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < kIters; ++i ) {
        if ( (lcheck::xs64(s) & 3u) == 0u ) {
          (void)in.load();
        } else {
          in.apply([](u64 &v) { ++v; });
          adds.fetch_add(1, memory_order::acq_rel);
        }
      }
    });

    require(in.load() == adds.get(memory_order::acquire));
    require_false(in.locked());
  }
  end_test_case();

  test_case("the other inlet flavours still work (mutex / spin / recursive)");
  {
    mutex_inlet<u64> a;
    spin_inlet<u64> b;
    recursive_inlet<u64> c;
    a.store(1);
    b.store(2);
    c.store(3);
    require(a.load() == 1ull);
    require(b.load() == 2ull);
    require(c.load() == 3ull);
  }
  end_test_case();

  sb::print("=== ALL QUEUING INLET TESTS PASSED ===");
  return 1;
}
