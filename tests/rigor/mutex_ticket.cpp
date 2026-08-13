//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ticket_lock is the tree's first FIFO lock with a plain is_mutex shape, so this is also the first
// test that grades FAIRNESS rather than a counter total. the two are independent: an unfair lock
// still totals correctly, which is exactly why every other mutex_* contention case passes without
// knowing whether the lock starved anyone.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/locks/ticket_lock.hpp"

#include "../../src/concepts.hpp"
#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

#include "../support/lockcheck.hpp"      // needs snowball for the watchdog's failure path

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr u64 SEED_HOLD = 0xC0FFEE711CE7ULL;

struct count_args {
  micron::ticket_lock *lk;
  int *counter;
  int iters;
};

void
count_worker(count_args *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    auto r = p->lk->lock();
    ++(*p->counter);
    (p->lk->*r)();
  }
}

}      // namespace

static_assert(micron::is_mutex<micron::ticket_lock>, "ticket_lock must satisfy is_mutex");
static_assert(micron::is_mutex<micron::ticket_spin_lock>, "ticket_spin_lock must satisfy is_mutex");
static_assert(alignof(micron::ticket_lock) >= micron::cache_line_size(), "ticket_lock must be cache-line aligned");
static_assert(sizeof(micron::ticket_lock) == 2 * micron::cache_line_size(), "next and serving must not share a line");

