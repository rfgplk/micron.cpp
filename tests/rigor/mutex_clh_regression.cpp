//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1
#define MICRON_LOCK_STATS 1

#ifndef CLH_REG_SCALE
#define CLH_REG_SCALE 1
#endif

#include "../../src/mutex/locks/clh_lock.hpp"

#include "../../src/concepts.hpp"
#include "../../src/std.hpp"

#include "../../src/memory/placement_new.hpp"
#include "../../src/mutex/locks.hpp"

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

#if !defined(__micron_freestanding) || defined(__micron_eh)
#define CLH_REG_EH 1
#endif

namespace
{

constexpr u64 SEED_CLHR = 0xC1BADA55F15ULL;

constexpr u32 kRotLocks = 9;

constexpr int kRotations = 400 * CLH_REG_SCALE;

template<typename Fn>
void
watched(u64 stall_ms, u32 threads, Fn fn)
{
  lcheck::watchdog wd;
  auto watcher = micron::solo::spawn<micron::auto_thread<>>([&]() { wd.watch(stall_ms, threads); });
  fn(wd);
  wd.disarm();
  micron::solo::join(watcher);
  require_true(wd.ok());
}

template<typename Lock>
void
stray_release_is_a_noop()
{
  Lock m;
  m.unlock();
  require_false(m.is_locked());

  m.lock();
  require_true(m.is_locked());
  m.unlock();
  m.unlock();
  require_false(m.is_locked());

  require_true(m.try_lock());
  require_true(m.is_locked());
  m.unlock();
  require_false(m.is_locked());
}

}      // namespace

static_assert(micron::is_mutex<micron::clh_lock>, "clh_lock must satisfy is_mutex");

