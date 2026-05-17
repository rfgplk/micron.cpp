//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/chrono.hpp"
#include "../../src/sync/pause.hpp"
#include "../../src/sync/yield.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::check;
using sb::end_test_case;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== YIELD / PAUSE / SLEEP TESTS ===");

  test_case("yield() returns control without crash");
  {
    for ( int i = 0; i < 1000; ++i ) yield();
    require_true(true);
  }
  end_test_case();

  test_case("cpu_pause<1000>() returns (smoke)");
  {
    cpu_pause<1000>();      // ~1us
    require_true(true);
  }
  end_test_case();

  test_case("sleep_for(10) sleeps for at least 5ms (loose)");
  {
    auto t0 = system_clock<>::now();
    sleep_for(10);
    auto t1 = system_clock<>::now();
    auto dt = t1 - t0;
    // loose: at least 5ms — clock granularity tolerance
    check(dt >= (fduration_t)5);
  }
  end_test_case();

  test_case("sleep_nano(1_000_000) returns (smoke)");
  {
    sleep_nano(1000000);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL YIELD/PAUSE/SLEEP TESTS PASSED ===");
  return 1;
}
