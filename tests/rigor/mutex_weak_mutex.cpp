//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// weak_mutex was the only member of the hand-written family with no file of its own -- it showed up
// only incidentally, in mutex_basic and the guard tests. two things went unpinned because of that.
// its release store was seq_cst against an acq_rel/acquire CAS, so the pair was asymmetric (the
// acquire side paid for ordering the release side then paid for again); and reset(), which is the
// path every RAII guard takes through the member-function pointer, DUPLICATED the store instead of
// calling unlock(), so the two release paths could drift apart silently. spin_lock's pair had
// already drifted -- seq_cst in reset(), release in unlock() -- and nothing caught it.
//
// a test cannot read the order literal back, and it cannot tell seq_cst from release: both are
// correct, one is just dearer. what it CAN pin is the contract underneath -- everything written
// before a release is visible to whoever acquires next -- and that it holds through BOTH release
// paths, the guard's reset() and a manual unlock(). that is what regressing to relaxed would break,
// and it is only observable on weak memory, so the --arm cell is the one that earns its keep here.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/locks.hpp"

#include "../../src/concepts.hpp"
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

#ifndef WEAK_MTX_SCALE
#define WEAK_MTX_SCALE 1
#endif

#if defined(__micron_arch_width_32)
constexpr u32 pub_rounds = 4000u * WEAK_MTX_SCALE;      // arm32 runs under qemu
#else
constexpr u32 pub_rounds = 20000u * WEAK_MTX_SCALE;
#endif

// four words, so a publish that is torn or reordered is visible rather than merely wrong-by-one
struct payload {
  u64 a, b, c, d;
};

// plain (non-atomic) state: the lock is the only thing ordering it. that is the point.
payload g_pub{ 0, 0, 0, 0 };
u64 g_stamp = 0;

// does M have the reset-PMF shape the guards dispatch through?
template<typename M>
concept has_reset_pmf = requires(M m) {
  { m.retrieve() };
  { m() };
};

}      // namespace

static_assert(micron::is_mutex<micron::weak_mutex>, "weak_mutex must satisfy is_mutex");
static_assert(has_reset_pmf<micron::weak_mutex>, "the guards dispatch through operator()/retrieve()");
static_assert(!micron::is_copy_constructible_v<micron::weak_mutex>);
static_assert(!micron::is_move_constructible_v<micron::weak_mutex>);

