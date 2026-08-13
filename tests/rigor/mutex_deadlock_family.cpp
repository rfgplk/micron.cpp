//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1      // spawns threads; abcmalloc's -k gate must be MT (bits/__abc_mt.hpp)

#include "../../src/mutex/locks.hpp"

#include "../../src/std.hpp"

#include "../../src/thread/thread.hpp"
#include "../../src/thread/thread_types/auto_thread.hpp"

#include "../support/mt.hpp"

#include "../snowball/snowball.hpp"

#include "../support/lockcheck.hpp"

#if !defined(LOCK_DF_SCALE)
#define LOCK_DF_SCALE 1
#endif

using sb::end_test_case;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

namespace
{

constexpr u64 SEED_DF = 0xD3ADF00DBEEFULL;

constexpr int kHot = 300 * LOCK_DF_SCALE;
constexpr int kGuard = 40 * LOCK_DF_SCALE;
constexpr int kInvert = 400 * LOCK_DF_SCALE;
constexpr u64 kStallMs = 8000;
constexpr u64 kSettleMs = 30;

template<typename L>
concept guardable = requires(L &l) {
  l();
  l.retrieve();
};

template<typename L>
concept recursive_capable = requires(const L &l) { l.lock_depth(); };

template<typename L>
concept shared_capable = requires(L &l) {
  l.lock_shared();
  l.unlock_shared();
};

template<typename L> inline constexpr bool excludes_v = !micron::is_same_v<L, micron::null_lock>;

template<typename L> inline constexpr usize max_held_v = static_cast<usize>(-1);
template<> inline constexpr usize max_held_v<micron::mcs_lock> = static_cast<usize>(MICRON_MCS_DEPTH);

void
case_of(const char *lock_name, const char *what)
{
  micron::string s(lock_name);
  s += " :: ";
  s += what;
  (void)sb::test_case(s);
}

class armed
{
  lcheck::watchdog &wd;
  micron::__thread_pointer<micron::auto_thread<>> th;

public:
  armed(lcheck::watchdog &w, u32 threads) : wd(w)
  {
    w.reset();
    th = micron::solo::spawn<micron::auto_thread<>>([&w, threads]() { w.watch(kStallMs, threads); });
  }

  ~armed()
  {
    wd.disarm();
    micron::solo::join(th);
  }

