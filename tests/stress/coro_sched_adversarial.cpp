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

constexpr u64 ROUNDS = 40;
constexpr u64 WIDE = 512;
constexpr i64 DEEP = 13;
constexpr int RACERS = 8;
constexpr u64 BITS = 16384;

micron::atomic_token<u64> g_bits[BITS / 64];
micron::atomic_token<u64> g_double{ 0 };
micron::atomic_token<u64> g_claims{ 0 };

void
bits_reset(void) noexcept
{
  for ( usize i = 0; i < BITS / 64; ++i ) g_bits[i].store(0, micron::memory_order_relaxed);
  g_double.store(0, micron::memory_order_release);
  g_claims.store(0, micron::memory_order_release);
}

void
claim(u64 k) noexcept
{
  const u64 word = (k % BITS) / 64u;
  const u64 mask = 1ull << ((k % BITS) % 64u);
  const u64 prev = g_bits[word].fetch_or(mask, micron::memory_order_acq_rel);
  if ( (prev & mask) != 0 ) g_double.fetch_add(1, micron::memory_order_relaxed);
  g_claims.fetch_add(1, micron::memory_order_relaxed);
}

u64
bits_popcount(void) noexcept
{
  u64 n = 0;
  for ( usize i = 0; i < BITS / 64; ++i ) n += static_cast<u64>(__builtin_popcountll(g_bits[i].get(micron::memory_order_acquire)));
  return n;
}

micron::task<i64>
tree(i64 depth, u64 base)
{
  if ( depth == 0 ) {
    claim(base);
    co_return 1;
  }
  const u64 half = 1ull << (depth - 1);
  i64 a = 0, b = 0;
  co_await coro::fork[&a, tree](depth - 1, base);
  co_await coro::call[&b, tree](depth - 1, base + half);
  co_await coro::join;
  co_return a + b;
}

micron::task<i64>
chain(i64 n, u64 base)
{
  i64 total = 0;
  for ( i64 i = 0; i < n; ++i ) {
    i64 got = 0;
    co_await coro::fork[&got, tree](1, base + static_cast<u64>(i) * 2u);
    co_await coro::join;
    total += got;
  }
  co_return total;
}

micron::task<i64>
ping(i64 steps, u64 slot)
{
  for ( i64 i = 0; i < steps; ++i ) co_await coro::reschedule();
  claim(slot);
  co_return steps;
}

micron::task<i64>
ping_fair(i64 steps, u64 slot)
{
  for ( i64 i = 0; i < steps; ++i ) co_await coro::reschedule_fair();
  claim(slot);
  co_return steps;
}

}      // namespace

