//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// memory-ORDERING tests, which the tree had none of.
//
// every existing concurrency test grades a counter total or an exact-partition bitmap. both of
// those pass under wrong orderings on x86, because TSO already gives you acquire/release for free
// on ordinary loads and stores -- so the whole ordering layer of src/atomic is unvalidated on the
// only arch it is routinely run on. these are the classic litmus tests, and the cells that earn
// their keep are the --arm and --arm64 lines in tests/locks/locks.duck, where a relaxed load
// genuinely can be reordered and a missing acquire genuinely does show up.
//
// the relaxed cases below COUNT the anomalies rather than assert them away: an anomaly there is
// permitted by the model, and its absence on x86 is a property of the hardware, not of the code.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/atomic/atomic.hpp"
#include "../../src/atomic/intrin.hpp"

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

// rounds are cheap; the window a reordering fits in is small, so volume is what finds it
constexpr int kRounds = 200000;

// the lockstep cases pay for three barrier rendezvous per round, so they run far fewer
constexpr int kLockstepRounds = 20000;

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== ATOMIC MEMORY-ORDERING LITMUS ===");

  // ---- message passing: the pattern every lock release/acquire is ----
  test_case("MP with release/acquire: seeing the flag implies seeing the data");
  {
    // writer:  data = 42 ; flag.store(1, release)
    // reader:  if (flag.load(acquire) == 1) then data MUST read 42
    atomic_token<u32> data(0);
    atomic_token<u32> flag(0);
    atomic_token<u64> observed(0);
    atomic_token<u64> violations(0);
    lcheck::start_gate gate(2);

    mtest::parallel(2, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      if ( tid == 0 ) {
        for ( int i = 1; i <= kRounds; ++i ) {
          data.store(static_cast<u32>(i), memory_order::relaxed);
          flag.store(static_cast<u32>(i), memory_order::release);
          while ( flag.get(memory_order::acquire) == static_cast<u32>(i) ) __cpu_pause();
        }
      } else {
        for ( int i = 1; i <= kRounds; ++i ) {
          u32 f;
          while ( (f = flag.get(memory_order::acquire)) != static_cast<u32>(i) ) __cpu_pause();
          const u32 d = data.get(memory_order::relaxed);
          observed.fetch_add(1, memory_order::relaxed);
          if ( d != f ) violations.fetch_add(1, memory_order::relaxed);
          flag.store(0, memory_order::release);
        }
      }
    });

    sb::print("     MP rel/acq: observed=", static_cast<usize>(observed.get(memory_order::acquire)),
              " violations=", static_cast<usize>(violations.get(memory_order::acquire)));
    require(observed.get(memory_order::acquire) == static_cast<u64>(kRounds));
    require(violations.get(memory_order::acquire) == 0ull);      // the release/acquire pair forbids it
  }
  end_test_case();

  test_case("MP with an explicit thread_fence pair behaves the same");
  {
    // release/acquire spelled as relaxed stores plus standalone fences -- the form
    // seqlock.hpp uses, since its payload copy is a plain memcpy and cannot carry an order
    atomic_token<u32> data(0);
    atomic_token<u32> flag(0);
    atomic_token<u64> violations(0);
    atomic_token<u64> observed(0);
    lcheck::start_gate gate(2);

    mtest::parallel(2, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      if ( tid == 0 ) {
        for ( int i = 1; i <= kRounds; ++i ) {
          data.store(static_cast<u32>(i), memory_order::relaxed);
          atom::thread_fence(atomic_release);
          flag.store(static_cast<u32>(i), memory_order::relaxed);
          while ( flag.get(memory_order::relaxed) == static_cast<u32>(i) ) __cpu_pause();
        }
      } else {
        for ( int i = 1; i <= kRounds; ++i ) {
          u32 f;
          while ( (f = flag.get(memory_order::relaxed)) != static_cast<u32>(i) ) __cpu_pause();
          atom::thread_fence(atomic_acquire);
          const u32 d = data.get(memory_order::relaxed);
          observed.fetch_add(1, memory_order::relaxed);
          if ( d != f ) violations.fetch_add(1, memory_order::relaxed);
          flag.store(0, memory_order::relaxed);
        }
      }
    });

    sb::print("     MP fences : observed=", static_cast<usize>(observed.get(memory_order::acquire)),
              " violations=", static_cast<usize>(violations.get(memory_order::acquire)));
    require(observed.get(memory_order::acquire) == static_cast<u64>(kRounds));
    require(violations.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  // ---- store buffering: the one pattern seq_cst is actually needed for ----
  //
  // both cases below rendezvous on a sense-reversing barrier every round. an earlier version
  // hand-rolled the lockstep with a round counter and one thread simply outran the other, which
  // both hid the overlap and hung the run; a barrier is what makes "the same round" mean something.
  test_case("SB with seq_cst: both threads reading 0 in one round is impossible");
  {
    // x=1; r0=y   ||   y=1; r1=x    -- seq_cst forbids r0 == 0 && r1 == 0
    atomic_token<u32> x(0);
    atomic_token<u32> y(0);
    u32 r[2] = { 0, 0 };
    u64 both_zero = 0;
    u64 rounds = 0;
    ltest::barrier_t bar;
    bar.n = 2;

    mtest::parallel(2, [&](int tid) {
      u32 sense = 0;
      for ( int i = 0; i < kLockstepRounds; ++i ) {
        ltest::barrier_wait(bar, sense);      // enter the round together
        if ( tid == 0 ) {
          x.store(1, memory_order::seq_cst);
          r[0] = y.get(memory_order::seq_cst);
        } else {
          y.store(1, memory_order::seq_cst);
          r[1] = x.get(memory_order::seq_cst);
        }
        ltest::barrier_wait(bar, sense);      // both reads are complete and visible
        if ( tid == 0 ) {
          if ( r[0] == 0 and r[1] == 0 ) ++both_zero;
          ++rounds;
          x.store(0, memory_order::seq_cst);
          y.store(0, memory_order::seq_cst);
        }
        ltest::barrier_wait(bar, sense);      // the reset is done before the next round
      }
    });

    sb::print("     SB seq_cst : rounds=", static_cast<usize>(rounds), " both-zero=", static_cast<usize>(both_zero));
    require(rounds == static_cast<u64>(kLockstepRounds));
    require(both_zero == 0ull);      // the outcome seq_cst exists to forbid
  }
  end_test_case();

  test_case("SB with relaxed: the anomaly is PERMITTED, so it is counted, not asserted");
  {
    atomic_token<u32> x(0);
    atomic_token<u32> y(0);
    u32 r[2] = { 0, 0 };
    u64 both_zero = 0;
    u64 rounds = 0;
    ltest::barrier_t bar;
    bar.n = 2;

    mtest::parallel(2, [&](int tid) {
      u32 sense = 0;
      for ( int i = 0; i < kLockstepRounds; ++i ) {
        ltest::barrier_wait(bar, sense);
        if ( tid == 0 ) {
          x.store(1, memory_order::relaxed);
          r[0] = y.get(memory_order::relaxed);
        } else {
          y.store(1, memory_order::relaxed);
          r[1] = x.get(memory_order::relaxed);
        }
        ltest::barrier_wait(bar, sense);
        if ( tid == 0 ) {
          if ( r[0] == 0 and r[1] == 0 ) ++both_zero;
          ++rounds;
          x.store(0, memory_order::relaxed);
          y.store(0, memory_order::relaxed);
        }
        ltest::barrier_wait(bar, sense);
      }
    });

    // NO assertion on both_zero: the model permits it, and whether it shows up is a property of the
    // hardware. on amd64 the store buffer makes it common; the number is printed so the arm cells
    // can be compared against it.
    sb::print("     SB relaxed : rounds=", static_cast<usize>(rounds), " both-zero=", static_cast<usize>(both_zero));
    require(rounds == static_cast<u64>(kLockstepRounds));
  }
  end_test_case();

  // ---- load buffering ----
  test_case("LB with acquire/release: both threads reading 1 in one round is impossible");
  {
    // r0=y(acq); x.store(1,rel)  ||  r1=x(acq); y.store(1,rel)
    // r0 == 1 && r1 == 1 would need each load to observe a store sequenced after it
    atomic_token<u32> x(0);
    atomic_token<u32> y(0);
    u32 r[2] = { 0, 0 };
    u64 both_one = 0;
    u64 rounds = 0;
    ltest::barrier_t bar;
    bar.n = 2;

    mtest::parallel(2, [&](int tid) {
      u32 sense = 0;
      for ( int i = 0; i < kLockstepRounds; ++i ) {
        ltest::barrier_wait(bar, sense);
        if ( tid == 0 ) {
          r[0] = y.get(memory_order::acquire);
          x.store(1, memory_order::release);
        } else {
          r[1] = x.get(memory_order::acquire);
          y.store(1, memory_order::release);
        }
        ltest::barrier_wait(bar, sense);
        if ( tid == 0 ) {
          if ( r[0] == 1 and r[1] == 1 ) ++both_one;
          ++rounds;
          x.store(0, memory_order::release);
          y.store(0, memory_order::release);
        }
        ltest::barrier_wait(bar, sense);
      }
    });

    sb::print("     LB acq/rel : rounds=", static_cast<usize>(rounds), " both-one=", static_cast<usize>(both_one));
    require(rounds == static_cast<u64>(kLockstepRounds));
    require(both_one == 0ull);
  }
  end_test_case();

  // ---- what the locks depend on ----
  test_case("a release store publishes everything written before it (the unlock contract)");
  {
    // exactly the shape mutex::unlock/lock rely on, with an eight-word payload so a partial
    // publication is detectable
    struct block {
      u64 w[8];
    };
    block shared{};
    atomic_token<u32> gen(0);
    atomic_token<u64> torn(0);
    lcheck::start_gate gate(2);

    mtest::parallel(2, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      if ( tid == 0 ) {
        for ( u32 g = 1; g <= static_cast<u32>(kRounds / 4); ++g ) {
          for ( int k = 0; k < 8; ++k ) shared.w[k] = static_cast<u64>(g) * (k + 1);
          gen.store(g, memory_order::release);
          while ( gen.get(memory_order::acquire) == g ) __cpu_pause();
        }
      } else {
        for ( u32 g = 1; g <= static_cast<u32>(kRounds / 4); ++g ) {
          while ( gen.get(memory_order::acquire) != g ) __cpu_pause();
          for ( int k = 0; k < 8; ++k )
            if ( shared.w[k] != static_cast<u64>(g) * (k + 1) ) torn.fetch_add(1, memory_order::relaxed);
          gen.store(0, memory_order::release);
        }
      }
    });

    sb::print("     release publication: torn words=", static_cast<usize>(torn.get(memory_order::acquire)));
    require(torn.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  test_case("relaxed RMW is still atomic even though it orders nothing");
  {
    atomic_token<u64> n(0);
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 50000;
    lcheck::start_gate gate(static_cast<u32>(kT));

    mtest::parallel(kT, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kIters; ++i ) n.fetch_add(1, memory_order::relaxed);
    });

    require(n.get(memory_order::seq_cst) == static_cast<u64>(kT) * kIters);
  }
  end_test_case();

  test_case("compare_and_swap does not write the observed value back (its by-value contract)");
  {
    // the contract that made atomic_ptr's RMW loops livelock; pinned so it cannot change silently
    atomic_token<u32> v(5);
    const u32 expected = 9;
    require_false(v.compare_and_swap(expected, 10));
    require(expected == 9u);      // untouched: compare_and_swap takes it BY VALUE
    require(v.get(memory_order::relaxed) == 5u);

    // compare_exchange_weak/strong DO write it back, which is what a retry loop needs
    u32 e = 9;
    require_false(v.compare_exchange_strong(e, 10, memory_order::acq_rel, memory_order::relaxed));
    require(e == 5u);
    require_true(v.compare_exchange_strong(e, 10, memory_order::acq_rel, memory_order::relaxed));
    require(e == 5u);      // untouched on SUCCESS
    require(v.get(memory_order::relaxed) == 10u);
  }
  end_test_case();

  sb::print("=== ALL LITMUS TESTS PASSED ===");
  return 1;
}
