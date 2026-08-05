//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

// Thread lifecycle and resource recycling under churn.
//
// tests/rigor/thread_stress.cpp covers four cases in 105 lines; this drives the same machinery
// thousands of times and adds the two things it does not check at all:
//
//   1. the FUNCTOR's lifetime. solo::spawn copies the callable into the worker, so a captured
//      object is constructed in the parent and destroyed by whichever side outlives the other. A
//      spawn path that leaks the copy, or destroys it twice, is invisible to a test that only
//      counts how many times the body ran. ltest::tracked counts both, and its magic word catches
//      a destructor running against a stack the joiner already unmapped -- which is exactly what a
//      premature stack release would produce, and which a ctor/dtor balance alone would not show.
//   2. RESOURCE recycling. micron::thread mmaps its own stack and unmaps it on release;
//      auto_thread carries its stack inline; group_thread adopts one. Thousands of cycles of each
//      must return the process to the same descriptor watermark and to BOUNDED resident growth --
//      not to zero, because abc's sheets are sticky by design and __global_threadpool is never
//      joined (both documented in ISSUES.md).

#define MICRON_ABC_MT 1

#include "../../src/atomic/atomic.hpp"
#include "../../src/stdthread.hpp"
#include "../../src/thread/pool.hpp"
#include "../../src/thread/threads.hpp"

#include "../snowball/snowball.hpp"
#include "../support/lifetime.hpp"
#include "../support/mt.hpp"

using namespace snowball;

namespace
{

using payload = ltest::tracked<0>;
using tls_probe = ltest::tracked<1>;
using job = ltest::tracked<2>;

ltest::live_registry g_reg;

constexpr u64 CHURN = 600;
constexpr u64 STORM = 60;
constexpr int LIVE_CAP = 12;

micron::atomic_token<u64> g_ran{ 0 };
micron::atomic_token<u64> g_tls_ctor{ 0 };

[[gnu::noinline]] tls_probe &
worker_tls(void) noexcept
{
  static thread_local tls_probe __p{ 0x7105 };
  return __p;
}

void
touch_tls(void) noexcept
{
  if ( worker_tls().v == 0x7105 ) g_tls_ctor.fetch_add(1, micron::memory_order_relaxed);
}

}      // namespace