int
main(void)
{
  using namespace micron;
  sb::print("=== CLH_LOCK / LOCK-FAMILY REGRESSION TESTS ===");

  test_case("one thread rotating over more clh_locks than the slot table holds never runs dry");
  {

    clh_lock m[kRotLocks];
    u64 total = 0;
    bool threw = false;

    watched(20000, 1, [&](lcheck::watchdog &wd) {
#if defined(CLH_REG_EH)
      try {
#endif
        for ( int r = 0; r < kRotations; ++r ) {
          for ( u32 i = 0; i < kRotLocks; ++i ) {
            m[i].lock();
            ++total;
            m[i].unlock();
          }
          wd.bump();
        }
#if defined(CLH_REG_EH)
      } catch ( ... ) {
        threw = true;
      }
#endif
    });

    require_false(threw);
    require(total == static_cast<u64>(kRotations) * kRotLocks);
    for ( u32 i = 0; i < kRotLocks; ++i ) {
      require_false(m[i].is_locked());
      require(m[i].participants() == 1u);
      require(m[i].stats().acquires() == static_cast<u64>(kRotations));
    }
  }
  end_test_case();

  test_case("a thread that EXITS leaves no arena node behind");
  {

    clh_lock m;
    constexpr int kThreads = 64 * CLH_REG_SCALE;
    constexpr int kEach = 8;
    atomic_token<u64> total(0);
    atomic_token<u32> failed(0);

    watched(20000, 1, [&](lcheck::watchdog &wd) {
      for ( int i = 0; i < kThreads; ++i ) {

        auto t = solo::spawn<auto_thread<>>([&]() {
#if defined(CLH_REG_EH)
          try {
#endif
            for ( int k = 0; k < kEach; ++k ) {
              m.lock();
              total.fetch_add(1, memory_order::relaxed);
              m.unlock();
            }
#if defined(CLH_REG_EH)
          } catch ( ... ) {
            failed.fetch_add(1, memory_order::acq_rel);
          }
#endif
        });
        solo::join(t);
        wd.bump();
      }
    });

    require(failed.get(memory_order::acquire) == 0u);
    require(total.get(memory_order::acquire) == static_cast<u64>(kThreads) * kEach);
    require(m.participants() == 1u);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case(" a narrow arena survives churn far past its own width");
  {

    basic_clh_lock<2> m;
    constexpr int kThreads = 24 * CLH_REG_SCALE;
    atomic_token<u64> total(0);
    atomic_token<u32> failed(0);

    watched(20000, 1, [&](lcheck::watchdog &wd) {
      for ( int i = 0; i < kThreads; ++i ) {
        auto t = solo::spawn<auto_thread<>>([&]() {
#if defined(CLH_REG_EH)
          try {
#endif
            for ( int k = 0; k < 32; ++k ) {
              m.lock();
              total.fetch_add(1, memory_order::relaxed);
              m.unlock();
            }
#if defined(CLH_REG_EH)
          } catch ( ... ) {
            failed.fetch_add(1, memory_order::acq_rel);
          }
#endif
        });
        solo::join(t);
        wd.bump();
      }
    });

    require(failed.get(memory_order::acquire) == 0u);
    require(total.get(memory_order::acquire) == static_cast<u64>(kThreads) * 32);
    require(m.participants() == 1u);
  }
  end_test_case();

  test_case("under concurrency: threads rotating over many locks, watchdogged");
  {
    clh_lock m[kRotLocks];
    lcheck::exclusion_probe pr[kRotLocks];
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 3000 * CLH_REG_SCALE;
    atomic_token<u32> failed(0);

    watched(30000, static_cast<u32>(kT), [&](lcheck::watchdog &wd) {
      lcheck::start_gate gate(static_cast<u32>(kT));
      mtest::parallel(kT, [&](int tid) {
        u64 s = SEED_CLHR ^ (static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL);
        u32 sense = 0;
        gate.wait(sense);
#if defined(CLH_REG_EH)
        try {
#endif
          for ( int i = 0; i < kIters; ++i ) {
            const u32 idx = static_cast<u32>(lcheck::xs64(s) % kRotLocks);
            wd.note_want(static_cast<u32>(tid), &m[idx]);
            m[idx].lock();
            wd.note_hold(static_cast<u32>(tid), &m[idx]);
            pr[idx].enter();
            pr[idx].dwell(4);
            pr[idx].leave();
            m[idx].unlock();
            wd.note_release(static_cast<u32>(tid));
            wd.bump();
          }
#if defined(CLH_REG_EH)
        } catch ( ... ) {
          failed.fetch_add(1, memory_order::acq_rel);
        }
#endif
      });
    });

    require(failed.get(memory_order::acquire) == 0u);
    u64 entries = 0;
    for ( u32 i = 0; i < kRotLocks; ++i ) {
      require(pr[i].violations.get(memory_order::acquire) == 0ull);
      require_true(pr[i].clean());
      require_false(m[i].is_locked());
      require_true(m[i].participants() >= 1u);
      require_true(m[i].participants() <= static_cast<u32>(kT));
      entries += pr[i].entries.get(memory_order::acquire);
    }
    require(entries == static_cast<u64>(kT) * kIters);
  }
  end_test_case();

  test_case("a clh_lock built over a destroyed held one inherits nothing");
  {

    alignas(clh_lock) unsigned char storage[sizeof(clh_lock)];
    atomic_token<u32> failed(0);

    watched(20000, 1, [&](lcheck::watchdog &wd) {
      clh_lock *p = new (storage) clh_lock();

      auto t = solo::spawn<auto_thread<>>([&]() { p->lock(); });
      solo::join(t);
      require_true(p->is_locked());
      wd.bump();

      p->~clh_lock();
      p = new (storage) clh_lock();

      require_false(p->is_locked());
      require(p->participants() == 0u);
      p->unlock();
      require_false(p->is_locked());
      wd.bump();

      require_true(p->try_lock());
      p->unlock();
      for ( int i = 0; i < 64; ++i ) {
        p->lock();
        p->unlock();
      }
      require_false(p->is_locked());
      require(p->participants() == 1u);
      require(p->stats().acquires() == 65ull);
      wd.bump();

      auto t2 = solo::spawn<auto_thread<>>([&]() {
        for ( int i = 0; i < 64; ++i ) {
          p->lock();
          p->unlock();
        }
      });
      solo::join(t2);
      require_false(p->is_locked());
      p->~clh_lock();
    });

    require(failed.get(memory_order::acquire) == 0u);
  }
  end_test_case();

  test_case("clh_lock: A acquires, B releases, C then acquires");
  {
    clh_lock m;
    atomic_token<bool> got(false);

    watched(15000, 3, [&](lcheck::watchdog &wd) {
      auto a = solo::spawn<auto_thread<>>([&]() { m.lock(); });
      solo::join(a);
      require_true(m.is_locked());
      wd.bump();

      auto b = solo::spawn<auto_thread<>>([&]() { m.unlock(); });
      solo::join(b);
      require_false(m.is_locked());
      wd.bump();

      auto c = solo::spawn<auto_thread<>>([&]() {
        m.lock();
        got.store(true, memory_order::release);
        m.unlock();
      });
      solo::join(c);
      wd.bump();
    });

    require_true(got.get(memory_order::acquire));
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("clh_lock: stray unlock and a second sequenced unlock are both no-ops");
  {
    clh_lock m;
    watched(15000, 1, [&](lcheck::watchdog &wd) {
      m.unlock();
      require_false(m.is_locked());
      wd.bump();

      m.lock();
      require_true(m.is_locked());
      m.unlock();
      m.unlock();
      require_false(m.is_locked());
      wd.bump();

      for ( int i = 0; i < 128; ++i ) {
        m.lock();
        m.unlock();
      }
      wd.bump();
    });
    require_false(m.is_locked());
    require(m.participants() == 1u);
  }
  end_test_case();

  test_case("clh_lock try_lock storm: a republished node never yields two holders");
  {

    clh_lock m;
    lcheck::exclusion_probe pr;
    const int kT = static_cast<int>(lcheck::wide_threads);
    constexpr int kIters = 6000 * CLH_REG_SCALE;
    atomic_token<u64> taken(0);

    watched(30000, static_cast<u32>(kT), [&](lcheck::watchdog &wd) {
      lcheck::start_gate gate(static_cast<u32>(kT));
      mtest::parallel(kT, [&](int tid) {
        u32 sense = 0;
        gate.wait(sense);
        for ( int i = 0; i < kIters; ++i ) {
          if ( (tid & 1) == 0 ) {
            m.lock();
            pr.enter();
            pr.dwell(2);
            pr.leave();
            m.unlock();
            taken.fetch_add(1, memory_order::relaxed);
          } else if ( m.try_lock() ) {
            pr.enter();
            pr.dwell(2);
            pr.leave();
            m.unlock();
            taken.fetch_add(1, memory_order::relaxed);
          }
          wd.bump();
        }
      });
    });

    require(pr.violations.get(memory_order::acquire) == 0ull);
    require_true(pr.clean());
    require(pr.entries.get(memory_order::acquire) == taken.get(memory_order::acquire));
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("ticket_lock: a stray unlock does not push serving past next");
  {
    ticket_lock t;
    watched(15000, 1, [&](lcheck::watchdog &wd) {
      t.unlock();
      require_false(t.is_locked());
      require(t.queued() == 0u);
      require(t.serving() == 0u);
      wd.bump();

      t.lock();
      require_true(t.is_locked());
      t.unlock();
      require_false(t.is_locked());
      wd.bump();
    });
  }
  end_test_case();

  test_case("ticket_lock: an adopt guard over an already-released lock leaves it usable");
  {
    ticket_lock t;
    watched(15000, 2, [&](lcheck::watchdog &wd) {
      t.lock();
      {
        lock_guard<ticket_lock> g(t, adopt_lock);
        t.unlock();
      }
      require_false(t.is_locked());
      require(t.queued() == 0u);
      wd.bump();

      u64 total = 0;
      const int kT = 4;
      constexpr int kIters = 2000 * CLH_REG_SCALE;
      lcheck::start_gate gate(static_cast<u32>(kT));
      mtest::parallel(kT, [&](int) {
        u32 sense = 0;
        gate.wait(sense);
        for ( int i = 0; i < kIters; ++i ) {
          t.lock();
          ++total;
          t.unlock();
          wd.bump();
        }
      });
      require(total == static_cast<u64>(kT) * kIters);
    });
    require_false(t.is_locked());
  }
  end_test_case();

  test_case("shared_mutex: a stray unlock_shared does not borrow into the writer bit");
  {
    shared_mutex sm;
    watched(15000, 1, [&](lcheck::watchdog &wd) {
      sm.unlock_shared();
      require(sm.readers() == 0u);
      require_false(sm.is_writer_held());
      require_false(sm.is_locked());
      require(sm.writers_queued() == 0u);
      wd.bump();

      require_true(sm.try_lock());
      require_true(sm.is_writer_held());
      sm.unlock();
      require_false(sm.is_locked());

      require_true(sm.try_lock_shared());
      require(sm.readers() == 1u);
      sm.unlock_shared();
      require(sm.readers() == 0u);
      wd.bump();
    });
  }
  end_test_case();

  test_case("shared_mutex: one unlock_shared too many after the last reader leaves it usable");
  {
    shared_mutex sm;
    watched(15000, 3, [&](lcheck::watchdog &wd) {
      sm.lock_shared();
      sm.lock_shared();
      require(sm.readers() == 2u);
      sm.unlock_shared();
      sm.unlock_shared();
      sm.unlock_shared();
      require(sm.readers() == 0u);
      require_false(sm.is_locked());
      wd.bump();

      atomic_token<u64> reads(0);
      u64 writes = 0;
      constexpr int kIters = 400 * CLH_REG_SCALE;
      mtest::parallel(3, [&](int tid) {
        for ( int i = 0; i < kIters; ++i ) {
          if ( tid == 0 ) {
            sm.lock();
            ++writes;
            sm.unlock();
          } else {
            sm.lock_shared();
            reads.fetch_add(1, memory_order::relaxed);
            sm.unlock_shared();
          }
          wd.bump();
        }
      });
      require(writes == static_cast<u64>(kIters));
      require(reads.get(memory_order::acquire) == 2ull * kIters);
    });
    require_false(sm.is_locked());
    require(sm.readers() == 0u);
  }
  end_test_case();

  test_case("stray and double release are no-ops across the whole family");
  {
    watched(15000, 1, [&](lcheck::watchdog &wd) {
      stray_release_is_a_noop<mutex>();
      wd.bump();
      stray_release_is_a_noop<weak_mutex>();
      wd.bump();
      stray_release_is_a_noop<fast_mutex>();
      wd.bump();
      stray_release_is_a_noop<spin_lock>();
      wd.bump();
      stray_release_is_a_noop<ttas_lock>();
      wd.bump();
      stray_release_is_a_noop<futex_mutex>();
      wd.bump();
      stray_release_is_a_noop<ticket_lock>();
      wd.bump();
      stray_release_is_a_noop<clh_lock>();
      wd.bump();
    });
  }
  end_test_case();

  sb::print("=== ALL CLH_LOCK REGRESSION TESTS PASSED ===");
  return 1;
}
