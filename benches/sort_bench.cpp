//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Sort benchmark — micron sort family (MICRON SIDE). The matching
// `sort_bench_std.cpp` runs std::sort / std::stable_sort / std::sort_heap with
// the SAME harness so the two binaries compare directly (std lives in its own TU:
// libstdc++ headers and micron's installed pthread shim cannot share a TU).
//
//   methodology: sorting is destructive, so each timed rep memcpy's a fresh
//   unsorted master into the work buffer and sorts it; a copy-only baseline is
//   measured and SUBTRACTED, leaving the pure sort cost. ns/op and cyc/op (rdtsc)
//   are the median of K_MEASUREMENTS, per element (ops = N).
//
//   sections: (1) general sorts vs patterns; (2) SIMD A/B for the bitonic network
//   (sort::bitonic uses the vectorised path for power-of-two int/float keys,
//   sort::__bitonic is the scalar network); (3) integer-specialised counting/radix.

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/sort/sorts.hpp"
#include "../src/vector.hpp"

namespace
{

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
  micron::timespec_t ts{};
  micron::clock_gettime(micron::clock_monotonic, ts);
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

struct fmt2 {
  u64 whole;
  u32 frac_x100;
};

[[gnu::always_inline]] inline fmt2
to_fmt2(f64 v) noexcept
{
  if ( v < 0 ) v = 0;
  const u64 s = static_cast<u64>(v * 100.0 + 0.5);
  return { s / 100, static_cast<u32>(s % 100) };
}

struct line {
  char buf[256];
  u32 pos;

  constexpr line() noexcept : pos(0) { }

  void
  s(const char *p) noexcept
  {
    while ( *p ) buf[pos++] = *p++;
  }

  void
  pad_to(u32 end_col, u32 written) noexcept
  {
    const u32 want = end_col >= written ? end_col - written : 0;
    if ( want < pos )
      buf[pos++] = ' ';
    else
      while ( pos < want ) buf[pos++] = ' ';
  }

  void
  u_at(u64 v, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    if ( v == 0 )
      tmp[n++] = '0';
    else {
      u64 vv = v;
      while ( vv ) {
        tmp[n++] = '0' + (vv % 10);
        vv /= 10;
      }
    }
    pad_to(end_col, n);
    while ( n ) buf[pos++] = tmp[--n];
  }

  void
  f2_at(fmt2 f, u32 end_col) noexcept
  {
    char tmp[24];
    u32 n = 0;
    u64 w = f.whole;
    if ( w == 0 )
      tmp[n++] = '0';
    else
      while ( w ) {
        tmp[n++] = '0' + (w % 10);
        w /= 10;
      }
    pad_to(end_col, n + 3);
    while ( n ) buf[pos++] = tmp[--n];
    buf[pos++] = '.';
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 / 10);
    buf[pos++] = '0' + static_cast<char>(f.frac_x100 % 10);
  }

  void
  s_at(const char *p, u32 end_col) noexcept
  {
    u32 n = 0;
    while ( p[n] ) ++n;
    pad_to(end_col, n);
    while ( *p ) buf[pos++] = *p++;
  }

  const char *
  str() noexcept
  {
    buf[pos] = '\0';
    return buf;
  }
};

[[gnu::cold]] void
print_col_header()
{
  line h;
  h.s("algo");
  h.s_at("pattern", 24);
  h.s_at("N", 38);
  h.s_at("ns/op", 50);
  h.s_at("cyc/op", 62);
  micron::io::println(h.str());
  micron::io::println("------------------------------------------------------------------------");
}

[[gnu::cold]] void
print_header(const char *section)
{
  micron::io::println("");
  micron::io::println("[", section, "]");
  print_col_header();
}

