//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// mcs_lock is queuing_mutex with the mcs_node drawn from a per-thread slot table, which is what
// lets it satisfy is_mutex and work with every guard in the tree. the interesting cases are the
// ones queuing_mutex could not have: NESTING several locks at once, releasing them out of order,
// and the per-thread isolation of the nodes -- sync/inlet.hpp's old adapter shared ONE node across
// every thread, which is what made queuing_inlet<T> unsafe.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)
#define MICRON_LOCK_STATS 1      // queuing_mutex::enqueued() is the only observable that pins arrival order

#include "../../src/mutex/locks/mcs_lock.hpp"

#include "../../src/concepts.hpp"
#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

#include "../support/lockcheck.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{
constexpr u64 SEED_MCS = 0x51EDBEEF0BADULL;
}

static_assert(micron::is_mutex<micron::mcs_lock>, "mcs_lock must satisfy is_mutex");

int
main(void)
{
  using namespace micron;
  sb::print("=== MCS_LOCK TESTS ===");

  test_case("default ctor unheld");
  {
    mcs_lock m;
    require_false(m.is_locked());
    require_false(m.holds());
  }
  end_test_case();

  test_case("lock / unlock roundtrip, holds() tracks this thread");
  {
    mcs_lock m;
    m.lock();
    require_true(m.is_locked());
    require_true(m.holds());
    m.unlock();
    require_false(m.is_locked());
    require_false(m.holds());
  }
  end_test_case();

  test_case("operator()(void) returns the fn-ptr; dispatch unlocks");
  {
    mcs_lock m;
    auto r = m();
    require_true(m.is_locked());
    (m.*r)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock succeeds when free, fails when held elsewhere");
  {
    mcs_lock m;
    require_true(m.try_lock());
    require_true(m.is_locked());
    m.unlock();

    atomic_token<bool> held(false);
    atomic_token<bool> go(false);
    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      held.store(true, memory_order::release);
      while ( !go.get(memory_order::acquire) ) micron::yield();
      m.unlock();
    });
    while ( !held.get(memory_order::acquire) ) micron::yield();
    require_false(m.try_lock());
    require_false(m.holds());      // a failed try_lock must not leave a slot claimed
    go.store(true, memory_order::release);
    solo::join(t);
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("unlock by a thread that does not hold it releases the acquisition");
  {
    // RELEASE IS BY THE LOCK, NOT BY THE THREAD. MCS's release needs the NODE, never the releaser's
    // identity, and the rest of the tree already assumes as much: adopt_lock, lock_set, the
    // variadic unlock(T &...) and sync/inlet.hpp's movable handle_t all exist precisely to release
    // something they did not take. Finding the node through the CALLER's slot table instead made
    // every one of those a silent no-op -- a lock never released, and every later locker parked
    // forever. This case used to assert that no-op; it now asserts the release.
    mcs_lock m;
    atomic_token<bool> held(false);
    atomic_token<bool> go(false);
    atomic_token<bool> third(false);
    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      held.store(true, memory_order::release);
      // stays ALIVE until the case is done: the node lives in this thread's TLS, and exiting with
      // an acquisition outstanding is out of contract for a different reason than the one under
      // test here.
      while ( !go.get(memory_order::acquire) ) micron::yield();
      m.unlock();      // the acquisition is already gone, so this is the no-op: a second SEQUENCED release
    });
    while ( !held.get(memory_order::acquire) ) micron::yield();

    m.unlock();      // we hold nothing, and that is exactly who is allowed to release it
    require_false(m.is_locked());

    // and it is a usable lock afterwards, not a wedged one
    auto u = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      third.store(true, memory_order::release);
      m.unlock();
    });
    while ( !third.get(memory_order::acquire) ) micron::yield();
    solo::join(u);
    require_false(m.is_locked());

    go.store(true, memory_order::release);
    solo::join(t);
    require_false(m.is_locked());      // the original acquirer's trailing unlock() changed nothing
  }
  end_test_case();

  // ---- the cases queuing_mutex's node-passing API could not express ----
  test_case("nesting: one thread holds MICRON_MCS_DEPTH distinct locks at once");
  {
    constexpr usize kDepth = MICRON_MCS_DEPTH;
    mcs_lock m[kDepth];
    for ( usize i = 0; i < kDepth; ++i ) {
      m[i].lock();
      require_true(m[i].holds());
    }
    for ( usize i = 0; i < kDepth; ++i ) require_true(m[i].is_locked());
    for ( usize i = 0; i < kDepth; ++i ) m[i].unlock();
    for ( usize i = 0; i < kDepth; ++i ) require_false(m[i].is_locked());
  }
  end_test_case();

  // The cases below hold a FIXED number of distinct locks at once, so they are out of contract when
  // MICRON_MCS_DEPTH is narrower than that -- the slot table raises `queue-lock slot table
  // exhausted` rather than silently over-subscribing, which mutex_mcs_regression.cpp is the place
  // that pins. locks.duck also builds this file at --def MICRON_MCS_DEPTH=1, so they are compiled
  // out there rather than asserted. (The nesting case above needs no guard: it is written against
  // MICRON_MCS_DEPTH itself.)
