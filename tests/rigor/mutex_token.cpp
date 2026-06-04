//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/flag.hpp"
#include "../../src/mutex/mutex.hpp"
#include "../../src/mutex/token.hpp"

#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

int g_cb_calls = 0;
micron::mutex *g_cb_mtx_seen = nullptr;

void
cb_fn(micron::mutex *m, void (micron::mutex::*)())
{
  ++g_cb_calls;
  g_cb_mtx_seen = m;
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== TOKEN TESTS ===");

  test_case("token<>(null fptr) first call returns true, second returns false");
  {
    token<> t;
    require_true(t());
    require_false(t());
    require_false(t());
  }
  end_test_case();

  test_case("token<>(cb) first call returns true; second invokes callback and returns false");
  {
    g_cb_calls = 0;
    g_cb_mtx_seen = nullptr;
    token<> t(cb_fn);
    require_true(t());
    require(g_cb_calls == 0);
    require_false(t());
    require(g_cb_calls == 1);
    require_true(g_cb_mtx_seen != nullptr);
    require_false(t());
    require(g_cb_calls == 2);
  }
  end_test_case();

  test_case("token is non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<token<>>);
    static_assert(!is_move_constructible_v<token<>>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL TOKEN TESTS PASSED ===");
  return 1;
}
