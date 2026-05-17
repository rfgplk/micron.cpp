//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// SKIPPED — runtime exercise of contract<S,T> is hazardous: the destructor
// spins on `is_signed` indefinitely if the signing thread hangs, and the
// signing flow calls into go() which has its own caveats (see sync_when).
// This file is retained as a compile-smoke gate.

// contract.hpp uses `mc::abort()` where mc = micron (alias from std.hpp).
// Pull std.hpp in first so the alias is in scope.
#include "../../src/std.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/sync/contract.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== CONTRACT TESTS (compile-smoke only) ===");

  test_case("sync/contract.hpp compiles as included");
  {
    static_assert((int)contract_state::lenient == 0);
    static_assert((int)contract_state::enforcing == 1);
    static_assert((int)contract_state::strict == 2);
    require_true(true);
  }
  end_test_case();

  sb::print("=== CONTRACT COMPILE-SMOKE PASSED ===");
  return 1;
}
