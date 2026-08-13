//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// the first reader-writer lock in the tree that a real thread can use -- coro::async_rwlock is a
// coroutine primitive and blocks nobody. the two properties that have to be shown, because a
// counter total shows neither: readers really do run CONCURRENTLY (an rwlock that serialised them
// would pass every total-based check), and a writer is not starved by a reader storm.

#define MICRON_ABC_MT 1      // spawns threads/coroutines; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/locks/shared_mutex.hpp"

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
constexpr u64 SEED_RW = 0x5A5EDF00DBEEFULL;
}

static_assert(micron::is_mutex<micron::shared_mutex>, "shared_mutex must satisfy is_mutex");

int
main(void)
{
  using namespace micron;
  sb::print("=== SHARED_MUTEX TESTS ===");

  test_case("default ctor: no writer, no readers, no writer queued");
  {
    shared_mutex m;
    require_false(m.is_locked());
    require_false(m.is_writer_held());
    require(m.readers() == 0u);
    require(m.writers_queued() == 0u);
  }
  end_test_case();

  test_case("exclusive lock / unlock roundtrip");
  {
    shared_mutex m;
    m.lock();
    require_true(m.is_locked());
    require_true(m.is_writer_held());
    require(m.readers() == 0u);
    m.unlock();
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("shared lock / unlock roundtrip, reader count is visible");
  {
    shared_mutex m;
    m.lock_shared();
    require(m.readers() == 1u);
    require_false(m.is_writer_held());
    require_true(m.is_locked());      // held, just not exclusively
    m.lock_shared();
    require(m.readers() == 2u);
    m.unlock_shared();
    m.unlock_shared();
    require(m.readers() == 0u);
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("try_lock / try_lock_shared refuse the incompatible states");
  {
    shared_mutex m;
    require_true(m.try_lock());
    require_false(m.try_lock());
    require_false(m.try_lock_shared());      // a writer holds it
    m.unlock();

    require_true(m.try_lock_shared());
    require_true(m.try_lock_shared());       // readers share freely
    require_false(m.try_lock());             // but a writer cannot get in
    m.unlock_shared();
    require_false(m.try_lock());             // still one reader
    m.unlock_shared();
    require_true(m.try_lock());
    m.unlock();
  }
  end_test_case();

  test_case("operator()(void) returns the fn-ptr; dispatch unlocks the writer side");
  {
    shared_mutex m;
    auto r = m();
    require_true(m.is_writer_held());
    (m.*r)();
    require_false(m.is_locked());
  }
  end_test_case();

  // ---- readers must actually be concurrent ----
  test_case("readers run concurrently: more than one inside at the same instant");
  {
    shared_mutex m;
    atomic_token<u32> inside(0);
    atomic_token<u32> peak(0);
    const int kT = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kT, [&](int) {
      for ( int i = 0; i < 200; ++i ) {
        m.lock_shared();
        const u32 n = inside.add_fetch(1, memory_order::acq_rel);
        u32 p = peak.get(memory_order::relaxed);
        while ( n > p ) {
          if ( peak.compare_exchange_weak(p, n, memory_order::acq_rel, memory_order::relaxed) ) break;
        }
        for ( u32 k = 0; k < 512; ++k ) __cpu_pause();      // overlap has to be wide enough to see
        inside.sub_fetch(1, memory_order::acq_rel);
        m.unlock_shared();
      }
    });

    sb::print("     peak concurrent readers: ", static_cast<usize>(peak.get(memory_order::acquire)), " of ",
              static_cast<usize>(kT));
    require(inside.get(memory_order::acquire) == 0u);
    require_true(peak.get(memory_order::acquire) > 1u);      // else this is just an expensive mutex
    require(m.readers() == 0u);
  }
  end_test_case();

  // ---- a writer must exclude everyone ----
  test_case("a writer excludes every reader and every other writer");
  {
    shared_mutex m;
    lcheck::exclusion_probe wr;      // counts only writers
    atomic_token<u32> readers_seen_during_write(0);
    atomic_token<u32> writer_active(0);
    u64 shared_value = 0;
    const int kT = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kT, [&](int tid) {
      u64 s = SEED_RW + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
      for ( int i = 0; i < 400; ++i ) {
        if ( (lcheck::xs64(s) & 3u) == 0u ) {      // ~25% writers
          m.lock();
          wr.enter();
          writer_active.store(1, memory_order::release);
          wr.dwell(64);
          ++shared_value;
          writer_active.store(0, memory_order::release);
          wr.leave();
          m.unlock();
        } else {
          m.lock_shared();
          if ( writer_active.get(memory_order::acquire) != 0u ) readers_seen_during_write.fetch_add(1, memory_order::acq_rel);
          for ( u32 k = 0; k < 32; ++k ) __cpu_pause();
          m.unlock_shared();
        }
      }
    });

    require(wr.violations.get(memory_order::acquire) == 0ull);      // no two writers overlapped
    require(readers_seen_during_write.get(memory_order::acquire) == 0u);
    require(shared_value == wr.entries.get(memory_order::acquire));
    require_false(m.is_locked());
  }
  end_test_case();

  // ---- writer preference ----
  test_case("a writer is not starved by a continuous reader storm");
  {
    shared_mutex m;
    atomic_token<bool> stop(false);
    atomic_token<bool> writer_done(false);
    const int kReaders = static_cast<int>(lcheck::wide_threads);

    mtest::parallel(kReaders + 1, [&](int tid) {
      if ( tid == kReaders ) {
        micron::sleep_for(20);      // let the storm establish itself first
        m.lock();                   // must complete; a reader-preferring lock hangs here
        writer_done.store(true, memory_order::release);
        m.unlock();
        stop.store(true, memory_order::release);
        return;
      }
      while ( !stop.get(memory_order::acquire) ) {
        m.lock_shared();
        for ( u32 k = 0; k < 64; ++k ) __cpu_pause();
        m.unlock_shared();
      }
    });

    require_true(writer_done.get(memory_order::acquire));
    require_false(m.is_locked());
  }
  end_test_case();

  // ---- the RAII guard the tree had no type for ----
  test_case("shared_lock: scope, defer, adopt, try, move");
  {
    shared_mutex m;
    {
      shared_lock<shared_mutex> g(m);
      require_true(g.owns_lock());
      require(m.readers() == 1u);
    }
    require(m.readers() == 0u);

    {
      shared_lock<shared_mutex> g(m, defer_lock);
      require_false(g.owns_lock());
      require(m.readers() == 0u);
      g.lock();
      require_true(g.owns_lock());
      require(m.readers() == 1u);
      g.unlock();
      require(m.readers() == 0u);
    }

    {
      shared_lock<shared_mutex> g(m, try_to_lock);
      require_true(g.owns_lock());
      require(m.readers() == 1u);
    }
    require(m.readers() == 0u);

    m.lock_shared();
    {
      shared_lock<shared_mutex> g(m, adopt_lock);
      require_true(g.owns_lock());
    }
    require(m.readers() == 0u);

    {
      shared_lock<shared_mutex> a(m);
      shared_lock<shared_mutex> b(micron::move(a));
      require_false(a.owns_lock());
      require_true(b.owns_lock());
      require(m.readers() == 1u);
    }
    require(m.readers() == 0u);
  }
  end_test_case();

  test_case("try_to_lock guard fails while a writer holds it");
  {
    shared_mutex m;
    m.lock();
    {
      shared_lock<shared_mutex> g(m, try_to_lock);
      require_false(g.owns_lock());
      require_false(static_cast<bool>(g));
    }
    m.unlock();
  }
  end_test_case();

  test_case("exclusive side works with the ordinary guards");
  {
    shared_mutex m;
    {
      lock_guard<shared_mutex> g(m);
      require_true(m.is_writer_held());
    }
    require_false(m.is_locked());
    {
      unique_lock<lock_starts::locked, shared_mutex> g(m);
      require_true(m.is_writer_held());
    }
    require_false(m.is_locked());
  }
  end_test_case();

  test_case("non-copyable / non-movable");
  {
    static_assert(!is_copy_constructible_v<shared_mutex>);
    static_assert(!is_move_constructible_v<shared_mutex>);
    require_true(true);
  }
  end_test_case();

  sb::print("=== ALL SHARED_MUTEX TESTS PASSED ===");
  return 1;
}
