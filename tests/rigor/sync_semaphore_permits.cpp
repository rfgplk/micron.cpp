//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// REGRESSION: basic_semaphore::wait() could return WITHOUT a permit.
//
// it used to park with wait_futex(counter.ptr(), v), and wait_futex loops only while the word is
// unchanged. so a SECOND waiter arriving -- which drives the counter -1 -> -2 -- ended the first
// waiter's wait, and wait() returned as though a permit had arrived. the counter accounting stayed
// exact either way (every wait() decrements once regardless), which is precisely why
// tests/rigor/sync_semaphore.cpp's total-based contended cases could not see it.
//
// waiters now park on a separate handoff word that only flag() ever publishes to, so the number of
// threads that get through is the number of permits posted, and nothing else.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/sync/semaphore.hpp"

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

int
main(void)
{
  using namespace micron;
  sb::print("=== SEMAPHORE PERMIT-EXACTNESS TESTS ===");

  test_case("the historical signed counter contract is unchanged");
  {
    basic_semaphore s(3);
    require(s.value() == 3);
    s.reset(-3);
    require(s.value() == -3);
    s.flag();
    require(s.value() == -2);      // flag() increments, it never stores a literal
    s.reset(0);
    require(s.value() == 0);
  }
  end_test_case();

  // ---- the regression proper ----
  test_case("N waiters, M permits: exactly M get through, N-M stay parked");
  {
    constexpr int kWaiters = 8;
    constexpr int kPermits = 3;
    basic_semaphore s(0);
    atomic_token<int> acquired(0);
    atomic_token<int> parked(0);
    lcheck::start_gate gate(kWaiters);

    micron::__thread_pointer<micron::auto_thread<>> ts[kWaiters];
    for ( int i = 0; i < kWaiters; ++i )
      ts[i] = solo::spawn<auto_thread<>>([&]() {
        u32 sense = 0;
        gate.wait(sense);
        parked.fetch_add(1, memory_order::acq_rel);
        s.wait();
        acquired.fetch_add(1, memory_order::acq_rel);
      });

    // every waiter has driven the counter negative before any permit is posted
    while ( parked.get(memory_order::acquire) != kWaiters ) micron::yield();
    while ( s.value() > -kWaiters ) micron::yield();
    require(s.value() == -kWaiters);

    for ( int i = 0; i < kPermits; ++i ) s.flag();

    // give the wrong answer plenty of time to show up: pre-fix, the arrival of each further waiter
    // released the ones already parked, so this count ran away well past kPermits
    micron::sleep_for(150);
    const int got = acquired.get(memory_order::acquire);
    sb::print("     posted ", static_cast<usize>(kPermits), " permits to ", static_cast<usize>(kWaiters),
              " waiters; released=", static_cast<usize>(got));
    require(got == kPermits);
    require(s.value() == -(kWaiters - kPermits));

    for ( int i = kPermits; i < kWaiters; ++i ) s.flag();      // let the rest go so we can join
    for ( int i = 0; i < kWaiters; ++i ) solo::join(ts[i]);

    require(acquired.get(memory_order::acquire) == kWaiters);
    require(s.value() == 0);
  }
  end_test_case();

  test_case("permits posted one at a time release exactly one waiter each");
  {
    constexpr int kWaiters = 6;
    basic_semaphore s(0);
    atomic_token<int> acquired(0);
    atomic_token<int> parked(0);
    lcheck::start_gate gate(kWaiters);

    micron::__thread_pointer<micron::auto_thread<>> ts[kWaiters];
    for ( int i = 0; i < kWaiters; ++i )
      ts[i] = solo::spawn<auto_thread<>>([&]() {
        u32 sense = 0;
        gate.wait(sense);
        parked.fetch_add(1, memory_order::acq_rel);
        s.wait();
        acquired.fetch_add(1, memory_order::acq_rel);
      });

    while ( parked.get(memory_order::acquire) != kWaiters ) micron::yield();
    while ( s.value() > -kWaiters ) micron::yield();

    for ( int n = 1; n <= kWaiters; ++n ) {
      s.flag();
      micron::sleep_for(40);
      require(acquired.get(memory_order::acquire) == n);
    }

    for ( int i = 0; i < kWaiters; ++i ) solo::join(ts[i]);
    require(s.value() == 0);
  }
  end_test_case();

  test_case("a permit posted before anyone waits is consumed without parking");
  {
    basic_semaphore s(0);
    s.flag();
    require(s.value() == 1);
    s.wait();      // must not block
    require(s.value() == 0);
  }
  end_test_case();

  test_case("try_wait never consumes a handoff token meant for a parked waiter");
  {
    constexpr int kWaiters = 4;
    basic_semaphore s(0);
    atomic_token<int> acquired(0);
    atomic_token<int> parked(0);
    lcheck::start_gate gate(kWaiters);

    micron::__thread_pointer<micron::auto_thread<>> ts[kWaiters];
    for ( int i = 0; i < kWaiters; ++i )
      ts[i] = solo::spawn<auto_thread<>>([&]() {
        u32 sense = 0;
        gate.wait(sense);
        parked.fetch_add(1, memory_order::acq_rel);
        s.wait();
        acquired.fetch_add(1, memory_order::acq_rel);
      });

    while ( parked.get(memory_order::acquire) != kWaiters ) micron::yield();
    while ( s.value() > -kWaiters ) micron::yield();

    // the counter is negative, so try_wait must refuse rather than steal from the queue
    for ( int i = 0; i < 100; ++i ) require_false(s.try_wait());
    require(s.value() == -kWaiters);

    for ( int i = 0; i < kWaiters; ++i ) s.flag();
    for ( int i = 0; i < kWaiters; ++i ) solo::join(ts[i]);
    require(acquired.get(memory_order::acquire) == kWaiters);
    require(s.value() == 0);
  }
  end_test_case();

  test_case("balanced post/wait storm still converges to zero");
  {
    constexpr int kPairs = 6;
    constexpr int kRounds = 400;
    basic_semaphore s(0);
    atomic_token<int> got(0);
    lcheck::start_gate gate(2 * kPairs);

    mtest::parallel(2 * kPairs, [&](int id) {
      u32 sense = 0;
      gate.wait(sense);
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

  sb::print("=== ALL SEMAPHORE PERMIT TESTS PASSED ===");
  return 1;
}
