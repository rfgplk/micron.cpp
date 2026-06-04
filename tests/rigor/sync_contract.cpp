//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/std.hpp"

#include "../../src/atomic/atomic.hpp"
#include "../../src/sync/contract.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using namespace micron;

int
main(void)
{
  sb::print("=== CONTRACT TESTS ===");

  test_case("contract_state enum values");
  {
    static_assert((int)contract_state::lenient == 0);
    static_assert((int)contract_state::enforcing == 1);
    static_assert((int)contract_state::strict == 2);
    require_true(true);
  }
  end_test_case();

  test_case("lenient: satisfied condition signs, completes, no violation");
  {
    atomic_token<bool> ready{ false };
    {
      contract<contract_state::lenient, bool> c([&ready] { return ready.get(memory_order_acquire); });
      c.duration(100.0);
      ready.store(true, memory_order_release);
      c.sign();
      c.wait();
      require_false(c.violated());
    }
    require_true(true);
  }
  end_test_case();

  test_case("lenient: unmet condition expires deadline -> violated(), no throw");
  {
    atomic_token<bool> never{ false };
    contract<contract_state::lenient, bool> c([&never] { return never.get(memory_order_acquire); });
    c.duration(40.0);
    c.sign();
    c.wait();
    require_true(c.violated());
  }
  end_test_case();

  test_case("in-flight double sign() throws future_error");
  {

    atomic_token<bool> ready{ false };
    contract<contract_state::lenient, bool> c([&ready] { return ready.get(memory_order_acquire); });
    c.duration(2000.0);
    c.sign();
    bool threw = false;
    try {
      c.sign();
    } catch ( except::future_error & ) {
      threw = true;
    }
    ready.store(true, memory_order_release);
    c.wait();
    require_true(threw);
  }
  end_test_case();

  test_case("require(): all tokens true -> satisfied");
  {
    atomic_token<bool> r1{ true };
    atomic_token<bool> r2{ true };
    atomic_token<bool> cond{ true };
    {
      contract<contract_state::lenient, bool> c([&cond] { return cond.get(memory_order_acquire); });
      c.duration(60.0);
      c.require(r1, r2);
      c.sign();
      c.wait();
      require_false(c.violated());
    }
    require_true(true);
  }
  end_test_case();

  test_case("require(): a false token -> violated");
  {
    atomic_token<bool> r1{ true };
    atomic_token<bool> r2{ false };
    atomic_token<bool> cond{ true };
    contract<contract_state::lenient, bool> c([&cond] { return cond.get(memory_order_acquire); });
    c.duration(40.0);
    c.require(r1, r2);
    c.sign();
    c.wait();
    require_true(c.violated());
  }
  end_test_case();

  test_case("enforcing: breach invokes enforcing callback (non-fatal)");
  {
    atomic_token<bool> never{ false };
    atomic_token<bool> callback_ran{ false };
    contract<contract_state::enforcing, bool> c([&never] { return never.get(memory_order_acquire); });
    c.duration(40.0);

    c.enforce([&callback_ran] {
      callback_ran.store(true, memory_order_release);
      return false;
    });
    c.sign();
    c.wait();
    require_true(c.violated());
    require_true(callback_ran.get(memory_order_acquire));
  }
  end_test_case();

  test_case("enforcing: satisfied condition does not invoke callback");
  {
    atomic_token<bool> ready{ true };
    atomic_token<bool> callback_ran{ false };
    {
      contract<contract_state::enforcing, bool> c([&ready] { return ready.get(memory_order_acquire); });
      c.duration(40.0);
      c.enforce([&callback_ran] {
        callback_ran.store(true, memory_order_release);
        return false;
      });
      c.sign();
      c.wait();
      require_false(c.violated());
    }
    require_false(callback_ran.get(memory_order_acquire));
  }
  end_test_case();

  test_case("strict: breach throws future_error from wait()");
  {
    atomic_token<bool> never{ false };
    bool threw = false;

    contract<contract_state::strict, bool> c([&never] { return never.get(memory_order_acquire); });
    c.duration(40.0);
    c.sign();
    try {
      c.wait();
    } catch ( except::future_error & ) {
      threw = true;
    }
    never.store(true, memory_order_release);
    require_true(threw);
  }
  end_test_case();

  test_case("condition flips true mid-wait -> satisfied");
  {
    atomic_token<bool> ready{ false };
    contract<contract_state::lenient, bool> c([&ready] { return ready.get(memory_order_acquire); });
    c.duration(500.0);
    c.sign();
    sleep_duration(30.0);
    ready.store(true, memory_order_release);
    c.wait();
    require_false(c.violated());
  }
  end_test_case();

  sb::print("=== ALL CONTRACT TESTS PASSED ===");
  return 1;
}
