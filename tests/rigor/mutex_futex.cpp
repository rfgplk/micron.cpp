//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// futex_mutex is the only lock in the tree that stops spinning. the cases that matter are the ones
// no other lock can pass: 4x oversubscription (every spin lock livelocks the scheduler there), the
// lost-wakeup window between "publish that I am waiting" and FUTEX_WAIT, and timed acquisition.
//
// build with --def MICRON_LOCK_STATS to make the park counters real; without it stats() reports 0
// and the "did we actually park" assertions below are skipped rather than faked.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/locks/futex_mutex.hpp"

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
constexpr u64 SEED_FTX = 0xDEADBEEFCAFE01ULL;

#if defined(MICRON_LOCK_STATS)
constexpr bool stats_live = true;
#else
constexpr bool stats_live = false;
#endif

u64
now_ns(void) noexcept
{
  micron::timespec_t t{};
  micron::clock_gettime(micron::clock_monotonic, t);
  return static_cast<u64>(t.tv_sec) * 1000000000ull + static_cast<u64>(t.tv_nsec);
}

}      // namespace

static_assert(micron::is_mutex<micron::futex_mutex>, "futex_mutex must satisfy is_mutex");
static_assert(micron::is_same_v<micron::timed_mutex, micron::futex_mutex>, "timed_mutex is futex_mutex");

