//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"
#include "../src/trees/octree.hpp"
#include "../src/trees/quadtree.hpp"
#include "../src/trees/rtree.hpp"

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

static volatile u64 sink = 0;

inline u64
now_ns() noexcept
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<u64>(ts.tv_sec) * 1000000000ULL + static_cast<u64>(ts.tv_nsec);
}

void
row_ns(const char *impl, const char *op, usize n, double ns)
{
  char buf[192];
  unsigned long long w = static_cast<unsigned long long>(ns);
  unsigned long long f = static_cast<unsigned long long>((ns - static_cast<double>(w)) * 100.0);
  std::snprintf(buf, sizeof(buf), "  %-10s %-14s N=%-8llu %6llu.%02llu ns/op", impl, op, static_cast<unsigned long long>(n), w, f);
  micron::io::println(buf);
}

void
row_us(const char *impl, const char *op, usize q, double us)
{
  char buf[192];
  unsigned long long w = static_cast<unsigned long long>(us);
  unsigned long long f = static_cast<unsigned long long>((us - static_cast<double>(w)) * 100.0);
  std::snprintf(buf, sizeof(buf), "  %-10s %-14s Q=%-8llu %6llu.%02llu us/query", impl, op, static_cast<unsigned long long>(q), w, f);
  micron::io::println(buf);
}

constexpr usize N = 100000;
constexpr usize Q = 2000;
constexpr float SPAN = 4000.f;

using vec2 = micron::math::vec<float, 2>;
using box2 = micron::math::geometry::aligned_box<float, 2>;
using vec3 = micron::math::vec<float, 3>;
using box3 = micron::math::geometry::aligned_box<float, 3>;

}      // namespace

