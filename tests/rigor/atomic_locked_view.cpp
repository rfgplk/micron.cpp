//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// atomic<T>'s locked-view machinery was entirely unvalidated: the identifier __locked_view appears
// in no test in the tree, and neither does the get()/release() pair. atomic<T> is the fallback for
// a T no CPU can do atomically, so its internal spinlock IS the mutual exclusion -- and that
// spinlock had no pause in it at all until now.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/atomic/atomic.hpp"

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

// four words with an invariant, so a lost or interleaved update is visible
struct quad {
  u64 a = 0, b = 0, c = 0, d = 0;

  [[nodiscard]] bool
  consistent() const noexcept
  {
    return b == a * 2ull and c == a * 3ull and d == a * 4ull;
  }

  void
  bump() noexcept
  {
    ++a;
    b = a * 2ull;
    c = a * 3ull;
    d = a * 4ull;
  }
};

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== ATOMIC<T> LOCKED-VIEW TESTS ===");

  test_case("operator-> hands out a view that unlocks at the end of the full expression");
  {
    atomic<quad> q;
    q->bump();
    require(q->a == 1ull);
    q->bump();
    require(q->a == 2ull);
    require_true(q->consistent());
  }
  end_test_case();

  test_case("get() / release(): the manual form, and the lock is really held between them");
  {
    atomic<quad> q;
    quad *p = q.get();
    p->bump();
    p->bump();
    require(p->a == 2ull);
    q.release();
    require(q->a == 2ull);
  }
  end_test_case();

  test_case("a held view excludes another thread until release()");
  {
    atomic<quad> q;
    atomic_token<bool> other_in(false);
    atomic_token<bool> proceed(false);

    quad *p = q.get();      // held from here
    auto t = solo::spawn<auto_thread<>>([&]() {
      proceed.store(true, memory_order::release);
      q->bump();      // must block until release() below
      other_in.store(true, memory_order::release);
    });

    while ( !proceed.get(memory_order::acquire) ) micron::yield();
    micron::sleep_for(30);
    require_false(other_in.get(memory_order::acquire));      // still shut out
    p->bump();
    q.release();

    solo::join(t);
    require_true(other_in.get(memory_order::acquire));
    require(q->a == 2ull);
  }
  end_test_case();

  test_case("concurrent bumps through operator-> keep the invariant and lose nothing");
  {
    atomic<quad> q;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 4000;
    lcheck::start_gate gate(static_cast<u32>(kT));
    lcheck::watchdog wd;

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(10000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        q->bump();
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    require(q->a == static_cast<u64>(kT) * kIters);
    require_true(q->consistent());
  }
  end_test_case();

  test_case("get()/release() pairs interleave correctly with operator->");
  {
    atomic<quad> q;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000;
    lcheck::start_gate gate(static_cast<u32>(kT));

    mtest::parallel(kT, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        if ( (tid + i) % 2 == 0 ) {
          q->bump();
        } else {
          quad *p = q.get();
          p->bump();
          q.release();
        }
      }
    });

    require(q->a == static_cast<u64>(kT) * kIters);
    require_true(q->consistent());
  }
  end_test_case();

  test_case("the compound-assign operators go through the same lock");
  {
    atomic<u64> n;
    n.__store(0);
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 5000;
    lcheck::start_gate gate(static_cast<u32>(kT));

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) ++n;
    });

    require(n.__get() == static_cast<u64>(kT) * kIters);
  }
  end_test_case();

  sb::print("=== ALL LOCKED-VIEW TESTS PASSED ===");
  return 1;
}
