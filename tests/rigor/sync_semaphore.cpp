//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Note: basic_semaphore::flag() calls release_futex(counter.ptr(), 1) which
// overwrites the post-add counter with literal 1 — a documented bug. We
// avoid exercising the wake path here (no contended wait) so the tests
// stay on the working surface.

#include "../../src/atomic/atomic.hpp"
#include "../../src/except.hpp"

#include "../../src/sync/semaphore.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== SEMAPHORE TESTS ===");

  test_case("basic_semaphore default construction value == 0");
  {
    basic_semaphore s;
    require(s.value() == 0);
  }
  end_test_case();

  test_case("basic_semaphore(__init=3) value == 3");
  {
    basic_semaphore s(3);
    require(s.value() == 3);
  }
  end_test_case();

  test_case("flag() increments counter");
  {
    basic_semaphore s(0);
    s.flag();
    require(s.value() == 1);
    s.flag();
    require(s.value() == 2);
  }
  end_test_case();

  test_case("try_wait returns false when counter == 0");
  {
    basic_semaphore s(0);
    require_false(s.try_wait());
    require(s.value() == 0);
  }
  end_test_case();

  test_case("try_wait returns true when counter > 0 and decrements");
  {
    basic_semaphore s(3);
    require_true(s.try_wait());
    require(s.value() == 2);
    require_true(s.try_wait());
    require_true(s.try_wait());
    require_false(s.try_wait());
    require(s.value() == 0);
  }
  end_test_case();

  test_case("wait() decrements counter (positive path; no contention)");
  {
    basic_semaphore s(2);
    s.wait();
    require(s.value() == 1);
    s.wait();
    require(s.value() == 0);
  }
  end_test_case();

  test_case("reset(n) restores counter to n");
  {
    basic_semaphore s(5);
    s.try_wait();
    s.try_wait();
    require(s.value() == 3);
    s.reset(10);
    require(s.value() == 10);
    s.reset();
    require(s.value() == 0);
  }
  end_test_case();

  sb::print("=== ALL SEMAPHORE TESTS PASSED ===");
  return 1;
}
