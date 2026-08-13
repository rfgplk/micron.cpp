//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// ttas_lock is spin_lock's algorithm with the backoff ladder attached. the assertions worth having
// are the ones about the LADDER: that a contended acquire really escalated (a contention case that
// never contended grades PASS and proves nothing), and that spin_only never escalates while
// spin_yield does.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)
#define MICRON_LOCK_STATS 1      // the whole point here is to read the escalation counters

#include "../../src/mutex/locks/ttas_lock.hpp"

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
constexpr u64 SEED_TTAS = 0xA5A5A5A50F0FULL;
constexpr u32 kHoldPauses = 4000;      // must outlast the ladder, or nothing escalates
}

static_assert(micron::is_mutex<micron::ttas_lock>, "ttas_lock must satisfy is_mutex");
static_assert(micron::is_mutex<micron::ttas_spin_lock>, "ttas_spin_lock must satisfy is_mutex");

int
main(void)
{
  using namespace micron;
  sb::print("=== TTAS_LOCK TESTS ===");

  test_case("default ctor unheld; explicit ctor can start held");
  {
    ttas_lock a;
    require_false(a.is_locked());
    ttas_lock b(true);
    require_true(b.is_locked());
    b.unlock();
    require_false(b.is_locked());
  }
  end_test_case();

  test_case("lock / unlock roundtrip and fn-ptr dispatch");
  {
    ttas_lock m;
    auto r = m.lock();
    require_true(m.is_locked());
    (m.*r)();
    require_false(m.is_locked());

    auto r2 = m();
    require_true(m.is_locked());
    (m.*r2)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock and operator!");
  {
    ttas_lock m;
    require_true(!m);      // operator! is "not locked"
    require_true(m.try_lock());
    require_false(m.try_lock());
    require_false(!m);
    m.unlock();
    require_true(!m);
  }
  end_test_case();

  test_case("uncontended acquire takes the fast path: one CAS, no backoff round");
  {
    ttas_lock m;
    for ( int i = 0; i < 100; ++i ) {
      m.lock();
      m.unlock();
    }
    require(m.stats().acquires() == 100ull);
    require(m.stats().spins() == 0ull);
    require(m.stats().yields() == 0ull);
  }
  end_test_case();

  // ---- the ladder ----
  test_case("contended acquire escalates: spins > 0, and yields once the hold is long");
  {
    ttas_lock m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 120;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        pr.enter();
        pr.dwell(kHoldPauses);
        pr.leave();
        m.unlock();
      }
    });

    sb::print("     spins=", static_cast<usize>(m.stats().spins()), " yields=", static_cast<usize>(m.stats().yields()),
              " acquires=", static_cast<usize>(m.stats().acquires()));

    require(m.stats().acquires() == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(m.stats().spins() > 0);       // it really did contend
    require_true(m.stats().yields() > 0);      // and the hold outlasted the pause tier
  }
  end_test_case();

  test_case("spin_only never yields; spin_yield does, on the same workload");
  {
    ttas_spin_lock so;
    ttas_lock sy;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 100;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        so.lock();
        for ( u32 p = 0; p < kHoldPauses; ++p ) __cpu_pause();
        so.unlock();
      }
    });
    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        sy.lock();
        for ( u32 p = 0; p < kHoldPauses; ++p ) __cpu_pause();
        sy.unlock();
      }
    });

    sb::print("     spin_only  yields=", static_cast<usize>(so.stats().yields()), " spins=", static_cast<usize>(so.stats().spins()));
    sb::print("     spin_yield yields=", static_cast<usize>(sy.stats().yields()), " spins=", static_cast<usize>(sy.stats().spins()));

    require_true(so.stats().spins() > 0);
    require(so.stats().yields() == 0ull);      // spin_only stays in userspace by construction
    require_true(sy.stats().yields() > 0);
  }
  end_test_case();

  test_case("no two threads inside the critical section at once");
  {
    ttas_lock m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 4000;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        lcheck::guarded_section<ttas_lock> g(m, pr, 8);
      }
    });

    require(pr.entries.get(memory_order::acquire) == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
  }
  end_test_case();

  test_case("counter total is exact under oversubscription");
  {
    ttas_lock m;
    u64 total = 0;
    const int kT = static_cast<int>(lcheck::over_threads);
    constexpr int kIters = 500;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        ++total;
        m.unlock();
      }
    });
    require(total == static_cast<u64>(kT) * kIters);
  }
  end_test_case();

  test_case("randomized hold lengths keep exclusion exact");
  {
    ttas_lock m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 1500;

    mtest::parallel(kT, [&](int tid) {
      u64 s = SEED_TTAS + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < kIters; ++i ) {
        m.lock();
        pr.enter();
        pr.dwell(static_cast<u32>(lcheck::xs64(s) & 0x7F));
        pr.leave();
        m.unlock();
      }
    });
    require(pr.violations.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  test_case("guards accept it");
  {
    ttas_lock m;
    {
      lock_guard<ttas_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      auto_guard<ttas_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      unique_lock<lock_starts::locked, ttas_lock> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<ttas_lock>);
    static_assert(!is_move_constructible_v<ttas_lock>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL TTAS_LOCK TESTS PASSED ===");
  return 1;
}