int
main(void)
{
  using namespace micron;
  sb::print("=== FUTEX_MUTEX TESTS ===  stats=", stats_live ? "on" : "off");

  test_case("default ctor unheld, uncontended");
  {
    futex_mutex m;
    require_false(m.is_locked());
    require_false(m.contended());
  }
  end_test_case();

  test_case("lock / unlock roundtrip stays out of the kernel");
  {
    futex_mutex m;
    m.lock();
    require_true(m.is_locked());
    require_false(m.contended());      // no waiter recorded itself, so unlock issues no FUTEX_WAKE
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("operator()(void) returns the fn-ptr; dispatch unlocks");
  {
    futex_mutex m;
    auto r = m();
    require_true(m.is_locked());
    (m.*r)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock succeeds when free, fails when held");
  {
    futex_mutex m;
    require_true(m.try_lock());
    require_false(m.try_lock());
    m.unlock();
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  // ---- the case every spin lock in the tree fails ----
  test_case("4x oversubscription: totals exact, exclusion clean, no livelock");
  {
    futex_mutex m;
    lcheck::exclusion_probe pr;
    u64 total = 0;
    const int kT = static_cast<int>(lcheck::over_threads);      // deliberately > cores
    constexpr int kIters = 120;

    // the hold has to outlast the spin ladder or nobody parks and this measures the fast path
    // only. at a 48-pause hold the same loop records ZERO parks across 19200 acquisitions -- the
    // lock/unlock pair costs more than the section, so arrivals barely overlap.
    constexpr u32 kHoldPauses = 4000;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        pr.enter();
        pr.dwell(kHoldPauses);
        ++total;
        pr.leave();
        m.unlock();
      }
    });

    require(total == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_false(m.is_locked());
    sb::print("     ", static_cast<usize>(kT), " threads x ", static_cast<usize>(kIters), "  parks=",
              static_cast<usize>(m.stats().parks()), " spins=", static_cast<usize>(m.stats().spins()));

    if constexpr ( stats_live ) {
      require(m.stats().acquires() == static_cast<u64>(kT) * kIters);
      require_true(m.stats().spins() > 0);
      // if nothing ever parked, this measured the spin path only and proves nothing about the futex
      require_true(m.stats().parks() > 0);
    }
  }
  end_test_case();

  test_case("no lost wakeup: a long hold parks every waiter and all are released");
  {
    futex_mutex m;
    const u32 kT = lcheck::wide_threads;
    atomic_token<u32> woke(0);
    atomic_token<bool> holding(false);

    mtest::parallel(static_cast<int>(kT) + 1, [&](int tid) {
      if ( tid == static_cast<int>(kT) ) {
        m.lock();
        holding.store(true, memory_order::release);
        micron::sleep_for(60);      // past every spin/yield tier: the waiters are asleep, not spinning
        m.unlock();
        return;
      }
      while ( !holding.get(memory_order::acquire) ) micron::yield();
      m.lock();
      woke.fetch_add(1, memory_order::acq_rel);
      m.unlock();
    });

    require(woke.get(memory_order::acquire) == kT);
    require_false(m.is_locked());
  }
  end_test_case();

  // ---- timed acquisition: the tree had none at all ----
  test_case("try_lock_for succeeds immediately when the lock is free");
  {
    futex_mutex m;
    const u64 t0 = now_ns();
    require_true(m.try_lock_for(500ull * 1000000ull));
    const u64 dt = now_ns() - t0;
    m.unlock();
    require_true(dt < 100ull * 1000000ull);      // must not have waited out the timeout
  }
  end_test_case();

  test_case("try_lock_for times out while another thread holds it, and waits ~the timeout");
  {
    futex_mutex m;
    atomic_token<bool> holding(false);
    atomic_token<bool> go(false);

    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      holding.store(true, memory_order::release);
      while ( !go.get(memory_order::acquire) ) micron::yield();
      m.unlock();
    });

    while ( !holding.get(memory_order::acquire) ) micron::yield();

    constexpr u64 kTimeoutNs = 120ull * 1000000ull;      // 120 ms
    const u64 t0 = now_ns();
    const bool got = m.try_lock_for(kTimeoutNs);
    const u64 dt = now_ns() - t0;

    go.store(true, memory_order::release);
    solo::join(t);

    require_false(got);
    sb::print("     timed out after ", static_cast<usize>(dt / 1000000ull), " ms (asked for 120)");
    // it must have actually blocked rather than returned early, and not massively overshot
    require_true(dt >= kTimeoutNs / 2);
    require_true(dt < kTimeoutNs * 8);
  }
  end_test_case();

  test_case("try_lock_for acquires once the holder releases inside the window");
  {
    futex_mutex m;
    atomic_token<bool> holding(false);

    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      holding.store(true, memory_order::release);
      micron::sleep_for(40);
      m.unlock();
    });

    while ( !holding.get(memory_order::acquire) ) micron::yield();
    require_true(m.try_lock_for(3000ull * 1000000ull));
    m.unlock();
    solo::join(t);
  }
  end_test_case();

  test_case("try_lock_until with a deadline already in the past fails without blocking");
  {
    futex_mutex m;
    atomic_token<bool> holding(false);
    atomic_token<bool> go(false);
    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      holding.store(true, memory_order::release);
      while ( !go.get(memory_order::acquire) ) micron::yield();
      m.unlock();
    });
    while ( !holding.get(memory_order::acquire) ) micron::yield();

    micron::timespec_t past{};
    micron::clock_gettime(micron::clock_monotonic, past);
    past.tv_sec -= 5;

    const u64 t0 = now_ns();
    require_false(m.try_lock_until(past));
    const u64 dt = now_ns() - t0;

    go.store(true, memory_order::release);
    solo::join(t);
    require_true(dt < 200ull * 1000000ull);
  }
  end_test_case();

  test_case("a timed-out waiter leaves the lock usable");
  {
    futex_mutex m;
    atomic_token<bool> holding(false);
    atomic_token<bool> go(false);
    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      holding.store(true, memory_order::release);
      while ( !go.get(memory_order::acquire) ) micron::yield();
      m.unlock();
    });
    while ( !holding.get(memory_order::acquire) ) micron::yield();
    require_false(m.try_lock_for(30ull * 1000000ull));
    go.store(true, memory_order::release);
    solo::join(t);

    require_false(m.is_locked());
    for ( int i = 0; i < 4; ++i ) {
      m.lock();
      m.unlock();
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("guards accept it, including a timed one through unique_lock");
  {
    futex_mutex m;
    {
      lock_guard<futex_mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      auto_guard<futex_mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      unique_lock<lock_starts::locked, futex_mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("randomized hold lengths under oversubscription");
  {
    futex_mutex m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::over_threads);
    constexpr int kIters = 400;
    u64 total = 0;

    mtest::parallel(kT, [&](int tid) {
      u64 s = SEED_FTX + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        pr.enter();
        pr.dwell(static_cast<u32>(lcheck::xs64(s) & 0xFF));
        ++total;
        pr.leave();
        m.unlock();
      }
    });

    require(total == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<futex_mutex>);
    static_assert(!is_move_constructible_v<futex_mutex>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL FUTEX_MUTEX TESTS PASSED ===");
  return 1;
}
