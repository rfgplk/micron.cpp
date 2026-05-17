//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/atomic/atomic.hpp"
#include "../../src/except.hpp"

#include "../../src/sync/expect.hpp"
#include "../../src/sync/invoke.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

struct ETArgs {
  micron::atomic_token<int> *tk;
};

void
et_writer(ETArgs *p)
{
  p->tk->store(99, micron::memory_order::release);
}

int
free_fn_double(int x)
{
  return x * 2;
}

struct Obj {
  int v = 10;

  int
  member(int n) const
  {
    return v + n;
  }
};

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== EXPECT + INVOKE TESTS ===");

  // ── expect ──────────────────────────────────────────────────────────────

  test_case("expect(atomic_token<int>, val): spins until value matches");
  {
    atomic_token<int> tk(0);
    ETArgs a{ &tk };
    {
      auto_thread<> writer(et_writer, &a);
      expect<memory_order::acquire>(tk, 99);
    }
    require(tk.get() == 99);
  }
  end_test_case();

  test_case("expect(token, otherToken): cross-spin");
  {
    atomic_token<int> tk1(7);
    atomic_token<int> tk2(7);
    expect<memory_order::acquire>(tk1, tk2);
    require_true(true);
  }
  end_test_case();

  test_case("expect(t, f) value equality returns bool");
  {
    bool eq = expect<int, int>(5, 5);
    bool neq = expect<int, int>(5, 6);
    require_true(eq);
    require_false(neq);
  }
  end_test_case();

  // ── invoke ──────────────────────────────────────────────────────────────

  test_case("invoke(free_fn, args...) calls correctly");
  {
    int r = invoke(free_fn_double, 21);
    require(r == 42);
  }
  end_test_case();

  test_case("invoke(member_fn_ptr, obj, args)");
  {
    Obj o;
    int r = invoke(&Obj::member, o, 5);
    require(r == 15);
  }
  end_test_case();

  test_case("invoke_r<int>(fn) returns the typed result");
  {
    int r = invoke_r<int>(free_fn_double, 4);
    require(r == 8);
  }
  end_test_case();

  test_case("not_fn(pred)() returns negation");
  {
    auto is_zero = [](int x) -> bool { return x == 0; };
    auto is_not_zero = not_fn(is_zero);
    require_true(is_not_zero(7));
    require_false(is_not_zero(0));
  }
  end_test_case();

  sb::print("=== ALL EXPECT + INVOKE TESTS PASSED ===");
  return 1;
}