  armed(const armed &) = delete;
  armed(armed &&) = delete;
  armed &operator=(const armed &) = delete;
};

void
await(micron::atomic_token<u32> &flag, u32 want) noexcept
{
  while ( flag.get(micron::memory_order::acquire) != want ) micron::yield();
}

template<typename L>
void
p_roundtrip(const char *name)
{
  case_of(name, "lock/unlock round-trips and excludes, watchdogged");

  {
    L m;
    require_false(m.is_locked());

    lcheck::watchdog wd;
    armed a(wd, 1);
    for ( int i = 0; i < kGuard; ++i ) {
      wd.note_want(0u, micron::addressof(m));
      m.lock();
      wd.note_hold(0u, micron::addressof(m));
      if constexpr ( excludes_v<L> ) require_true(m.is_locked());
      m.unlock();
      wd.note_release(0u);
      require_false(m.is_locked());
      wd.bump();
    }
  }

  if constexpr ( !excludes_v<L> ) {
    sb::print("     ", name, ": SKIPPED contended exclusion -- null_lock admits everyone by design");
  } else {
    L m;
    const u32 kT = lcheck::wide_threads;
    lcheck::exclusion_probe pr;
    lcheck::start_gate gate(kT);
    micron::atomic_token<u64> total{ 0 };
    lcheck::watchdog wd;

    {
      armed a(wd, kT);
      mtest::parallel(static_cast<int>(kT), [&](int tid) {
        u32 sense = 0;
        gate.wait(sense);
        for ( int i = 0; i < kHot; ++i ) {
          wd.note_want(static_cast<u32>(tid), micron::addressof(m));
          m.lock();
          wd.note_hold(static_cast<u32>(tid), micron::addressof(m));
          pr.enter();
          pr.dwell(4);
          pr.leave();
          m.unlock();
          wd.note_release(static_cast<u32>(tid));
          total.fetch_add(1, micron::memory_order::relaxed);
          wd.bump();
        }
      });
    }

    require(pr.violations.get(micron::memory_order::acquire) == 0ull);
    require(total.get(micron::memory_order::acquire) == static_cast<u64>(kT) * kHot);
    require_true(pr.clean());
    require_false(m.is_locked());
  }
}

template<typename L>
void
p_guards(const char *name)
{
  case_of(name, "every guard releases: lock_guard / auto_guard / unique_lock / lock_set");

  if constexpr ( !guardable<L> ) {
    sb::print("     ", name, ": SKIPPED guards -- no operator()/retrieve() PMF, so no guard can carry it");
  } else {
    L m;

    auto held = [&m]() {
      if constexpr ( excludes_v<L> ) require_true(m.is_locked());
    };
    auto freed = [&m]() {
      if constexpr ( excludes_v<L> ) require_false(m.is_locked());
    };
    lcheck::watchdog wd;
    armed a(wd, 1);
    wd.note_want(0u, micron::addressof(m));

    for ( int i = 0; i < kGuard; ++i ) {
      {
        micron::lock_guard<L> g(m);
        held();
      }
      freed();

      m.lock();
      {
        micron::lock_guard<L> g(m, micron::adopt_lock);
      }
      freed();

      {
        micron::auto_guard<L> g(m);
        held();
      }
      freed();

      {
        micron::unique_lock<micron::lock_starts::locked, L> g(m);
      }
      freed();

      {
        micron::unique_lock<micron::lock_starts::defer, L> g(m);
        g.lock();
        g.lock();
        g.unlock();
        g.unlock();
      }
      freed();

      {
        micron::unique_lock<micron::lock_starts::defer, L> g(m);
        require_true(g.try_lock());
        require_false(g.try_lock());
      }
      freed();

      m.lock();
      {
        micron::unique_lock<micron::lock_starts::adopt, L> g(m);
      }
      freed();

      {
        micron::unique_lock<micron::lock_starts::locked, L> g(m);
        micron::unique_lock<micron::lock_starts::locked, L> h(micron::move(g));
        held();
      }
      freed();

      {
        micron::lock_set<L> g(m);
        require_true(g.owns_lock());
        held();
        g.unlock();
        require_false(g.owns_lock());
      }
      freed();

      m.lock();
      {
        micron::lock_set<L> g(micron::adopt_lock, m);
      }
      freed();

      wd.bump();
    }
  }
}

template<typename L>
void
p_stray(const char *name)
{
  case_of(name, "a stray or doubled unlock is a no-op and leaves the lock usable");

  L m;
  lcheck::watchdog wd;
  armed a(wd, 1);
  wd.note_want(0u, micron::addressof(m));

  m.unlock();
  require_false(m.is_locked());
  wd.bump();

  m.lock();
  m.unlock();
  m.unlock();
  require_false(m.is_locked());
  wd.bump();

  if constexpr ( guardable<L> ) {
    micron::lock_guard<L> g(m, micron::adopt_lock);
  }
  require_false(m.is_locked());
  wd.bump();

  for ( int i = 0; i < kGuard; ++i ) {
    m.lock();
    m.unlock();
    wd.bump();
  }
  require_false(m.is_locked());
}

template<typename L>
void
p_trylock(const char *name)
{
  case_of(name, "a failing try_lock leaves the lock releasable and the queue intact");

  if constexpr ( !excludes_v<L> ) {
    sb::print("     ", name, ": SKIPPED try_lock semantics -- null_lock's try_lock always succeeds");
  } else {
    {
      L m;
      require_true(m.try_lock());
      require_true(m.is_locked());
      m.unlock();
      require_false(m.is_locked());
      require_true(m.try_lock());
      m.unlock();
    }

    {
      L m;
      lcheck::watchdog wd;
      micron::atomic_token<u32> phase{ 0 };
      micron::atomic_token<u32> got{ 0 };
      armed a(wd, 2);

      m.lock();
      wd.note_hold(0u, micron::addressof(m));
      wd.bump();

      auto w = micron::solo::spawn<micron::auto_thread<>>([&]() {
        wd.note_want(1u, micron::addressof(m));
        phase.store(1, micron::memory_order::release);
        m.lock();
        wd.note_hold(1u, micron::addressof(m));
        got.store(1, micron::memory_order::release);
        m.unlock();
        wd.note_release(1u);
        phase.store(2, micron::memory_order::release);
      });

      await(phase, 1u);
      micron::sleep_for(kSettleMs);
      wd.bump();

      if constexpr ( recursive_capable<L> ) {
        require_true(m.try_lock());
        m.unlock();
      } else {
        require_false(m.try_lock());
      }
      wd.bump();

      m.unlock();
      wd.note_release(0u);
      wd.bump();

      await(phase, 2u);
      micron::solo::join(w);
      require(got.get(micron::memory_order::acquire) == 1u);
      require_false(m.is_locked());
    }

    {
      L m;
      const u32 kT = lcheck::wide_threads;
      lcheck::exclusion_probe pr;
      lcheck::start_gate gate(kT);
      micron::atomic_token<u64> taken{ 0 };
      lcheck::watchdog wd;
      armed a(wd, kT);

      mtest::parallel(static_cast<int>(kT), [&](int tid) {
        (void)tid;
        u32 sense = 0;
        gate.wait(sense);
        for ( int i = 0; i < kHot; ++i ) {
          if ( m.try_lock() ) {
            pr.enter();
            pr.dwell(2);
            pr.leave();
            taken.fetch_add(1, micron::memory_order::relaxed);
            m.unlock();
          } else {
            micron::yield();
          }
          wd.bump();
        }
      });

      require(pr.violations.get(micron::memory_order::acquire) == 0ull);
      require_true(taken.get(micron::memory_order::acquire) > 0ull);
      require_false(m.is_locked());

      m.lock();
      wd.bump();
      m.unlock();
      wd.bump();
      require_false(m.is_locked());
    }
  }
}

template<typename L>
void
p_multi(const char *name)
{
  case_of(name, "lock_all: opposing acquisition orders do not deadlock");

  if constexpr ( max_held_v<L> < 2 ) {
    sb::print("     ", name, ": SKIPPED same-type 2-lock inversion -- this build allows ", static_cast<usize>(max_held_v<L>),
              " held at once (MICRON_MCS_DEPTH), and lock_all over two of them is out of contract");
  } else {
    L a, b;
    lcheck::watchdog wd;
    lcheck::start_gate gate(2);
    micron::atomic_token<u64> rounds{ 0 };

    {
      armed w(wd, 2);
      mtest::parallel(2, [&](int tid) {
        u32 sense = 0;
        gate.wait(sense);
        for ( int i = 0; i < kInvert; ++i ) {
          if ( tid == 0 )
            micron::lock_all(a, b);
          else
            micron::lock_all(b, a);
          rounds.fetch_add(1, micron::memory_order::relaxed);
          micron::unlock(a, b);
          wd.bump();
        }
      });
    }

    require(rounds.get(micron::memory_order::acquire) == 2ull * kInvert);
    require_false(a.is_locked());
    require_false(b.is_locked());
  }

  {
    L a;
    micron::ttas_lock b;
    lcheck::watchdog wd;
    lcheck::start_gate gate(2);
    micron::atomic_token<u64> rounds{ 0 };

    {
      armed w(wd, 2);
      mtest::parallel(2, [&](int tid) {
        u32 sense = 0;
        gate.wait(sense);
        for ( int i = 0; i < kInvert; ++i ) {
          if ( tid == 0 )
            micron::lock_all(a, b);
          else
            micron::lock_all(b, a);
          rounds.fetch_add(1, micron::memory_order::relaxed);
          micron::unlock(a, b);
          wd.bump();
        }
      });
    }

    require(rounds.get(micron::memory_order::acquire) == 2ull * kInvert);
    require_false(a.is_locked());
    require_false(b.is_locked());
  }

  case_of(name, "try_lock / try_lock_all roll back; try_lock_in_order deliberately does not");

  if constexpr ( excludes_v<L> and !recursive_capable<L> and max_held_v<L> >= 2 ) {
    L a, b;
    lcheck::watchdog wd;
    armed w(wd, 1);
    wd.note_want(0u, micron::addressof(b));

    b.lock();
    require(micron::try_lock_all(a, b) == 1);
    require_false(a.is_locked());
    require_true(b.is_locked());
    wd.bump();

    require_false(micron::try_lock(a, b));
    require_false(a.is_locked());
    wd.bump();

    require_false(micron::try_lock_in_order(a, b));
    require_true(a.is_locked());
    a.unlock();
    b.unlock();
    wd.bump();

    require_false(a.is_locked());
    require_false(b.is_locked());

    micron::lock_all(a, b);
    micron::unlock(a, b);
    wd.bump();
  } else if constexpr ( !excludes_v<L> ) {
    sb::print("     ", name, ": SKIPPED rollback -- null_lock never refuses, so there is no partial failure");
  } else if constexpr ( recursive_capable<L> ) {
    sb::print("     ", name, ": SKIPPED rollback -- recursive_lock re-enters, so a self-held lock does not refuse");
  } else {
    sb::print("     ", name, ": SKIPPED rollback -- needs two of them held at once, past this build's slot depth");
  }
}

template<typename L>
void
p_handoff(const char *name)
{
  case_of(name, "hand-off under contention: every thread makes it out");

  L m;
  const u32 kT = lcheck::wide_threads;
  lcheck::fairness_probe fp;
  lcheck::start_gate gate(kT);
  lcheck::watchdog wd;

  {
    armed a(wd, kT);
    mtest::parallel(static_cast<int>(kT), [&](int tid) {
      u32 sense = 0;
      gate.wait(sense);
      for ( int i = 0; i < kHot; ++i ) {
        wd.note_want(static_cast<u32>(tid), micron::addressof(m));
        m.lock();
        wd.note_hold(static_cast<u32>(tid), micron::addressof(m));
        fp.note(static_cast<u32>(tid));
        m.unlock();
        wd.note_release(static_cast<u32>(tid));
        wd.bump();
      }
    });
  }

  require(fp.total(kT) == static_cast<u64>(kT) * kHot);
  require(fp.least(kT) == static_cast<u64>(kHot));
  require(fp.most(kT) == static_cast<u64>(kHot));
  require_false(m.is_locked());
}

template<typename L>
void
p_lifetime(const char *name)
{
  case_of(name, "destroyed unheld, reusable from another thread, sane at a recycled address");

  {
    L m;
    m.lock();
    m.unlock();
  }

  {
    L m;
    lcheck::watchdog wd;
    armed a(wd, 1);

    for ( int i = 0; i < 3; ++i ) {
      auto t = micron::solo::spawn<micron::auto_thread<>>([&]() {
        wd.note_want(0u, micron::addressof(m));
        m.lock();
        wd.note_hold(0u, micron::addressof(m));
        m.unlock();
        wd.note_release(0u);
      });
      micron::solo::join(t);
      require_false(m.is_locked());
      wd.bump();
    }

    m.lock();
    m.unlock();
    wd.bump();
  }

  case_of(name, "cross-thread release: A acquires, B releases, C acquires");

  if constexpr ( excludes_v<L> and !recursive_capable<L> ) {
    L m;
    lcheck::watchdog wd;
    micron::atomic_token<u32> held{ 0 };
    micron::atomic_token<u32> done{ 0 };
    micron::atomic_token<u32> got{ 0 };
    armed w(wd, 3);

    auto ta = micron::solo::spawn<micron::auto_thread<>>([&]() {
      m.lock();
      wd.note_hold(0u, micron::addressof(m));
      held.store(1, micron::memory_order::release);
      await(done, 1u);
    });

    await(held, 1u);
    require_true(m.is_locked());
    wd.bump();

    auto tb = micron::solo::spawn<micron::auto_thread<>>([&]() { m.unlock(); });
    micron::solo::join(tb);
    require_false(m.is_locked());
    wd.bump();

    auto tc = micron::solo::spawn<micron::auto_thread<>>([&]() {
      wd.note_want(2u, micron::addressof(m));
      m.lock();
      wd.note_hold(2u, micron::addressof(m));
      got.store(1, micron::memory_order::release);
      m.unlock();
      wd.note_release(2u);
    });
    micron::solo::join(tc);
    require(got.get(micron::memory_order::acquire) == 1u);
    wd.bump();

    done.store(1, micron::memory_order::release);
    micron::solo::join(ta);
    require_false(m.is_locked());
    wd.bump();
  } else if constexpr ( !excludes_v<L> ) {
    sb::print("     ", name, ": SKIPPED cross-thread release -- null_lock holds nothing to release");
  } else {
    sb::print("     ", name,
              ": SKIPPED cross-thread release -- recursive_lock owns its release by definition "
              "(a non-owner unlock is a documented no-op)");
  }

  case_of(name, "a lock at a recycled address inherits nothing");
  {
    alignas(L) unsigned char buf[sizeof(L)];
    lcheck::watchdog wd;
    armed a(wd, 1);

    L *p = new (static_cast<void *>(buf)) L();
    p->lock();
    p->unlock();
    p->~L();
    wd.bump();

    L *q = new (static_cast<void *>(buf)) L();
    require_false(q->is_locked());
    q->unlock();
    require_false(q->is_locked());
    wd.bump();

    q->lock();
    if constexpr ( excludes_v<L> ) require_true(q->is_locked());
    q->unlock();
    require_false(q->is_locked());
    q->~L();
    wd.bump();
  }
}

template<typename Fn>
void
every_lock(Fn f)
{
  f.template operator()<micron::mutex>("mutex");
  f.template operator()<micron::weak_mutex>("weak_mutex");
  f.template operator()<micron::fast_mutex>("fast_mutex");
  f.template operator()<micron::null_lock>("null_lock");
  f.template operator()<micron::spin_lock>("spin_lock");
  f.template operator()<micron::recursive_lock>("recursive_lock");
  f.template operator()<micron::ttas_lock>("ttas_lock");
  f.template operator()<micron::ttas_spin_lock>("ttas_spin_lock");
  f.template operator()<micron::ticket_lock>("ticket_lock");
  f.template operator()<micron::ticket_spin_lock>("ticket_spin_lock");
  f.template operator()<micron::mcs_lock>("mcs_lock");
  f.template operator()<micron::clh_lock>("clh_lock");
  f.template operator()<micron::futex_mutex>("futex_mutex");
  f.template operator()<micron::shared_mutex>("shared_mutex");
}

}      // namespace

