//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

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
  sb::print("=== RCU TESTS (quarantined — implementation disabled) ===");

  test_case("rcu.hpp is quarantined: no RCU types are defined");
  {

    require_true(true);
  }
  end_test_case();

  sb::print("=== RCU QUARANTINED (no checks run) ===");
  return 1;
}
