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

  test_case("auto_guard<mutex> locks the referenced mutex for its scope");
  {
    mutex m;
    {
      auto_guard<mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("auto_guard<mutex> sequential scopes lock/unlock and do not deadlock");
  {
    mutex m;
    {
      auto_guard<mutex> g1(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    {
      auto_guard<mutex> g2(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("auto_guard<spin_lock> locks the referenced spin_lock");
  {
    spin_lock s;
    {
      auto_guard<spin_lock> g(s);
      require_true(s.is_locked());
    }
    require_false(s.is_locked());
  }
  end_test_case();

  test_case("auto_guard<recursive_lock> locks (and nests on) the referenced lock");
  {
    recursive_lock r;
    {
      auto_guard<recursive_lock> g1(r);
      require_true(r.is_locked());
      {
        auto_guard<recursive_lock> g2(r);
        require_true(r.is_locked());
      }
      require_true(r.is_locked());
    }
    require_false(r.is_locked());
  }
  end_test_case();

  test_case("auto_guard<weak_mutex> locks the referenced weak_mutex");
  {
    weak_mutex w;
    {
      auto_guard<weak_mutex> g(w);
      require_true(w.is_locked());
    }
    require_false(w.is_locked());
  }
  end_test_case();

  test_case("auto_guard<TestMutex>: reset called once per dtor on the SAME instance");
  {
    TestMutex tm;
    TestMutex::reset_calls = 0;
    {
      auto_guard<TestMutex> g(tm);
      require_true(tm.is_locked());
    }
    require(TestMutex::reset_calls == 1);
    require_false(tm.is_locked());

    {
      auto_guard<TestMutex> g1(tm);
      {
        auto_guard<TestMutex> g2(tm);
      }
    }
    require(TestMutex::reset_calls == 3);
  }
  end_test_case();

  sb::print("=== ALL AUTO_GUARD TESTS PASSED ===");
  return 1;
}
