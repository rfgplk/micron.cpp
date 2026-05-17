//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

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

struct ULArgs {
  micron::mutex *m;
  int *counter;
  int iters;
};

void
ul_worker(ULArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    micron::unique_lock<micron::lock_starts::locked, micron::mutex> u(*p->m);
    ++(*p->counter);
    u.unlock();
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== UNIQUE_LOCK TESTS ===");

  test_case("unique_lock<locked, mutex> locks on ctor");
  {
    mutex m;
    {
      unique_lock<lock_starts::locked, mutex> u(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("unique_lock<adopt, mutex> assumes prior lock state");
  {
    mutex m;
    m.lock();
    require_true(m.is_locked());
    {
      unique_lock<lock_starts::adopt, mutex> u(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("unique_lock<unlocked, mutex> starts unowned, dtor does not unlock");
  {
    mutex m;
    {
      unique_lock<lock_starts::unlocked, mutex> u(m);
      require_false(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("unique_lock<defer, mutex> starts unowned, lock() acquires later");
  {
    mutex m;
    {
      unique_lock<lock_starts::defer, mutex> u(m);
      require_false(m.is_locked());
      u.lock();
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("unlock() releases lock and clears rptr");
  {
    mutex m;
    unique_lock<lock_starts::locked, mutex> u(m);
    require_true(m.is_locked());
    u.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock() acquires when free");
  {
    mutex m;
    unique_lock<lock_starts::defer, mutex> u(m);
    u.try_lock();
    require_true(m.is_locked());
  }
  end_test_case();

  test_case("release() returns mtx, clears ownership; dtor does not unlock");
  {
    mutex m;
    unique_lock<lock_starts::locked, mutex> u(m);
    require_true(m.is_locked());
    auto *p = u.release();
    require(p == &m);
    require_true(m.is_locked());
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("swap() exchanges state between two unique_locks");
  {
    mutex m1, m2;
    unique_lock<lock_starts::locked, mutex> u1(m1);
    unique_lock<lock_starts::locked, mutex> u2(m2);
    require_true(m1.is_locked());
    require_true(m2.is_locked());
    u1.swap(u2);

    require_true(m1.is_locked());
    require_true(m2.is_locked());
    u1.unlock();
    u2.unlock();
    require_false(m1.is_locked());
    require_false(m2.is_locked());
  }
  end_test_case();

  test_case("move ctor transfers state; source no longer unlocks");
  {
    mutex m;
    unique_lock<lock_starts::locked, mutex> src(m);
    require_true(m.is_locked());
    unique_lock<lock_starts::locked, mutex> dst(static_cast<unique_lock<lock_starts::locked, mutex> &&>(src));
    require_true(m.is_locked());
  }
  end_test_case();

  test_case("unique_lock<locked, spin_lock> rptr is typed for spin_lock");
  {
    spin_lock sl;
    {
      unique_lock<lock_starts::locked, spin_lock> u(sl);
      require_true(sl.is_locked());
    }
    require_false(sl.is_locked());
  }
  end_test_case();

  test_case("unique_lock 4-thread mutual exclusion");
  {
    mutex m;
    int counter = 0;
    constexpr int kIters = 10000;
    ULArgs a{ &m, &counter, kIters };
    {
      auto_thread<> t1(ul_worker, &a);
      auto_thread<> t2(ul_worker, &a);
      auto_thread<> t3(ul_worker, &a);
      auto_thread<> t4(ul_worker, &a);
    }
    require(counter == 4 * kIters);
  }
  end_test_case();

  sb::print("=== ALL UNIQUE_LOCK TESTS PASSED ===");
  return 1;
}