#if MICRON_MCS_DEPTH >= 3
  test_case("non-LIFO release: locks may be dropped in any order");
  {
    mcs_lock a, b, c;
    a.lock();
    b.lock();
    c.lock();
    b.unlock();      // middle first -- a stack-shaped node pool would mis-associate here
    require_false(b.is_locked());
    require_true(a.is_locked());
    require_true(c.is_locked());
    a.unlock();
    require_false(a.is_locked());
    require_true(c.is_locked());
    c.unlock();
    require_false(c.is_locked());

    b.lock();      // and the freed slots are reusable
    b.unlock();
    require_false(b.is_locked());
  }
  end_test_case();
#else
  sb::print("     skipped, needs MICRON_MCS_DEPTH >= 3: non-LIFO release");
#endif

  test_case("nodes are per-thread: two threads on the same lock never share one");
  {
    // the regression for sync/inlet.hpp's old queuing_mutex_adapter. with a shared node, concurrent
    // lockers scribble on the same next/waiting pair; the exclusion probe is what sees it.
    mcs_lock m;
    lcheck::exclusion_probe pr;
    constexpr int kIters = 4000;
    const int kT = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        lcheck::guarded_section<mcs_lock> g(m, pr, 8);
      }
    });

    require(pr.entries.get(memory_order::acquire) == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("FIFO order: waiters are served in the order they enqueued");
  {
    // arrival order can only be pinned from INSIDE the lock: a waiter blocks the instant it swaps
    // itself onto the tail, so any flag it raises beforehand races that swap. enqueued() counts
    // arrivals at the swap itself, which is exactly the ordering point. (an earlier version of this
    // test used a baton raised before lock() and reported 6 of 8 "out of order" against a lock that
    // is FIFO by construction -- the test was wrong, not the lock.)
    mcs_lock m;
    lcheck::fairness_probe fp;
    const u32 kT = lcheck::wide_threads;
    atomic_token<bool> holding(false);

    mtest::parallel(static_cast<int>(kT) + 1, [&](int tid) {
      if ( tid == static_cast<int>(kT) ) {
        m.lock();      // arrival #1
        holding.store(true, memory_order::release);
        while ( m.enqueued() != 1u + kT ) micron::yield();
        m.unlock();
        return;
      }
      const u32 me = static_cast<u32>(tid);
      while ( !holding.get(memory_order::acquire) ) micron::yield();
      while ( m.enqueued() != 1u + me ) micron::yield();      // our turn to take a queue position
      m.lock();
      fp.note(me);
      m.unlock();
    });

    require(fp.logged() == static_cast<u64>(kT));
    u64 out_of_order = 0;
    for ( u32 i = 0; i < kT; ++i )
      if ( fp.at(i) != i ) ++out_of_order;
    sb::print("     enqueue-order handoffs, out of order: ", static_cast<usize>(out_of_order), "/", static_cast<usize>(kT));
    require(out_of_order == 0ull);
  }
  end_test_case();

  test_case("every guard in the tree accepts it");
  {
    mcs_lock m;
    {
      lock_guard<mcs_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      auto_guard<mcs_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      unique_lock<lock_starts::locked, mcs_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

#if MICRON_MCS_DEPTH >= 3
  test_case("lock_set over several mcs_locks");
  {
    mcs_lock a, b, c;
    {
      lock_set<mcs_lock, mcs_lock, mcs_lock> g(a, b, c);
      require_true(a.is_locked());
      require_true(b.is_locked());
      require_true(c.is_locked());
    }
    require_false(a.is_locked());
    require_false(b.is_locked());
    require_false(c.is_locked());
  }
  end_test_case();
#else
  sb::print("     skipped, needs MICRON_MCS_DEPTH >= 3: lock_set over several mcs_locks");
#endif

#if MICRON_MCS_DEPTH >= 4
  test_case("randomized nesting across threads: exclusion holds on every lock");
  {
    constexpr usize kLocks = 4;
    mcs_lock m[kLocks];
    lcheck::exclusion_probe pr[kLocks];
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 1200;

    mtest::parallel(kT, [&](int tid) {
      u64 s = SEED_MCS + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < kIters; ++i ) {
        // take an ordered subset (ascending index) so this cannot deadlock, then release in a
        // random order -- which is precisely what the slot table has to survive
        usize idx[kLocks];
        usize n = 0;
        for ( usize k = 0; k < kLocks; ++k )
          if ( lcheck::xs64(s) & 1u ) idx[n++] = k;
        for ( usize k = 0; k < n; ++k ) {
          m[idx[k]].lock();
          pr[idx[k]].enter();
        }
        pr[0].dwell(static_cast<u32>(lcheck::xs64(s) & 0x1F));
        const bool reverse = (lcheck::xs64(s) & 1u) != 0;
        for ( usize k = 0; k < n; ++k ) {
          const usize j = reverse ? (n - 1 - k) : k;
          pr[idx[j]].leave();
          m[idx[j]].unlock();
        }
      }
    });

    for ( usize k = 0; k < kLocks; ++k ) {
      require(pr[k].violations.get(memory_order::acquire) == 0ull);
      require_false(m[k].is_locked());
    }
  }
  end_test_case();
#else
  sb::print("     skipped, needs MICRON_MCS_DEPTH >= 4: randomized nesting across threads");
#endif

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<mcs_lock>);
    static_assert(!is_move_constructible_v<mcs_lock>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL MCS_LOCK TESTS PASSED ===");
  return 1;
}