int
main(void)
{
  using namespace micron;
  sb::print("=== WEAK_MUTEX TESTS ===");

  test_case("uncontended acquire / release / observe");
  {
    weak_mutex m;
    require_false(m.is_locked());
    require_true(!m);      // operator! is "not held"

    m.lock();
    require_true(m.is_locked());
    require_false(m.try_lock());      // already held, must refuse

    m.unlock();
    require_false(m.is_locked());
    require_true(m.try_lock());      // free again
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("the guard release path (reset PMF) leaves it free, exactly like unlock()");
  {
    weak_mutex m;
    {
      lock_guard<weak_mutex> g(m);
      require_true(m.is_locked());
    }
    require_false(m.is_locked());
    require_true(m.try_lock());      // reset() really released it
    m.unlock();

    // and the same lock, released the manual way, ends in the same state
    {
      unique_lock<lock_starts::locked, weak_mutex> u(m);
      require_true(m.is_locked());
      u.unlock();
      require_false(m.is_locked());
    }
    require_false(m.is_locked());
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("a manual unlock() publishes everything written before it");
  {
    weak_mutex m;
    lcheck::watchdog wd;
    lcheck::start_gate gate(2);
    atomic_token<u64> seen_torn{ 0 };
    atomic_token<u64> reads{ 0 };

    g_pub = payload{ 0, 0, 0, 0 };
    g_stamp = 0;

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(20000, 2); });
    mtest::parallel(2, [&](int id) {
      u32 sense = 0;
      gate.wait(sense);

      if ( id == 0 ) {
        for ( u32 i = 1; i <= pub_rounds; ++i ) {
          m.lock();
          g_pub.a = i;
          g_pub.b = i * 2u;
          g_pub.c = i * 3u;
          g_pub.d = i * 4u;
          g_stamp = i;
          m.unlock();      // the release under test
          wd.bump();
        }
      } else {
        for ( u32 i = 0; i < pub_rounds; ++i ) {
          m.lock();
          const u64 s = g_stamp;
          // whoever acquires after a release must see ALL of that release's writes
          if ( g_pub.a != s or g_pub.b != s * 2u or g_pub.c != s * 3u or g_pub.d != s * 4u )
            seen_torn.fetch_add(1, memory_order::relaxed);
          m.unlock();
          reads.fetch_add(1, memory_order::relaxed);
          wd.bump();
        }
      }
    });
    wd.disarm();
    solo::join(watcher);

    require_true(wd.ok());
    require(seen_torn.get(memory_order::acquire), (u64)0);
    require(reads.get(memory_order::acquire), (u64)pub_rounds);
  }
  end_test_case();

  test_case("the guard's release path publishes identically");
  {
    weak_mutex m;
    lcheck::watchdog wd;
    lcheck::start_gate gate(2);
    atomic_token<u64> seen_torn{ 0 };

    g_pub = payload{ 0, 0, 0, 0 };
    g_stamp = 0;

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(20000, 2); });
    mtest::parallel(2, [&](int id) {
      u32 sense = 0;
      gate.wait(sense);

      if ( id == 0 ) {
        for ( u32 i = 1; i <= pub_rounds; ++i ) {
          lock_guard<weak_mutex> g(m);      // released by reset(), not unlock()
          g_pub.a = i;
          g_pub.b = i * 2u;
          g_pub.c = i * 3u;
          g_pub.d = i * 4u;
          g_stamp = i;
          wd.bump();
        }
      } else {
        for ( u32 i = 0; i < pub_rounds; ++i ) {
          lock_guard<weak_mutex> g(m);
          const u64 s = g_stamp;
          if ( g_pub.a != s or g_pub.b != s * 2u or g_pub.c != s * 3u or g_pub.d != s * 4u )
            seen_torn.fetch_add(1, memory_order::relaxed);
          wd.bump();
        }
      }
    });
    wd.disarm();
    solo::join(watcher);

    require_true(wd.ok());
    require(seen_torn.get(memory_order::acquire), (u64)0);
  }
  end_test_case();

  test_case("mutual exclusion holds under contention");
  {
    weak_mutex m;
    lcheck::exclusion_probe probe;
    lcheck::watchdog wd;
    const u32 n = lcheck::wide_threads;
    lcheck::start_gate gate(n);

#if defined(__micron_arch_width_32)
    constexpr u32 rounds = 500u;
#else
    constexpr u32 rounds = 2000u;
#endif

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(20000, n); });
    mtest::parallel((int)n, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( u32 i = 0; i < rounds; ++i ) {
        m.lock();
        probe.enter();
        probe.dwell(16);      // a single increment is nearly impossible to catch two threads inside
        probe.leave();
        m.unlock();
        wd.bump();
      }
    });
    wd.disarm();
    solo::join(watcher);

    require_true(wd.ok());
    require_true(probe.clean());
    require(probe.entries.get(memory_order::acquire), (u64)n * rounds);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock under contention never hands the same lock to two threads");
  {
    weak_mutex m;
    lcheck::exclusion_probe probe;
    const u32 n = lcheck::wide_threads;
    lcheck::start_gate gate(n);
    atomic_token<u64> got{ 0 };

    mtest::parallel((int)n, [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( u32 i = 0; i < 4000u; ++i ) {
        if ( m.try_lock() ) {
          probe.enter();
          probe.dwell(8);
          probe.leave();
          m.unlock();
          got.fetch_add(1, memory_order::relaxed);
        }
      }
    });

    require_true(probe.clean());
    require_false(m.is_locked());
    require_true(got.get(memory_order::acquire) > 0);      // it must actually succeed sometimes
  }
  end_test_case();

  sb::print("=== ALL WEAK_MUTEX TESTS PASSED ===");
  return 1;
}
