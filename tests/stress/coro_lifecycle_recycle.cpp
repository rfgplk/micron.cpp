//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ABC_MT 1
#define MICRON_CORO_URING

#include "../../src/stdcoro.hpp"

#include "../snowball/snowball.hpp"
#include "../support/lifetime.hpp"
#include "../support/mt.hpp"

using namespace snowball;
namespace coro = micron::coro;

namespace
{

using val = ltest::tracked<0>;

constexpr u64 CYCLES = 24;
constexpr u64 WORK = 96;
constexpr int RACERS = 8;

micron::atomic_token<u64> g_ran{ 0 };

micron::task<val>
unit(i64 v)
{
  g_ran.fetch_add(1, micron::memory_order_relaxed);
  co_return val{ v };
}

micron::task<i64>
spread(i64 n)
{
  if ( n <= 1 ) co_return 1;
  i64 a = 0, b = 0;
  co_await coro::fork[&a, spread](n / 2);
  co_await coro::call[&b, spread](n - n / 2);
  co_await coro::join;
  co_return a + b;
}

void
cycle(u32 nw, u64 work)
{
  coro::start_coroutine_runtime(nw);
  i64 sum = 0;
  for ( u64 i = 0; i < work; ++i ) {
    val r = coro::sync_wait(unit(static_cast<i64>(i)));
    sum += r.v;
  }
  require(sum, static_cast<i64>(work * (work - 1) / 2));
  require(coro::sync_wait(spread(64)), static_cast<i64>(64));
  coro::stop_coroutine_runtime();
}

}      // namespace

int
main(void)
{
  sb::print("=== CORO RUNTIME LIFECYCLE / RECYCLING ===");
  sb::print("    cycles/column: ", static_cast<usize>(ltest::scaled(CYCLES)), "  work/cycle: ", static_cast<usize>(WORK),
            "  scale: ", static_cast<usize>(ltest::stress_scale));

  const u32 ncpu = static_cast<u32>(micron::cpu_count());
  sb::print("    cpu_count: ", static_cast<usize>(ncpu));

  const i32 wm0 = ltest::fd_watermark();
  const u64 rss0 = ltest::rss_kb();

  test_case("start/stop entry points are idempotent in every order");
  {
    coro::stop_coroutine_runtime();

    coro::start_coroutine_runtime(2);
    coro::start_coroutine_runtime(2);
    coro::start_coroutine_runtime(7);
    require(coro::sync_wait(unit(5)).v, static_cast<i64>(5));

    coro::stop_coroutine_runtime();
    coro::stop_coroutine_runtime();

    coro::start_coroutine_runtime(2);
    require(coro::sync_wait(unit(9)).v, static_cast<i64>(9));
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  test_case("start/stop cycles across worker counts 1..cpu, plus both clamps");
  {
    val::reset();
    g_ran.store(0, micron::memory_order_release);
    const u32 widths[] = { 1u, 2u, 3u, 4u, 0u /*=cpu_count*/, 64u /*clamped to 32*/ };
    const u64 n = ltest::scaled(CYCLES / 4);
    u64 cycles_run = 0;
    for ( usize w = 0; w < sizeof(widths) / sizeof(widths[0]); ++w ) {
      for ( u64 c = 0; c < n; ++c ) {
        cycle(widths[w], WORK);
        ++cycles_run;
      }

      require(coro::io_pending(), static_cast<u64>(0));
    }
    require(g_ran.get(micron::memory_order_acquire), cycles_run * WORK);
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
    sb::print("     cycles=", static_cast<usize>(cycles_run), " tasks=", static_cast<usize>(cycles_run * WORK), " io_pending=0");
  }
  end_test_case();

  test_case("empty cycles: teardown races worker ring init");
  {
    const u64 n = ltest::scaled(CYCLES * 2);
    for ( u64 c = 0; c < n; ++c ) {
      coro::start_coroutine_runtime((c % 4u) + 1u);
      coro::stop_coroutine_runtime();
    }
    require(coro::io_pending(), static_cast<u64>(0));
    const i32 wm = ltest::fd_watermark();
    require(wm0, wm);
    sb::print("     empty cycles=", static_cast<usize>(n), " fd watermark held at ", static_cast<usize>(wm));
  }
  end_test_case();

  test_case("concurrent start from OS threads: one winner, losers wait for ready");
  {
    val::reset();
    g_ran.store(0, micron::memory_order_release);
    const u64 rounds = ltest::scaled(CYCLES / 4);
    for ( u64 r = 0; r < rounds; ++r ) {
      ltest::barrier_t bar;
      bar.n = static_cast<u32>(RACERS);
      mtest::parallel(RACERS, [&bar](int) {
        u32 sense = 0;
        ltest::barrier_wait(bar, sense);
        coro::start_coroutine_runtime(4);

        (void)coro::sync_wait(unit(1));
      });
      coro::stop_coroutine_runtime();
    }
    require(g_ran.get(micron::memory_order_acquire), rounds * static_cast<u64>(RACERS));
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
    sb::print("     races=", static_cast<usize>(rounds), " x ", static_cast<usize>(RACERS), " racers");
  }
  end_test_case();

  test_case("work submitted up to the last moment still completes before teardown");
  {
    val::reset();
    const u64 rounds = ltest::scaled(CYCLES / 4);
    for ( u64 r = 0; r < rounds; ++r ) {
      coro::start_coroutine_runtime(4);
      micron::vector<micron::futex_future<i64>> fs;
      for ( u64 i = 0; i < 32; ++i ) fs.push_back(coro::schedule(spread(16)));
      for ( usize i = 0; i < fs.size(); ++i ) require(fs[i].get(), static_cast<i64>(16));
      coro::stop_coroutine_runtime();
    }
    require(val::live(), static_cast<i64>(0));
    require(val::faults(), static_cast<u64>(0));
  }
  end_test_case();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);
  require(coro::io_pending(), static_cast<u64>(0));

  const u64 rss1 = ltest::rss_kb();
  if ( rss0 != 0 && rss1 != 0 ) {

    const u64 bound = rss0 + (384u * 1024u);
    if ( rss1 > bound ) sb::print("     rss grew ", static_cast<usize>(rss0), " -> ", static_cast<usize>(rss1), " KiB");
    require_true(rss1 <= bound);
    sb::print("     rss ", static_cast<usize>(rss0), " -> ", static_cast<usize>(rss1), " KiB (bound ", static_cast<usize>(bound), ")");
  } else {
    sb::print("     /proc unreadable - rss growth check skipped");
  }

  sb::print("=== ALL CORO LIFECYCLE TESTS PASSED ===");
  return 1;
}
