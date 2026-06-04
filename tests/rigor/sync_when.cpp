//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Exercises the sync/when.hpp fixes:
//   * when(atomic_token<bool>*, fn, args...) — condition is an ATOMIC flag polled with
//     acquire (was a non-atomic *result load racing a cross-thread writer), and fn/args are
//     captured BY VALUE into the worker closure (was a [&] capture of when()'s locals → UAF
//     the instant when() returned). when_any() similarly re-typed to a deducible signature
//     (callable first, trailing homogeneous flag pack) and value-captured.
//
// The UAF case is the third test: the argument handed to when() lives in an inner scope that
// is destroyed BEFORE the flag is raised. With value-capture the worker reads its own copy and
// the result is correct; with the old [&] capture it dereferenced freed stack memory.

#include "../../src/atomic/atomic.hpp"
#include "../../src/sync/when.hpp"

#include "../../src/std.hpp"

#include "../../src/sync/pause.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::require;
using sb::require_true;
using sb::test_case;

namespace
{

micron::atomic_token<int> g_sum(0);
micron::atomic_token<int> g_done(0);

void
add_worker(int x)
{
  g_sum.fetch_add(x, micron::memory_order::acq_rel);
  g_done.fetch_add(1, micron::memory_order::acq_rel);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== WHEN TESTS ===");

  test_case("when(flag, fn, arg): worker runs only after the atomic flag is raised");
  {
    g_sum.store(0, memory_order::seq_cst);
    g_done.store(0, memory_order::seq_cst);

    atomic_token<bool> flag(false);
    when(&flag, add_worker, 7);

    // give the worker a moment to be spinning; it must NOT have fired yet
    sleep_for(20);
    require(g_done.get(memory_order::acquire) == 0);

    flag.store(true, memory_order::release);
    while ( g_done.get(memory_order::acquire) < 1 ) yield();
    require(g_sum.get() == 7);
  }
  end_test_case();

  test_case("when_any(fn, &f1, &f2): one worker per flag, each fires when its flag is raised");
  {
    g_sum.store(0, memory_order::seq_cst);
    g_done.store(0, memory_order::seq_cst);

    atomic_token<bool> f1(false), f2(false);
    when_any([]() { add_worker(3); }, &f1, &f2);

    f1.store(true, memory_order::release);
    f2.store(true, memory_order::release);
    while ( g_done.get(memory_order::acquire) < 2 ) yield();
    require(g_sum.get() == 6);      // 3 + 3, both workers ran
  }
  end_test_case();

  test_case("when() value-captures args: worker is safe after the arg's scope dies (UAF fix)");
  {
    g_sum.store(0, memory_order::seq_cst);
    g_done.store(0, memory_order::seq_cst);

    atomic_token<bool> flag(false);

    {
      // a stack local that goes out of scope BEFORE the flag fires. The old [&] capture made the
      // worker read this freed slot; value-capture copies it into the closure.
      volatile int scoped_val = 0;
      scoped_val = 41;
      int arg = scoped_val;
      when(&flag, add_worker, arg);
      // arg / scoped_val die here, while the worker is still spinning on `flag`.
    }

    // clobber the just-freed stack region to surface any dangling read
    {
      volatile int clobber[64];
      for ( int i = 0; i < 64; ++i ) clobber[i] = 0xDEAD;
      (void)clobber[0];
    }

    flag.store(true, memory_order::release);
    while ( g_done.get(memory_order::acquire) < 1 ) yield();
    require(g_sum.get() == 41);      // captured copy, not garbage from the dead frame
  }
  end_test_case();

  sb::print("=== ALL WHEN TESTS PASSED ===");
  return 1;
}
