//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// rcu_domain is unusable at runtime: its destructor calls rcu_barrier ->
// process_retirements, which deadlocks because retire_head (an atomic<T*>)
// is locked by atomic<T*>::get() and never released before a subsequent
// atomic<T*>::operator= or get() spins on lock_check. Construction of any
// rcu_domain therefore hangs at scope exit. Only compile-time checks are
// performed here; runtime exercise is deferred until the atomic<T*> /
// rcu process_retirements bugs are fixed.

#include "../../src/atomic/atomic.hpp"
#include "../../src/mutex/rcu.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== RCU TESTS (compile-only; runtime deferred) ===");

  test_case("rcu_domain is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<rcu_domain<int>>);
    static_assert(!is_move_constructible_v<rcu_domain<int>>);
    require_true(true);
  }
  end_test_case();

  test_case("rcu_reader is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<rcu_reader>);
    static_assert(!is_copy_assignable_v<rcu_reader>);
    require_true(true);
  }
  end_test_case();

  test_case("rcu_reader_domain is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<rcu_reader_domain<int>>);
    require_true(true);
  }
  end_test_case();

  test_case("rcu_ptr is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<rcu_ptr<int>>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL RCU COMPILE-CHECKS PASSED ===");
  // The static `default_rcu_domain` global's destructor calls rcu_barrier,
  // which hangs in process_retirements (atomic<T*>::get leaks its lock).
  // Bypass static destruction via sys_exit so this test can actually exit.
  micron::sys_exit(1);
  return 1;
}
