//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Contention benchmark for the whole lock family.
//
//   sweeps:
//     [uncontended]  one thread, lock+unlock round trip. this is the number that decides which
//                    lock a container reaches for, because it is the case containers are in.
//     [contended]    2 / 4 / 8 threads, short and long critical sections. reports throughput AND
//                    the fairness spread (most/least acquisitions), because a lock can buy
//                    throughput by starving somebody and the two numbers have to be read together.
//     [oversubscribed]  2x cores. the case that separates a parking lock from a spinning one:
//                    a spin lock here is not slow, it is pathological.
//
//   deliberately NOT bbench: this measures wall-clock throughput under contention, where the
//   interesting quantity is ops/sec and per-thread distribution rather than IPC, and it should run
//   without kernel.perf_event_paranoid <= 2.
//
//   build:  duck run benches/locks_contention.cpp --perf --fp --no-ssp --no-lto -o bin/bench
//   pin:    taskset -c 0-7 bin/bench/locks_contention
//
//   duck defaults to -fstack-protector-all -- a canary on every function, which does not cancel
//   out of a ratio between two locks. --no-ssp matters here.

#define MICRON_ABC_MT 1

#include "../src/mutex/locks.hpp"

#include "../src/io/console.hpp"
#include "../src/io/echo.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../src/thread/thread.hpp"
#include "../src/thread/thread_types/auto_thread.hpp"

namespace
{

constexpr u32 kMaxThreads = 64;

struct alignas(64) counter {
  u64 n = 0;
  char pad[64 - sizeof(u64)];
};

u64
now_ns(void) noexcept
{
  micron::timespec_t t{};
  micron::clock_gettime(micron::clock_monotonic, t);
  return static_cast<u64>(t.tv_sec) * 1000000000ull + static_cast<u64>(t.tv_nsec);
}

void
burn(u32 pauses) noexcept
{
  for ( u32 i = 0; i < pauses; ++i ) __cpu_pause();
}

struct alignas(64) gate {
  micron::atomic_token<u32> arrived{ 0 };
  char pad0[64 - sizeof(micron::atomic_token<u32>)];
  micron::atomic_token<u32> go{ 0 };
  char pad1[64 - sizeof(micron::atomic_token<u32>)];

  void
  wait(u32 n) noexcept
  {
    arrived.fetch_add(1, micron::memory_order::acq_rel);
    while ( go.get(micron::memory_order::acquire) == 0 ) __cpu_pause();
    (void)n;
  }
};

struct result {
  double ns_per_op = 0.0;
  double ops_per_sec = 0.0;
  double spread = 0.0;
  u64 total = 0;
};

template<typename Lock> struct alignas(64) isolated {
  Lock lk;
  char pad[64];
};

template<typename Lock>
result
bench_uncontended(u64 iters)
{
  isolated<Lock> box;
  Lock &m = box.lk;

  for ( u64 i = 0; i < 10000; ++i ) {
    m.lock();
    m.unlock();
  }

  const u64 t0 = now_ns();
  for ( u64 i = 0; i < iters; ++i ) {
    m.lock();
    m.unlock();
  }
  const u64 dt = now_ns() - t0;

  result r;
  r.total = iters;
  r.ns_per_op = static_cast<double>(dt) / static_cast<double>(iters);
  r.ops_per_sec = static_cast<double>(iters) * 1e9 / static_cast<double>(dt);
  r.spread = 1.0;
  return r;
}

template<typename Lock>
result
bench_contended(u32 threads, u64 ms, u32 hold_pauses)
{
  isolated<Lock> box;
  Lock &m = box.lk;
  gate g;
  counter cnt[kMaxThreads];
  alignas(64) micron::atomic_token<u32> stop{ 0 };
  alignas(64) char stop_pad[64];
  (void)stop_pad;

  micron::__thread_pointer<micron::auto_thread<>> ts[kMaxThreads];
  for ( u32 t = 0; t < threads; ++t )
    ts[t] = micron::solo::spawn<micron::auto_thread<>>([&, t]() {
      g.wait(threads);
      u64 n = 0;
      while ( stop.get(micron::memory_order::acquire) == 0 ) {
        m.lock();
        burn(hold_pauses);
        ++n;
        m.unlock();
      }
      cnt[t].n = n;
    });

  while ( g.arrived.get(micron::memory_order::acquire) != threads ) __cpu_pause();

  const u64 t0 = now_ns();
  g.go.store(1, micron::memory_order::release);
  micron::sleep_for(ms);
  stop.store(1, micron::memory_order::release);
  const u64 dt = now_ns() - t0;

  for ( u32 t = 0; t < threads; ++t ) micron::solo::join(ts[t]);

  u64 total = 0;
  u64 lo = ~0ull;
  u64 hi = 0;
  for ( u32 t = 0; t < threads; ++t ) {
    total += cnt[t].n;
    if ( cnt[t].n < lo ) lo = cnt[t].n;
    if ( cnt[t].n > hi ) hi = cnt[t].n;
  }

  result r;
  r.total = total;
  r.ns_per_op = total ? static_cast<double>(dt) * static_cast<double>(threads) / static_cast<double>(total) : 0.0;
  r.ops_per_sec = static_cast<double>(total) * 1e9 / static_cast<double>(dt);
  r.spread = lo ? static_cast<double>(hi) / static_cast<double>(lo) : 0.0;
  return r;
}

void
row(const char *name, const result &r)
{
  micron::io::println("  ", name, "  ns/op=", static_cast<u64>(r.ns_per_op * 100.0),
                      "/100  Mops/s=", static_cast<u64>(r.ops_per_sec / 1000.0), "/1000  spread=", static_cast<u64>(r.spread * 100.0),
                      "/100");
}

template<typename Lock>
void
sweep(const char *name, u32 cores)
{
  micron::io::println("[", name, "]");
  row("uncontended            ", bench_uncontended<Lock>(2000000));
  row("2 threads,  short hold ", bench_contended<Lock>(2, 200, 0));
  row("4 threads,  short hold ", bench_contended<Lock>(4, 200, 0));
  row("8 threads,  short hold ", bench_contended<Lock>(8, 200, 0));
  row("8 threads,  long hold  ", bench_contended<Lock>(8, 200, 256));
  row("oversubscribed, short  ", bench_contended<Lock>(cores * 2, 200, 0));
  row("oversubscribed, long   ", bench_contended<Lock>(cores * 2, 200, 256));
}

}      // namespace

int
main(void)
{
  using namespace micron;

  constexpr u32 cores = 8;

  io::println("=== micron lock contention benchmark ===");
  io::println("cores=", static_cast<u64>(cores), "  windows=200ms  spread = most/least acquisitions (100 = perfectly even)");
  io::println("");

  sweep<mutex>("mutex", cores);
  sweep<weak_mutex>("weak_mutex", cores);
  sweep<fast_mutex>("fast_mutex", cores);
  sweep<spin_lock>("spin_lock", cores);
  sweep<recursive_lock>("recursive_lock", cores);
  sweep<ttas_lock>("ttas_lock", cores);
  sweep<ttas_spin_lock>("ttas_spin_lock", cores);
  sweep<ticket_lock>("ticket_lock", cores);
  sweep<mcs_lock>("mcs_lock", cores);
  sweep<clh_lock>("clh_lock", cores);
  sweep<futex_mutex>("futex_mutex", cores);
  sweep<shared_mutex>("shared_mutex (exclusive side)", cores);

  io::println("");
  io::println("=== done ===");
  return 0;
}
