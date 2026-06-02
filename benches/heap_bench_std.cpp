//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <queue>
#include <vector>

namespace
{

using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f64 = double;

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 3;

constexpr u64 SIZES[] = { 256, 1ULL << 12, 1ULL << 16 };

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
  if ( r > 1ULL << 16 ) r = 1ULL << 16;
  return r;
}

void
print_header(const char *section)
{
  std::printf("\n[%s]\n", section);
  std::printf("%-20s%10s%10s%14s%12s\n", "op", "N", "elem_B", "ns/op", "cyc/op");
  std::printf("------------------------------------------------------------------------\n");
}

void
print_cell(const char *op, u64 N, u64 elem_b, f64 ns, f64 cyc)
{
  std::printf("%-20s%10llu%10llu%14.2f%12.2f\n", op, (unsigned long long)N, (unsigned long long)elem_b, ns, cyc);
}

template<typename Setup, typename Kernel>
[[gnu::noinline]] void
measure(const char *name, u64 N, u64 elem_b, u64 ops_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
    setup();
    kernel();
  }
  f64 ns_s[K_MEASUREMENTS];
  f64 cyc_s[K_MEASUREMENTS];
  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    setup();
    const u64 t0 = now_ns();
    const u64 c0 = rdtsc();
    for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
    const u64 c1 = rdtsc();
    const u64 t1 = now_ns();
    const f64 total = static_cast<f64>(reps_per_meas) * static_cast<f64>(ops_per_rep);
    ns_s[m] = total > 0 ? static_cast<f64>(t1 - t0) / total : 0.0;
    cyc_s[m] = total > 0 ? static_cast<f64>(c1 - c0) / total : 0.0;
  }
  print_cell(name, N, elem_b, median_f64(ns_s, K_MEASUREMENTS), median_f64(cyc_s, K_MEASUREMENTS));
}

struct std_heap_vec {
  std::vector<int> v;

  void
  push(int x)
  {
    v.push_back(x);
    std::push_heap(v.begin(), v.end());
  }

  int
  pop()
  {
    std::pop_heap(v.begin(), v.end());
    int t = v.back();
    v.pop_back();
    return t;
  }

  int
  top() const
  {
    return v.front();
  }
};

template<typename Heap, typename Make, typename Push, typename Pop, typename Peek>
void
sweep_heap(const char *tag, Make &&make, Push &&push, Pop &&pop, Peek &&peek)
{
  print_header(tag);
  for ( u64 N : SIZES ) {
    {
      Heap h = make(N);
      auto setup = [&] { };
      auto kernel = [&] {
        u64 s = 0x9E3779B97F4A7C15ULL;
        for ( u64 i = 0; i < N; ++i ) push(h, (int)(lcg_next(s) & 0xffffff));
        for ( u64 i = 0; i < N; ++i ) clobber_val((u64)pop(h));
      };
      measure("fill+drain", N, sizeof(int), 2 * N, reps_for(2 * N), setup, kernel);
    }
    {
      Heap h = make(N);
      u64 s = 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < N; ++i ) push(h, (int)(lcg_next(s) & 0xffffff));
      auto setup = [&] { };
      auto kernel = [&] {
        for ( u64 i = 0; i < 1024; ++i ) clobber_val((u64)peek(h));
      };
      measure("peek x1024", N, sizeof(int), 1024, reps_for(1024), setup, kernel);
    }
  }
}

}      // namespace

int
main(void)
{
  std::printf("=== std heap benchmark (STD side) ===\n");
  std::printf("sizes: 256, 4096, 65536 elements; element type i32\n");
  std::printf("ops:   fill+drain (push N + pop N), peek x1024\n");
  std::printf("timing: rdtsc cyc/op + CLOCK_MONOTONIC ns/op; median of %u\n", K_MEASUREMENTS);
  std::printf("compare against: bin/heap_bench (micron, same methodology)\n");

  sweep_heap<std::priority_queue<int>>(
      "std::priority_queue<i32>  (max)", [](u64) { return std::priority_queue<int>(); },
      [](std::priority_queue<int> &h, int v) { h.push(v); },
      [](std::priority_queue<int> &h) {
        int t = h.top();
        h.pop();
        return t;
      },
      [](std::priority_queue<int> &h) { return h.top(); });

  sweep_heap<std_heap_vec>(
      "std::vector+make_heap<i32> (max)", [](u64) { return std_heap_vec{}; }, [](std_heap_vec &h, int v) { h.push(v); },
      [](std_heap_vec &h) { return h.pop(); }, [](std_heap_vec &h) { return h.top(); });

  std::printf("\n=== done ===\n");
  return 0;
}
