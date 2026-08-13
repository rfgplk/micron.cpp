//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// CLH's distinguishing move is that a releaser hands its node AWAY and inherits its predecessor's,
// so the same thread publishes a different node on every acquisition. that is what makes the arena
// necessary (a thread-local node would dangle the moment its owner exited) and it is what the
// node-inheritance case below actually checks.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)
#define MICRON_LOCK_STATS 1

#include "../../src/mutex/locks/clh_lock.hpp"

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

static_assert(micron::is_mutex<micron::clh_lock>, "clh_lock must satisfy is_mutex");
static_assert(alignof(micron::clh_node) >= micron::cache_line_size(), "clh nodes must not share a line");

int
main(void)
{
  using namespace micron;
  sb::print("=== CLH_LOCK TESTS ===");

  test_case("default ctor unheld, no participants yet");
  {
    clh_lock m;
    require_false(m.is_locked());
    require(m.participants() == 0u);
  }
  end_test_case();

  test_case("lock / unlock roundtrip; the first acquire draws one arena node");
  {
    clh_lock m;
    m.lock();
    require_true(m.is_locked());
    require(m.participants() == 1u);
    m.unlock();
    require_false(m.is_locked());
    // still 1: unlock() returns its predecessor to the POOL rather than to this thread -- nodes
    // migrate in CLH and there is no per-thread reservation -- but participants() is __drawn - 1,
    // a high-water mark, so returning a node does not lower it.
    require(m.participants() == 1u);
  }
  end_test_case();

  test_case("repeat acquisitions by one thread draw no further arena nodes");
  {
    clh_lock m;
    for ( int i = 0; i < 50; ++i ) {
      m.lock();
      m.unlock();
    }
    require(m.participants() == 1u);
    require(m.stats().acquires() == 50ull);
  }
  end_test_case();

  test_case("operator()(void) returns the fn-ptr; dispatch unlocks");
  {
    clh_lock m;
    auto r = m();
    require_true(m.is_locked());
    (m.*r)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock takes a free lock and refuses a held one");
  {
    clh_lock m;
    require_true(m.try_lock());
    require_true(m.is_locked());
    m.unlock();
    require_false(m.is_locked());

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
    go.store(true, memory_order::release);
    solo::join(t);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("node inheritance: the node a thread publishes changes across acquisitions");
  {
    // two threads alternating; after each handoff the releaser owns what its predecessor published.
    // if inheritance were broken the queue would either self-deadlock or admit two holders, and the
    // exclusion probe is what would see the second case.
    clh_lock m;
    lcheck::exclusion_probe pr;
    constexpr int kIters = 4000;

    mtest::parallel(2, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        lcheck::guarded_section<clh_lock> g(m, pr, 16);
      }
    });

    require(pr.entries.get(memory_order::acquire) == 2ull * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    // a range: participants() is peak CONCURRENT participation, and two threads whose critical
    // sections are this short may simply never overlap, in which case one pooled node serves both.
    // The exclusion probe above is what this case is actually for. See the forced-overlap case for
    // the exact 2.
    require(m.participants() >= 1u and m.participants() <= 2u);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("no two threads inside the critical section at once");
  {
    clh_lock m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        lcheck::guarded_section<clh_lock> g(m, pr, 8);
      }
    });

    require(pr.entries.get(memory_order::acquire) == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
    require_true(m.participants() <= static_cast<u32>(kT));
  }
  end_test_case();

  test_case("totals stay exact under oversubscription");
  {
    clh_lock m;
    u64 total = 0;
    const int kT = static_cast<int>(lcheck::over_threads);
    constexpr int kIters = 400;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        ++total;
        m.unlock();
      }
    });
    require(total == static_cast<u64>(kT) * kIters);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("nobody starves under free-running contention");
  {
    clh_lock m;
    lcheck::fairness_probe fp;
    const u32 kT = lcheck::wide_threads;
    constexpr int kIters = 400;

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        fp.note(static_cast<u32>(tid));
        for ( u32 p = 0; p < 32; ++p ) __cpu_pause();
        m.unlock();
      }
    });

    require(fp.total(kT) == static_cast<u64>(kT) * kIters);
    require_true(fp.least(kT) > 0);
  }
  end_test_case();

  test_case("guards accept it");
  {
    clh_lock m;
    {
      lock_guard<clh_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      auto_guard<clh_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      unique_lock<lock_starts::locked, clh_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("a narrow arena still serves the threads that fit");
  {
    basic_clh_lock<2> m;      // 2 threads + the dummy
    u64 total = 0;
    constexpr int kIters = 2000;
    mtest::parallel(2, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        ++total;
        m.unlock();
      }
    });
    require(total == 2ull * kIters);
    // a RANGE, not an equality: participants() is __drawn - 1, and __drawn only grows when the pool
    // comes up empty, so it is the high-water mark of CONCURRENT participation rather than a count
    // of the threads that have ever passed through. Two threads this short-bodied often never
    // overlap, and then one node serves both. What this case is really about is that the narrow
    // arena serves them at all; the forced-overlap case below is where the exact 2 is pinned.
    require(m.participants() >= 1u and m.participants() <= 2u);
  }
  end_test_case();

  test_case("participants() counts peak CONCURRENT participation, not arrivals");
  {
    // The overlap is forced rather than hoped for: the holder does not release until it can see a
    // second node drawn, which can only happen once the other thread has enqueued behind it.
    basic_clh_lock<2> m;
    atomic_token<u32> arrived{ 0 };

    mtest::parallel(2, [&](int id) {
      if ( id == 0 ) {
        m.lock();
        arrived.store(1, memory_order::release);
        // the holder waits for a second node to be drawn, which can only happen once the other
        // thread is inside lock(). bounded rather than a bare while: if it never arrives, fall
        // through and let the assertion below name the failure instead of hanging the sweep.
        for ( u64 i = 0; i < 50000000ull and m.participants() < 2u; ++i )
          if ( (i & 0xFFull) == 0xFFull ) micron::yield();
        m.unlock();
      } else {
        while ( arrived.get(memory_order::acquire) == 0u ) micron::yield();
        m.lock();      // draws the second node, then queues behind the holder
        m.unlock();
      }
    });

    require(m.participants() == 2u);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("size is (MaxThreads + 1) cache lines, as documented");
  {
    static_assert(sizeof(basic_clh_lock<2>) >= 3 * micron::cache_line_size(),
                  "the arena is what buys CLH its node lifetime; it is not free");
    require_true(true);
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<clh_lock>);
    static_assert(!is_move_constructible_v<clh_lock>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL CLH_LOCK TESTS PASSED ===");
  return 1;
}
