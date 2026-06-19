//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/pause.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace
{
int
ret_int(int n)
{
  return n * 3;
}

float
ret_float(float x)
{
  return x + 0.5f;
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
  sb::print("=== AUTO_THREAD RIGOR ===");

  test_case("non-copyable / non-movable (compile-time contract)");
  {
    static_assert(!micron::is_copy_constructible_v<auto_thread<>>, "auto_thread must not be copyable");
    static_assert(!micron::is_move_constructible_v<auto_thread<>>, "auto_thread must not be movable (stack is local)");
    static_assert(!micron::is_move_assignable_v<auto_thread<>>, "auto_thread must not be move-assignable");
    require(true);
  }
  end_test_case();

  test_case("literal result<int> + join");
  {
    auto_thread<> t(ret_int, 7);
    int got = t.result<int>();
    t.join();
    require(got, 21);
  }
  end_test_case();

  test_case("result<float> round-trips bit-for-bit");
  {
    auto_thread<> t(ret_float, 2.0f);
    float got = t.result<float>();
    t.join();
    require(got == 2.5f, true);
  }
  end_test_case();

  test_case("void worker: join then destructor is clean");
  {
    {
      auto_thread<> t(ret_void);
      t.join();
    }
    require(true);
  }
  end_test_case();

  test_case("wait_for then idempotent join/try_join");
  {
    auto_thread<> t(ret_int, 1);
    t.wait_for();
    require(!t.alive(), true);
    int r1 = t.join();
    require(r1 == 0 || r1 == 1, true);
    require(t.try_join(), 1);
  }
  end_test_case();

  test_case("operator[] re-spawns after an implicit join");
  {
    atomic_token<u32> hits{ 0 };
    auto inc = [&hits]() { hits.fetch_add(1, memory_order_acq_rel); };
    auto_thread<> t(inc);
    t.join();
    t[inc];
    t.join();
    require(hits.get(memory_order_acquire), (u32)2);
  }
  end_test_case();

  test_case("stack_size() reflects the template parameter");
  {
    auto_thread<> t(ret_void);
    require(t.stack_size(), (usize)auto_thread_stack_size);
    t.join();
  }
  end_test_case();

  test_case("alive()/active() observe a running worker, then reap");
  {
    atomic_token<bool> stop{ false };
    auto_thread<> t([&stop]() {
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
    auto_thread<> t([&stop, &ticks]() {
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

  test_case("terminate(): hard-stops a runaway worker and self-reaps (no corpse join)");
  {
    atomic_token<bool> never{ false };
    auto_thread<> t([&never]() {
      while ( !never.get(memory_order_acquire) ) micron::sleep(1);
    });
    micron::ssleep(1);
    require(t.alive(), true);
    t.terminate();
    require(!t.alive(), true);
    require(t.try_join(), 1);
  }
  end_test_case();

  test_case("cancel(): unwinds a worker blocked in micron::sleep() and reaps");
  {
    atomic_token<bool> never{ false };
    auto_thread<> t([&never]() {
      while ( !never.get(memory_order_acquire) ) {
        micron::sleep(1);
      }
    });
    micron::ssleep(1);
    require(t.alive(), true);
    t.cancel();
    require(!t.alive(), true);
    require(t.try_join(), 1);
  }
  end_test_case();

  test_case("stats() after join returns usage without crashing");
  {
    auto_thread<> t(ret_int, 2);
    t.join();
    posix::rusage_t u = t.stats();
    (void)u;
    require(true);
  }
  end_test_case();

  test_case("mtest::parallel: N auto_threads increment a shared atomic exactly N*iters");
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

  sb::print("=== ALL AUTO_THREAD TESTS PASSED ===");
  return 1;
}
