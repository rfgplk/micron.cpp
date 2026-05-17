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

struct RLArgs {
  micron::recursive_lock *rl;
  int *counter;
  int iters;
};

void
rl_worker(RLArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    auto rptr = p->rl->lock();
    ++(*p->counter);
    (p->rl->*rptr)();
  }
}

struct TryArgs {
  micron::recursive_lock *rl;
  micron::atomic_token<bool> *seen_fail;
  micron::atomic_token<bool> *release_now;
};

void
try_attacker(TryArgs *p)
{
  // wait until parent has locked
  while ( !p->rl->is_locked() ) micron::yield();
  if ( !p->rl->try_lock() ) p->seen_fail->store(true, micron::memory_order::release);
  // signal parent
  p->release_now->store(true, micron::memory_order::release);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== RECURSIVE_LOCK TESTS ===");

  test_case("default ctor unlocked, depth 0");
  {
    recursive_lock rl;
    require_false(rl.is_locked());
    require(rl.lock_depth() == (usize)0);
  }
  end_test_case();

  test_case("single-thread lock then unlock — depth 0→1→0");
  {
    recursive_lock rl;
    auto rptr = rl.lock();
    require_true(rl.is_locked());
    require(rl.lock_depth() == (usize)1);
    (rl.*rptr)();
    require_false(rl.is_locked());
    require(rl.lock_depth() == (usize)0);
  }
  end_test_case();

  test_case("nested lock 3 deep — depth 1→2→3 then unwinds");
  {
    recursive_lock rl;
    auto r1 = rl.lock();
    require(rl.lock_depth() == (usize)1);
    auto r2 = rl.lock();
    require(rl.lock_depth() == (usize)2);
    auto r3 = rl.lock();
    require(rl.lock_depth() == (usize)3);
    require_true(rl.is_locked());
    (rl.*r3)();
    require(rl.lock_depth() == (usize)2);
    (rl.*r2)();
    require(rl.lock_depth() == (usize)1);
    (rl.*r1)();
    require(rl.lock_depth() == (usize)0);
    require_false(rl.is_locked());
  }
  end_test_case();

  test_case("try_lock by owner re-enters, increments depth");
  {
    recursive_lock rl;
    rl.lock();
    require_true(rl.try_lock());
    require(rl.lock_depth() == (usize)2);
    rl.unlock();
    rl.unlock();
    require_false(rl.is_locked());
  }
  end_test_case();

  test_case("try_lock by non-owner fails while held");
  {
    recursive_lock rl;
    rl.lock();
    atomic_token<bool> seen_fail(false);
    atomic_token<bool> release_now(false);
    TryArgs a{ &rl, &seen_fail, &release_now };
    {
      auto_thread<> attacker(try_attacker, &a);
      while ( !release_now.get(memory_order::acquire) ) yield();
    }
    require_true(seen_fail.get(memory_order::acquire));
    rl.unlock();
  }
  end_test_case();

  test_case("operator!() reports unlocked state");
  {
    recursive_lock rl;
    require_true(!rl);
    rl.lock();
    require_false(!rl);
    rl.unlock();
  }
  end_test_case();

  test_case("4-thread serialization via lock_guard<recursive_lock>");
  {
    recursive_lock rl;
    int counter = 0;
    constexpr int kIters = 5000;
    RLArgs a{ &rl, &counter, kIters };
    {
      auto_thread<> t1(rl_worker, &a);
      auto_thread<> t2(rl_worker, &a);
      auto_thread<> t3(rl_worker, &a);
      auto_thread<> t4(rl_worker, &a);
    }
    require(counter == 4 * kIters);
  }
  end_test_case();

  test_case("retrieve() returns reset fn-ptr without locking");
  {
    recursive_lock rl;
    auto r = rl.retrieve();
    (void)r;
    require_false(rl.is_locked());
  }
  end_test_case();

  sb::print("=== ALL RECURSIVE_LOCK TESTS PASSED ===");
  return 1;
}
