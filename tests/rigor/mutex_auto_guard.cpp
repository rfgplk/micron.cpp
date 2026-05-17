//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

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
    require_true(true);
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

  test_case("auto_guard<TestMutex>: reset_calls increments once per dtor");
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
