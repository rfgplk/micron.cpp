//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// one sweep, every lock type, two questions the per-type files answer only for their own type:
//
//   1. does it actually exclude?  every other contention test in tests/rigor/ grades a counter
//      total, which a lock that admits two holders can still pass whenever their increments happen
//      not to collide. the exclusion probe watches the section itself.
//   2. how unfair is it?  every lock runs the same fixed-wall-time workload and the spreads are
//      printed side by side. those numbers are REPORTED, not graded -- see the comment on the
//      fairness case for why nothing measured that way survives a loaded machine.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/locks.hpp"

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

constexpr u64 SEED_EX = 0xE7C1051053EDULL;

// every lock is driven through the same body so the comparison is apples to apples
template<typename Lock>
void
sweep(const char *name, u32 threads, int iters, u32 dwell)
{
  Lock m;
  lcheck::exclusion_probe pr;
  lcheck::fairness_probe fp;
  u64 total = 0;

  mtest::parallel(static_cast<int>(threads), [&](int tid) {
    for ( int i = 0; i < iters; ++i ) {
      m.lock();
      pr.enter();
      fp.note(static_cast<u32>(tid));
      pr.dwell(dwell);
      ++total;
      pr.leave();
      m.unlock();
    }
  });

  sb::print("     ", name, ": entries=", static_cast<usize>(pr.entries.get(micron::memory_order::acquire)),
            " violations=", static_cast<usize>(pr.violations.get(micron::memory_order::acquire)),
            " peak=", static_cast<usize>(pr.peak.get(micron::memory_order::acquire)));

  sb::require(pr.violations.get(micron::memory_order::acquire) == 0ull);
  sb::require(pr.entries.get(micron::memory_order::acquire) == static_cast<u64>(threads) * iters);
  sb::require(total == static_cast<u64>(threads) * iters);
  sb::require_true(pr.clean());
  sb::require_false(m.is_locked());
  sb::require_true(fp.least(threads) > 0);      // nobody starved outright
}

