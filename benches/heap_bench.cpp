//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/linux/sys/sched.hpp"
#include "../src/linux/sys/time.hpp"
#include "../src/std.hpp"

#include "../src/heap/binary_heap.hpp"
#include "../src/heap/binomial.hpp"
#include "../src/heap/fibonacci_heap.hpp"
#include "../src/heap/heapq.hpp"
#include "../src/heap/quake_heap.hpp"

namespace
{

constexpr u32 K_MEASUREMENTS = 5;
constexpr u64 WARMUP_REPS = 3;

constexpr u64 SIZES[] = {
  256,
  1ULL << 12,
  1ULL << 16,
};

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

struct quake_scalar_lt {
  bool
  operator()(const int &a, const int &b) const
  {
    return a < b;
  }
};

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

struct cell {
  const char *name;
  u64 size;
  u64 elem_bytes;
  f64 ns_per_op;
  f64 cyc_per_op;
};

[[gnu::cold]] void
print_col_header()
{
  line h;
  h.s("op");
  h.s_at("N", 32);
  h.s_at("elem_B", 42);
  h.s_at("ns/op", 54);
  h.s_at("cyc/op", 66);
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
print_cell(const cell &c)
{
  line ln;
  ln.s(c.name);
  ln.u_at(c.size, 32);
  ln.u_at(c.elem_bytes, 42);
  ln.f2_at(to_fmt2(c.ns_per_op), 54);
  ln.f2_at(to_fmt2(c.cyc_per_op), 66);
  micron::io::println(ln.str());
}

template<typename Setup, typename Kernel>
[[gnu::noinline]] cell
measure(const char *name, u64 size, u64 elem_bytes, u64 ops_per_rep, u64 reps_per_meas, Setup &&setup, Kernel &&kernel) noexcept
{
  for ( u64 i = 0; i < WARMUP_REPS; ++i ) {
    setup();
    kernel();
  }

  f64 ns_samples[K_MEASUREMENTS];
  f64 cyc_samples[K_MEASUREMENTS];

  for ( u32 m = 0; m < K_MEASUREMENTS; ++m ) {
    setup();
    const u64 t0 = now_ns();
    const u64 c0 = rdtsc();
    for ( u64 i = 0; i < reps_per_meas; ++i ) kernel();
    const u64 c1 = rdtsc();
    const u64 t1 = now_ns();
    const f64 total_ops = static_cast<f64>(reps_per_meas) * static_cast<f64>(ops_per_rep);
    ns_samples[m] = total_ops > 0 ? static_cast<f64>(t1 - t0) / total_ops : 0.0;
    cyc_samples[m] = total_ops > 0 ? static_cast<f64>(c1 - c0) / total_ops : 0.0;
  }

  return cell{ name, size, elem_bytes, median_f64(ns_samples, K_MEASUREMENTS), median_f64(cyc_samples, K_MEASUREMENTS) };
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
      print_cell(measure("fill+drain         ", N, sizeof(int), 2 * N, reps_for(2 * N), setup, kernel));
    }
    {
      Heap h = make(N);
      u64 s = 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < N; ++i ) push(h, (int)(lcg_next(s) & 0xffffff));
      auto setup = [&] { };
      auto kernel = [&] {
        for ( u64 i = 0; i < 1024; ++i ) clobber_val((u64)peek(h));
      };
      print_cell(measure("peek x1024         ", N, sizeof(int), 1024, reps_for(1024), setup, kernel));
    }
  }
}

template<typename Heap, typename Handle, typename Make, typename Ins, typename Dec, typename Pop>
void
sweep_dk(const char *tag, Make &&make, Ins &&ins, Dec &&dec, Pop &&pop)
{
  print_header(tag);
  for ( u64 N : SIZES ) {
    auto setup = [&] { };
    auto kernel = [&] {
      Heap h = make(N);
      micron::fvector<Handle> hs;
      micron::fvector<int> keys;
      hs.reserve(N);
      keys.reserve(N);
      u64 s = 0x9E3779B97F4A7C15ULL;
      for ( u64 i = 0; i < N; ++i ) {
        int v = (int)(lcg_next(s) & 0xffffff);
        keys.push_back(v);
        hs.push_back(ins(h, v));
      }
      for ( u64 i = 0; i < N / 2; ++i ) {
        keys[i] -= (int)((lcg_next(s) & 7) + 1);
        dec(h, hs[i], keys[i]);
      }
      for ( u64 i = 0; i < N; ++i ) clobber_val((u64)pop(h));
    };
    const u64 ops = N + N / 2 + N;
    print_cell(measure("build+dk.5+drain   ", N, sizeof(int), ops, reps_for(ops), setup, kernel));
  }
}

};      // namespace