int
main()
{
  micron::io::println("=== micron spatial-index benchmark (N=", N, ", Q=", Q, ") ===");

  {
    micron::rtree<u32, float, 2> t;
    u64 rng = 1;
    box2 boxes_q[64];
    double ins = 0;
    {
      u64 t0 = now_ns();
      for ( usize i = 0; i < N; ++i ) {
        float x = static_cast<float>(splitmix64(rng++) % 4000), y = static_cast<float>(splitmix64(rng++) % 4000);
        box2 b;
        b.min_corner = vec2{ x, y };
        b.max_corner = vec2{ x + 3.f, y + 3.f };
        t.insert(b, static_cast<u32>(i));
      }
      u64 t1 = now_ns();
      ins = static_cast<double>(t1 - t0) / static_cast<double>(N);
    }
    (void)boxes_q;
    double qt = 0;
    {
      u64 t0 = now_ns();
      for ( usize q = 0; q < Q; ++q ) {
        float x = static_cast<float>(splitmix64(rng++) % 4000), y = static_cast<float>(splitmix64(rng++) % 4000);
        box2 qb;
        qb.min_corner = vec2{ x, y };
        qb.max_corner = vec2{ x + 100.f, y + 100.f };
        t.query(qb, [&](const box2 &, const u32 &v) { sink += v; });
      }
      u64 t1 = now_ns();
      qt = static_cast<double>(t1 - t0) / 1000.0 / static_cast<double>(Q);
    }
    double kt = 0;
    {
      u64 t0 = now_ns();
      for ( usize q = 0; q < Q; ++q ) {
        float x = static_cast<float>(splitmix64(rng++) % 4000), y = static_cast<float>(splitmix64(rng++) % 4000);
        t.nearest(vec2{ x, y }, 10, [&](const box2 &, const u32 &v, float) { sink += v; });
      }
      u64 t1 = now_ns();
      kt = static_cast<double>(t1 - t0) / 1000.0 / static_cast<double>(Q);
    }
    row_ns("rtree", "insert", N, ins);
    row_us("rtree", "range-query", Q, qt);
    row_us("rtree", "knn(k=10)", Q, kt);
  }

  {
    box2 uni;
    uni.min_corner = vec2{ 0, 0 };
    uni.max_corner = vec2{ SPAN, SPAN };
    micron::quadtree<u32> t(uni);
    u64 rng = 2;
    double ins = 0;
    {
      u64 t0 = now_ns();
      for ( usize i = 0; i < N; ++i )
        t.insert(vec2{ static_cast<float>(splitmix64(rng++) % 4000), static_cast<float>(splitmix64(rng++) % 4000) }, static_cast<u32>(i));
      u64 t1 = now_ns();
      ins = static_cast<double>(t1 - t0) / static_cast<double>(N);
    }
    double qt = 0;
    {
      u64 t0 = now_ns();
      for ( usize q = 0; q < Q; ++q ) {
        float x = static_cast<float>(splitmix64(rng++) % 4000), y = static_cast<float>(splitmix64(rng++) % 4000);
        box2 qb;
        qb.min_corner = vec2{ x, y };
        qb.max_corner = vec2{ x + 100.f, y + 100.f };
        t.query(qb, [&](const vec2 &, const u32 &v) { sink += v; });
      }
      u64 t1 = now_ns();
      qt = static_cast<double>(t1 - t0) / 1000.0 / static_cast<double>(Q);
    }
    double kt = 0;
    {
      u64 t0 = now_ns();
      for ( usize q = 0; q < Q; ++q ) {
        float x = static_cast<float>(splitmix64(rng++) % 4000), y = static_cast<float>(splitmix64(rng++) % 4000);
        t.nearest(vec2{ x, y }, 10, [&](const vec2 &, const u32 &v, float) { sink += v; });
      }
      u64 t1 = now_ns();
      kt = static_cast<double>(t1 - t0) / 1000.0 / static_cast<double>(Q);
    }
    row_ns("quadtree", "insert", N, ins);
    row_us("quadtree", "range-query", Q, qt);
    row_us("quadtree", "knn(k=10)", Q, kt);
  }

  {
    box3 uni;
    uni.min_corner = vec3{ 0, 0, 0 };
    uni.max_corner = vec3{ 400, 400, 400 };
    micron::octree<u32> t(uni);
    u64 rng = 3;
    double ins = 0;
    {
      u64 t0 = now_ns();
      for ( usize i = 0; i < N; ++i )
        t.insert(vec3{ static_cast<float>(splitmix64(rng++) % 400), static_cast<float>(splitmix64(rng++) % 400),
                       static_cast<float>(splitmix64(rng++) % 400) },
                 static_cast<u32>(i));
      u64 t1 = now_ns();
      ins = static_cast<double>(t1 - t0) / static_cast<double>(N);
    }
    double qt = 0;
    {
      u64 t0 = now_ns();
      for ( usize q = 0; q < Q; ++q ) {
        float x = static_cast<float>(splitmix64(rng++) % 400), y = static_cast<float>(splitmix64(rng++) % 400),
              z = static_cast<float>(splitmix64(rng++) % 400);
        box3 qb;
        qb.min_corner = vec3{ x, y, z };
        qb.max_corner = vec3{ x + 50.f, y + 50.f, z + 50.f };
        t.query(qb, [&](const vec3 &, const u32 &v) { sink += v; });
      }
      u64 t1 = now_ns();
      qt = static_cast<double>(t1 - t0) / 1000.0 / static_cast<double>(Q);
    }
    double kt = 0;
    {
      u64 t0 = now_ns();
      for ( usize q = 0; q < Q; ++q ) {
        float x = static_cast<float>(splitmix64(rng++) % 400), y = static_cast<float>(splitmix64(rng++) % 400),
              z = static_cast<float>(splitmix64(rng++) % 400);
        t.nearest(vec3{ x, y, z }, 10, [&](const vec3 &, const u32 &v, float) { sink += v; });
      }
      u64 t1 = now_ns();
      kt = static_cast<double>(t1 - t0) / 1000.0 / static_cast<double>(Q);
    }
    row_ns("octree", "insert", N, ins);
    row_us("octree", "range-query", Q, qt);
    row_us("octree", "knn(k=10)", Q, kt);
  }

  micron::io::println("=== done (anti-DCE sink: ", sink, ") ===");
  return 0;
}
