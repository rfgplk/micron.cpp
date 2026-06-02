//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/maps/b_map.hpp"
#include "../src/maps/rb_map.hpp"
#include "../src/std.hpp"
#include "../src/trees/b.hpp"

#include <cstdio>
#include <cstdlib>
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

static volatile u64 sink = 0;

inline u64
now_ns() noexcept
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

bool g_color = false;

void
print_row(const char *impl, bool highlight, const char *op, usize n, double ns_per_op)
{
  char buf[192];
  const char *b0 = (g_color && highlight) ? "\033[1m" : "";
  const char *b1 = (g_color && highlight) ? "\033[0m" : "";
  unsigned long long whole = static_cast<unsigned long long>(ns_per_op);
  unsigned long long frac = static_cast<unsigned long long>((ns_per_op - static_cast<double>(whole)) * 100.0);
  std::snprintf(buf, sizeof(buf), "  %s%-12s%s %-10s N=%-9llu %6llu.%02llu ns/op", b0, impl, b1, op, static_cast<unsigned long long>(n),
                whole, frac);
  micron::io::println(buf);
}

constexpr usize K_N = 200000;

template<typename Fn>
double
timed(usize ops, Fn &&fn)
{
  u64 t0 = now_ns();
  fn();
  u64 t1 = now_ns();
  return static_cast<double>(t1 - t0) / static_cast<double>(ops);
}

}      // namespace

int
main()
{
  g_color = std::getenv("TREE_BENCH_COLOR") != nullptr;
  micron::io::println("=== micron ordered-map benchmark (N=", K_N, ") ===");

  {
    micron::b_tree<u64, u64> m;
    double ins = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.insert(splitmix64(i), i);
    });
    double fh = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        u64 *p = m.find(splitmix64(i));
        if ( p ) sink += *p;
      }
    });
    double fm = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        u64 *p = m.find(splitmix64(i + K_N + 0xD00D));
        if ( p ) sink += *p;
      }
    });
    double it = timed(K_N, [&]() { m.for_each([&](const u64 &, u64 &v) { sink += v; }); });
    double er = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.erase(splitmix64(i));
    });
    print_row("b_tree", true, "insert", K_N, ins);
    print_row("b_tree", true, "find-hit", K_N, fh);
    print_row("b_tree", true, "find-miss", K_N, fm);
    print_row("b_tree", true, "iterate", K_N, it);
    print_row("b_tree", true, "erase", K_N, er);
  }

  {
    micron::rb_map<u64, u64> m;
    double ins = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.insert_or_assign(splitmix64(i), i);
    });
    double fh = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        u64 *p = m.find(splitmix64(i));
        if ( p ) sink += *p;
      }
    });
    double fm = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        u64 *p = m.find(splitmix64(i + K_N + 0xD00D));
        if ( p ) sink += *p;
      }
    });
    double er = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.erase(splitmix64(i));
    });
    print_row("rb_map", false, "insert", K_N, ins);
    print_row("rb_map", false, "find-hit", K_N, fh);
    print_row("rb_map", false, "find-miss", K_N, fm);
    print_row("rb_map", false, "erase", K_N, er);
  }

  {
    micron::btree_map<u64, u64> m;
    double ins = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.insert(splitmix64(i), i);
    });
    double fh = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        u64 *p = m.find(splitmix64(i));
        if ( p ) sink += *p;
      }
    });
    double fm = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) {
        u64 *p = m.find(splitmix64(i + K_N + 0xD00D));
        if ( p ) sink += *p;
      }
    });
    double er = timed(K_N, [&]() {
      for ( usize i = 0; i < K_N; ++i ) m.erase(splitmix64(i));
    });
    print_row("btree_map", false, "insert", K_N, ins);
    print_row("btree_map", false, "find-hit", K_N, fh);
    print_row("btree_map", false, "find-miss", K_N, fm);
    print_row("btree_map", false, "erase", K_N, er);
  }

  micron::io::println("=== done (anti-DCE sink: ", sink, ") ===");
  return 0;
}
