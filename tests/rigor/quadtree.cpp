//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/trees/quadtree.hpp"
#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"
#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

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

using vec2 = micron::math::vec<float, 2>;
using box2 = micron::math::geometry::aligned_box<float, 2>;

box2
mkbox(float x0, float y0, float x1, float y1)
{
  box2 b;
  b.min_corner = vec2{ x0, y0 };
  b.max_corner = vec2{ x1, y1 };
  return b;
}

struct rec {
  vec2 p;
  int val;
};

bool
in_box(const vec2 &p, const box2 &b) noexcept
{
  for ( int d = 0; d < 2; ++d )
    if ( p.data[d] < b.min_corner.data[d] || p.data[d] > b.max_corner.data[d] ) return false;
  return true;
}

float
d2(const vec2 &a, const vec2 &b) noexcept
{
  float s = 0.f;
  for ( int d = 0; d < 2; ++d ) s += (a.data[d] - b.data[d]) * (a.data[d] - b.data[d]);
  return s;
}

}      // namespace

int
main(void)
{
  sb::print("=== QUADTREE TESTS ===");

  const box2 uni = mkbox(0, 0, 1000, 1000);

  sb::test_case("construction + reject out of bounds");
  {
    micron::quadtree<int> q(uni);
    sb::require(q.empty());
    sb::require(q.insert(vec2{ 10, 10 }, 1));
    sb::require(!q.insert(vec2{ -5, 5 }, 2));
    sb::require(!q.insert(vec2{ 5, 2000 }, 3));
    sb::require(q.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("box query vs brute-force oracle");
  {
    micron::quadtree<int, float, 4, 16> q(uni);
    std::vector<rec> all;
    u64 rng = 0xABCDULL;
    const int N = 3000;
    for ( int i = 0; i < N; ++i ) {
      vec2 p{ static_cast<float>(splitmix64(rng++) % 1000), static_cast<float>(splitmix64(rng++) % 1000) };
      q.insert(p, i);
      all.push_back(rec{ p, i });
    }
    sb::require(q.size() == static_cast<usize>(N));

    bool ok = true;
    for ( int t = 0; t < 250 && ok; ++t ) {
      float x = static_cast<float>(splitmix64(rng++) % 1000), y = static_cast<float>(splitmix64(rng++) % 1000);
      float w = static_cast<float>(splitmix64(rng++) % 300), h = static_cast<float>(splitmix64(rng++) % 300);
      box2 qb = mkbox(x, y, x + w, y + h);
      std::vector<int> got;
      q.query(qb, [&](const vec2 &, const int &v) { got.push_back(v); });
      std::sort(got.begin(), got.end());
      std::vector<int> want;
      for ( auto &r : all )
        if ( in_box(r.p, qb) ) want.push_back(r.val);
      std::sort(want.begin(), want.end());
      if ( got != want ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("radius query vs brute force");
  {
    micron::quadtree<int, float, 8, 16> q(uni);
    std::vector<rec> all;
    u64 rng = 0x5151ULL;
    const int N = 2000;
    for ( int i = 0; i < N; ++i ) {
      vec2 p{ static_cast<float>(splitmix64(rng++) % 1000), static_cast<float>(splitmix64(rng++) % 1000) };
      q.insert(p, i);
      all.push_back(rec{ p, i });
    }
    bool ok = true;
    for ( int t = 0; t < 150 && ok; ++t ) {
      vec2 c{ static_cast<float>(splitmix64(rng++) % 1000), static_cast<float>(splitmix64(rng++) % 1000) };
      float r = static_cast<float>(10 + splitmix64(rng++) % 150);
      std::vector<int> got;
      q.query_radius(c, r, [&](const vec2 &, const int &v) { got.push_back(v); });
      std::sort(got.begin(), got.end());
      std::vector<int> want;
      for ( auto &rr : all )
        if ( d2(c, rr.p) <= r * r ) want.push_back(rr.val);
      std::sort(want.begin(), want.end());
      if ( got != want ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("k-NN vs brute force (distances)");
  {
    micron::quadtree<int, float, 4, 16> q(uni);
    std::vector<rec> all;
    u64 rng = 0x7777ULL;
    const int N = 2000;
    for ( int i = 0; i < N; ++i ) {
      vec2 p{ static_cast<float>(splitmix64(rng++) % 1000), static_cast<float>(splitmix64(rng++) % 1000) };
      q.insert(p, i);
      all.push_back(rec{ p, i });
    }
    bool ok = true;
    for ( int t = 0; t < 200 && ok; ++t ) {
      vec2 c{ static_cast<float>(splitmix64(rng++) % 1000), static_cast<float>(splitmix64(rng++) % 1000) };
      usize k = 1 + (splitmix64(rng++) % 16);
      std::vector<float> got;
      float last = -1.f;
      bool asc = true;
      q.nearest(c, k, [&](const vec2 &, const int &, float dd) {
        if ( dd + 1e-3f < last ) asc = false;
        last = dd;
        got.push_back(dd);
      });
      if ( !asc ) {
        ok = false;
        break;
      }
      std::vector<float> sc;
      for ( auto &rr : all ) sc.push_back(d2(c, rr.p));
      std::sort(sc.begin(), sc.end());
      usize kk = k < sc.size() ? k : sc.size();
      if ( got.size() != kk ) {
        ok = false;
        break;
      }
      for ( usize i = 0; i < kk; ++i ) {
        float diff = got[i] - sc[i];
        if ( diff < 0 ) diff = -diff;
        if ( diff > 1e-2f * (1.f + sc[i]) ) ok = false;
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("coincident points - overflow chain / MaxDepth guard");
  {
    micron::quadtree<int, float, 4, 8> q(uni);
    for ( int i = 0; i < 2000; ++i ) q.insert(vec2{ 500.f, 500.f }, i);
    sb::require(q.size() == 2000ULL);
    int hits = 0;
    q.query(mkbox(499, 499, 501, 501), [&](const vec2 &, const int &) { ++hits; });
    sb::require(hits == 2000);
    for ( int i = 0; i < 2000; ++i ) sb::require(q.erase(vec2{ 500.f, 500.f }, i));
    sb::require(q.empty());
  }
  sb::end_test_case();

  sb::test_case("erase + re-query");
  {
    micron::quadtree<int, float, 8, 16> q(uni);
    std::vector<rec> all;
    u64 rng = 0x2020ULL;
    const int N = 2000;
    for ( int i = 0; i < N; ++i ) {
      vec2 p{ static_cast<float>(splitmix64(rng++) % 1000), static_cast<float>(splitmix64(rng++) % 1000) };
      q.insert(p, i);
      all.push_back(rec{ p, i });
    }
    int removed = 0;
    for ( int i = 0; i < N; i += 2 ) {
      sb::require(q.erase(all[i].p, all[i].val));
      ++removed;
    }
    sb::require(q.size() == static_cast<usize>(N - removed));
    std::vector<int> got;
    q.for_each([&](const vec2 &, const int &v) { got.push_back(v); });
    std::sort(got.begin(), got.end());
    std::vector<int> want;
    for ( int i = 1; i < N; i += 2 ) want.push_back(i);
    std::sort(want.begin(), want.end());
    sb::require(got == want);
  }
  sb::end_test_case();

  sb::test_case("move ctor / assign");
  {
    micron::quadtree<int> a(uni);
    for ( int i = 0; i < 1000; ++i ) a.insert(vec2{ static_cast<float>(i % 1000), static_cast<float>((i * 7) % 1000) }, i);
    micron::quadtree<int> b(micron::move(a));
    sb::require(b.size() == 1000ULL);
    sb::require(a.size() == 0ULL);
    int hits = 0;
    b.query(uni, [&](const vec2 &, const int &) { ++hits; });
    sb::require(hits == 1000);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
