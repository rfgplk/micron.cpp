//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/mutex/locks/spin_lock.hpp"
#include "../../src/std.hpp"

#include "../../src/concepts.hpp"
#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

struct StressArgs {
  micron::spin_lock *sl;
  int *counter;
  int iters;
};

void
stress_worker(StressArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    auto rptr = p->sl->lock();
    ++(*p->counter);
    (p->sl->*rptr)();
  }
}

struct ObservAgs {
  micron::spin_lock *sl;
  micron::atomic_token<bool> *held;
  micron::atomic_token<bool> *release_now;
};

void
observ_holder(ObservAgs *p)
{
  auto rptr = p->sl->lock();
  p->held->store(true, micron::memory_order::release);
  while ( !p->release_now->get(micron::memory_order::acquire) ) micron::yield();
  (p->sl->*rptr)();
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== SPIN_LOCK TESTS ===");

  static_assert(is_mutex<spin_lock>, "spin_lock must satisfy is_mutex");

  test_case("default ctor unlocked");
  {
    spin_lock sl;
    require_false(sl.is_locked());
  }
  end_test_case();

  test_case("explicit spin_lock(true) starts locked");
  {
    spin_lock sl(true);
    require_true(sl.is_locked());
    sl.unlock();
    require_false(sl.is_locked());
  }
  end_test_case();

  test_case("lock() / operator() returns reset fn-ptr; dispatch unlocks");
  {
    spin_lock sl;
    auto rptr = sl.lock();
    require_true(sl.is_locked());
    (sl.*rptr)();
    require_false(sl.is_locked());
    auto rptr2 = sl();
    require_true(sl.is_locked());
    (sl.*rptr2)();
    require_false(sl.is_locked());
  }
  end_test_case();

  test_case("try_lock: free succeeds, held fails");
  {
    spin_lock sl;
    require_true(sl.try_lock());
    require_false(sl.try_lock());
    sl.unlock();
    require_true(sl.try_lock());
    sl.unlock();
  }
  end_test_case();

  test_case("operator!() reports unlocked");
  {
    spin_lock sl;
    require_true(!sl);
    sl.try_lock();
    require_false(!sl);
    sl.unlock();
  }
  end_test_case();

  test_case("4-thread contention: counter == 4 * iters");
  {
    spin_lock sl;
    int counter = 0;
    constexpr int kIters = 10000;
    StressArgs a{ &sl, &counter, kIters };
    mtest::parallel(4, [&](int) { stress_worker(&a); });
    require(counter == 4 * kIters);
  }
  end_test_case();

  test_case("is_locked() observable across threads while held");
  {
    spin_lock sl;
    atomic_token<bool> held(false);
    atomic_token<bool> release_now(false);
    ObservAgs a{ &sl, &held, &release_now };
    {
      auto_thread<> holder(observ_holder, &a);
      while ( !held.get(memory_order::acquire) ) micron::yield();
      require_true(sl.is_locked());
      release_now.store(true, memory_order::release);
    }
    require_false(sl.is_locked());
  }
  end_test_case();

  sb::print("=== ALL SPIN_LOCK TESTS PASSED ===");
  return 1;
}