int
main(void)
{
  micron::posix::cpu_set_t set;
  set.cpu_zero();
  set.cpu_set(0);
  micron::posix::sched_setaffinity(0, sizeof(set), set);

  micron::io::println("=== micron heap benchmark (MICRON side) ===");
  micron::io::println("sizes: 256, 4096, 65536 elements; element type i32");
  micron::io::println("ops:   fill+drain (push N + pop N), peek x1024");
  micron::io::println("timing: rdtsc cyc/op + CLOCK_MONOTONIC ns/op; median of ", K_MEASUREMENTS);
  micron::io::println("compare against: bin/heap_bench_std (same methodology)");

  sweep_heap<micron::binary_heap<int>>(
      "micron::binary_heap<i32>  (max)", [](u64 N) { return micron::binary_heap<int>(N); },
      [](micron::binary_heap<int> &h, int v) { h.insert(int(v)); }, [](micron::binary_heap<int> &h) { return h.get(); },
      [](micron::binary_heap<int> &h) { return h.max(); });

  sweep_heap<micron::fibonacci_heap<int>>(
      "micron::fibonacci_heap<i32> (max)", [](u64) { return micron::fibonacci_heap<int>(); },
      [](micron::fibonacci_heap<int> &h, int v) { h.insert(v); }, [](micron::fibonacci_heap<int> &h) { return h.pop(); },
      [](micron::fibonacci_heap<int> &h) { return h.max(); });

  sweep_heap<micron::quake_heap<int>>(
      "micron::quake_heap<i32>   (min)", [](u64) { return micron::quake_heap<int>(); },
      [](micron::quake_heap<int> &h, int v) { (void)h.insert(v); }, [](micron::quake_heap<int> &h) { return h.extract_min(); },
      [](micron::quake_heap<int> &h) { return h.find_min(); });

  sweep_heap<micron::heapq<int>>(
      "micron::heapq<i32>        (min)", [](u64) { return micron::heapq<int>(); }, [](micron::heapq<int> &h, int v) { h.push(v); },
      [](micron::heapq<int> &h) { return h.pop(); }, [](micron::heapq<int> &h) { return h.top(); });

  sweep_heap<micron::__binomial_heap<int>>(
      "micron::__binomial_heap<i32> (min)", [](u64) { return micron::__binomial_heap<int>(); },
      [](micron::__binomial_heap<int> &h, int v) { h.insert(v); }, [](micron::__binomial_heap<int> &h) { return h.extract_min(); },
      [](micron::__binomial_heap<int> &h) { return h.find_min(); });

  micron::io::println("");
  micron::io::println("--- decrease_key section (handle heaps only) ---");

  sweep_dk<micron::quake_heap<int>, micron::quake_heap<int>::handle>(
      "micron::quake_heap<i32>   (min, dec_key)", [](u64) { return micron::quake_heap<int>(); },
      [](micron::quake_heap<int> &h, int v) { return h.insert(v); },
      [](micron::quake_heap<int> &h, micron::quake_heap<int>::handle hd, int v) { h.decrease_key(hd, v); },
      [](micron::quake_heap<int> &h) { return h.extract_min(); });

  sweep_dk<micron::quake_heap<int, quake_scalar_lt>, micron::quake_heap<int, quake_scalar_lt>::handle>(
      "micron::quake_heap<i32>   (SIMD OFF)", [](u64) { return micron::quake_heap<int, quake_scalar_lt>(); },
      [](micron::quake_heap<int, quake_scalar_lt> &h, int v) { return h.insert(v); },
      [](micron::quake_heap<int, quake_scalar_lt> &h, micron::quake_heap<int, quake_scalar_lt>::handle hd, int v) {
        h.decrease_key(hd, v);
      },
      [](micron::quake_heap<int, quake_scalar_lt> &h) { return h.extract_min(); });

  using bn_node = micron::__binomial_node<int, micron::__binomial_less<int>>;
  sweep_dk<micron::__binomial_heap<int>, bn_node *>(
      "micron::__binomial_heap<i32> (min, dec_key)", [](u64) { return micron::__binomial_heap<int>(); },
      [](micron::__binomial_heap<int> &h, int v) { return h.insert(v); },
      [](micron::__binomial_heap<int> &h, bn_node *hd, int v) { h.decrease_key(hd, v); },
      [](micron::__binomial_heap<int> &h) { return h.extract_min(); });

  micron::io::println("");
  micron::io::println("=== done ===");
  return 0;
}