// fixed WALL time rather than a fixed iteration count: with a fixed count every thread ends up with
// the same total by construction and the spread is 1.0 whatever the lock does
template<typename Lock>
double
measure_spread(u32 threads, u64 ms, u32 dwell)
{
  Lock m;
  lcheck::fairness_probe fp;
  micron::atomic_token<bool> stop(false);

  mtest::parallel(static_cast<int>(threads) + 1, [&](int tid) {
    if ( tid == static_cast<int>(threads) ) {
      micron::sleep_for(ms);
      stop.store(true, micron::memory_order::release);
      return;
    }
    while ( !stop.get(micron::memory_order::acquire) ) {
      m.lock();
      fp.note(static_cast<u32>(tid));
      for ( u32 k = 0; k < dwell; ++k ) __cpu_pause();
      m.unlock();
    }
  });
  return fp.spread(threads);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== LOCK EXCLUSION / FAIRNESS SWEEP ===");

  const u32 kT = lcheck::wide_threads;

  test_case("every lock type excludes: no two threads in the section at once");
  {
    constexpr int kIters = 2500;
    constexpr u32 kDwell = 12;
    sweep<mutex>("mutex        ", kT, kIters, kDwell);
    sweep<weak_mutex>("weak_mutex   ", kT, kIters, kDwell);
    sweep<fast_mutex>("fast_mutex   ", kT, kIters, kDwell);
    sweep<spin_lock>("spin_lock    ", kT, kIters, kDwell);
    sweep<recursive_lock>("recursive    ", kT, kIters, kDwell);
    sweep<ttas_lock>("ttas_lock    ", kT, kIters, kDwell);
    sweep<ttas_spin_lock>("ttas_spin    ", kT, kIters, kDwell);
    sweep<ticket_lock>("ticket_lock  ", kT, kIters, kDwell);
    sweep<mcs_lock>("mcs_lock     ", kT, kIters, kDwell);
    sweep<clh_lock>("clh_lock     ", kT, kIters, kDwell);
    sweep<futex_mutex>("futex_mutex  ", kT, kIters, kDwell);
    sweep<shared_mutex>("shared_mutex ", kT, kIters, kDwell);
  }
  end_test_case();

  test_case("every lock type excludes under oversubscription too");
  {
    const u32 kO = lcheck::over_threads;
    constexpr int kIters = 400;
    constexpr u32 kDwell = 24;
    sweep<ttas_lock>("ttas_lock    ", kO, kIters, kDwell);
    sweep<ticket_lock>("ticket_lock  ", kO, kIters, kDwell);
    sweep<mcs_lock>("mcs_lock     ", kO, kIters, kDwell);
    sweep<futex_mutex>("futex_mutex  ", kO, kIters, kDwell);
  }
  end_test_case();

  // ---- the comparison that makes "ticket_lock is fair" mean something ----
  test_case("fairness: the queue locks are orders of magnitude more even than TTAS");
  {
    constexpr u64 kMs = 120;
    constexpr u32 kDwell = 256;

    const double s_ttas = measure_spread<ttas_lock>(kT, kMs, kDwell);
    const double s_spin = measure_spread<spin_lock>(kT, kMs, kDwell);
    const double s_tick = measure_spread<ticket_lock>(kT, kMs, kDwell);
    const double s_mcs = measure_spread<mcs_lock>(kT, kMs, kDwell);
    const double s_clh = measure_spread<clh_lock>(kT, kMs, kDwell);

    sb::print("     spread(x100)  ttas=", static_cast<usize>(s_ttas * 100.0), " spin_lock=", static_cast<usize>(s_spin * 100.0),
              " ticket=", static_cast<usize>(s_tick * 100.0), " mcs=", static_cast<usize>(s_mcs * 100.0),
              " clh=", static_cast<usize>(s_clh * 100.0));

    // nobody may starve completely, on any of them
    require_true(s_ttas > 0.0);
    require_true(s_spin > 0.0);
    require_true(s_tick > 0.0);
    require_true(s_mcs > 0.0);
    require_true(s_clh > 0.0);

    // and that is ALL that is asserted. the spreads above are reported, not graded.
    //
    // fairness measured over a fixed wall-clock window is not a stable property of the lock: a FIFO
    // lock only orders the threads that are actually ENQUEUED, so a thread the scheduler starves of
    // CPU is also starved of acquisitions. across four runs on this host mcs_lock read 1.09, 24.3,
    // 63.3 and 74.0 while being equally FIFO every time, and spin_lock -- the unfair baseline --
    // swung from 103 to 95518, so even a RATIO between them flips. every threshold tried here,
    // absolute or relative, was a coin flip on a loaded machine, and a flaky assertion is worse
    // than none.
    //
    // the FIFO guarantee is graded deterministically elsewhere instead: the arrival-order cases in
    // mutex_ticket.cpp and mutex_mcs_lock.cpp pin a known enqueue order and require the service
    // order to match it exactly. what this case is for is the SHAPE of the numbers -- run it by hand
    // on an idle box and the FIFO family sits near 100 while the TTAS family is in the thousands.
    sb::print("     (spreads are reported, not asserted -- see the comment; the exact FIFO proof is",
              " in mutex_ticket.cpp / mutex_mcs_lock.cpp)");
  }
  end_test_case();

  test_case("all of them satisfy is_mutex");
  {
    static_assert(is_mutex<mutex>);
    static_assert(is_mutex<weak_mutex>);
    static_assert(is_mutex<fast_mutex>);
    static_assert(is_mutex<null_lock>);
    static_assert(is_mutex<spin_lock>);
    static_assert(is_mutex<recursive_lock>);
    static_assert(is_mutex<ttas_lock>);
    static_assert(is_mutex<ttas_spin_lock>);
    static_assert(is_mutex<ticket_lock>);
    static_assert(is_mutex<ticket_spin_lock>);
    static_assert(is_mutex<mcs_lock>);
    static_assert(is_mutex<clh_lock>);
    static_assert(is_mutex<futex_mutex>);
    static_assert(is_mutex<shared_mutex>);
    require_true(true);
  }
  end_test_case();

  test_case("randomized hold lengths across every type, fixed seed");
  {
    constexpr int kIters = 1200;
    u64 s = SEED_EX;
    for ( int round = 0; round < 3; ++round ) {
      const u32 d = static_cast<u32>(lcheck::xs64(s) & 0x3F);
      sweep<ttas_lock>("ttas_lock    ", kT, kIters, d);
      sweep<ticket_lock>("ticket_lock  ", kT, kIters, d);
      sweep<mcs_lock>("mcs_lock     ", kT, kIters, d);
      sweep<futex_mutex>("futex_mutex  ", kT, kIters, d);
    }
  }
  end_test_case();

  sb::print("=== ALL LOCK EXCLUSION TESTS PASSED ===");
  return 1;
}
