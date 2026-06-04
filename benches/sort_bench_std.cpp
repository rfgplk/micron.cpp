//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Sort benchmark — C++ standard library baseline (STD SIDE). Companion to
// benches/sort_bench.cpp in a SEPARATE TU (libstdc++ + micron pthread shim cannot
// share a TU). NO micron headers — only libstdc++/libc — with the exact same
// copy-baseline-subtracted rdtsc + CLOCK_MONOTONIC harness so columns line up.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <vector>

namespace
{

using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;
using f64 = double;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 2;

[[gnu::always_inline]] inline u64
rdtsc() noexcept
{
  u32 lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (static_cast<u64>(hi) << 32) | lo;
}

[[gnu::always_inline]] inline u64
now_ns() noexcept
{
  struct timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

[[gnu::always_inline]] inline u64
lcg_next(u64 &s) noexcept
{
  s = s * 6364136223846793005ULL + 1442695040888963407ULL;
  return s;
}

[[gnu::always_inline]] inline void
clobber_val(u64 v) noexcept
{
  asm volatile("" : : "r"(v));
}

f64
median_f64(f64 *xs, u32 n) noexcept
{
  for ( u32 i = 1; i < n; ++i ) {
    const f64 key = xs[i];
    u32 j = i;
    while ( j > 0 && xs[j - 1] > key ) {
      xs[j] = xs[j - 1];
      --j;
    }
    xs[j] = key;
  }
  return xs[n / 2];
}

[[gnu::always_inline]] inline u64
reps_for(u64 ops_per_rep) noexcept
{
  constexpr u64 TARGET = 1ULL << 21;
  if ( ops_per_rep == 0 ) return 64;
  u64 r = TARGET / ops_per_rep;
  if ( r < 4 ) r = 4;
  if ( r > 1ULL << 14 ) r = 1ULL << 14;
  return r;
}

void
print_header(const char *section)
{
  std::printf("\n[%s]\n", section);
  std::printf("%-22s %-12s %10s %10s %10s\n", "algo", "pattern", "N", "ns/op", "cyc/op");
  std::printf("------------------------------------------------------------------------\n");
}

void
print_row(const char *algo, const char *pat, u64 n, f64 ns, f64 cyc)
{
  std::printf("%-22s %-12s %10llu %10.2f %10.2f\n", algo, pat, static_cast<unsigned long long>(n), ns, cyc);
}

template<typename Kernel>
void
measure(u64 reps, f64 &ns_out, f64 &cyc_out, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) kernel();
  f64 ns_s[K_MEASUREMENTS], cyc_s[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    const u64 t0 = now_ns(), c0 = rdtsc();
    for ( u64 i = 0; i < reps; ++i ) kernel();
    const u64 c1 = rdtsc(), t1 = now_ns();
    ns_s[m] = static_cast<f64>(t1 - t0) / static_cast<f64>(reps);
    cyc_s[m] = static_cast<f64>(c1 - c0) / static_cast<f64>(reps);
  }
  ns_out = median_f64(ns_s, K_MEASUREMENTS);
  cyc_out = median_f64(cyc_s, K_MEASUREMENTS);
}

template<typename T>
void
fill_pattern(T *m, usize n, int pat, u64 seed) noexcept
{
  u64 s = seed;
  for ( usize i = 0; i < n; ++i ) {
    if ( pat == 0 ) {
      s = lcg_next(s);
      m[i] = static_cast<T>(s >> 11);
    } else if ( pat == 1 )
      m[i] = static_cast<T>(i);
    else if ( pat == 2 )
      m[i] = static_cast<T>(n - i);
    else
      m[i] = static_cast<T>(i & 7);
  }
}

const char *PAT_NAME[4] = { "random", "sorted", "reverse", "few-uniq" };

template<typename T, typename SortFn>
void
bench_one(const char *algo, const char *pat, const T *master, std::vector<T> &work, usize n, SortFn sortfn) noexcept
{
  const u64 reps = reps_for(n);
  f64 base_ns, base_cyc, full_ns, full_cyc;
  measure(reps, base_ns, base_cyc, [&]() {
    std::memcpy(work.data(), master, n * sizeof(T));
    clobber_val(static_cast<u64>(work[0]));
  });
  measure(reps, full_ns, full_cyc, [&]() {
    std::memcpy(work.data(), master, n * sizeof(T));
    sortfn(work);
    clobber_val(static_cast<u64>(work[n - 1]));
  });
  const f64 ns = full_ns > base_ns ? (full_ns - base_ns) / static_cast<f64>(n) : 0.0;
  const f64 cyc = full_cyc > base_cyc ? (full_cyc - base_cyc) / static_cast<f64>(n) : 0.0;
  print_row(algo, pat, n, ns, cyc);
}

}      // namespace

int
main(void)
{
  std::printf("=== STD SORT BENCH (ns/op, cyc/op per element; copy-baseline subtracted) ===\n");

  const usize gsizes[] = { 1024, 65536, 1048576 };

  print_header("general sorts  (i32)");
  for ( usize n : gsizes ) {
    std::int32_t *master = new std::int32_t[n];
    std::vector<std::int32_t> work(n);
    for ( int pat = 0; pat < 4; ++pat ) {
      fill_pattern(master, n, pat, 0x12345 + pat);
      bench_one<std::int32_t>("std::sort", PAT_NAME[pat], master, work, n, [](std::vector<std::int32_t> &w) { std::sort(w.begin(), w.end()); });
      bench_one<std::int32_t>("std::stable_sort", PAT_NAME[pat], master, work, n, [](std::vector<std::int32_t> &w) { std::stable_sort(w.begin(), w.end()); });
      bench_one<std::int32_t>("std::sort_heap", PAT_NAME[pat], master, work, n, [](std::vector<std::int32_t> &w) {
        std::make_heap(w.begin(), w.end());
        std::sort_heap(w.begin(), w.end());
      });
    }
    delete[] master;
  }

  const usize psizes[] = { 256, 1024, 4096, 16384, 65536 };
  print_header("std::sort baseline  (i32, random)  [vs micron bitonic]");
  for ( usize n : psizes ) {
    std::int32_t *master = new std::int32_t[n];
    std::vector<std::int32_t> work(n);
    fill_pattern(master, n, 0, 0xBEEF);
    bench_one<std::int32_t>("std::sort", "random", master, work, n, [](std::vector<std::int32_t> &w) { std::sort(w.begin(), w.end()); });
    delete[] master;
  }
  print_header("std::sort baseline  (f32, random)  [vs micron bitonic]");
  for ( usize n : psizes ) {
    float *master = new float[n];
    std::vector<float> work(n);
    fill_pattern(master, n, 0, 0xF00D);
    bench_one<float>("std::sort", "random", master, work, n, [](std::vector<float> &w) { std::sort(w.begin(), w.end()); });
    delete[] master;
  }

  std::printf("\n=== STD SORT BENCH DONE ===\n");
  return 0;
}
