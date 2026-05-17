//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// NOTE: do_once<F> explicitly deletes the default ctor (`do_once(void) =
// delete;`), so even `do_once<F>{}` fails to compile against the variadic
// ctor. Test init functions take a dummy int so they can be invoked via
// `do_once<f> _(0)`. Documented bug — zero-arg F can't be used today.

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/mutex/once.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

micron::atomic_token<int> g_calls(0);

void
init_fn(int)
{
  g_calls.fetch_add(1, micron::memory_order::seq_cst);
}

micron::atomic_token<int> g_calls_b(0);

void
init_fn_b(int)
{
  g_calls_b.fetch_add(1, micron::memory_order::seq_cst);
}

micron::atomic_token<int> g_arg_seen(-1);

void
init_with_arg(int v)
{
  g_arg_seen.store(v, micron::memory_order::seq_cst);
}

micron::atomic_token<int> g_race_calls(0);

void
init_race(int)
{
  g_race_calls.fetch_add(1, micron::memory_order::seq_cst);
}

struct OnceArgs {
  micron::atomic_token<bool> *start;
};

void
once_worker(OnceArgs *p)
{
  while ( !p->start->get(micron::memory_order::acquire) ) micron::yield();
  micron::do_once<init_race> _(0);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== DO_ONCE TESTS ===");

  test_case("do_once<f> single call increments init counter once");
  {
    g_calls.store(0, memory_order::seq_cst);
    do_once<init_fn> _(0);
    require(g_calls.get() == 1);
  }
  end_test_case();

  test_case("do_once<f> called twice still increments only once");
  {
    int before = g_calls.get();
    do_once<init_fn> _(0);
    require(g_calls.get() == before);
  }
  end_test_case();

  test_case("do_once<g> with different function template-instantiates a new gate");
  {
    g_calls_b.store(0, memory_order::seq_cst);
    do_once<init_fn_b> _(0);
    require(g_calls_b.get() == 1);
    do_once<init_fn_b> _2(0);
    require(g_calls_b.get() == 1);
  }
  end_test_case();

  test_case("do_once<f> forwards constructor args to f");
  {
    g_arg_seen.store(-1, memory_order::seq_cst);
    do_once<init_with_arg> _(42);
    require(g_arg_seen.get() == 42);
  }
  end_test_case();

  test_case("do_once<f> 4 threads racing — exactly one invocation");
  {
    g_race_calls.store(0, memory_order::seq_cst);
    atomic_token<bool> start(false);
    OnceArgs a{ &start };
    {
      auto_thread<> t1(once_worker, &a);
      auto_thread<> t2(once_worker, &a);
      auto_thread<> t3(once_worker, &a);
      auto_thread<> t4(once_worker, &a);
      start.store(true, memory_order::release);
    }
    require(g_race_calls.get() == 1);
  }
  end_test_case();

  sb::print("=== ALL DO_ONCE TESTS PASSED ===");
  return 1;
}
