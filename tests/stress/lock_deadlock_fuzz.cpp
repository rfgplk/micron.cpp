//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// deadlock fuzzing, with the point being that a run which does NOT deadlock proves nothing on its
// own. so there are two graders here:
//
//   watchdog     -- catches an actual stall, and fails with a per-thread hold/want dump rather than
//                   letting duck's --timeout report a bare 124 with no location.
//   order_graph  -- records every (held, wanted) pair any thread ever produced and looks for a
//                   CYCLE afterwards. that finds a latent lock-order inversion even in a run where
//                   the interleaving happened never to close it.
//
// the naive fixed-order path is run in a DIAGNOSTIC case: it reports the inversions it creates
// without asserting them away, because taking locks in a fixed order is the documented contract of
// lock_in_order() (which is what micron::lock() used to be -- the unqualified spelling now routes to
// lock_all). lock_all is the one held to the deadlock-free standard.
//
// scales on --def STRESS_SCALE=N.

#define MICRON_ABC_MT 1

#include "../../src/mutex/locks.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"
#include "../../src/thread/threads.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

#include "../support/lockcheck.hpp"

using namespace snowball;

namespace
{

constexpr u64 ROUNDS = 6000;
constexpr u64 DEEP = 1500;

constexpr u64 SEED_DF = 0xDEAD10CCF0221ULL;

constexpr usize kLocks = 6;
constexpr usize kMaxHeld = 4;

void
shuffle(usize *a, usize n, u64 &s) noexcept
{
  for ( usize i = n; i > 1; --i ) {
    const usize j = static_cast<usize>(lcheck::xs64(s) % i);
    const usize t = a[i - 1];
    a[i - 1] = a[j];
    a[j] = t;
  }
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== LOCK DEADLOCK FUZZ ===");
  sb::print("    rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  deep: ", static_cast<usize>(ltest::scaled(DEEP)),
            "  locks: ", static_cast<usize>(kLocks), "  scale: ", static_cast<usize>(ltest::stress_scale));

  if ( !micron::threads_available() ) {
    sb::print("threads unavailable on this build/kernel - SKIPPED");
    return 1;
  }

  const i32 wm0 = ltest::fd_watermark();
  const u32 kT = lcheck::wide_threads;

  test_case("lock_all over randomized subsets and orders never stalls");
  {
    ttas_lock lk[kLocks];
    lcheck::exclusion_probe pr[kLocks];
    lcheck::watchdog wd;
    lcheck::start_gate gate(kT);
    const u64 n = ltest::scaled(ROUNDS);
    atomic_token<u64> acquisitions{ 0 };

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(20000, kT); });

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      u64 s = SEED_DF + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;

      for ( u64 i = 0; i < n; ++i ) {
        usize idx[kLocks];
        for ( usize k = 0; k < kLocks; ++k ) idx[k] = k;
        shuffle(idx, kLocks, s);
        const usize take = 2 + static_cast<usize>(lcheck::xs64(s) % (kMaxHeld - 1));

        wd.note_want(static_cast<u32>(tid), &lk[idx[0]]);
        switch ( take ) {
        case 2:
          lock_all(lk[idx[0]], lk[idx[1]]);
          break;
        case 3:
          lock_all(lk[idx[0]], lk[idx[1]], lk[idx[2]]);
          break;
        default:
          lock_all(lk[idx[0]], lk[idx[1]], lk[idx[2]], lk[idx[3]]);
          break;
        }
        wd.note_hold(static_cast<u32>(tid), &lk[idx[0]]);

        for ( usize k = 0; k < take; ++k ) pr[idx[k]].enter();
        pr[idx[0]].dwell(static_cast<u32>(lcheck::xs64(s) & 0x1F));
        for ( usize k = take; k-- > 0; ) pr[idx[k]].leave();

        switch ( take ) {
        case 2:
          unlock(lk[idx[0]], lk[idx[1]]);
          break;
        case 3:
          unlock(lk[idx[0]], lk[idx[1]], lk[idx[2]]);
          break;
        default:
          unlock(lk[idx[0]], lk[idx[1]], lk[idx[2]], lk[idx[3]]);
          break;
        }
        wd.note_release(static_cast<u32>(tid));
        acquisitions.fetch_add(take, memory_order::relaxed);
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);

