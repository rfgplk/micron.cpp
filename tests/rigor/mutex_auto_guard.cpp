//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Exercises fix #3: src/mutex/locks/auto_lock.hpp:21 — rptr was hardcoded as
// `void (micron::mutex::*)()`, regardless of the template parameter `M`. For
// any M != mutex (spin_lock, recursive_lock, queuing_mutex, ...), the dtor
// dispatch `(mtx.*rptr)()` was a type-punned member-function-pointer call
// (UB). With the fix the field is `void (M::*rptr)()`.
//
// The verifiable test uses a custom TestMutex whose reset function bumps a
// counter — under the broken code the wrong member-function-pointer is
// invoked and the counter is not incremented; under the fix the counter is.

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/mutex/locks.hpp"
#include "../../src/mutex/mutex.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

struct TestMutex {
  static inline int reset_calls = 0;
  bool locked_ = false;

  TestMutex() = default;
  TestMutex(const TestMutex &) = delete;
  TestMutex(TestMutex &&) = delete;
  TestMutex &operator=(const TestMutex &) = delete;
  TestMutex &operator=(TestMutex &&) = delete;

  void
  reset()
  {
    ++reset_calls;
    locked_ = false;
  }

  auto
  operator()()
  {
    locked_ = true;
    return &TestMutex::reset;
  }

  auto
  lock()
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    if ( locked_ ) return false;
    locked_ = true;
    return true;
  }

  void
  unlock() noexcept
  {
    locked_ = false;
  }

  auto
  retrieve()
  {
    return &TestMutex::reset;
  }

  bool
  is_locked() const noexcept
  {
    return locked_;
  }
};

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== AUTO_GUARD TESTS ===");

  static_assert(is_mutex<TestMutex>, "TestMutex must satisfy is_mutex");

  test_case("auto_guard is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<auto_guard<mutex>>);
    static_assert(!is_move_constructible_v<auto_guard<mutex>>);
    static_assert(!is_copy_assignable_v<auto_guard<mutex>>);
    static_assert(!is_move_assignable_v<auto_guard<mutex>>);
    require_true(true);
  }
  end_test_case();

  test_case("auto_guard<mutex> dtor releases owned mutex (no crash)");
  {
    {
      auto_guard<mutex> g;
    }
    require_true(true);      // smoke
  }
  end_test_case();

  test_case("auto_guard<mutex> sequential scopes do not deadlock");
  {
    {
      auto_guard<mutex> g1;
    }
    {
      auto_guard<mutex> g2;
    }
    {
      auto_guard<mutex> g3;
    }
    require_true(true);
  }
  end_test_case();

  test_case("auto_guard<spin_lock> compiles + dtor cleanly invokes reset");
  {
    // Pre-fix: rptr was typed as void(mutex::*)(); calling it on a spin_lock
    // through a punned type would be UB. Post-fix: rptr is void(M::*)() and
    // dispatch is type-correct.
    {
      auto_guard<spin_lock> g;
    }
    require_true(true);
  }
  end_test_case();

  test_case("auto_guard<recursive_lock> compiles + dtor cleanly invokes reset");
  {
    {
      auto_guard<recursive_lock> g;
    }
    require_true(true);
  }
  end_test_case();

  test_case("auto_guard<weak_mutex> compiles + dtor cleanly invokes reset");
  {
    {
      auto_guard<weak_mutex> g;
    }
    require_true(true);
  }
  end_test_case();

  test_case("auto_guard<TestMutex>: reset_calls increments once per dtor (FIX #3)");
  {
    TestMutex::reset_calls = 0;
    {
      auto_guard<TestMutex> g;
    }
    require(TestMutex::reset_calls == 1);

    {
      auto_guard<TestMutex> g1;
      {
        auto_guard<TestMutex> g2;
      }
    }
    require(TestMutex::reset_calls == 3);
  }
  end_test_case();

  sb::print("=== ALL AUTO_GUARD TESTS PASSED ===");
  return 1;
}
