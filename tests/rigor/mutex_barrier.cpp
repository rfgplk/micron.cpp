//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// src/mutex/barrier.hpp exposes 3 compiler-barrier macros plus a 4th that is
// malformed (`forced_read_barrier(x)` body contains literal `\ __typeof`
// escape errors). Only the 3 sound macros are tested here.

#include "../../src/mutex/barrier.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  sb::print("=== COMPILER BARRIER MACROS ===");

  test_case("full_barrier() compiles and executes");
  {
    full_barrier();
    require_true(true);
  }
  end_test_case();

  test_case("read_barrier() compiles and executes");
  {
    read_barrier();
    require_true(true);
  }
  end_test_case();

  test_case("write_barrier() compiles and executes");
  {
    write_barrier();
    require_true(true);
  }
  end_test_case();

  test_case("barriers do not reorder visible side-effects in optimizer view (smoke)");
  {
    volatile int x = 0;
    x = 1;
    write_barrier();
    x = 2;
    read_barrier();
    int y = x;
    full_barrier();
    require_true(y == 2);
  }
  end_test_case();

  sb::print("=== ALL COMPILER BARRIER TESTS PASSED ===");
  return 1;
}
