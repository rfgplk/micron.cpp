//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/mutex/mutex.hpp"
#include "../src/thread/thread.hpp"

#include "../src/array/conarray.hpp"
#include "../src/heap/heapq.hpp"
#include "../src/queue/conqueue.hpp"
#include "../src/vector/convector.hpp"

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP = 2;

f64 g_tsc_ghz = 1.0;

[[gnu::always_inline]] inline u64
rdtsc() noexcept
{
  u32 lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
}

[[gnu::always_inline]] inline u64
wall_ns() noexcept
{
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

void
calibrate_tsc() noexcept
{
  const u64 n0 = wall_ns();
  const u64 c0 = rdtsc();
  while ( wall_ns() - n0 < 50000000ULL ) {
  }
  const u64 c1 = rdtsc();
  const u64 n1 = wall_ns();
  g_tsc_ghz = static_cast<f64>(c1 - c0) / static_cast<f64>(n1 - n0);
}

[[gnu::always_inline]] inline f64
cyc_to_ns(u64 cyc) noexcept
{
  return static_cast<f64>(cyc) / g_tsc_ghz;
}

[[gnu::always_inline]] inline void
clobber(u64 v) noexcept
{
  asm volatile("" : : "r"(v) : "memory");
}

f64
median(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    f64 k = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > k ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = k;
  }
  return xs[n / 2];
}

void
report(const char *name, f64 nsop) noexcept
{
  if ( nsop < 0 ) nsop = 0;
  u64 centi = static_cast<u64>(nsop * 100.0 + 0.5);
  micron::io::print("  ", name, "  ", centi / 100, ".", (centi % 100 < 10 ? "0" : ""), centi % 100, " ns/op\n");
}

template<class M>
f64
bench_lockunlock_uncontended(u64 n) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    M mtx;
    u64 sink = 0;
    const u64 c0 = rdtsc();
    for ( u64 i = 0; i < n; ++i ) {
      mtx.lock();
      sink += i;
      mtx.unlock();
    }
    const u64 c1 = rdtsc();
    clobber(sink);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(n);
  }
  return median(samples, K_MEASUREMENTS);
}

template<class M>
f64
bench_lockunlock_contended(u64 n_per_thread, u32 threads) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    M mtx;
    u64 sink = 0;
    auto work = [&] {
      for ( u64 i = 0; i < n_per_thread; ++i ) {
        mtx.lock();
        sink += 1;
        mtx.unlock();
      }
    };
    const u64 c0 = rdtsc();
    {
      micron::auto_thread<> t0(work);
      micron::auto_thread<> t1(work);
      micron::auto_thread<> t2(work);
      micron::auto_thread<> t3(work);
      (void)threads;
    }
    const u64 c1 = rdtsc();
    clobber(sink);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(n_per_thread * 4);
  }
  return median(samples, K_MEASUREMENTS);
}

f64
bench_conqueue(u64 n) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    micron::conqueue<int> q;
    const u64 c0 = rdtsc();
    for ( u64 i = 0; i < n; ++i ) q.push(static_cast<int>(i));
    for ( u64 i = 0; i < n; ++i ) q.pop();
    const u64 c1 = rdtsc();
    clobber(q.size());
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

f64
bench_convector(u64 n) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    micron::convector<int> v;
    const u64 c0 = rdtsc();
    for ( u64 i = 0; i < n; ++i ) v.push_back(static_cast<int>(i));
    for ( u64 i = 0; i < n; ++i ) v.pop_back();
    const u64 c1 = rdtsc();
    clobber(v.size());
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

f64
bench_heapq(u64 n) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    micron::heapq<int> h;
    const u64 c0 = rdtsc();
    for ( u64 i = 0; i < n; ++i ) h.push(static_cast<int>((i * 2654435761ULL) & 0xffff));
    for ( u64 i = 0; i < n; ++i ) (void)h.pop();
    const u64 c1 = rdtsc();
    clobber(h.size());
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

f64
bench_conarray(u64 n) noexcept
{
  f64 samples[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    micron::conarray<int, 64> a(0);
    const u64 c0 = rdtsc();
    for ( u64 i = 0; i < n; ++i ) a.at(i & 63u, static_cast<int>(i));
    const u64 c1 = rdtsc();
    clobber(static_cast<u64>(a.at(0)));
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(c1 - c0) / static_cast<f64>(n);
  }
  return median(samples, K_MEASUREMENTS);
}

}      // namespace

int
main(void)
{
  calibrate_tsc();
  micron::io::print("=== conmutex bench (TSC ", static_cast<u64>(g_tsc_ghz * 1000.0), " MHz) ===\n");

  micron::io::print("\n-- Bench A: identical critical section, lock type varied (THE swap delta) --\n");
  micron::io::print(" uncontended (1 thread, lock+inc+unlock):\n");
  const f64 mu = bench_lockunlock_uncontended<micron::mutex>(5000000ULL);
  const f64 fu = bench_lockunlock_uncontended<micron::fast_mutex>(5000000ULL);
  report("micron::mutex     ", mu);
  report("micron::fast_mutex", fu);
  if ( mu > 0.0 ) micron::io::print("  -> fast_mutex saves ", static_cast<u64>(((mu - fu) / mu) * 100.0 + 0.5), "% vs mutex\n");

  micron::io::print(" contended (4 threads):\n");
  const f64 mc = bench_lockunlock_contended<micron::mutex>(1000000ULL, 4);
  const f64 fc = bench_lockunlock_contended<micron::fast_mutex>(1000000ULL, 4);
  report("micron::mutex     ", mc);
  report("micron::fast_mutex", fc);
  if ( mc > 0.0 ) micron::io::print("  -> fast_mutex saves ", static_cast<u64>(((mc - fc) / mc) * 100.0 + 0.5), "% vs mutex\n");

  micron::io::print("\n-- Bench B: end-to-end container hot ops (now on fast_mutex) --\n");
  report("conqueue push/pop ", bench_conqueue(65536));
  report("convector push/pop", bench_convector(65536));
  report("heapq push/pop    ", bench_heapq(65536));
  report("conarray at(i,v)  ", bench_conarray(1000000));

  micron::io::print("\n=== done ===\n");
  return 0;
}
