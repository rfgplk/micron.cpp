//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/inlet.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/mutex/locks.hpp"
#include "../../src/mutex/mutex.hpp"

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

struct ApplyArgs {
  micron::mutex_inlet<int> *in;
  int iters;
};

void
apply_inc_worker(ApplyArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    p->in->apply([](int &v) { ++v; });
  }
}

struct SpinApplyArgs {
  micron::spin_inlet<int> *in;
  int iters;
};

void
spin_inc_worker(SpinApplyArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    p->in->apply([](int &v) { ++v; });
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== INLET TESTS ===");

  test_case("default ctor zero-init value");
  {
    inlet<int> i;
    require(i.load() == 0);
    require_false(i.locked());
  }
  end_test_case();

  test_case("inlet(initial_value) stores initial");
  {
    inlet<int> i(42);
    require(i.load() == 42);
  }
  end_test_case();

  test_case("tag() returns provided tag string");
  {
    inlet<int> i("counter");
    require(i.tag() != nullptr);
  }
  end_test_case();

  test_case("access() locks; handle dtor unlocks");
  {
    inlet<int> i(0);
    require_false(i.locked());
    {
      auto h = i.access();
      require_true(i.locked());
      *h = 7;
    }
    require_false(i.locked());
    require(i.load() == 7);
  }
  end_test_case();

  test_case("handle's get()/operator-> access value");
  {
    inlet<int> i(10);
    {
      auto h = i.access();
      require(h.get() == 10);
      require(*h.operator->() == 10);
      h.set(99);
      require(h.get() == 99);
    }
    require(i.load() == 99);
  }
  end_test_case();

  test_case("load/store round-trip");
  {
    inlet<int> i;
    i.store(123);
    require(i.load() == 123);
    i.store(456);
    require(i.load() == 456);
  }
  end_test_case();

  test_case("operator=(T) / operator T() store/load");
  {
    inlet<int> i;
    i = 17;
    require(static_cast<int>(i) == 17);
    i = 25;
    require(static_cast<int>(i) == 25);
  }
  end_test_case();

  test_case("apply(fn) holds lock for duration of fn");
  {
    inlet<int> i(0);
    i.apply([](int &v) { v = 42; });
    require(i.load() == 42);
  }
  end_test_case();

  test_case("reset() returns value to default");
  {
    inlet<int> i(99);
    i.reset();
    require(i.load() == 0);
  }
  end_test_case();

  test_case("handle_t move ctor — moved-from does not unlock at dtor");
  {
    inlet<int> i;
    {
      auto h1 = i.access();
      require_true(i.locked());
      auto h2 = micron::move(h1);
      require_true(i.locked());
      // h1 dtor here: src == nullptr, no unlock
    }
    // h2 dtor unlocks
    require_false(i.locked());
  }
  end_test_case();

  test_case("mutex_inlet<int> 4-thread apply stress totals correctly");
  {
    mutex_inlet<int> in(0);
    constexpr int kIters = 5000;
    ApplyArgs a{ &in, kIters };
    {
      auto_thread<> t1(apply_inc_worker, &a);
      auto_thread<> t2(apply_inc_worker, &a);
      auto_thread<> t3(apply_inc_worker, &a);
      auto_thread<> t4(apply_inc_worker, &a);
    }
    require(in.load() == 4 * kIters);
  }
  end_test_case();

  test_case("spin_inlet<int> 4-thread apply stress totals correctly");
  {
    spin_inlet<int> in(0);
    constexpr int kIters = 5000;
    SpinApplyArgs a{ &in, kIters };
    {
      auto_thread<> t1(spin_inc_worker, &a);
      auto_thread<> t2(spin_inc_worker, &a);
      auto_thread<> t3(spin_inc_worker, &a);
      auto_thread<> t4(spin_inc_worker, &a);
    }
    require(in.load() == 4 * kIters);
  }
  end_test_case();

  sb::print("=== ALL INLET TESTS PASSED ===");
  return 1;
}
