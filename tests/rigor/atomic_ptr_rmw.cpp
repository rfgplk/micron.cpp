//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// REGRESSION: atomic_ptr's ++/--/+=/-= used to livelock the instant a second thread touched the
// same pointer.
//
//   P old = load(); P desired = old + n;
//   while ( !tk.compare_and_swap(old, desired) ) desired = old + n;
//
// atomic_token::compare_and_swap takes `old` BY VALUE, so a failed CAS never wrote the observed
// value back -- the retry recomputed an identical `desired` against an identical stale `old` and
// the loop could never converge. single-threaded the first CAS always succeeds, which is why
// atomicpointer.cpp (643 lines) never saw it.
//
// so this test must FAIL, not hang, against the old code: every contended case runs under a
// watchdog that fails through snowball if progress stops.

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
constexpr usize kArena = 1u << 17;      // covers the widest run below, plus the +/- cases around the midpoint
int g_arena[kArena];
}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== ATOMIC_PTR RMW TESTS ===");

  test_case("single-threaded pre/post increment and decrement");
  {
    atomic_ptr<int *> p(&g_arena[100]);
    require((++p) == &g_arena[101]);
    require(p.load() == &g_arena[101]);
    require((p++) == &g_arena[101]);
    require(p.load() == &g_arena[102]);
    require((--p) == &g_arena[101]);
    require((p--) == &g_arena[101]);
    require(p.load() == &g_arena[100]);
  }
  end_test_case();

  test_case("single-threaded += and -=");
  {
    atomic_ptr<int *> p(&g_arena[0]);
    p += 50;
    require(p.load() == &g_arena[50]);
    p -= 20;
    require(p.load() == &g_arena[30]);
    p += 0;
    require(p.load() == &g_arena[30]);
  }
  end_test_case();

  // ---- the regression proper ----
  test_case("contended ++ converges: N threads each advance it kIters times");
  {
    atomic_ptr<int *> p(&g_arena[0]);
    lcheck::watchdog wd;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 4000;
    require_true(static_cast<usize>(kT) * kIters < kArena);

    lcheck::start_gate gate(static_cast<u32>(kT));
    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(8000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        ++p;
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);

    require_true(wd.ok());      // the old code stalls here instead of finishing
    require(p.load() == &g_arena[static_cast<usize>(kT) * kIters]);
  }
  end_test_case();

  test_case("contended post-increment returns the pre-value and loses nothing");
  {
    atomic_ptr<int *> p(&g_arena[0]);
    lcheck::watchdog wd;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000;
    atomic_token<u64> seen_sum(0);
    lcheck::start_gate gate(static_cast<u32>(kT));

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(8000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      u64 local = 0;
      for ( int i = 0; i < kIters; ++i ) {
        int *before = p++;
        local += static_cast<u64>(before - &g_arena[0]);
        wd.bump();
      }
      seen_sum.fetch_add(local, memory_order::acq_rel);
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());

    const u64 n = static_cast<u64>(kT) * kIters;
    require(p.load() == &g_arena[n]);
    // every thread saw a distinct pre-value, so the sum is 0 + 1 + ... + (n-1)
    require(seen_sum.get(memory_order::acquire) == n * (n - 1ull) / 2ull);
  }
  end_test_case();

  test_case("contended += and -= cancel out exactly");
  {
    atomic_ptr<int *> p(&g_arena[kArena / 2]);
    lcheck::watchdog wd;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000;
    lcheck::start_gate gate(static_cast<u32>(kT));

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(8000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      const bool up = (tid % 2) == 0;
      for ( int i = 0; i < kIters; ++i ) {
        if ( up )
          p += 3;
        else
          p -= 3;
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    require(p.load() == &g_arena[kArena / 2]);      // equal numbers up and down
  }
  end_test_case();

  test_case("contended -- converges");
  {
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000;
    atomic_ptr<int *> p(&g_arena[static_cast<usize>(kT) * kIters]);
    lcheck::watchdog wd;
    lcheck::start_gate gate(static_cast<u32>(kT));

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(8000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        --p;
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    require(p.load() == &g_arena[0]);
  }
  end_test_case();

  test_case("mixed RMW forms on one pointer, oversubscribed");
  {
    atomic_ptr<int *> p(&g_arena[kArena / 2]);
    lcheck::watchdog wd;
    const int kT = static_cast<int>(lcheck::over_threads);
    constexpr int kIters = 800;
    lcheck::start_gate gate(static_cast<u32>(kT));

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(10000, static_cast<u32>(kT)); });

    mtest::parallel(kT, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) {
        switch ( tid % 4 ) {
        case 0:
          ++p;
          break;
        case 1:
          p++;
          break;
        case 2:
          --p;
          break;
        default:
          p--;
          break;
        }
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    // kT is a multiple of 4, so the four arms cancel exactly
    require((static_cast<u32>(kT) % 4u) == 0u);
    require(p.load() == &g_arena[kArena / 2]);
  }
  end_test_case();

  test_case("the surrounding atomic_ptr API is unchanged");
  {
    atomic_ptr<int *> p(&g_arena[10]);
    require(p.load() == &g_arena[10]);
    require((p + 5) == &g_arena[15]);
    require((p - 5) == &g_arena[5]);
    require((p - &g_arena[0]) == 10);
    require_true(static_cast<bool>(p));
    p.store(nullptr);
    require_false(static_cast<bool>(p));
    p = &g_arena[3];
    require(p.get() == &g_arena[3]);
    require(p.exchange(&g_arena[4]) == &g_arena[3]);
    int *expected = &g_arena[4];
    require_true(p.compare_exchange_strong(expected, &g_arena[5]));
    require(p.load() == &g_arena[5]);
  }
  end_test_case();

  sb::print("=== ALL ATOMIC_PTR RMW TESTS PASSED ===");
  return 1;
}