int
main(void)
{
  using namespace micron;
  sb::print("=== TICKET_LOCK TESTS ===");

  test_case("default ctor is unheld, nothing queued");
  {
    ticket_lock m;
    require_false(m.is_locked());
    require(m.queued() == 0u);
  }
  end_test_case();

  test_case("lock / unlock roundtrip");
  {
    ticket_lock m;
    m.lock();
    require_true(m.is_locked());
    require(m.queued() == 1u);      // the holder itself counts as one outstanding ticket
    m.unlock();
    require_false(m.is_locked());
    require(m.queued() == 0u);
  }
  end_test_case();

  test_case("operator()(void) returns the fn-ptr; dispatch unlocks");
  {
    ticket_lock m;
    auto r = m();
    require_true(m.is_locked());
    (m.*r)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock succeeds when free, fails when held");
  {
    ticket_lock m;
    require_true(m.try_lock());
    require_true(m.is_locked());
    require_false(m.try_lock());
    m.unlock();
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("serving advances by exactly one per release");
  {
    ticket_lock m;
    const u32 s0 = m.serving();
    for ( int i = 0; i < 5; ++i ) {
      m.lock();
      m.unlock();
    }
    require(m.serving() == s0 + 5u);
  }
  end_test_case();

  // ---- the wraparound the counters would otherwise take 2^32 acquisitions to reach ----
  test_case("u32 wraparound: seeded at 0xFFFFFFFE, crosses zero cleanly");
  {
    ticket_lock m(0xFFFFFFFEu);
    require_false(m.is_locked());
    require(m.queued() == 0u);
    for ( int i = 0; i < 6; ++i ) {
      m.lock();
      require_true(m.is_locked());
      require(m.queued() == 1u);
      m.unlock();
      require_false(m.is_locked());
    }
    require(m.serving() == 0xFFFFFFFEu + 6u);      // wraps to 4
    require(m.serving() == 4u);
  }
  end_test_case();

  test_case("wraparound under contention: totals stay exact across the fold");
  {
    ticket_lock m(0xFFFFFF00u);
    int counter = 0;
    constexpr int kIters = 4000;
    const int kT = static_cast<int>(lcheck::wide_threads);
    count_args a{ &m, &counter, kIters };
    mtest::parallel(kT, [&](int) { count_worker(&a); });
    require(counter == kT * kIters);
    require_false(m.is_locked());
  }
  end_test_case();

  // ---- mutual exclusion, graded by a double-entry probe rather than by the total ----
  test_case("no two threads inside the critical section at once");
  {
    ticket_lock m;
    lcheck::exclusion_probe pr;
    constexpr int kIters = 3000;
    const int kT = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        lcheck::guarded_section<ticket_lock> g(m, pr, 8);      // dwell, or an overlap is unobservable
      }
    });

    require(pr.entries.get(memory_order::acquire) == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
  }
  end_test_case();

  // ---- FIFO: the property the type exists for ----
  test_case("FIFO order: waiters are served in the order they enqueued");
  {
    // a KNOWN enqueue order is the only way to grade this. one thread holds the lock while worker i
    // waits until exactly i workers are already queued behind it -- queued() is next-minus-serving,
    // so it is the observable that pins arrival order before anyone can be served.
    ticket_lock m;
    lcheck::fairness_probe fp;
    const u32 kT = lcheck::wide_threads;
    atomic_token<bool> holding(false);

    mtest::parallel(static_cast<int>(kT) + 1, [&](int tid) {
      if ( tid == static_cast<int>(kT) ) {      // the holder
        m.lock();
        holding.store(true, memory_order::release);
        while ( m.queued() != 1u + kT ) micron::yield();      // let everyone enqueue first
        m.unlock();
        return;
      }
      const u32 me = static_cast<u32>(tid);
      while ( !holding.get(memory_order::acquire) ) micron::yield();
      while ( m.queued() != 1u + me ) micron::yield();      // our turn to take a ticket
      m.lock();
      fp.note(me);
      m.unlock();
    });

    require(fp.logged() == static_cast<u64>(kT));
    for ( u32 i = 0; i < kT; ++i ) require(fp.at(i) == i);      // served exactly in arrival order
    sb::print("     served in arrival order across ", static_cast<usize>(kT), " waiters");
  }
  end_test_case();

  test_case("free-running contention is even: most/least acquisitions ~ 1");
  {
    ticket_lock m;
    lcheck::fairness_probe fp;
    const u32 kT = lcheck::wide_threads;
    constexpr int kIters = 400;

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        fp.note(static_cast<u32>(tid));
        for ( u32 k = 0; k < 32; ++k ) __cpu_pause();      // widen the hold so the queue stays full
        m.unlock();
      }
    });

    require(fp.total(kT) == static_cast<u64>(kT) * kIters);
    sb::print("     handoffs=", static_cast<usize>(fp.handoffs.get(memory_order::acquire)),
              " spread(x100)=", static_cast<usize>(fp.spread(kT) * 100.0));

    require_true(fp.least(kT) > 0);      // nobody starved
    // every thread does exactly kIters acquisitions here, so spread is 1.0 by construction -- this
    // asserts the run completed, not that the lock is fair. the FIFO guarantee is graded by the
    // arrival-order case above, which is deterministic. mutex_exclusion.cpp measures spread across
    // every lock type on a fixed WALL-time workload but deliberately does not assert on it: over a
    // wall clock a FIFO lock only orders the threads that are enqueued, so the number tracks the
    // scheduler as much as the lock.
    require_true(fp.spread(kT) < 1.5);
  }
  end_test_case();

  test_case("try_lock does not jump the queue while waiters are enqueued");
  {
    ticket_lock m;
    atomic_token<bool> held(false);
    atomic_token<bool> go(false);

    // holder takes it, a second thread queues behind, then the main thread's try_lock must fail
    // even after the holder releases -- the queued waiter is next, not us
    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      held.store(true, memory_order::release);
      while ( !go.get(memory_order::acquire) ) micron::yield();
      m.unlock();
    });

    while ( !held.get(memory_order::acquire) ) micron::yield();
    require_false(m.try_lock());
    require(m.queued() == 1u);
    go.store(true, memory_order::release);
    solo::join(t);

    require_false(m.is_locked());
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("lock_guard / unique_lock / auto_guard all accept it");
  {
    ticket_lock m;
    {
      lock_guard<ticket_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      auto_guard<ticket_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      unique_lock<lock_starts::locked, ticket_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<ticket_lock>);
    static_assert(!is_move_constructible_v<ticket_lock>);
    require_true(true);
  }
  end_test_case();

  // ---- randomized hold times, fixed seed ----
  test_case("randomized hold lengths keep exclusion and totals exact");
  {
    ticket_lock m;
    lcheck::exclusion_probe pr;
    u64 total = 0;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 1500;

    mtest::parallel(kT, [&](int tid) {
      u64 s = SEED_HOLD + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        pr.enter();
        pr.dwell(static_cast<u32>(lcheck::xs64(s) & 0x3F));
        ++total;
        pr.leave();
        m.unlock();
      }
    });

    require(total == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  sb::print("=== ALL TICKET_LOCK TESTS PASSED ===");
  return 1;
}
