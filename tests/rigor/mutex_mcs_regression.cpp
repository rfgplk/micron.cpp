//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1
#define MICRON_LOCK_STATS 1

#include "../../src/mutex/locks/mcs_lock.hpp"

#include "../../src/concepts.hpp"
#include "../../src/std.hpp"

#include "../../src/sync/inlet.hpp"

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

#ifndef MCS_REG_ITERS
#define MCS_REG_ITERS 2000
#endif

#ifndef MCS_REG_STEP_MS
#define MCS_REG_STEP_MS 5000
#endif

#ifndef MCS_REG_CASE_MS
#define MCS_REG_CASE_MS 60000
#endif

#ifndef MCS_REG_REACQUIRE_ROUNDS
#define MCS_REG_REACQUIRE_ROUNDS 100000
#endif

namespace
{

constexpr u64 SEED_MCSREG = 0x9C5DEAD10CB0ULL;

static_assert(micron::is_mutex<micron::mcs_lock>, "mcs_lock must satisfy is_mutex");

template<typename Pred>
[[nodiscard]] bool
poll_until(Pred p, u64 budget_ms) noexcept
{
  for ( u64 waited = 0; waited < budget_ms; waited += 2 ) {
    if ( p() ) return true;
    micron::sleep_for(2);
  }
  return p();
}

template<typename Pred>
[[nodiscard]] bool
spin_until(Pred p, u64 rounds = 20000000ull) noexcept
{
  for ( u64 i = 0; i < rounds; ++i ) {
    if ( p() ) return true;
    if ( (i & 0xFFull) == 0xFFull ) micron::yield();
  }
  return p();
}

void
must_reach(const char *what, bool ok)
{
  if ( ok ) return;
  sb::print("     DEADLINE MISSED: ", what);
  sb::require(false);
}

template<typename Fn>
void
guarded_step(const char *what, u64 budget_ms, Fn fn)
{
  micron::atomic_token<u32> done{ 0 };
  auto t = micron::solo::spawn<micron::auto_thread<>>([&]() {
    fn();
    done.store(1, micron::memory_order::release);
  });
  must_reach(what, poll_until([&]() { return done.get(micron::memory_order::acquire) != 0u; }, budget_ms));
  micron::solo::join(t);
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== MCS_LOCK DEADLOCK REGRESSIONS ===");
  sb::print("    depth: ", static_cast<usize>(MICRON_MCS_DEPTH), "  iters: ", static_cast<usize>(MCS_REG_ITERS),
            "  step budget: ", static_cast<usize>(MCS_REG_STEP_MS), " ms");

  test_case("try_lock() by the holder, with a real successor queued behind it");
  {

    mcs_lock m;
    atomic_token<u32> b_got{ 0 };
    atomic_token<u32> try_answer{ 2 };
    atomic_token<u32> holds_after{ 2 };
    atomic_token<u32> released{ 0 };

    guarded_step("the holder's unlock() with a successor queued", MCS_REG_CASE_MS, [&]() {
      m.lock();
      auto b = solo::spawn<auto_thread<>>([&]() {
        m.lock();
        b_got.store(1, memory_order::release);
        m.unlock();
      });
      must_reach("a successor enqueued behind the holder", poll_until([&]() { return m.enqueued() >= 2u; }, MCS_REG_STEP_MS));

      micron::sleep_for(5);

      try_answer.store(m.try_lock() ? 1u : 0u, memory_order::release);
      holds_after.store(m.holds() ? 1u : 0u, memory_order::release);

      m.unlock();
      released.store(1, memory_order::release);

      must_reach("the successor was served after the holder released",
                 poll_until([&]() { return b_got.get(memory_order::acquire) != 0u; }, MCS_REG_STEP_MS));
      solo::join(b);
    });

    require_true(try_answer.get(memory_order::acquire) == 0u);
    require_true(holds_after.get(memory_order::acquire) == 1u);
    require_true(released.get(memory_order::acquire) == 1u);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("the same re-entrancy through queuing_inlet::try_access()");
  {
    queuing_inlet<i64> q;
    atomic_token<u32> refused{ 2 };
    atomic_token<u32> waiter_served{ 0 };
    atomic_token<i64> seen{ 0 };

    guarded_step("try_access() while this thread holds the handle, with a waiter behind it", MCS_REG_CASE_MS, [&]() {
      __thread_pointer<auto_thread<>> w;
      {
        auto h = q.access();
        *h = 41;
        w = solo::spawn<auto_thread<>>([&]() {
          seen.store(q.load(), memory_order::release);
          waiter_served.store(1, memory_order::release);
        });

        micron::sleep_for(20);
        refused.store(q.try_access().is_second() ? 1u : 0u, memory_order::release);
        *h = 42;
      }

      must_reach("the inlet's waiter was served after the handle dropped",
                 poll_until([&]() { return waiter_served.get(memory_order::acquire) != 0u; }, MCS_REG_STEP_MS));
      solo::join(w);
    });

    require_true(refused.get(memory_order::acquire) == 1u);
    require_true(seen.get(memory_order::acquire) == 42);
    require_false(q.locked());
  }
  end_test_case();

  test_case("try_lock_all() over the same lock twice rolls back instead of wedging");
  {

    mcs_lock m;
    atomic_token<i32> who{ -2 };

    guarded_step("try_lock_all(m, m)", MCS_REG_STEP_MS, [&]() {
      who.store(try_lock_all(m, m), memory_order::release);
      require_false(m.is_locked());
      require_true(m.try_lock());
      m.unlock();
    });

    require_true(who.get(memory_order::acquire) == 1);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("unlock() from a thread that did not acquire releases the acquisition");
  {
    mcs_lock m;
    atomic_token<u32> a_in{ 0 };
    atomic_token<u32> a_go{ 0 };
    atomic_token<u32> b_done{ 0 };
    atomic_token<u32> c_in{ 0 };

    guarded_step("A acquires, B releases, C acquires", MCS_REG_CASE_MS, [&]() {
      auto a = solo::spawn<auto_thread<>>([&]() {
        m.lock();
        a_in.store(1, memory_order::release);

        (void)poll_until([&]() { return a_go.get(memory_order::acquire) != 0u; }, MCS_REG_CASE_MS);
      });
      must_reach("A acquired", poll_until([&]() { return a_in.get(memory_order::acquire) != 0u; }, MCS_REG_STEP_MS));
      require_true(m.is_locked());

      auto b = solo::spawn<auto_thread<>>([&]() {
        m.unlock();
        b_done.store(1, memory_order::release);
      });
      must_reach("B's unlock() returned", poll_until([&]() { return b_done.get(memory_order::acquire) != 0u; }, MCS_REG_STEP_MS));
      must_reach("the lock is free after the cross-thread release", poll_until([&]() { return !m.is_locked(); }, MCS_REG_STEP_MS));

      auto c = solo::spawn<auto_thread<>>([&]() {
        m.lock();
        c_in.store(1, memory_order::release);
        m.unlock();
      });
      must_reach("C acquired after the cross-thread release",
                 poll_until([&]() { return c_in.get(memory_order::acquire) != 0u; }, MCS_REG_STEP_MS));

      a_go.store(1, memory_order::release);
      solo::join(a);
      solo::join(b);
      solo::join(c);
    });

    require_false(m.is_locked());
  }
  end_test_case();

  test_case("a queuing_inlet handle_t moved to another thread releases there");
  {
    using handle_type = queuing_inlet<i64>::handle_t;
    queuing_inlet<i64> q;
    atomic_token<u32> dropped{ 0 };

    guarded_step("access() on A, handle move-constructed and destroyed on B", MCS_REG_CASE_MS, [&]() {
      auto h = q.access();
      *h = 7;
      auto b = solo::spawn<auto_thread<>>([&]() {
        {
          handle_type mine(micron::move(h));
          mine.set(42);
        }
        dropped.store(1, memory_order::release);
      });
      must_reach("the moved handle was dropped on the other thread",
                 poll_until([&]() { return dropped.get(memory_order::acquire) != 0u; }, MCS_REG_STEP_MS));
      must_reach("the inlet is unlocked after its handle was dropped elsewhere",
                 poll_until([&]() { return !q.locked(); }, MCS_REG_STEP_MS));

      require_true(q.load() == 42);
      solo::join(b);
    });

    require_false(q.locked());
    require_true(q.load() == 42);
  }
  end_test_case();

  test_case("the acquirer re-acquires after a foreign release, with no handshake between");
  {
    constexpr u64 kRounds = MCS_REG_REACQUIRE_ROUNDS;
    mcs_lock m;
    atomic_token<u64> handed{ 0 };
    atomic_token<u32> raised{ 0 };
    atomic_token<u32> stalled{ 0 };
    atomic_token<u32> done{ 0 };

    guarded_step("A locks, B unlocks, A re-locks -- round after round", MCS_REG_CASE_MS, [&]() {
      auto b = solo::spawn<auto_thread<>>([&]() {
        for ( u64 r = 1; r <= kRounds; ++r ) {
          if ( !spin_until([&]() { return handed.get(memory_order::acquire) >= r; }) ) {
            stalled.store(1, memory_order::release);
            return;
          }
          m.unlock();
        }
      });

      auto a = solo::spawn<auto_thread<>>([&]() {
        for ( u64 r = 1; r <= kRounds; ++r ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
          try {
            m.lock();
          } catch ( ... ) {
            raised.fetch_add(1, memory_order::acq_rel);
            return;
          }
#else
          m.lock();
#endif
          handed.store(r, memory_order::release);

          if ( !spin_until([&]() { return !m.is_locked(); }) ) {
            stalled.store(1, memory_order::release);
            return;
          }
        }
        done.store(1, memory_order::release);
      });

      solo::join(a);
      solo::join(b);
    });

    require_true(raised.get(memory_order::acquire) == 0u);
    require_true(stalled.get(memory_order::acquire) == 0u);
    require_true(done.get(memory_order::acquire) == 1u);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock() after a foreign release refuses at worst transiently");
  {
    constexpr u64 kRounds = MCS_REG_REACQUIRE_ROUNDS / 10;
    mcs_lock m;
    atomic_token<u64> handed{ 0 };
    atomic_token<u64> refusals{ 0 };
    atomic_token<u32> stalled{ 0 };
    atomic_token<u32> done{ 0 };

    guarded_step("A try_locks into the tail of B's release", MCS_REG_CASE_MS, [&]() {
      auto b = solo::spawn<auto_thread<>>([&]() {
        for ( u64 r = 1; r <= kRounds; ++r ) {
          if ( !spin_until([&]() { return handed.get(memory_order::acquire) >= r; }) ) {
            stalled.store(1, memory_order::release);
            return;
          }
          m.unlock();
        }
      });

      auto a = solo::spawn<auto_thread<>>([&]() {
        for ( u64 r = 1; r <= kRounds; ++r ) {
          u64 tries = 0;
          if ( !spin_until([&]() {
                 ++tries;
                 return m.try_lock();
               }) ) {
            stalled.store(1, memory_order::release);
            return;
          }
          refusals.fetch_add(tries - 1, memory_order::acq_rel);
          handed.store(r, memory_order::release);
          if ( !spin_until([&]() { return !m.is_locked(); }) ) {
            stalled.store(1, memory_order::release);
            return;
          }
        }
        done.store(1, memory_order::release);
      });

      solo::join(a);
      solo::join(b);
    });

    require_true(stalled.get(memory_order::acquire) == 0u);
    require_true(done.get(memory_order::acquire) == 1u);
    require_false(m.is_locked());
    sb::print("     try_lock refusals across ", static_cast<usize>(kRounds),
              " rounds: ", static_cast<usize>(refusals.get(memory_order::acquire)));
  }
  end_test_case();

  test_case("a lock built at a recycled address does not inherit the dead one's node");
  {

    alignas(mcs_lock) unsigned char storage[sizeof(mcs_lock)];
    atomic_token<u32> holds_fresh{ 2 };
    atomic_token<u32> unlock_returned{ 0 };
    lcheck::exclusion_probe pr;
    ltest::tracked<9>::reset();
    ltest::tracked<9> cell;

    guarded_step("destroy a held lock, rebuild at the same address, use it", MCS_REG_CASE_MS, [&]() {
      mcs_lock *m1 = micron::construct_at(reinterpret_cast<mcs_lock *>(storage));
      require_true(m1->try_lock());
      require_true(m1->holds());
      m1->~mcs_lock();

      mcs_lock *m2 = micron::construct_at(reinterpret_cast<mcs_lock *>(storage));
      require_false(m2->is_locked());
      holds_fresh.store(m2->holds() ? 1u : 0u, memory_order::release);

      m2->unlock();
      unlock_returned.store(1, memory_order::release);
      require_false(m2->is_locked());

      m2->lock();
      require_true(m2->holds());
      m2->unlock();
      require_false(m2->is_locked());
      require_true(m2->try_lock());
      m2->unlock();

      constexpr i64 kIters = MCS_REG_ITERS;
      mtest::parallel(2, [&](int tid) {
        u64 s = SEED_MCSREG + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
        for ( i64 i = 0; i < kIters; ++i ) {
          m2->lock();
          pr.enter();
          pr.dwell(static_cast<u32>(lcheck::xs64(s) & 0x7u));
          cell.v += 1;
          pr.leave();
          m2->unlock();
        }
      });
      m2->~mcs_lock();
    });

    require_true(holds_fresh.get(memory_order::acquire) == 0u);
    require_true(unlock_returned.get(memory_order::acquire) == 1u);
    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
    require_true(cell.v == 2 * static_cast<i64>(MCS_REG_ITERS));
    require(ltest::tracked<9>::faults() == 0ull);
  }
  end_test_case();

  test_case("capacity: try_lock fails closed at MICRON_MCS_DEPTH, lock() raises");
  {
    constexpr usize kDepth = MICRON_MCS_DEPTH;
    atomic_token<u32> refused{ 2 };
    atomic_token<u32> raised{ 2 };

    guarded_step("hold DEPTH locks at once, then reach for a DEPTH+1-th", MCS_REG_CASE_MS, [&]() {
      mcs_lock m[kDepth + 1];
      for ( usize i = 0; i < kDepth; ++i ) {
        m[i].lock();
        require_true(m[i].holds());
      }

      refused.store(m[kDepth].try_lock() ? 0u : 1u, memory_order::release);
      require_false(m[kDepth].is_locked());
      require_false(m[kDepth].holds());

#if !defined(__micron_freestanding) || defined(__micron_eh)
      bool caught = false;
      try {
        m[kDepth].lock();
      } catch ( const except::thread_error & ) {
        caught = true;
      } catch ( ... ) {
      }
      raised.store(caught ? 1u : 0u, memory_order::release);
#else
      raised.store(1u, memory_order::release);
#endif

      for ( usize i = 0; i < kDepth; ++i ) m[i].unlock();
      for ( usize i = 0; i < kDepth; ++i ) require_false(m[i].is_locked());

      require_true(m[kDepth].try_lock());
      m[kDepth].unlock();
    });

    require_true(refused.get(memory_order::acquire) == 1u);
    require_true(raised.get(memory_order::acquire) == 1u);
  }
  end_test_case();

  test_case("capacity: one thread over far more distinct locks than the table has slots");
  {

    guarded_step("4*DEPTH+8 distinct locks, three rounds, one thread", MCS_REG_CASE_MS, [&]() {
      constexpr usize kMany = 4 * MICRON_MCS_DEPTH + 8;
      mcs_lock many[kMany];
      for ( usize r = 0; r < 3; ++r )
        for ( usize i = 0; i < kMany; ++i ) {
          many[i].lock();
          require_true(many[i].holds());
          many[i].unlock();
          require_false(many[i].is_locked());
        }
    });
    require_true(true);
  }
  end_test_case();

  test_case("release order: the middle lock drops first and its waiter is served");
  {
    constexpr usize kNest = MICRON_MCS_DEPTH < 3 ? MICRON_MCS_DEPTH : 3;
    if constexpr ( kNest < 2 ) {
      sb::print("     MICRON_MCS_DEPTH < 2: one slot per thread, so there is no order to reorder");
      guarded_step("single-slot roundtrip", MCS_REG_STEP_MS, [&]() {
        mcs_lock m;
        m.lock();
        require_true(m.holds());
        m.unlock();
        require_false(m.is_locked());
      });
    } else {
      atomic_token<u32> w_in{ 0 };
      atomic_token<u32> w_go{ 0 };

      guarded_step("release the middle lock first, with a waiter queued on it", MCS_REG_CASE_MS, [&]() {
        mcs_lock m[kNest];
        for ( usize i = 0; i < kNest; ++i ) m[i].lock();

        auto w = solo::spawn<auto_thread<>>([&]() {
          m[1].lock();
          w_in.store(1, memory_order::release);
          (void)poll_until([&]() { return w_go.get(memory_order::acquire) != 0u; }, MCS_REG_CASE_MS);
          m[1].unlock();
        });
        must_reach("the waiter enqueued on the middle lock", poll_until([&]() { return m[1].enqueued() >= 2u; }, MCS_REG_STEP_MS));

        m[1].unlock();
        must_reach("the waiter was served off the middle lock",
                   poll_until([&]() { return w_in.get(memory_order::acquire) != 0u; }, MCS_REG_STEP_MS));

        for ( usize i = 0; i < kNest; ++i ) {
          if ( i == 1 ) continue;
          require_true(m[i].is_locked());
          require_true(m[i].holds());
        }
        require_false(m[1].holds());

        for ( usize i = kNest; i-- > 0; ) {
          if ( i == 1 ) continue;
          m[i].unlock();
        }
        w_go.store(1, memory_order::release);
        solo::join(w);
        for ( usize i = 0; i < kNest; ++i ) require_false(m[i].is_locked());
      });
    }
  }
  end_test_case();

  sb::print("=== ALL MCS_LOCK DEADLOCK REGRESSIONS PASSED ===");
  return 1;
}
