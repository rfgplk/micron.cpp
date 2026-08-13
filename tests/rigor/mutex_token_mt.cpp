//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// token<> is a one-shot gate: the FIRST caller gets true, everyone after gets false and dispatches
// the fallback. thread safety is the entire point of the type, and tests/rigor/mutex_token.cpp is
// single-threaded -- three cases, one thread, no race. this is the concurrent half.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/token.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

#include "../support/lockcheck.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

micron::atomic_token<u64> g_fallbacks{ 0 };

void
on_late(micron::mutex *m, void (micron::mutex::*pmf)())
{
  if ( m != nullptr and pmf != nullptr ) g_fallbacks.fetch_add(1, micron::memory_order::acq_rel);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== TOKEN (CONCURRENT) TESTS ===");

  test_case("N threads race one token: exactly one wins");
  {
    const int kT = static_cast<int>(lcheck::wide_threads);
    for ( int round = 0; round < 200; ++round ) {
      token<> tk;
      atomic_token<int> winners(0);
      lcheck::start_gate gate(static_cast<u32>(kT));

      mtest::parallel(kT, [&](int) {
        u32 sense = 0;
        gate.wait(sense);
        if ( tk() ) winners.fetch_add(1, memory_order::acq_rel);
      });

      require(winners.get(memory_order::acquire) == 1);
    }
  }
  end_test_case();

  test_case("every loser dispatches the fallback exactly once");
  {
    const int kT = static_cast<int>(lcheck::wide_threads);
    g_fallbacks.store(0, memory_order::release);
    token<void> tk(&on_late);
    atomic_token<int> winners(0);
    lcheck::start_gate gate(static_cast<u32>(kT));

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      if ( tk() ) winners.fetch_add(1, memory_order::acq_rel);
    });

    require(winners.get(memory_order::acquire) == 1);
    require(g_fallbacks.get(memory_order::acquire) == static_cast<u64>(kT) - 1ull);
  }
  end_test_case();

  test_case("repeated calls from the winning thread still return false after the first");
  {
    token<> tk;
    require_true(tk());
    for ( int i = 0; i < 100; ++i ) require_false(tk());
  }
  end_test_case();

  test_case("oversubscribed race still admits exactly one winner");
  {
    const int kT = static_cast<int>(lcheck::over_threads);
    token<> tk;
    atomic_token<int> winners(0);
    lcheck::start_gate gate(static_cast<u32>(kT));

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < 50; ++i )
        if ( tk() ) winners.fetch_add(1, memory_order::acq_rel);
    });

    require(winners.get(memory_order::acquire) == 1);
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<token<>>);
    static_assert(!is_move_constructible_v<token<>>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL TOKEN CONCURRENT TESTS PASSED ===");
  return 1;
}