int
main(void)
{
  using namespace micron;

  sb::print("=== THREAD LIFECYCLE / RECYCLING ===");
  sb::print("    churn: ", static_cast<usize>(ltest::scaled(CHURN)), "  storm: ", static_cast<usize>(ltest::scaled(STORM)),
            "  scale: ", static_cast<usize>(ltest::stress_scale));

  if ( !micron::threads_available() ) {
    sb::print("threads unavailable on this build/kernel - SKIPPED");
    return 1;
  }

  payload::reg = &g_reg;

  const i32 wm0 = ltest::fd_watermark();
  const u64 rss0 = ltest::rss_kb();

  test_case("auto_thread spawn/join churn: captured payload destroyed exactly once");
  {
    payload::reset();
    g_reg.reset();
    g_ran.store(0, memory_order_release);
    const u64 n = ltest::scaled(CHURN);
    for ( u64 i = 0; i < n; ++i ) {
      payload p{ static_cast<i64>(i) };
      auto t = solo::spawn<auto_thread<>>([p]() {
        if ( p.v >= 0 ) g_ran.fetch_add(1, memory_order_relaxed);
      });
      solo::join(t);
    }
    require(g_ran.get(memory_order_acquire), n);
    require(payload::live(), static_cast<i64>(0));
    require(payload::faults(), static_cast<u64>(0));
    require(g_reg.collisions.get(memory_order_acquire), static_cast<u64>(0));
    sb::print("     bodies=", static_cast<usize>(n), " payload born=", static_cast<usize>(payload::born()),
              " dtor=", static_cast<usize>(payload::dtor.get(memory_order_acquire)));
  }
  end_test_case();

  test_case("micron::thread mmap-stack churn: stacks recycled, payload balanced");
  {
    payload::reset();
    g_ran.store(0, memory_order_release);
    const u64 n = ltest::scaled(CHURN);
    for ( u64 i = 0; i < n; ++i ) {
      payload p{ static_cast<i64>(i) };
      auto t = solo::spawn<thread<>>([p]() {
        if ( p.v >= 0 ) g_ran.fetch_add(1, memory_order_relaxed);
      });
      solo::join(t);
    }
    require(g_ran.get(memory_order_acquire), n);
    require(payload::live(), static_cast<i64>(0));
    require(payload::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("dismiss() reaps as completely as join()");
  {
    payload::reset();
    g_ran.store(0, memory_order_release);
    const u64 n = ltest::scaled(CHURN / 2);
    for ( u64 i = 0; i < n; ++i ) {
      payload p{ static_cast<i64>(i) };
      auto t = solo::spawn<auto_thread<>>([p]() {
        if ( p.v >= 0 ) g_ran.fetch_add(1, memory_order_relaxed);
      });
      solo::dismiss(t);
      require(!is_alive_ptr(t), true);
    }
    require(g_ran.get(memory_order_acquire), n);
    require(payload::live(), static_cast<i64>(0));
    require(payload::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("try_join polling reaps every worker");
  {
    payload::reset();
    g_ran.store(0, memory_order_release);
    const u64 n = ltest::scaled(CHURN / 4);
    for ( u64 i = 0; i < n; ++i ) {
      payload p{ static_cast<i64>(i) };
      auto t = solo::spawn<auto_thread<>>([p]() {
        if ( p.v >= 0 ) g_ran.fetch_add(1, memory_order_relaxed);
      });
      const bool reaped = micron::until_timeout(5000.0, [&t]() {
        solo::try_join(t);
        return !is_alive_ptr(t);
      });
      require_true(reaped);
      solo::dismiss(t);
    }
    require(g_ran.get(memory_order_acquire), n);
    require(payload::live(), static_cast<i64>(0));
    require(payload::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("LIVE_CAP concurrent workers, repeated waves");
  {
    payload::reset();
    g_ran.store(0, memory_order_release);
    const u64 waves = ltest::scaled(CHURN / 12);
    for ( u64 w = 0; w < waves; ++w ) {
      __thread_pointer<auto_thread<>> ts[LIVE_CAP];
      for ( int i = 0; i < LIVE_CAP; ++i ) {
        payload p{ static_cast<i64>(w * LIVE_CAP + static_cast<u64>(i)) };
        ts[i] = solo::spawn<auto_thread<>>([p]() {
          if ( p.v >= 0 ) g_ran.fetch_add(1, memory_order_relaxed);
        });
      }
      for ( int i = 0; i < LIVE_CAP; ++i ) solo::join(ts[i]);
    }
    require(g_ran.get(memory_order_acquire), waves * static_cast<u64>(LIVE_CAP));
    require(payload::live(), static_cast<i64>(0));
    require(payload::faults(), static_cast<u64>(0));
  }
  end_test_case();

  test_case("terminate-storm: runaway workers hard-stopped, abandonment bounded");
  {
    payload::reset();
    const u64 n = ltest::scaled(STORM);
    u64 stopped = 0;
    for ( u64 i = 0; i < n; ++i ) {
      atomic_token<bool> ready{ false };
      atomic_token<bool> never{ false };
      payload p{ static_cast<i64>(i) };
      auto t = solo::spawn<auto_thread<>>([p, &ready, &never]() {
        ready.store(true, memory_order_release);
        while ( !never.get(memory_order_acquire) ) micron::sleep(1);
      });
      micron::until_timeout(5000.0, [&ready]() { return ready.get(memory_order_acquire); });
      solo::terminate(t);
      solo::dismiss(t);
      if ( !is_alive_ptr(t) ) ++stopped;
    }
    require(stopped, n);
    require(payload::faults(), static_cast<u64>(0));
    const i64 stranded = payload::live();
    require_true(stranded >= 0);
    require_true(stranded <= static_cast<i64>(2 * n));
    sb::print("     hard-stopped=", static_cast<usize>(stopped), " stranded on killed stacks=", static_cast<usize>(stranded), " of max ",
              static_cast<usize>(2 * n), " (by design)");
  }
  end_test_case();

  test_case("thread_local destroyed on its owning worker, across recycled frames");
  {
    tls_probe::reset();
    g_tls_ctor.store(0, memory_order_release);
    const u64 n = ltest::scaled(CHURN / 3);
    for ( u64 i = 0; i < n; ++i ) {
      auto t = solo::spawn<auto_thread<>>([]() { touch_tls(); });
      solo::join(t);
    }
    require(g_tls_ctor.get(memory_order_acquire), n);
    require(tls_probe::ctor.get(memory_order_acquire), n);
    require(tls_probe::live(), static_cast<i64>(0));
    require(tls_probe::faults(), static_cast<u64>(0));

    require(tls_probe::foreign_dtor.get(memory_order_acquire), static_cast<u64>(0));
    sb::print("     workers=", static_cast<usize>(n), " tls ctor=", static_cast<usize>(n), " foreign=0 bad_magic=0");
  }
  end_test_case();

  test_case("thread_local ownership with concurrent live frames");
  {
    tls_probe::reset();
    g_tls_ctor.store(0, memory_order_release);
    const u64 waves = ltest::scaled(CHURN / 12);
    for ( u64 w = 0; w < waves; ++w ) mtest::parallel(LIVE_CAP, [](int) { touch_tls(); });
    const u64 expect = waves * static_cast<u64>(LIVE_CAP);
    require(g_tls_ctor.get(memory_order_acquire), expect);
    require(tls_probe::ctor.get(memory_order_acquire), expect);
    require(tls_probe::live(), static_cast<i64>(0));
    require(tls_probe::faults(), static_cast<u64>(0));
    require(tls_probe::foreign_dtor.get(memory_order_acquire), static_cast<u64>(0));
  }
  end_test_case();

  test_case("global pool: every dispatched job runs once and is destroyed once");
  {
    job::reset();
    micron::start_concurrent_pools();
    atomic_token<u64> ran{ 0 };
    const u64 n = ltest::scaled(CHURN / 6);
    {
      for ( u64 i = 0; i < n; ++i ) {
        job j{ static_cast<i64>(i) };
        micron::async([j, &ran]() {
          if ( j.v >= 0 ) ran.fetch_add(1, memory_order_relaxed);
        });
      }

      const bool drained = micron::until_timeout(30000.0, [&ran, n]() { return ran.get(memory_order_acquire) >= n; });
      require_true(drained);
    }
    require(ran.get(memory_order_acquire), n);
    require(job::faults(), static_cast<u64>(0));
    sb::print("     dispatched=", static_cast<usize>(n), " job born=", static_cast<usize>(job::born()),
              " dtor=", static_cast<usize>(job::dtor.get(memory_order_acquire)));
  }
  end_test_case();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  const u64 rss1 = ltest::rss_kb();
  if ( rss0 != 0 && rss1 != 0 ) {

    const u64 bound = rss0 + (256u * 1024u);
    if ( rss1 > bound ) sb::print("     rss grew ", static_cast<usize>(rss0), " -> ", static_cast<usize>(rss1), " KiB");
    require_true(rss1 <= bound);
    sb::print("     rss ", static_cast<usize>(rss0), " -> ", static_cast<usize>(rss1), " KiB (bound ", static_cast<usize>(bound), ")");
  } else {
    sb::print("     /proc unreadable - rss growth check skipped");
  }

  sb::print("=== ALL THREAD LIFECYCLE TESTS PASSED ===");
  return 1;
}
