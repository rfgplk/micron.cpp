//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// tests/rigor/mutex_* covers each lock's API and one contention case each; this drives every one of
// them for thousands of rounds and adds the three things none of those check:
//
//   1. LIFETIME of what the lock guards -- ltest::tracked counters on the payload, so a lock that
//      lets two threads construct or destroy the same object shows up as a live/faults imbalance
//      rather than only as a wrong total.
//   2. lockstep release -- ltest::barrier_wait puts every thread at the lock's door in the same
//      instant, which is the widest collision window available and is used by no lock test today.
//   3. RESOURCE recycling -- the process must come back to the same descriptor watermark and to
//      bounded resident growth, not to zero (abc's sheets are sticky by design).
//
// scales on --def STRESS_SCALE=N.

#define MICRON_ABC_MT 1
#define MICRON_LOCK_STATS 1

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

using guarded = ltest::tracked<0>;
using slot = ltest::tracked<1>;

ltest::live_registry g_reg;

constexpr u64 ROUNDS = 1200;
constexpr u64 SOAK = 400;
constexpr u64 CHURN = 300;

constexpr u64 SEED_ADV = 0xADEADBEEF5EEDULL;

struct shared_cell {
  guarded *obj = nullptr;
  u64 generation = 0;
};

template<typename Lock>
void
adversarial(const char *name, u32 threads, u64 rounds, u64 seed)
{
  Lock m;
  lcheck::exclusion_probe pr;
  lcheck::fairness_probe fp;
  lcheck::watchdog wd;
  shared_cell cell;
  ltest::barrier_t bar;
  bar.n = threads;

  guarded::reset();
  g_reg.reset();
  cell.obj = nullptr;
  cell.generation = 0;

  auto watcher = micron::solo::spawn<micron::auto_thread<>>([&]() { wd.watch(20000, threads); });

  mtest::parallel(static_cast<int>(threads), [&](int tid) {
    u32 sense = 0;
    u64 s = seed + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;

    for ( u64 i = 0; i < rounds; ++i ) {

      ltest::barrier_wait(bar, sense);

      m.lock();
      pr.enter();
      fp.note(static_cast<u32>(tid));

      guarded *fresh = new guarded(static_cast<i64>(cell.generation));
      guarded *old = cell.obj;
      cell.obj = fresh;
      ++cell.generation;

      pr.dwell(static_cast<u32>(lcheck::xs64(s) & 0x3F));

      pr.leave();
      m.unlock();

      delete old;
      wd.bump();
    }
  });

  wd.disarm();
  micron::solo::join(watcher);

  delete cell.obj;
  cell.obj = nullptr;

  require_true(wd.ok());
  require(pr.violations.get(micron::memory_order::acquire), static_cast<u64>(0));
  require(pr.entries.get(micron::memory_order::acquire), static_cast<u64>(threads) * rounds);
  require(cell.generation, static_cast<u64>(threads) * rounds);
  require(guarded::live(), static_cast<i64>(0));
  require(guarded::faults(), static_cast<u64>(0));
  require(g_reg.collisions.get(micron::memory_order::acquire), static_cast<u64>(0));
  require_false(m.is_locked());
  require_true(fp.least(threads) > 0);

  if constexpr ( requires(const Lock &l) { l.stats(); } ) {
    sb::print("     ", name, " rounds=", static_cast<usize>(threads * rounds), " born=", static_cast<usize>(guarded::born()),
              " spread(x100)=", static_cast<usize>(fp.spread(threads) * 100.0), " spins=", static_cast<usize>(m.stats().spins()),
              " parks=", static_cast<usize>(m.stats().parks()));
  } else {
    sb::print("     ", name, " rounds=", static_cast<usize>(threads * rounds), " born=", static_cast<usize>(guarded::born()),
              " spread(x100)=", static_cast<usize>(fp.spread(threads) * 100.0));
  }
}