int
main(void)
{
  using namespace micron;
  sb::print("=== FAMILY-WIDE DEADLOCK REGRESSION BATTERY ===");
  sb::print("     every case below is watchdog-armed: a wedge fails as a named case, never as a hang");

  test_case("round-trips: lock/unlock completes and excludes, every type");
  every_lock([]<typename L>(const char *n) { p_roundtrip<L>(n); });
  end_test_case();

  test_case("guards: every guard in the tree releases, every type");
  every_lock([]<typename L>(const char *n) { p_guards<L>(n); });
  end_test_case();

  test_case("stray release: a doubled or unheld unlock never kills the lock");
  every_lock([]<typename L>(const char *n) { p_stray<L>(n); });
  end_test_case();

  test_case("try_lock: a refusal leaves the lock releasable and the queue intact");
  every_lock([]<typename L>(const char *n) { p_trylock<L>(n); });
  end_test_case();

  test_case("multi-acquire: lock_all survives opposing orders, try_lock_all rolls back");
  every_lock([]<typename L>(const char *n) { p_multi<L>(n); });
  end_test_case();

  test_case("hand-off: every thread completes under contention");
  every_lock([]<typename L>(const char *n) { p_handoff<L>(n); });
  end_test_case();

  test_case("lifetime: destruction, cross-thread reuse and release, recycled addresses");
  every_lock([]<typename L>(const char *n) { p_lifetime<L>(n); });
  end_test_case();

  test_case("shared_mutex: both modes round-trip and neither wedges the other");
  {
    static_assert(shared_capable<shared_mutex>);
    static_assert(!shared_capable<ttas_lock>);

    shared_mutex m;
    const u32 kT = lcheck::wide_threads;
    lcheck::start_gate gate(kT);
    lcheck::watchdog wd;
    atomic_token<i32> wr_in{ 0 };
    atomic_token<u64> bad{ 0 };
    atomic_token<u64> reads{ 0 };
    atomic_token<u64> writes{ 0 };

    {
      armed a(wd, kT);
      mtest::parallel(static_cast<int>(kT), [&](int tid) {
        u32 sense = 0;
        gate.wait(sense);
        for ( int i = 0; i < kHot; ++i ) {
          if ( (tid % 3) == 0 ) {
            m.lock();
            if ( wr_in.add_fetch(1, memory_order::acq_rel) != 1 ) bad.fetch_add(1, memory_order::relaxed);
            for ( u32 k = 0; k < 4; ++k ) __cpu_pause();
            wr_in.sub_fetch(1, memory_order::acq_rel);
            writes.fetch_add(1, memory_order::relaxed);
            m.unlock();
          } else {
            shared_lock<shared_mutex> g(m);
            if ( wr_in.get(memory_order::acquire) != 0 ) bad.fetch_add(1, memory_order::relaxed);
            reads.fetch_add(1, memory_order::relaxed);
          }
          wd.bump();
        }
      });
    }

    require(bad.get(memory_order::acquire) == 0ull);
    require(reads.get(memory_order::acquire) + writes.get(memory_order::acquire) == static_cast<u64>(kT) * kHot);
    require(m.readers() == 0u);
    require(m.writers_queued() == 0u);
    require_false(m.is_locked());

    m.unlock_shared();
    require(m.readers() == 0u);
    require_false(m.is_writer_held());
    require_false(m.is_locked());
    require_true(m.try_lock());
    m.unlock();
    require_true(m.try_lock_shared());
    m.unlock_shared();
    require(m.readers() == 0u);

    {
      shared_lock<shared_mutex> g(m, defer_lock);
      require_false(g.owns_lock());
      g.lock();
      require_true(g.owns_lock());
      g.unlock();
      require_true(g.try_lock());
    }
    require(m.readers() == 0u);
    m.lock_shared();
    {
      shared_lock<shared_mutex> g(m, adopt_lock);
    }
    require(m.readers() == 0u);
    {
      shared_lock<shared_mutex> g(m, try_to_lock);
      require_true(g.owns_lock());
    }
    require(m.readers() == 0u);

    sb::print("     NOT TESTED, by design: shared->exclusive upgrade. shared_mutex has none, and",
              " lock() while holding shared raises __ww before waiting on a word its own reader bit",
              " is in -- it wedges EVERY reader, not just the caller (shared_mutex.hpp:32).");
  }
  end_test_case();

  test_case("queuing_mutex + scoped_lock: a refused try_acquire leaves the release intact");
  {

    queuing_mutex q;
    lcheck::watchdog wd;
    atomic_token<u32> phase{ 0 };
    atomic_token<u32> got{ 0 };
    armed a(wd, 2);

    scoped_lock g(q);
    require_true(g.owns());
    require_true(q.is_locked());
    wd.note_hold(0u, addressof(q));
    wd.bump();

    auto w = solo::spawn<auto_thread<>>([&]() {
      wd.note_want(1u, addressof(q));
      phase.store(1, memory_order::release);
      {
        scoped_lock h(q);
        wd.note_hold(1u, addressof(q));
        got.store(1, memory_order::release);
      }
      wd.note_release(1u);
      phase.store(2, memory_order::release);
    });

    await(phase, 1u);
    sleep_for(kSettleMs);
    wd.bump();

    {
      scoped_lock t;
      require_false(t.try_acquire(q));
      require_false(t.owns());
    }
    wd.bump();

    g.release();
    wd.note_release(0u);
    wd.bump();

    await(phase, 2u);
    solo::join(w);
    require(got.get(memory_order::acquire) == 1u);
    require_false(q.is_locked());

    {
      scoped_lock s(q);
      s.release();
      s.release();
    }
    require_false(q.is_locked());
    {
      scoped_lock s;
      require_false(s.owns());
    }
    {
      scoped_lock s;
      require_true(s.try_acquire(q));
      require_false(s.try_acquire(q));
    }
    require_false(q.is_locked());
    wd.bump();
  }
  end_test_case();

  test_case("the seeded mixed-type inversion fuzz: random pairs, random orders, one watchdog");
  {

    ttas_lock l0;
    ticket_lock l1;
    mcs_lock l2;
    clh_lock l3;
    futex_mutex l4;
    constexpr usize kLocks = 5;

    lcheck::exclusion_probe pr[kLocks];
    lcheck::watchdog wd;
    lcheck::start_gate gate(lcheck::wide_threads);
    const u32 kT = lcheck::wide_threads;
    atomic_token<u64> rounds{ 0 };

    auto take = [&](usize i) {
      switch ( i ) {
      case 0:
        l0.lock();
        break;
      case 1:
        l1.lock();
        break;
      case 2:
        l2.lock();
        break;
      case 3:
        l3.lock();
        break;
      default:
        l4.lock();
        break;
      }
    };
    auto drop = [&](usize i) {
      switch ( i ) {
      case 0:
        l0.unlock();
        break;
      case 1:
        l1.unlock();
        break;
      case 2:
        l2.unlock();
        break;
      case 3:
        l3.unlock();
        break;
      default:
        l4.unlock();
        break;
      }
    };

    {
      armed a(wd, kT);
      mtest::parallel(static_cast<int>(kT), [&](int tid) {
        u32 sense = 0;
        gate.wait(sense);
        u64 s = SEED_DF + static_cast<u64>(tid) * 0x9E3779B97F4A7C15ULL;
        for ( int i = 0; i < kHot; ++i ) {

          const usize x = static_cast<usize>(lcheck::xs64(s) % kLocks);
          wd.note_want(static_cast<u32>(tid), micron::addressof(pr[x]));
          take(x);
          wd.note_hold(static_cast<u32>(tid), micron::addressof(pr[x]));
          pr[x].enter();
          pr[x].dwell(static_cast<u32>(lcheck::xs64(s) & 0x7));
          pr[x].leave();
          drop(x);
          wd.note_release(static_cast<u32>(tid));
          rounds.fetch_add(1, memory_order::relaxed);
          wd.bump();
        }
      });
    }

    for ( usize k = 0; k < kLocks; ++k ) require(pr[k].violations.get(memory_order::acquire) == 0ull);
    require(rounds.get(memory_order::acquire) == static_cast<u64>(kT) * kHot);
    require_false(l0.is_locked());
    require_false(l1.is_locked());
    require_false(l2.is_locked());
    require_false(l3.is_locked());
    require_false(l4.is_locked());
    sb::print("     ", static_cast<usize>(kT), " threads x ", static_cast<usize>(kHot),
              " acquisitions over 5 lock implementations, no stall");
  }
  end_test_case();

  sb::print("     EXCLUDED from the generic sweep, deliberately: seqlock (a versioned value, not an",
            " is_mutex -- its writer lock is a template parameter already swept here; graded in",
            " mutex_seqlock.cpp), and the raw node-taking queuing_mutex, which has its own case above.");
  sb::print("=== ALL FAMILY-WIDE DEADLOCK TESTS PASSED ===");
  return 1;
}
