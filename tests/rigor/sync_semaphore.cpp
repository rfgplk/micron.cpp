//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// basic_semaphore::flag() now does add_fetch(1, release) + wake_futex (a pure
// wake that does NOT store) instead of release_futex(counter.ptr(), 1) (which
// overwrote the post-add counter with literal 1). The contended test below
// exercises that wake path directly: N waiters drive the counter negative and
// park; M posters flag() them. No thread may block forever and the counter
// must stay exact.

#include "../../src/atomic/atomic.hpp"
#include "../../src/except.hpp"

#include "../../src/sync/semaphore.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../support/mt.hpp"      // mtest::parallel (micron auto_thread; NOT <thread>)

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== SEMAPHORE TESTS ===");

  test_case("basic_semaphore default construction value == 0");
  {
    basic_semaphore s;
    require(s.value() == 0);
  }
  end_test_case();

  test_case("basic_semaphore(__init=3) value == 3");
  {
    basic_semaphore s(3);
    require(s.value() == 3);
  }
  end_test_case();

  test_case("flag() increments counter");
  {
    basic_semaphore s(0);
    s.flag();
    require(s.value() == 1);
    s.flag();
    require(s.value() == 2);
  }
  end_test_case();

  test_case("try_wait returns false when counter == 0");
  {
    basic_semaphore s(0);
    require_false(s.try_wait());
    require(s.value() == 0);
  }
  end_test_case();

  test_case("try_wait returns true when counter > 0 and decrements");
  {
    basic_semaphore s(3);
    require_true(s.try_wait());
    require(s.value() == 2);
    require_true(s.try_wait());
    require_true(s.try_wait());
    require_false(s.try_wait());
    require(s.value() == 0);
  }
  end_test_case();

  test_case("wait() decrements counter (positive path; no contention)");
  {
    basic_semaphore s(2);
    s.wait();
    require(s.value() == 1);
    s.wait();
    require(s.value() == 0);
  }
  end_test_case();

  test_case("reset(n) restores counter to n");
  {
    basic_semaphore s(5);
    s.try_wait();
    s.try_wait();
    require(s.value() == 3);
    s.reset(10);
    require(s.value() == 10);
    s.reset();
    require(s.value() == 0);
  }
  end_test_case();

  // ── flag() must NOT clobber a negative counter (regression for bug #4) ──
  test_case("flag() on a negative counter increments (never stores literal 1)");
  {
    basic_semaphore s(0);
    s.reset(-3);      // simulate 3 parked waiters
    require(s.value() == -3);
    s.flag();
    require(s.value() == -2);      // pre-fix this jumped to 1
    s.flag();
    require(s.value() == -1);
    s.flag();
    require(s.value() == 0);
  }
  end_test_case();

  // ── CONTENDED: N waiters park on a negative count, M posters wake them ──
  // Exercises the flag() wake path the original suite deliberately avoided.
  // If flag() clobbered the counter (old bug) or failed to wake the last
  // waiter (o<0 vs o<=0), some acquire below would block forever and the
  // process would hang (test harness times out) — i.e. a HARD failure, not a
  // soft assert.
  test_case("contended: N acquire vs N post — no thread blocks forever, counter exact");
  {
    constexpr int kThreads = 8;
    constexpr int kRounds = 250;      // each thread does kRounds acquire+post pairs
    basic_semaphore s(0);
    atomic_token<int> acquired(0);

    // Half the threads are pure acquirers (they will park on a negative
    // counter); the other half are pure posters. Acquirers each wait()
    // kRounds times; posters each flag() kRounds times. Total posts == total
    // waits, so every parked acquirer is eventually released.
    mtest::parallel(kThreads, [&](int id) {
      if ( id % 2 == 0 ) {
        for ( int i = 0; i < kRounds; ++i ) {
          s.wait();
          acquired.fetch_add(1, memory_order::acq_rel);
        }
      } else {
        for ( int i = 0; i < kRounds; ++i ) {
          s.flag();
        }
      }
    });

    // kThreads/2 acquirers each acquired kRounds permits.
    require(acquired.get(memory_order::acquire) == (kThreads / 2) * kRounds);
    // posts == waits → counter settles back to its initial value (0).
    require(s.value() == 0);
  }
  end_test_case();

  // ── CONTENDED: producers post ahead, consumers drain, then balanced run ──
  test_case("contended: balanced post/wait converges to exact zero counter");
  {
    constexpr int kPairs = 6;      // kPairs poster threads + kPairs waiter threads
    constexpr int kRounds = 300;
    basic_semaphore s(0);
    atomic_token<int> got(0);

    mtest::parallel(2 * kPairs, [&](int id) {
      if ( id < kPairs ) {
        for ( int i = 0; i < kRounds; ++i ) s.flag();
      } else {
        for ( int i = 0; i < kRounds; ++i ) {
          s.wait();
          got.fetch_add(1, memory_order::acq_rel);
        }
      }
    });

    require(got.get(memory_order::acquire) == kPairs * kRounds);
    require(s.value() == 0);
  }
  end_test_case();

  sb::print("=== ALL SEMAPHORE TESTS PASSED ===");
  return 1;
}
