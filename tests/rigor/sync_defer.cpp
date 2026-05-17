//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/sync/defer.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

int
main(void)
{
  sb::print("=== DEFER TESTS ===");

  test_case("defer runs on scope exit");
  {
    int x = 0;
    {
      defer(x = 42);
      require(x == 0);
    }
    require(x == 42);
  }
  end_test_case();

  test_case("multiple defers run in reverse declaration order (FIFO unwind)");
  {
    int order[4] = { 0, 0, 0, 0 };
    int n = 0;
    {
      defer(order[n++] = 1);
      defer(order[n++] = 2);
      defer(order[n++] = 3);
    }
    // dtors run in reverse order of construction:
    // defer 3 runs first → order[0] = 3
    // defer 2 next      → order[1] = 2
    // defer 1 last      → order[2] = 1
    require(order[0] == 3);
    require(order[1] == 2);
    require(order[2] == 1);
  }
  end_test_case();

  test_case("defer fires on exception unwind");
  {
    int x = 0;
    try {
      defer(x = 99);
      throw 1;
    } catch ( ... ) {
      // defer ran during unwind
    }
    require(x == 99);
  }
  end_test_case();

  test_case("__defer_guard move ctor transfers ownership; source's dtor is inert");
  {
    int x = 0;
    {
      auto g1 = __make_defer([&]() noexcept { ++x; });
      auto g2 = static_cast<decltype(g1) &&>(g1);
      // g1 is now inert (active=false); g2 owns the action.
    }
    require(x == 1);
  }
  end_test_case();

  test_case("defer with no-op lambda runs without effect");
  {
    int x = 5;
    {
      defer(((void)0));
    }
    require(x == 5);
  }
  end_test_case();

  sb::print("=== ALL DEFER TESTS PASSED ===");
  return 1;
}