    require_true(wd.ok());
    for ( usize k = 0; k < kLocks; ++k ) {
      require(pr[k].violations.get(memory_order::acquire), static_cast<u64>(0));
      require_false(lk[k].is_locked());
    }
    sb::print("     lock_all: ", static_cast<usize>(kT * n), " rounds, ", static_cast<usize>(acquisitions.get(memory_order::acquire)),
              " lock acquisitions, no stall");
  }
  end_test_case();

  test_case("lock_all across MIXED lock types, randomized order");
  {

    ttas_lock a;
    ticket_lock b;
    mcs_lock c;
    futex_mutex d;
    clh_lock e;
    lcheck::watchdog wd;
    lcheck::start_gate gate(kT);
    const u64 n = ltest::scaled(DEEP);
    atomic_token<u64> rounds{ 0 };

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(20000, kT); });

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      u64 s = SEED_DF + 0x100 + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < n; ++i ) {
        switch ( lcheck::xs64(s) % 5u ) {
        case 0:
          lock_all(a, b, c);
          unlock(a, b, c);
          break;
        case 1:
          lock_all(c, b, a);
          unlock(c, b, a);
          break;
        case 2:
          lock_all(d, e);
          unlock(d, e);
          break;
        case 3:
          lock_all(e, d, a);
          unlock(e, d, a);
          break;
        default:
          lock_all(b, e, c, d);
          unlock(b, e, c, d);
          break;
        }
        rounds.fetch_add(1, memory_order::relaxed);
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    require(rounds.get(memory_order::acquire), static_cast<u64>(kT) * n);
    require_false(a.is_locked());
    require_false(b.is_locked());
    require_false(c.is_locked());
    require_false(d.is_locked());
    require_false(e.is_locked());
    sb::print("     mixed-type lock_all: ", static_cast<usize>(kT * n), " rounds, no stall");
  }
  end_test_case();

  test_case("try_lock_all always rolls back: no lock is ever left held on a refusal");
  {
    ttas_lock lk[kLocks];
    lcheck::start_gate gate(kT);
    const u64 n = ltest::scaled(ROUNDS);
    atomic_token<u64> ok{ 0 };
    atomic_token<u64> refused{ 0 };
    atomic_token<u64> leaked{ 0 };

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      u64 s = SEED_DF + 0x200 + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < n; ++i ) {
        usize idx[kLocks];
        for ( usize k = 0; k < kLocks; ++k ) idx[k] = k;
        shuffle(idx, kLocks, s);

        const int r = try_lock_all(lk[idx[0]], lk[idx[1]], lk[idx[2]]);
        if ( r == -1 ) {
          ok.fetch_add(1, memory_order::relaxed);
          unlock(lk[idx[0]], lk[idx[1]], lk[idx[2]]);
        } else {
          refused.fetch_add(1, memory_order::relaxed);

          if ( r < 0 or r > 2 ) leaked.fetch_add(1, memory_order::relaxed);
        }
      }
    });

    sb::print("     try_lock_all: ok=", static_cast<usize>(ok.get(memory_order::acquire)),
              " refused=", static_cast<usize>(refused.get(memory_order::acquire)));
    require(leaked.get(memory_order::acquire), static_cast<u64>(0));
    require((ok.get(memory_order::acquire) + refused.get(memory_order::acquire)), static_cast<u64>(kT) * ltest::scaled(ROUNDS));

    for ( usize k = 0; k < kLocks; ++k ) require_false(lk[k].is_locked());

    require_true(refused.get(memory_order::acquire) > 0ull);
  }
  end_test_case();

  test_case("order_graph over lock_all records no cycle (it never blocks while holding)");
  {
    lcheck::order_graph g;
    ttas_lock lk[kLocks];
    lcheck::start_gate gate(kT);
    const u64 n = ltest::scaled(DEEP);

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      u64 s = SEED_DF + 0x300 + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < n; ++i ) {
        usize idx[kLocks];
        for ( usize k = 0; k < kLocks; ++k ) idx[k] = k;
        shuffle(idx, kLocks, s);

        lock_all(lk[idx[0]], lk[idx[1]]);
        unlock(lk[idx[0]], lk[idx[1]]);
      }
    });

    require_true(g.acyclic());
    require(g.edges.get(memory_order::acquire), static_cast<u64>(0));
    sb::print("     order_graph over lock_all: ", static_cast<usize>(g.edges.get(memory_order::acquire)), " edges, acyclic");
  }
  end_test_case();

  test_case("DIAGNOSTIC: the naive fixed-order path does produce a cyclic order graph");
  {

    lcheck::order_graph g;
    ttas_lock a, b;
    lcheck::start_gate gate(2);
    const u64 n = ltest::scaled(DEEP);

    mtest::parallel(2, [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      for ( u64 i = 0; i < n; ++i ) {

        ttas_lock &first = (tid == 0) ? a : b;
        ttas_lock &second = (tid == 0) ? b : a;
        while ( !first.try_lock() ) micron::yield();
        g.note(&first, &second);

        if ( second.try_lock() ) second.unlock();
        first.unlock();
      }
    });

    const bool cyclic = !g.acyclic();
    sb::print("     naive fixed order: edges=", static_cast<usize>(g.edges.get(memory_order::acquire)),
              " cyclic=", cyclic ? "YES (as expected)" : "no");
    require_true(cyclic);
    require_false(a.is_locked());
    require_false(b.is_locked());
  }
  end_test_case();

  test_case("lock_set under the same fuzz, RAII release on every path");
  {
    ttas_lock a;
    ticket_lock b;
    mcs_lock c;
    lcheck::watchdog wd;
    lcheck::start_gate gate(kT);
    const u64 n = ltest::scaled(DEEP);

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(20000, kT); });

    mtest::parallel(static_cast<int>(kT), [&](int) {
      u32 sense = 0;
      gate.wait(sense);
      for ( u64 i = 0; i < n; ++i ) {
        {
          lock_set<ttas_lock, ticket_lock, mcs_lock> g(a, b, c);
          for ( u32 k = 0; k < 8; ++k ) __cpu_pause();
        }
        wd.bump();
      }
    });

    wd.disarm();
    solo::join(watcher);
    require_true(wd.ok());
    require_false(a.is_locked());
    require_false(b.is_locked());
    require_false(c.is_locked());
  }
  end_test_case();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  sb::print("=== ALL DEADLOCK FUZZ TESTS PASSED ===");
  return 1;
}
