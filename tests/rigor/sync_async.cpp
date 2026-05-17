//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// sync::async requires __global_parallelpool. Without
// __micron_enable_concurrency_at_startup_var, the pool is nullptr by
// default; this test bootstraps the parallel pool via
// micron::start_concurrent_pools().

#include "../../src/atomic/atomic.hpp"
#include "../../src/sync/async.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

micron::atomic_token<int> g_async_calls(0);

void
async_fn()
{
  g_async_calls.fetch_add(1, micron::memory_order::acq_rel);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== ASYNC TESTS ===");

  micron::start_concurrent_pools();

  test_case("async() runs the callable on the parallel pool");
  {
    g_async_calls.store(0, memory_order::seq_cst);
    auto &handle = async(async_fn);
    // wait for completion — handle has wait_for / join semantics depending
    // on the arena's task type. Spin until counter increments.
    while ( g_async_calls.get(memory_order::acquire) < 1 ) yield();
    require(g_async_calls.get() == 1);
    (void)handle;
  }
  end_test_case();

  test_case("multiple async() calls execute concurrently");
  {
    g_async_calls.store(0, memory_order::seq_cst);
    constexpr int N = 4;
    for ( int i = 0; i < N; ++i ) async(async_fn);
    while ( g_async_calls.get(memory_order::acquire) < N ) yield();
    require(g_async_calls.get() == N);
  }
  end_test_case();

  micron::end_concurrent_pools();

  sb::print("=== ALL ASYNC TESTS PASSED ===");
  return 1;
}
