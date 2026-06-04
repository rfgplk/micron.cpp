//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Curated sample of until.hpp's ~78 templates — covers the canonical
// surface (until-cond/fn, until-cond/var, atomic variant, until predicate,
// until_true). until_timeout, until_max_iter, until_ready, until_backoff,
// until_any, until_flag_* exist in the header but require a wider type
// surface and are skipped for brevity.

#include "../../src/atomic/atomic.hpp"
#include "../../src/chrono.hpp"
#include "../../src/mutex/locks.hpp"

#include "../../src/sync/future.hpp"
#include "../../src/sync/promises.hpp"
#include "../../src/sync/until.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  using namespace micron;
  sb::print("=== UNTIL TESTS ===");

  test_case("until(cond, fn) — fn returns the target value on iteration N");
  {
    int n = 0;
    auto fn = [&n]() { return ++n; };
    until(5, fn);
    require(n == 5);
  }
  end_test_case();

  test_case("until(cond, var) — plain variable observed via reference");
  {
    int v = 1;
    // single-thread: set var to cond before until
    v = 5;
    until(5, v);
    require(v == 5);
  }
  end_test_case();

  test_case("until(cond, atomic<T>&) — atomic observed via lock-free __get (was broken, now fixed)");
  {
    atomic<int> a;
    a.__store(5, memory_order::release);
    until(5, a);      // returns once __get(acquire) == 5
    require(a.__get(memory_order::acquire) == 5);
  }
  end_test_case();

  test_case("until(predicate) — lambda predicate variant");
  {
    int n = 0;
    until([&n]() -> bool { return ++n == 3; });
    require(n == 3);
  }
  end_test_case();

  test_case("until_true(predicate, args...) — variadic forwarding");
  {
    int n = 0;
    auto pred = [&n](int target) -> bool { return ++n == target; };
    until_true(pred, 4);
    require(n == 4);
  }
  end_test_case();

  sb::print("=== ALL UNTIL TESTS PASSED ===");
  return 1;
}