template<typename Lock>
void
recycle(const char *name, u32 threads, u64 rounds)
{
  micron::atomic_token<u64> total{ 0 };
  lcheck::watchdog wd;

  auto watcher = micron::solo::spawn<micron::auto_thread<>>([&]() { wd.watch(20000, threads); });

  for ( u64 r = 0; r < rounds; ++r ) {
    Lock m;
    lcheck::exclusion_probe pr;
    mtest::parallel(static_cast<int>(threads), [&](int) {
      for ( int i = 0; i < 40; ++i ) {
        m.lock();
        pr.enter();
        pr.dwell(4);
        total.fetch_add(1, micron::memory_order::relaxed);
        pr.leave();
        m.unlock();
        wd.bump();
      }
    });
    require(pr.violations.get(micron::memory_order::acquire), static_cast<u64>(0));
    require_false(m.is_locked());
  }

  wd.disarm();
  micron::solo::join(watcher);
  require_true(wd.ok());
  require(total.get(micron::memory_order::acquire), static_cast<u64>(threads) * rounds * 40ull);
  sb::print("     ", name, " recycled ", static_cast<usize>(rounds), " lock instances, ",
            static_cast<usize>(total.get(micron::memory_order::acquire)), " acquisitions");
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== LOCK ADVERSARIAL SOAK ===");
  sb::print("    rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  soak: ", static_cast<usize>(ltest::scaled(SOAK)),
            "  churn: ", static_cast<usize>(ltest::scaled(CHURN)), "  scale: ", static_cast<usize>(ltest::stress_scale));

  if ( !micron::threads_available() ) {
    sb::print("threads unavailable on this build/kernel - SKIPPED");
    return 1;
  }

  guarded::reg = &g_reg;

  const i32 wm0 = ltest::fd_watermark();
  const u64 rss0 = ltest::rss_kb();

  const u32 kT = lcheck::wide_threads;
  const u32 kO = lcheck::over_threads;

  test_case("lockstep adversarial rounds, every lock type, lifetime-accounted");
  {
    const u64 n = ltest::scaled(ROUNDS);
    adversarial<mutex>("mutex       ", kT, n, SEED_ADV);
    adversarial<weak_mutex>("weak_mutex  ", kT, n, SEED_ADV + 1);
    adversarial<fast_mutex>("fast_mutex  ", kT, n, SEED_ADV + 2);
    adversarial<spin_lock>("spin_lock   ", kT, n, SEED_ADV + 3);
    adversarial<recursive_lock>("recursive   ", kT, n, SEED_ADV + 4);
    adversarial<ttas_lock>("ttas_lock   ", kT, n, SEED_ADV + 5);
    adversarial<ticket_lock>("ticket_lock ", kT, n, SEED_ADV + 6);
    adversarial<mcs_lock>("mcs_lock    ", kT, n, SEED_ADV + 7);
    adversarial<clh_lock>("clh_lock    ", kT, n, SEED_ADV + 8);
    adversarial<futex_mutex>("futex_mutex ", kT, n, SEED_ADV + 9);
    adversarial<shared_mutex>("shared_mutex", kT, n, SEED_ADV + 10);
  }
  end_test_case();

  test_case("oversubscribed: the parking locks under more threads than cores");
  {
    const u64 n = ltest::scaled(SOAK);
    adversarial<futex_mutex>("futex_mutex ", kO, n, SEED_ADV + 20);
    adversarial<ticket_lock>("ticket_lock ", kO, n, SEED_ADV + 21);
    adversarial<mcs_lock>("mcs_lock    ", kO, n, SEED_ADV + 22);
    adversarial<ttas_lock>("ttas_lock   ", kO, n, SEED_ADV + 23);
  }
  end_test_case();

  test_case("lock instances recycled at the same addresses");
  {

    const u64 n = ltest::scaled(CHURN);
    recycle<mcs_lock>("mcs_lock    ", kT, n);
    recycle<clh_lock>("clh_lock    ", kT, n);
    recycle<ticket_lock>("ticket_lock ", kT, n);
    recycle<futex_mutex>("futex_mutex ", kT, n);
  }
  end_test_case();

  test_case("multi-lock acquisition under the same lockstep pressure");
  {
    constexpr usize kLocks = 4;
    ttas_lock lk[kLocks];
    lcheck::exclusion_probe pr[kLocks];
    lcheck::watchdog wd;
    ltest::barrier_t bar;
    bar.n = kT;
    const u64 n = ltest::scaled(SOAK);

    auto watcher = solo::spawn<auto_thread<>>([&]() { wd.watch(20000, kT); });

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u32 sense = 0;
      u64 s = SEED_ADV + 30 + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < n; ++i ) {
        ltest::barrier_wait(bar, sense);
        const usize x = static_cast<usize>(lcheck::xs64(s) % kLocks);
        usize y = static_cast<usize>(lcheck::xs64(s) % kLocks);
        if ( y == x ) y = (y + 1) % kLocks;

        lock_all(lk[x], lk[y]);
        pr[x].enter();
        pr[y].enter();
        pr[x].dwell(static_cast<u32>(lcheck::xs64(s) & 0x1F));
        pr[y].leave();
        pr[x].leave();
        unlock(lk[x], lk[y]);
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
    sb::print("     lock_all lockstep rounds=", static_cast<usize>(kT * n));
  }
  end_test_case();

  test_case("reader/writer mix under lockstep pressure");
  {
    shared_mutex m;
    lcheck::exclusion_probe wr;
    micron::atomic_token<u32> writer_active{ 0 };
    micron::atomic_token<u64> readers_during_write{ 0 };
    ltest::barrier_t bar;
    bar.n = kT;
    const u64 n = ltest::scaled(SOAK);

    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u32 sense = 0;
      u64 s = SEED_ADV + 40 + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < n; ++i ) {
        ltest::barrier_wait(bar, sense);
        if ( (lcheck::xs64(s) & 3u) == 0u ) {
          m.lock();
          wr.enter();
          writer_active.store(1, memory_order::release);
          wr.dwell(32);
          writer_active.store(0, memory_order::release);
          wr.leave();
          m.unlock();
        } else {
          m.lock_shared();
          if ( writer_active.get(memory_order::acquire) != 0u ) readers_during_write.fetch_add(1, memory_order::acq_rel);
          for ( u32 k = 0; k < 16; ++k ) __cpu_pause();
          m.unlock_shared();
        }
      }
    });

    require(wr.violations.get(memory_order::acquire), static_cast<u64>(0));
    require(readers_during_write.get(memory_order::acquire), static_cast<u64>(0));
    require_false(m.is_locked());
    sb::print("     rw mix rounds=", static_cast<usize>(kT * n),
              " writer sections=", static_cast<usize>(wr.entries.get(memory_order::acquire)));
  }
  end_test_case();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  const u64 rss1 = ltest::rss_kb();
  if ( rss0 != 0 and rss1 != 0 ) {
    const u64 bound = rss0 + (256u * 1024u);
    if ( rss1 > bound ) sb::print("     rss grew ", static_cast<usize>(rss1 - rss0), " KiB");
    require_true(rss1 <= bound);
    sb::print("     rss ", static_cast<usize>(rss0), " -> ", static_cast<usize>(rss1), " KiB (bound ", static_cast<usize>(bound), ")");
  } else {
    sb::print("     /proc unreadable - rss growth check skipped");
  }

  sb::print("=== ALL LOCK ADVERSARIAL TESTS PASSED ===");
  return 1;
}
