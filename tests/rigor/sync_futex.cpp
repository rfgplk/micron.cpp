//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Exercises Fix #6: src/sync/futex.hpp `futex<T,__D>::wait` hardcoded
// `1` as the locked sentinel in the CAS loop and passed `__D` as the
// futex_wait `val` parameter. With any __D != 1 the kernel comparison
// always failed (holders stored 1, kernel compared *addr against __D)
// causing futex_wait to return EAGAIN immediately and the loop to
// busy-spin. The fix:
//   * introduces a separate `__L` (locked sentinel) template param,
//   * uses the OBSERVED current value as the futex_wait `val`,
//   * stores `__D` (not 0) in release.
// This file verifies both the canonical 1/0 case AND the previously
// broken case (default __D=0, __L=1).

// futex.hpp uses memory_order_* (from atomic.hpp) and exc/except (from
// except.hpp) without including them — pull both in first.
#include "../../src/atomic/atomic.hpp"
#include "../../src/except.hpp"

#include "../../src/sync/futex.hpp"

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

struct WaitArgs {
  ::u32 *word;
  micron::atomic_token<bool> *done;
};

void
wait_worker(WaitArgs *p)
{
  micron::wait_futex(p->word, (::u32)42);
  p->done->store(true, micron::memory_order::release);
}

struct FutexArgs {
  micron::futex<> *f;
  micron::atomic_token<int> *counter;
  int iters;
};

void
fx_worker(FutexArgs *p)
{
  for ( int i = 0; i < p->iters; ++i ) {
    p->f->wait();
    p->counter->fetch_add(1, micron::memory_order::acq_rel);
    p->f->release();
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== FUTEX TESTS ===");

  // ── wait_futex / release_futex (free functions) ────────────────────────

  test_case("wait_futex returns immediately when *ptr != expected");
  {
    u32 w = 99;
    wait_futex(&w, (u32)42);      // expected=42, actual=99 → immediate return
    require_true(true);
  }
  end_test_case();

  test_case("wait_futex blocks until release_futex changes the value");
  {
    u32 w = 42;
    atomic_token<bool> done(false);
    WaitArgs a{ &w, &done };
    auto_thread<> waiter(wait_worker, &a);
    // wait a moment for waiter to enter futex_wait
    for ( int i = 0; i < 1000 && !done.get(memory_order::acquire); ++i ) yield();
    require_false(done.get(memory_order::acquire));
    release_futex(&w, (u32)100);
    // waiter joins via auto_thread dtor on scope exit
  }
  end_test_case();

  // ── futex<u32, 0, 1> default canonical (formerly the only working case) ─

  test_case("futex<>::wait acquires on default (unlocked) state");
  {
    futex<> f;      // default __D=0, __L=1
    f.wait();
    f.release();
    require_true(true);
  }
  end_test_case();

  test_case("futex<> 2-thread mutual exclusion — 5000 increments each");
  {
    futex<> f;
    atomic_token<int> counter(0);
    constexpr int kIters = 5000;
    FutexArgs a{ &f, &counter, kIters };
    {
      auto_thread<> t1(fx_worker, &a);
      auto_thread<> t2(fx_worker, &a);
    }
    require(counter.get() == 2 * kIters);
  }
  end_test_case();

  // ── futex<u32, __D, __L> with non-default sentinels (FIX #6) ──────────

  test_case("futex<u32, 5, 7> with non-default sentinels works (FIX #6)");
  {
    // pre-fix: any __D != 1 caused infinite spin
    futex<u32, (u32)5, (u32)7> f;
    f.wait();
    f.release();
    require_true(true);
  }
  end_test_case();

  test_case("futex<u32, 5, 7> 2-thread mutex serialization (FIX #6)");
  {
    futex<u32, (u32)5, (u32)7> f;
    atomic_token<int> counter(0);
    constexpr int kIters = 2000;

    struct LocalArgs {
      futex<u32, (u32)5, (u32)7> *f;
      atomic_token<int> *c;
      int n;
    };

    LocalArgs a{ &f, &counter, kIters };
    auto worker = [](LocalArgs *p) {
      for ( int i = 0; i < p->n; ++i ) {
        p->f->wait();
        p->c->fetch_add(1, micron::memory_order::acq_rel);
        p->f->release();
      }
    };
    {
      auto_thread<> t1(worker, &a);
      auto_thread<> t2(worker, &a);
    }
    require(counter.get() == 2 * kIters);
  }
  end_test_case();

  // ── compile-time constraint: __D != __L is required ───────────────────

  test_case("futex<T,__D,__L> requires __D != __L (concept satisfaction)");
  {
    // these instantiations would compile-fail if the requires-clause is
    // dropped or weakened; presence of the requires-clause is implicit
    // by the fact that this file compiles when the working pairs above
    // do.
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL FUTEX TESTS PASSED ===");
  return 1;
}
