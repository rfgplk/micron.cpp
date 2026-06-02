//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Stack benchmark — C++ standard library baselines (STD SIDE).
//
// Mirrors stack_bench.cpp's kernels (fill+drain = push N + pop N; peek x1024),
// sizes {256, 4096, 65536}, median of K_MEASUREMENTS, ns/op. Lives in its own TU
// because libstdc++ and micron's installed __special/pthread shim cannot share a
// translation unit. Subjects: std::stack<int> (deque), std::vector<int> used as a
// stack, and a std::mutex-guarded vector (baseline for micron::constack).

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <stack>
#include <vector>

namespace
{

constexpr unsigned K_MEASUREMENTS = 5;
constexpr unsigned WARMUP = 2;
constexpr std::uint64_t SIZES[] = { 256, 1ULL << 12, 1ULL << 16 };

// identical rdtsc methodology to the micron side (so the two binaries compare
// directly and neither pays a timer tax in the hot loop).
double g_tsc_ghz = 1.0;

inline std::uint64_t
rdtsc() noexcept
{
  unsigned lo, hi;
  asm volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<std::uint64_t>(hi) << 32) | lo;
}

inline std::uint64_t
wall_ns() noexcept
{
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

void
calibrate_tsc() noexcept
{
  const std::uint64_t n0 = wall_ns();
  const std::uint64_t c0 = rdtsc();
  while ( wall_ns() - n0 < 50000000ULL ) {
  }
  const std::uint64_t c1 = rdtsc();
  const std::uint64_t n1 = wall_ns();
  g_tsc_ghz = static_cast<double>(c1 - c0) / static_cast<double>(n1 - n0);
}

inline double
cyc_to_ns(std::uint64_t cyc) noexcept
{
  return static_cast<double>(cyc) / g_tsc_ghz;
}

inline std::uint64_t
lcg(std::uint64_t &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s >> 16;
}

inline void
clobber(std::uint64_t v) noexcept
{
  asm volatile("" : : "r"(v));
}

double
median(double *xs, unsigned n) noexcept
{
  for ( unsigned i = 1; i < n; ++i ) {
    double k = xs[i];
    unsigned j = i;
    while ( j > 0 && xs[j - 1] > k ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = k;
  }
  return xs[n / 2];
}

void
report(const char *name, std::uint64_t size, double nsop) noexcept
{
  std::printf("  %-22s N=%-6llu %6.2f ns/op\n", name, static_cast<unsigned long long>(size), nsop);
}

double
bench_std_stack(std::uint64_t n, std::uint64_t seed) noexcept
{
  double samples[K_MEASUREMENTS];
  for ( unsigned m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    std::stack<int> s;
    std::uint64_t rng = seed + m;
    const std::uint64_t t0 = rdtsc();
    for ( std::uint64_t i = 0; i < n; ++i ) s.push(static_cast<int>(lcg(rng)));
    std::uint64_t acc = 0;
    for ( std::uint64_t i = 0; i < n; ++i ) {
      acc += static_cast<std::uint64_t>(s.top());
      s.pop();
    }
    const std::uint64_t t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / static_cast<double>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

double
bench_std_vector(std::uint64_t n, std::uint64_t seed) noexcept
{
  double samples[K_MEASUREMENTS];
  for ( unsigned m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    std::vector<int> s;
    std::uint64_t rng = seed + m;
    const std::uint64_t t0 = rdtsc();
    for ( std::uint64_t i = 0; i < n; ++i ) s.push_back(static_cast<int>(lcg(rng)));
    std::uint64_t acc = 0;
    for ( std::uint64_t i = 0; i < n; ++i ) {
      acc += static_cast<std::uint64_t>(s.back());
      s.pop_back();
    }
    const std::uint64_t t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / static_cast<double>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

double
bench_std_mutex_vector(std::uint64_t n, std::uint64_t seed) noexcept
{
  double samples[K_MEASUREMENTS];
  for ( unsigned m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    std::vector<int> s;
    std::mutex mtx;
    std::uint64_t rng = seed + m;
    const std::uint64_t t0 = rdtsc();
    for ( std::uint64_t i = 0; i < n; ++i ) {
      std::lock_guard<std::mutex> lk(mtx);
      s.push_back(static_cast<int>(lcg(rng)));
    }
    std::uint64_t acc = 0;
    for ( std::uint64_t i = 0; i < n; ++i ) {
      std::lock_guard<std::mutex> lk(mtx);
      acc += static_cast<std::uint64_t>(s.back());
      s.pop_back();
    }
    const std::uint64_t t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / static_cast<double>(2 * n);
  }
  return median(samples, K_MEASUREMENTS);
}

template<class Make>
double
bench_peek(std::uint64_t n, std::uint64_t seed, Make make) noexcept
{
  double samples[K_MEASUREMENTS];
  for ( unsigned m = 0; m < K_MEASUREMENTS + WARMUP; ++m ) {
    auto s = make();
    std::uint64_t rng = seed + m;
    for ( std::uint64_t i = 0; i < n; ++i ) s.push_back(static_cast<int>(lcg(rng)));
    const std::uint64_t t0 = rdtsc();
    std::uint64_t acc = 0;
    for ( unsigned i = 0; i < 1024; ++i ) acc += static_cast<std::uint64_t>(s.back());
    const std::uint64_t t1 = rdtsc();
    clobber(acc);
    if ( m >= WARMUP ) samples[m - WARMUP] = cyc_to_ns(t1 - t0) / 1024.0;
  }
  return median(samples, K_MEASUREMENTS);
}

}      // namespace

int
main()
{
  calibrate_tsc();
  std::printf("=== C++ stdlib baselines — fill+drain (push N + pop N), ns/op ===\n");
  for ( std::uint64_t sz : SIZES ) {
    report("std::stack(deque)", sz, bench_std_stack(sz, 0x1111));
    report("std::vector", sz, bench_std_vector(sz, 0x2222));
    report("std::mutex+vector", sz, bench_std_mutex_vector(sz, 0x4444));
    std::printf("\n");
  }
  std::printf("=== peek x1024 (non-destructive back()), ns/op ===\n");
  report("std::vector", 4096, bench_peek(4096, 0x6661, [] { return std::vector<int>(); }));
  return 0;
}
