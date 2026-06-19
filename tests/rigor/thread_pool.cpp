//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/pause.hpp"
#include "../../src/sync/until.hpp"
#include "../../src/thread/pool.hpp"
#include "../../src/thread/spawn.hpp"

#include "../snowball/snowball.hpp"

using namespace snowball;

namespace
{
int
ret_int(int n)
{
  return n * 6;
}
}      // namespace

int
main(int, char **)
{
  using namespace micron;
  sb::print("=== THREAD POOL / ARENA RIGOR ===");

  test_case("standard threadpool is constructed at startup");
  {
    require(__global_threadpool != nullptr, true);
  }
  end_test_case();

  test_case("go() runs a task on the standard arena; await + result");
  {
    auto &t = micron::go(ret_int, 7);
    micron::await(t);
    int got = t().result<int>();
    require(got, 42);
  }
  end_test_case();

  test_case("go() with a side-effecting task completes");
  {
    atomic_token<u64> sum{ 0 };
    auto &t = micron::go([&sum]() {
      for ( int i = 0; i < 1000; ++i ) sum.fetch_add(1, memory_order_acq_rel);
    });
    micron::await(t);
    require(sum.get(memory_order_acquire), (u64)1000);
  }
  end_test_case();

  test_case("threads() lists the arena's spawned threads");
  {
    auto list = micron::threads();
    require(list.size() > 0, true);
  }
  end_test_case();

  test_case("concurrent pool: start + add() runs work on a void_thread worker");
  {
    micron::start_concurrent_pools();
    require(__global_parallelpool != nullptr, true);
    atomic_token<u64> done{ 0 };
    auto job = [&done]() { done.fetch_add(5, memory_order_acq_rel); };
    __global_parallelpool->add(job);
    bool ok = micron::until_timeout(3000.0, [&done]() { return done.get(memory_order_acquire) == 5u; });
    require(ok, true);
  }
  end_test_case();

  test_case("concurrent pool: many add()s all execute");
  {
    atomic_token<u64> n{ 0 };
    constexpr int J = 64;
    for ( int i = 0; i < J; ++i ) {
      auto job = [&n]() { n.fetch_add(1, memory_order_acq_rel); };
      __global_parallelpool->add(job);
    }
    bool ok = micron::until_timeout(4000.0, [&n]() { return n.get(memory_order_acquire) == (u64)J; });
    require(ok, true);
  }
  end_test_case();

  sb::print("=== ALL THREAD POOL TESTS PASSED ===");
  return 1;
}