[[gnu::cold]] void
print_row(const char *algo, const char *pat, u64 n, f64 ns, f64 cyc)
{
  line ln;
  ln.s(algo);
  ln.s_at(pat, 24);
  ln.u_at(n, 38);
  ln.f2_at(to_fmt2(ns), 50);
  ln.f2_at(to_fmt2(cyc), 62);
  micron::io::println(ln.str());
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

// bench one sort over (master pattern) with copy-baseline subtraction.
template<typename T, typename SortFn>
void
bench_one(const char *algo, const char *pat, const T *master, micron::vector<T> &work, usize n, SortFn sortfn) noexcept
{
  const u64 reps = reps_for(n);
  f64 base_ns, base_cyc, full_ns, full_cyc;
  measure(reps, base_ns, base_cyc, [&]() {
    micron::memcpy(work.data(), master, n);
    clobber_val(static_cast<u64>(work[0]));
  });
  measure(reps, full_ns, full_cyc, [&]() {
    micron::memcpy(work.data(), master, n);
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
  using namespace micron;
  io::println("=== MICRON SORT BENCH (ns/op, cyc/op per element; copy-baseline subtracted) ===");

  // <=64K all-pattern sweep: bounds the sort::quick O(n^2) few-unique cliff (the
  // 1M case would run ~12 min; the cliff is already visible at 65536). The 1M
  // scale numbers follow in a random-only section.
  const usize gsizes[] = { 1024, 65536 };

  // ---- (1) general comparison sorts on i32, all patterns ----
  print_header("general sorts  (i32)");
  for ( usize n : gsizes ) {
    i32 *master = new i32[n];
    vector<i32> work(n);
    for ( int pat = 0; pat < 4; ++pat ) {
      fill_pattern(master, n, pat, 0x12345 + pat);
      bench_one<i32>("sort(intro)", PAT_NAME[pat], master, work, n, [](vector<i32> &w) { sort::sort(w); });
      bench_one<i32>("quick", PAT_NAME[pat], master, work, n, [](vector<i32> &w) { sort::quick(w); });
      bench_one<i32>("merge", PAT_NAME[pat], master, work, n, [](vector<i32> &w) { sort::merge(w); });
      bench_one<i32>("heap", PAT_NAME[pat], master, work, n, [](vector<i32> &w) { sort::heap(w); });
      bench_one<i32>("shell", PAT_NAME[pat], master, work, n, [](vector<i32> &w) { sort::shell(w); });
    }
    delete[] master;
  }

  // ---- (1b) scale: 1M random only (quick is safe on random) ----
  print_header("general sorts  (i32, 1M random)");
  {
    const usize n = 1048576;
    i32 *master = new i32[n];
    vector<i32> work(n);
    fill_pattern(master, n, 0, 0x999);
    bench_one<i32>("sort(intro)", "random", master, work, n, [](vector<i32> &w) { sort::sort(w); });
    bench_one<i32>("quick", "random", master, work, n, [](vector<i32> &w) { sort::quick(w); });
    bench_one<i32>("merge", "random", master, work, n, [](vector<i32> &w) { sort::merge(w); });
    bench_one<i32>("heap", "random", master, work, n, [](vector<i32> &w) { sort::heap(w); });
    bench_one<i32>("shell", "random", master, work, n, [](vector<i32> &w) { sort::shell(w); });
    delete[] master;
  }

  // ---- (2) SIMD A/B: bitonic vectorised vs scalar (power-of-two i32 & f32) ----
  const usize psizes[] = { 256, 1024, 4096, 16384, 65536 };
  print_header("bitonic SIMD vs scalar  (i32, random)");
  for ( usize n : psizes ) {
    i32 *master = new i32[n];
    vector<i32> work(n);
    fill_pattern(master, n, 0, 0xBEEF);
    bench_one<i32>("bitonic-simd", "random", master, work, n, [](vector<i32> &w) { sort::bitonic(w); });
    bench_one<i32>("bitonic-scalar", "random", master, work, n,
                   [](vector<i32> &w) { sort::__bitonic(w, [](const i32 &a, const i32 &b) { return a < b; }); });
    delete[] master;
  }
  print_header("bitonic SIMD vs scalar  (f32, random)");
  for ( usize n : psizes ) {
    float *master = new float[n];
    vector<float> work(n);
    fill_pattern(master, n, 0, 0xF00D);
    bench_one<float>("bitonic-simd", "random", master, work, n, [](vector<float> &w) { sort::bitonic(w); });
    bench_one<float>("bitonic-scalar", "random", master, work, n,
                     [](vector<float> &w) { sort::__bitonic(w, [](const float &a, const float &b) { return a < b; }); });
    delete[] master;
  }

  // ---- (3) integer-specialised: counting / radix vs quick (i32, random) ----
  const usize spec[] = { 1024, 65536, 1048576 };
  print_header("non-comparison sorts  (i32, random)");
  for ( usize n : spec ) {
    i32 *master = new i32[n];
    vector<i32> work(n);
    fill_pattern(master, n, 0, 0xC0FFEE);      // wide random -> counting falls back to radix
    bench_one<i32>("radix(b256)", "random", master, work, n, [](vector<i32> &w) { sort::radix(w); });
    bench_one<i32>("counting", "random", master, work, n, [](vector<i32> &w) { sort::counting(w); });
    bench_one<i32>("quick", "random", master, work, n, [](vector<i32> &w) { sort::quick(w); });
    delete[] master;
  }
  // counting in its sweet spot: small key range (0..255)
  print_header("counting in-range  (i32 keys 0..255, random)");
  for ( usize n : spec ) {
    i32 *master = new i32[n];
    vector<i32> work(n);
    u64 s = 0xAB;
    for ( usize i = 0; i < n; ++i ) {
      s = lcg_next(s);
      master[i] = static_cast<i32>((s >> 11) & 0xFF);
    }
    bench_one<i32>("counting", "k0..255", master, work, n, [](vector<i32> &w) { sort::counting(w); });
    bench_one<i32>("radix(b256)", "k0..255", master, work, n, [](vector<i32> &w) { sort::radix(w); });
    bench_one<i32>("quick", "k0..255", master, work, n, [](vector<i32> &w) { sort::quick(w); });
    delete[] master;
  }

  io::println("");
  io::println("=== MICRON SORT BENCH DONE ===");
  return 0;
}
