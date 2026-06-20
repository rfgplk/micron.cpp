//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/pause.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/reg_thread.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace
{
int
ret_int(int n)
{
  return n * 4;
}

void
ret_void(void)
{
}
}      // namespace

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== THREAD (reg_thread) RIGOR ===");

  test_case("type contract: movable, non-copyable");
  {
    static_assert(micron::is_move_constructible_v<thread<>>, "thread<> must be movable");
    static_assert(!micron::is_copy_constructible_v<thread<>>, "thread<> must not be copyable");
    require(true);
  }
  end_test_case();

  test_case("literal result<int> + join");
  {
    thread<> t(ret_int, 6);
    int got = t.result<int>();
    t.join();
    require(got, 24);
  }
  end_test_case();

  test_case("void worker: join then destructor clean");
  {
    {
      thread<> t(ret_void);
      t.join();
    }
    require(true);
  }
  end_test_case();

  test_case("default-constructed thread: join/try_join are safe no-ops");
  {
    thread<> t;
    require(t.join(), 0);
    require(t.try_join(), 0);
  }
  end_test_case();

  test_case("move-assign over a live thread reaps the old one");
  {
    atomic_token<u32> ran{ 0 };
    thread<> live([&ran]() { ran.fetch_add(1, memory_order_acq_rel); });
    thread<> empty;
    live = micron::move(empty);
    require(ran.get(memory_order_acquire), (u32)1);
    require(live.join(), 0);
  }
  end_test_case();

  test_case("operator[] re-spawns after join");
  {
    atomic_token<u32> hits{ 0 };
    auto inc = [&hits]() { hits.fetch_add(1, memory_order_acq_rel); };
    thread<> t(inc);
    t.join();
    t[inc];
    t.join();
    require(hits.get(memory_order_acquire), (u32)2);
  }
  end_test_case();

  test_case("alive()/active() observe a running worker, then reap");
  {
    atomic_token<bool> stop{ false };
    thread<> t([&stop]() {
      while ( !stop.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    require(t.alive(), true);
    require(t.active(), true);
    stop.store(true, memory_order_release);
    t.join();
    require(!t.alive(), true);
  }
  end_test_case();

  test_case("sleep()/awaken(): true per-thread suspend pauses ONLY this worker");
  {
    atomic_token<bool> stop{ false };
    atomic_token<u64> ticks{ 0 };
    thread<> t([&stop, &ticks]() {
      while ( !stop.get(memory_order_acquire) ) {
        ticks.fetch_add(1, memory_order_acq_rel);
        micron::sleep(1);
      }
    });
    micron::ssleep(1);
    t.sleep();
    micron::sleep(50);
    u64 a = ticks.get(memory_order_acquire);
    micron::sleep(80);
    u64 b = ticks.get(memory_order_acquire);
    require(a == b, true);
    t.awaken();
    micron::sleep(80);
    require(ticks.get(memory_order_acquire) > b, true);
    stop.store(true, memory_order_release);
    t.join();
  }
  end_test_case();

  test_case("terminate(): hard-stops a runaway worker and self-reaps");
  {
    atomic_token<bool> never{ false };
    thread<> t([&never]() {
      while ( !never.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    require(t.alive(), true);
    t.terminate();
    require(!t.alive(), true);
    require(t.try_join(), 0);
  }
  end_test_case();

  test_case("cancel(): unified cancel+join+release at an explicit cancellation point");
  {
    atomic_token<bool> never{ false };
    thread<> t([&never]() {
      while ( !never.get(memory_order_acquire) ) {
        micron::micthread::cancel();
        micron::sleep(1);
      }
    });
    micron::ssleep(1);
    require(t.alive(), true);
    t.cancel();
    require(!t.alive(), true);
  }
  end_test_case();

  test_case("mtest::parallel shared-counter contention");
  {
    atomic_token<u64> c{ 0 };
    constexpr int kT = 8;
    constexpr int kI = 5000;
    mtest::parallel(kT, [&c](int) {
      for ( int i = 0; i < kI; ++i ) c.fetch_add(1, memory_order_acq_rel);
    });
    require(c.get(memory_order_acquire), (u64)((u64)kT * kI));
  }
  end_test_case();

  sb::print("=== ALL THREAD (reg_thread) TESTS PASSED ===");
  return 1;
}
