//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/mutex/mutex.hpp"
#include "../../src/std.hpp"

#include "../../src/concepts.hpp"

#include "../snowball/snowball.hpp"

#include <thread>
#include <vector>

using sb::check;
using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== MUTEX + WEAK_MUTEX TESTS ===");

  // ── concept satisfaction ────────────────────────────────────────────────

  static_assert(is_mutex<mutex>, "micron::mutex must satisfy is_mutex");
  static_assert(is_mutex<weak_mutex>, "micron::weak_mutex must satisfy is_mutex");

  // ── mutex ───────────────────────────────────────────────────────────────

  test_case("mutex default ctor is unlocked");
  {
    mutex m;
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("mutex lock() returns fn-ptr; dispatch unlocks");
  {
    mutex m;
    auto rptr = m.lock();
    require_true(m.is_locked());
    (m.*rptr)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("mutex operator()() locks identically to lock()");
  {
    mutex m;
    auto rptr = m();
    require_true(m.is_locked());
    (m.*rptr)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("mutex try_lock succeeds when free, fails when held");
  {
    mutex m;
    require_true(m.try_lock());
    require_false(m.try_lock());
    m.unlock();
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("mutex unlock() noexcept and idempotent on already-unlocked");
  {
    mutex m;
    m.unlock();
    require_false(m.is_locked());
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("mutex retrieve() returns reset fn-ptr without locking");
  {
    mutex m;
    auto rptr = m.retrieve();
    require_false(m.is_locked());
    // calling rptr on unlocked mutex still safe (just stores OPEN again)
    (m.*rptr)();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("mutex operator!() reports unlocked state");
  {
    mutex m;
    require_true(!m);
    m.try_lock();
    require_false(!m);
    m.unlock();
    require_true(!m);
  }
  end_test_case();

  test_case("mutex 4-thread contention: counter == 4 * iters");
  {
    mutex m;
    int counter = 0;
    constexpr int kT = 4, kIters = 10000;
    std::vector<std::thread> th;
    for ( int i = 0; i < kT; ++i ) {
      th.emplace_back([&]() {
        for ( int j = 0; j < kIters; ++j ) {
          auto rptr = m.lock();
          ++counter;
          (m.*rptr)();
        }
      });
    }
    for ( auto &t : th ) t.join();
    require(counter == kT * kIters);
  }
  end_test_case();

  // ── weak_mutex ──────────────────────────────────────────────────────────

  test_case("weak_mutex default ctor is unlocked");
  {
    weak_mutex m;
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("weak_mutex lock/try_lock/unlock surface mirrors mutex");
  {
    weak_mutex m;
    auto rptr = m.lock();
    require_true(m.is_locked());
    require_false(m.try_lock());
    (m.*rptr)();
    require_false(m.is_locked());
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("weak_mutex 4-thread contention");
  {
    weak_mutex m;
    int counter = 0;
    constexpr int kT = 4, kIters = 10000;
    std::vector<std::thread> th;
    for ( int i = 0; i < kT; ++i ) {
      th.emplace_back([&]() {
        for ( int j = 0; j < kIters; ++j ) {
          auto rptr = m.lock();
          ++counter;
          (m.*rptr)();
        }
      });
    }
    for ( auto &t : th ) t.join();
    require(counter == kT * kIters);
  }
  end_test_case();

  sb::print("=== ALL MUTEX TESTS PASSED ===");
  return 1;
}
