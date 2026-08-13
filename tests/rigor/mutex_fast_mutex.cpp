//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// fast_mutex had NO direct test anywhere in the tree. it appeared only in compiletests/threads.cpp
// (built, never run) and incidentally inside conarray/conqueue, which grade their own containers
// rather than the lock. it is also the only one of the three mutex.hpp spin locks with matched
// acquire/release ordering, so it is the one whose ordering is worth pinning.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/mutex.hpp"

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
constexpr u64 SEED_FM = 0xFA57BEEF17ULL;

struct payload {
  u64 a;
  u64 b;      // invariant under the lock: b == a * 2 + 1
};
}      // namespace

static_assert(micron::is_mutex<micron::fast_mutex>, "fast_mutex must satisfy is_mutex");

int
main(void)
{
  using namespace micron;
  sb::print("=== FAST_MUTEX TESTS ===");

  test_case("default ctor unheld");
  {
    fast_mutex m;
    require_false(m.is_locked());
    require_true(!m);
  }
  end_test_case();

  test_case("lock / unlock roundtrip");
  {
    fast_mutex m;
    m.lock();
    require_true(m.is_locked());
    require_false(!m);
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("operator()(void) and lock() both return the reset fn-ptr");
  {
    fast_mutex m;
    auto r1 = m();
    require_true(m.is_locked());
    (m.*r1)();
    require_false(m.is_locked());

    auto r2 = m.lock();
    require_true(m.is_locked());
    (m.*r2)();
    require_false(m.is_locked());

    auto r3 = m.retrieve();
    m.lock();
    (m.*r3)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock succeeds when free, fails when held");
  {
    fast_mutex m;
    require_true(m.try_lock());
    require_false(m.try_lock());
    m.unlock();
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("held state is visible from another thread");
  {
    fast_mutex m;
    atomic_token<bool> held(false);
    atomic_token<bool> go(false);
    auto t = solo::spawn<auto_thread<>>([&]() {
      m.lock();
      held.store(true, memory_order::release);
      while ( !go.get(memory_order::acquire) ) micron::yield();
      m.unlock();
    });
    while ( !held.get(memory_order::acquire) ) micron::yield();
    require_true(m.is_locked());
    require_false(m.try_lock());
    go.store(true, memory_order::release);
    solo::join(t);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("no two threads inside the critical section at once");
  {
    fast_mutex m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 4000;

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < kIters; ++i ) {
        lcheck::guarded_section<fast_mutex> g(m, pr, 8);
      }
    });

    require(pr.entries.get(memory_order::acquire) == static_cast<u64>(kT) * kIters);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
  }
  end_test_case();

  // ---- acquire/release ordering: fast_mutex is the one that has it right ----
  test_case("release ordering: a two-word invariant published under it is never seen half-written");
  {
    // the reader only ever looks while HOLDING, so a broken release would show up as b != a*2+1
    fast_mutex m;
    payload p{ 0, 1 };
    atomic_token<u64> torn(0);
    atomic_token<bool> stop(false);
    const int kT = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kT, [&](int tid) {
      if ( tid == 0 ) {
        for ( u64 i = 1; i <= 200000ull; ++i ) {
          m.lock();
          p.a = i;
          p.b = i * 2ull + 1ull;
          m.unlock();
        }
        stop.store(true, memory_order::release);
        return;
      }
      while ( !stop.get(memory_order::acquire) ) {
        m.lock();
        if ( p.b != p.a * 2ull + 1ull ) torn.fetch_add(1, memory_order::acq_rel);
        m.unlock();
      }
    });

    require(torn.get(memory_order::acquire) == 0ull);
    require(p.b == p.a * 2ull + 1ull);
  }
  end_test_case();

  test_case("counter total is exact under oversubscription");
  {
    fast_mutex m;
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
  }
  end_test_case();

  test_case("randomized hold lengths, fixed seed");
  {
    fast_mutex m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    mtest::parallel(kT, [&](int tid) {
      u64 s = SEED_FM + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < 1500; ++i ) {
        m.lock();
        pr.enter();
        pr.dwell(static_cast<u32>(lcheck::xs64(s) & 0x3F));
        pr.leave();
        m.unlock();
      }
    });
    require(pr.violations.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  test_case("guards accept it");
  {
    fast_mutex m;
    {
      lock_guard<fast_mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      auto_guard<fast_mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      unique_lock<lock_starts::defer, fast_mutex> g(m);
      require_false(m.is_locked());
      g.lock();
      require_true(m.is_locked());
      g.unlock();
      require_false(m.is_locked());
    }
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<fast_mutex>);
    static_assert(!is_move_constructible_v<fast_mutex>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL FAST_MUTEX TESTS PASSED ===");
  return 1;
}
