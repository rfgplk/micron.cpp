//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1

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
constexpr u64 SEED_ML = 0xD3AD10CC0FFEEULL;
constexpr usize kLocks = 5;
}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== MULTI-LOCK / DEADLOCK TESTS ===");

  test_case("try_lock_all: -1 on full success, everything held");
  {
    ttas_lock a, b, c;
    require(try_lock_all(a, b, c) == -1);
    require_true(a.is_locked());
    require_true(b.is_locked());
    require_true(c.is_locked());
    unlock(a, b, c);
  }
  end_test_case();

  test_case("try_lock_all: on partial failure it ROLLS BACK and names the index");
  {
    ttas_lock a, b, c;
    b.lock();
    const int r = try_lock_all(a, b, c);
    require(r == 1);
    require_false(a.is_locked());
    require_true(b.is_locked());
    require_false(c.is_locked());
    b.unlock();
  }
  end_test_case();

  test_case("try_lock(...) rolls back on partial failure");
  {
    ttas_lock a, b;
    b.lock();
    require_false(micron::try_lock(a, b));
    require_false(a.is_locked());
    require_true(b.is_locked());
    b.unlock();
  }
  end_test_case();

  test_case("try_lock_in_order(...) is the old fold, and still leaks on partial failure");
  {
    ttas_lock a, b;
    b.lock();
    require_false(micron::try_lock_in_order(a, b));
    require_true(a.is_locked());
    a.unlock();
    b.unlock();
  }
  end_test_case();

  test_case("lock_all on a single lock, and on an empty pack");
  {
    ttas_lock a;
    lock_all(a);
    require_true(a.is_locked());
    a.unlock();
    lock_all();
    require_true(true);
  }
  end_test_case();

  test_case("lock_set acquires in its ctor and releases in its dtor");
  {
    ttas_lock a;
    ticket_lock b;
    mcs_lock c;
    {
      lock_set<ttas_lock, ticket_lock, mcs_lock> g(a, b, c);
      require_true(g.owns_lock());
      require_true(a.is_locked());
      require_true(b.is_locked());
      require_true(c.is_locked());
      g.unlock();
      require_false(g.owns_lock());
      require_false(a.is_locked());
      require_false(b.is_locked());
      require_false(c.is_locked());
    }
    require_false(a.is_locked());
  }
  end_test_case();

  test_case("lock_set with adopt_lock takes over locks already held");
  {
    ttas_lock a, b;
    a.lock();
    b.lock();
    {
      lock_set<ttas_lock, ttas_lock> g(adopt_lock, a, b);
      require_true(g.owns_lock());
    }
    require_false(a.is_locked());
    require_false(b.is_locked());
  }
  end_test_case();

  test_case("lock_all: opposing acquisition orders do not deadlock");
  {
    ttas_lock a, b;
    lcheck::watchdog wd;
    u64 rounds = 0;
    constexpr int kIters = 20000;

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(4000, 2); });

    mtest::parallel(2, [&](int tid) {
      for ( int i = 0; i < kIters; ++i ) {
        if ( tid == 0 )
          lock_all(a, b);
        else
          lock_all(b, a);
        ++rounds;
        wd.bump();
        unlock(a, b);
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    require(rounds == 2ull * kIters);
    require_false(a.is_locked());
    require_false(b.is_locked());
  }
  end_test_case();

  test_case("lock_all holds all of them at once: exclusion probe on each");
  {
    ttas_lock a, b, c;
    lcheck::exclusion_probe pa, pb, pc;
    constexpr int kIters = 5000;

    mtest::parallel(static_cast<int>(lcheck::wide_threads), [&](int tid) {
      for ( int i = 0; i < kIters; ++i ) {
        switch ( tid % 3 ) {
        case 0:
          lock_all(a, b, c);
          break;
        case 1:
          lock_all(c, a, b);
          break;
        default:
          lock_all(b, c, a);
          break;
        }
        pa.enter();
        pb.enter();
        pc.enter();
        pa.dwell(4);
        pc.leave();
        pb.leave();
        pa.leave();
        unlock(a, b, c);
      }
    });

    require(pa.violations.get(memory_order::acquire) == 0ull);
    require(pb.violations.get(memory_order::acquire) == 0ull);
    require(pc.violations.get(memory_order::acquire) == 0ull);
  }
  end_test_case();

  test_case("deadlock fuzz: random subsets, random orders, watchdog armed");
  {
    ttas_lock lk[kLocks];
    lcheck::watchdog wd;
    lcheck::exclusion_probe pr[kLocks];
    const u32 kT = lcheck::wide_threads;
    constexpr int kIters = 4000;

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(5000, kT); });

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u64 s = SEED_ML + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < kIters; ++i ) {

        const usize x = static_cast<usize>(lcheck::xs64(s) % kLocks);
        usize y = static_cast<usize>(lcheck::xs64(s) % kLocks);
        if ( y == x ) y = (y + 1) % kLocks;

        wd.note_want(static_cast<u32>(tid), &lk[x]);
        lock_all(lk[x], lk[y]);
        wd.note_hold(static_cast<u32>(tid), &lk[x]);

        pr[x].enter();
        pr[y].enter();
        pr[x].dwell(static_cast<u32>(lcheck::xs64(s) & 0xF));
        pr[y].leave();
        pr[x].leave();

        unlock(lk[x], lk[y]);
        wd.note_release(static_cast<u32>(tid));
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);

    require_true(wd.ok());
    for ( usize k = 0; k < kLocks; ++k ) {
      require(pr[k].violations.get(memory_order::acquire) == 0ull);
      require_false(lk[k].is_locked());
    }
    sb::print("     ", static_cast<usize>(kT), " threads x ", static_cast<usize>(kIters), " randomized 2-lock acquisitions, no stall");
  }
  end_test_case();

  test_case("order_graph flags the cycle a naive ordered acquire would leave behind");
  {
    lcheck::order_graph g;
    ttas_lock a, b;

    g.note(&a, &b);
    require_true(g.acyclic());
    g.note(&b, &a);
    require_false(g.acyclic());

    lcheck::order_graph g2;
    g2.note(&a, &b);
    g2.note(&a, &b);
    require_true(g2.acyclic());
  }
  end_test_case();

  test_case("order_graph over a real lock_all run: no cycle is recorded");
  {

    lcheck::order_graph g;
    ttas_lock a, b, c;

    mtest::parallel(3, [&](int tid) {
      for ( int i = 0; i < 2000; ++i ) {
        if ( tid == 0 ) {
          lock_all(a, b);
          unlock(a, b);
        } else if ( tid == 1 ) {
          lock_all(b, c);
          unlock(b, c);
        } else {
          lock_all(c, a);
          unlock(c, a);
        }
      }
    });

    require_true(g.acyclic());
    require_false(a.is_locked());
    require_false(b.is_locked());
    require_false(c.is_locked());
  }
  end_test_case();

  test_case("try_lock_all across mixed lock types");
  {
    ttas_lock a;
    ticket_lock b;
    futex_mutex c;
    require(try_lock_all(a, b, c) == -1);
    unlock(a, b, c);

    b.lock();
    require(try_lock_all(a, b, c) == 1);
    require_false(a.is_locked());
    require_false(c.is_locked());
    b.unlock();
  }
  end_test_case();

  sb::print("=== ALL MULTI-LOCK TESTS PASSED ===");
  return 1;
}
