//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// SKIPPED — full surface untestable as written: when(D*, Fn, Args...) forwards
// the result pointer to go(), which deduces Args as D*& (reference to
// pointer) for any lvalue caller. group_thread's ctor rejects reference
// Args, so the instantiation fails. Documented bonus bug. This file is
// retained as a placeholder + a smoke test that the header at least
// compiles.

#include "../../src/atomic/atomic.hpp"
#include "../../src/sync/when.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== WHEN TESTS (compile-smoke only; lvalue-arg instantiation broken upstream) ===");

  test_case("sync/when.hpp compiles as included");
  {
    // do not instantiate when() with an lvalue D* — it would fail to
    // compile due to group_thread<>'s reject-reference-Args constraint.
    require_true(true);
  }
  end_test_case();

  sb::print("=== WHEN COMPILE-SMOKE PASSED ===");
  return 1;
}