int
main(void)
{
  sb::print("=== CORO SCHEDULER ADVERSARIAL ===");
  sb::print("    rounds: ", static_cast<usize>(ltest::scaled(ROUNDS)), "  wide: ", static_cast<usize>(WIDE),
            "  deep: ", static_cast<usize>(DEEP), "  scale: ", static_cast<usize>(ltest::stress_scale));

  const i32 wm0 = ltest::fd_watermark();

  test_case("single worker: fork/join and reschedule are correct with no parallelism at all");
  {
    coro::start_coroutine_runtime(1);
    bits_reset();
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      require(coro::sync_wait(tree(8, 0)), static_cast<i64>(256));
      bits_reset();
    }

    require(coro::sync_wait(ping(2000, 1)), static_cast<i64>(2000));
    require(g_double.get(micron::memory_order_acquire), static_cast<u64>(0));
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  test_case("deep fork tree across all workers: every leaf runs exactly once");
  {
    coro::start_coroutine_runtime();
    const u64 n = ltest::scaled(ROUNDS / 8);
    for ( u64 r = 0; r < n; ++r ) {
      bits_reset();
      const i64 leaves = coro::sync_wait(tree(DEEP, 0));
      require(leaves, static_cast<i64>(1) << DEEP);
      require(bits_popcount(), static_cast<u64>(1) << DEEP);
      require(g_double.get(micron::memory_order_acquire), static_cast<u64>(0));
      require(g_claims.get(micron::memory_order_acquire), static_cast<u64>(1) << DEEP);
    }
    coro::stop_coroutine_runtime();
    sb::print("     trees=", static_cast<usize>(n), " leaves each=", static_cast<usize>(1u << DEEP));
  }
  end_test_case();

  test_case("wide fan-out from one frame: deque growth and steal pressure");
  {
    coro::start_coroutine_runtime();
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      bits_reset();
      auto out = coro::sync_wait(coro::spawn_many(WIDE, [](usize i) -> micron::task<i64> {
        claim(static_cast<u64>(i));
        co_return static_cast<i64>(i);
      }));
      require(out.size(), static_cast<usize>(WIDE));
      require(bits_popcount(), WIDE);
      require(g_double.get(micron::memory_order_acquire), static_cast<u64>(0));
      i64 sum = 0;
      for ( usize i = 0; i < out.size(); ++i ) sum += out[i];
      require(sum, static_cast<i64>(WIDE * (WIDE - 1) / 2));
    }
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  test_case("reschedule and reschedule_fair storms: every frame finishes exactly once");
  {
    coro::start_coroutine_runtime();
    const u64 n = ltest::scaled(ROUNDS / 8);
    for ( u64 r = 0; r < n; ++r ) {
      bits_reset();
      constexpr u64 K = 128;
      micron::vector<micron::futex_future<i64>> fs;
      for ( u64 i = 0; i < K; ++i ) fs.push_back(coro::schedule((i & 1) ? ping(64, i) : ping_fair(64, i)));
      for ( usize i = 0; i < fs.size(); ++i ) require(fs[i].get(), static_cast<i64>(64));
      require(bits_popcount(), K);
      require(g_double.get(micron::memory_order_acquire), static_cast<u64>(0));
    }
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  test_case("lockstep bursts: idle -> saturated -> idle, repeatedly");
  {
    coro::start_coroutine_runtime();
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {
      bits_reset();
      ltest::barrier_t bar;
      bar.n = static_cast<u32>(RACERS);
      mtest::parallel(RACERS, [&bar](int id) {
        u32 sense = 0;
        ltest::barrier_wait(bar, sense);
        const i64 got = coro::sync_wait(tree(6, static_cast<u64>(id) * 64u));
        require(got, static_cast<i64>(64));
        ltest::barrier_wait(bar, sense);
      });
      require(bits_popcount(), static_cast<u64>(RACERS) * 64u);
      require(g_double.get(micron::memory_order_acquire), static_cast<u64>(0));
    }
    coro::stop_coroutine_runtime();
    sb::print("     bursts=", static_cast<usize>(n), " x ", static_cast<usize>(RACERS), " racers");
  }
  end_test_case();

  test_case("starve then flood: parked workers all wake for the burst");
  {
    coro::start_coroutine_runtime();
    const u64 n = ltest::scaled(ROUNDS / 4);
    for ( u64 r = 0; r < n; ++r ) {

      coro::sync_wait([]() -> micron::task<void> { co_await coro::sleep_for_ms(12); }());
      bits_reset();
      constexpr u64 K = 96;
      micron::vector<micron::futex_future<i64>> fs;
      for ( u64 i = 0; i < K; ++i ) fs.push_back(coro::schedule(tree(3, i * 8u)));
      for ( usize i = 0; i < fs.size(); ++i ) require(fs[i].get(), static_cast<i64>(8));
      require(g_double.get(micron::memory_order_acquire), static_cast<u64>(0));
      require(g_claims.get(micron::memory_order_acquire), K * 8u);
    }
    coro::stop_coroutine_runtime();
  }
  end_test_case();

  test_case("all shapes concurrently on one engine");
  {
    coro::start_coroutine_runtime();
    const u64 n = ltest::scaled(ROUNDS / 8);
    for ( u64 r = 0; r < n; ++r ) {
      bits_reset();
      micron::vector<micron::futex_future<i64>> fs;
      for ( u64 i = 0; i < 16; ++i ) {
        fs.push_back(coro::schedule(tree(7, i * 128u)));
        fs.push_back(coro::schedule(chain(16, 4096u + i * 32u)));
        fs.push_back(coro::schedule(ping_fair(32, 8192u + i)));
      }
      for ( usize i = 0; i < fs.size(); ++i ) require_true(fs[i].get() > 0);
      require(g_double.get(micron::memory_order_acquire), static_cast<u64>(0));
    }
    require(coro::io_pending(), static_cast<u64>(0));
    coro::stop_coroutine_runtime();
    sb::print("     mixed rounds=", static_cast<usize>(n));
  }
  end_test_case();

  const i32 wm1 = ltest::fd_watermark();
  require(wm0, wm1);

  sb::print("=== ALL CORO SCHEDULER ADVERSARIAL TESTS PASSED ===");
  return 1;
}
