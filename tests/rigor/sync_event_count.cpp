//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// event_count had NO test anywhere. it is the tree's closest thing to a condition variable -- a
// futex epoch counter with prepare_wait/commit_wait/notify -- and the prepare/commit split exists
// specifically to close the lost-wakeup window between "I decided to sleep" and "I am asleep".
// that window is what the cases below aim at.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/sync/event_count.hpp"

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
  sb::print("=== EVENT_COUNT TESTS ===");

  test_case("prepare_wait returns a key; cancel_wait abandons cleanly");
  {
    event_count ec;
    const u32 k = ec.prepare_wait();
    ec.cancel_wait();
    const u32 k2 = ec.prepare_wait();
    ec.cancel_wait();
    require(k == k2);      // no notify in between, so the epoch did not move
  }
  end_test_case();

  test_case("notify_one moves the epoch even with nobody waiting");
  {
    event_count ec;
    const u32 k = ec.prepare_wait();
    ec.cancel_wait();
    ec.notify_one();
    const u32 k2 = ec.prepare_wait();
    ec.cancel_wait();
    require(k2 != k);
  }
  end_test_case();

  test_case("commit_wait returns immediately when the epoch already moved (no lost wakeup)");
  {
    // the whole point of the prepare/commit split: the key is taken BEFORE the condition is
    // re-checked, so a notify landing in between cannot be missed
    event_count ec;
    const u32 k = ec.prepare_wait();
    ec.notify_one();          // arrives after prepare, before commit
    ec.commit_wait(k);        // must not block
    require_true(true);
  }
  end_test_case();

  test_case("a parked waiter is released by notify_one");
  {
    event_count ec;
    atomic_token<bool> woke(false);
    atomic_token<bool> parked(false);

    auto t = solo::spawn<auto_thread<>>([&]() {
      const u32 k = ec.prepare_wait();
      parked.store(true, memory_order::release);
      ec.commit_wait(k);
      woke.store(true, memory_order::release);
    });

    while ( !parked.get(memory_order::acquire) ) micron::yield();
    micron::sleep_for(30);
    ec.notify_one();
    solo::join(t);
    require_true(woke.get(memory_order::acquire));
  }
  end_test_case();

  test_case("notify_all releases every parked waiter");
  {
    constexpr int kT = 8;
    event_count ec;
    atomic_token<int> woke(0);
    atomic_token<int> parked(0);
    lcheck::start_gate gate(kT);

    micron::__thread_pointer<micron::auto_thread<>> ts[kT];
    for ( int i = 0; i < kT; ++i )
      ts[i] = solo::spawn<auto_thread<>>([&]() {
        u32 sense = 0;
        gate.wait(sense);
        const u32 k = ec.prepare_wait();
        parked.fetch_add(1, memory_order::acq_rel);
        ec.commit_wait(k);
        woke.fetch_add(1, memory_order::acq_rel);
      });

    while ( parked.get(memory_order::acquire) != kT ) micron::yield();
    micron::sleep_for(40);
    ec.notify_all();
    for ( int i = 0; i < kT; ++i ) solo::join(ts[i]);
    require(woke.get(memory_order::acquire) == kT);
  }
  end_test_case();

  test_case("commit_wait with a timeout returns even without a notify");
  {
    event_count ec;
    micron::timespec_t t0{};
    micron::clock_gettime(micron::clock_monotonic, t0);

    const u32 k = ec.prepare_wait();
    micron::timespec_t to{ 0, 80 * 1000 * 1000 };      // 80 ms
    ec.commit_wait(k, &to);

    micron::timespec_t t1{};
    micron::clock_gettime(micron::clock_monotonic, t1);
    const u64 dt_ms = (static_cast<u64>(t1.tv_sec - t0.tv_sec) * 1000ull)
                      + static_cast<u64>((t1.tv_nsec - t0.tv_nsec) / 1000000);
    sb::print("     timed commit_wait returned after ~", static_cast<usize>(dt_ms), " ms (asked 80)");
    require_true(dt_ms >= 40ull);
    require_true(dt_ms < 4000ull);
  }
  end_test_case();

  // ---- the producer/consumer shape event_count exists for ----
  test_case("producer/consumer over a shared queue: nothing is lost, nobody sleeps forever");
  {
    event_count ec;
    atomic_token<u64> items(0);
    atomic_token<u64> taken(0);
    atomic_token<bool> done(false);
    constexpr u64 kItems = 20000;
    const int kConsumers = 4;

    mtest::parallel(kConsumers + 1, [&](int tid) {
      if ( tid == kConsumers ) {      // producer
        for ( u64 i = 0; i < kItems; ++i ) {
          items.fetch_add(1, memory_order::acq_rel);
          ec.notify_one();
        }
        done.store(true, memory_order::release);
        ec.notify_all();
        return;
      }
      for ( ;; ) {
        u64 n = items.get(memory_order::acquire);
        while ( n > 0 ) {
          if ( items.compare_exchange_weak(n, n - 1, memory_order::acq_rel, memory_order::acquire) ) {
            taken.fetch_add(1, memory_order::acq_rel);
            break;
          }
        }
        if ( n > 0 ) continue;
        if ( done.get(memory_order::acquire) and items.get(memory_order::acquire) == 0 ) return;

        const u32 k = ec.prepare_wait();
        if ( items.get(memory_order::acquire) != 0
             or (done.get(memory_order::acquire) and items.get(memory_order::acquire) == 0) ) {
          ec.cancel_wait();      // the re-check the prepare/commit split makes safe
          continue;
        }
        micron::timespec_t to{ 0, 20 * 1000 * 1000 };
        ec.commit_wait(k, &to);
      }
    });

    require(taken.get(memory_order::acquire) == kItems);
    require(items.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  test_case("non-copyable");
  {
    static_assert(!is_copy_constructible_v<event_count>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL EVENT_COUNT TESTS PASSED ===");
  return 1;
}
