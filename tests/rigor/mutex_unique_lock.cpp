//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Exercises fix #4 (unique_lock::unlocks → unlock rename). Test bodies call
// `u.unlock()`; pre-fix this file did not compile.
// Also exercises the fix already in unique_lock.hpp where rptr is templated
// on M, by instantiating unique_lock<lock_starts::locked, spin_lock>.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

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

  // ── construction with each lock_starts mode ─────────────────────────────

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

  // ── unlock() rename (Fix #4) ────────────────────────────────────────────

  test_case("unlock() releases lock and clears rptr (FIX #4)");
  {
    mutex m;
    unique_lock<lock_starts::locked, mutex> u(m);
    require_true(m.is_locked());
    u.unlock();
    require_false(m.is_locked());
    // dtor must not double-unlock
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
    require_true(m.is_locked());      // release does not unlock
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
    // ownership of the resets swaps; both mutexes remain held
    require_true(m1.is_locked());
    require_true(m2.is_locked());
    u1.unlock();      // releases m2 (post-swap u1's mtx is m2)
    u2.unlock();      // releases m1
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
    // src dtor runs first, should be a no-op (rptr nulled)
    // dst dtor runs second, unlocks
  }
  end_test_case();

  // ── unique_lock with spin_lock (verifies templated rptr) ───────────────

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

  // ── stress ──────────────────────────────────────────────────────────────

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

  // lock_starts::attempt had no constructor at all until now: the enumerator was declared at
  // locks.hpp:18 and unique_lock only ever handled locked/adopt/unlocked/defer, so the only way to
  // get one was the move ctor. owns_lock() is the other half -- a try that may fail is useless if
  // you cannot ask whether it did.
  test_case("lock_starts::attempt takes a free lock and reports ownership");
  {
    mutex m;
    {
      unique_lock<lock_starts::attempt, mutex> u(m);
      require_true(u.owns_lock());
      require_true(static_cast<bool>(u));
      require_true(m.is_locked());
    }
    require_false(m.is_locked());      // released on scope exit like any other owning form
  }
  end_test_case();

  test_case("lock_starts::attempt on a held lock reports failure and does not release it");
  {
    mutex m;
    m.lock();
    {
      unique_lock<lock_starts::attempt, mutex> u(m);
      require_false(u.owns_lock());
      require_false(static_cast<bool>(u));
      require_true(m.is_locked());      // still the original holder's
    }
    require_true(m.is_locked());      // a failed attempt must NOT unlock on the way out
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("a failed attempt can still be retried through lock()/try_lock()");
  {
    mutex m;
    m.lock();
    unique_lock<lock_starts::attempt, mutex> u(&m);      // pointer ctor
    require_false(u.owns_lock());
    require_false(u.try_lock());      // still held elsewhere
    m.unlock();
    require_true(u.try_lock());
    require_true(u.owns_lock());
    u.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  sb::print("=== ALL UNIQUE_LOCK TESTS PASSED ===");
  return 1;
}
