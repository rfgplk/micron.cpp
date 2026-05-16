//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/conmap.hpp"
#include "../src/maps/heap_swiss.hpp"
#include "../src/maps/pmap.hpp"
#include "../src/maps/rb_map.hpp"
#include "../src/std.hpp"
#include "../src/trees/art.hpp"

#include <cstdio>
#include <time.h>

namespace
{

[[gnu::always_inline]] inline u64
splitmix64(u64 x) noexcept
{
  x += 0x9E3779B97F4A7C15ULL;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
  return x ^ (x >> 31);
}

[[gnu::always_inline]] inline u64
key_u64(u64 i) noexcept
{
  return splitmix64(i + 1);
}

static volatile u64 sink = 0;

inline u64
now_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

template<typename Fn>
inline double
time_ns_per_op(usize ops, Fn &&fn)
{
  u64 t0 = now_ns();
  fn();
  u64 t1 = now_ns();
  return static_cast<double>(t1 - t0) / static_cast<double>(ops);
}

inline void
print_row(const char *impl, const char *op, usize n, double ns_per_op)
{
  char buf[160];
  unsigned long long whole = static_cast<unsigned long long>(ns_per_op);
  unsigned long long frac = static_cast<unsigned long long>((ns_per_op - static_cast<double>(whole)) * 100.0);
  std::snprintf(buf, sizeof(buf), "  %-16s %-10s N=%-8llu %llu.%02llu ns/op", impl, op, static_cast<unsigned long long>(n), whole, frac);
  micron::io::println(buf);
}

constexpr usize K_N = 50000;

}      // namespace

int
main()
{
  micron::io::println("=== NEW MAPS BENCH ===");

  {
    micron::heap_swiss_map<u64, u64> m;
    double ins = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.insert(key_u64(i), i);
    });
    double fnd = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        auto *p = m.find(key_u64(i));
        if ( p ) sink += *p;
      }
    });
    double era = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.erase(key_u64(i));
    });
    print_row("heap_swiss_map", "insert", K_N, ins);
    print_row("heap_swiss_map", "find", K_N, fnd);
    print_row("heap_swiss_map", "erase", K_N, era);
  }

  {
    micron::conmap<u64, u64> m(K_N * 2);
    double ins = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.insert(key_u64(i), i);
    });
    double fnd = time_ns_per_op(K_N, [&]() {
      u64 v;
      for ( usize i = 0; i < K_N; ++i )
        if ( m.find(key_u64(i), v) ) sink += v;
    });
    double era = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.erase(key_u64(i));
    });
    print_row("conmap", "insert", K_N, ins);
    print_row("conmap", "find", K_N, fnd);
    print_row("conmap", "erase", K_N, era);
  }

  {
    micron::rb_map<u64, u64> m;
    double ins = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.insert(key_u64(i), i);
    });
    double fnd = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        auto *p = m.find(key_u64(i));
        if ( p ) sink += *p;
      }
    });
    double era = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.erase(key_u64(i));
    });
    print_row("rb_map", "insert", K_N, ins);
    print_row("rb_map", "find", K_N, fnd);
    print_row("rb_map", "erase", K_N, era);
  }

  {

    micron::pmap<u64, u64> m;
    constexpr usize SMALL = 5000;
    double ins = time_ns_per_op(SMALL, [&]() {
      for ( usize i = 0; i < SMALL; ++i ) m = m.insert(key_u64(i), i);
    });
    double fnd = time_ns_per_op(SMALL, [&]() {
      for ( usize i = 0; i < SMALL; ++i ) {
        auto *p = m.find(key_u64(i));
        if ( p ) sink += *p;
      }
    });
    print_row("pmap", "insert", SMALL, ins);
    print_row("pmap", "find", SMALL, fnd);
  }

  {
    micron::art<u64, u64> m;
    double ins = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.insert(key_u64(i), i);
    });
    double fnd = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        auto *p = m.find(key_u64(i));
        if ( p ) sink += *p;
      }
    });
    double era = time_ns_per_op(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.erase(key_u64(i));
    });
    print_row("art", "insert", K_N, ins);
    print_row("art", "find", K_N, fnd);
    print_row("art", "erase", K_N, era);
  }

  micron::io::println("sink=", sink);
  return 0;
}
