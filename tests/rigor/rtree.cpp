//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Rigor for the R*-tree micron::rtree<Value,F,Dim> (src/trees/rtree.hpp).

#include "../src/trees/rtree.hpp"
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

using box2 = micron::math::geometry::aligned_box<float, 2>;

box2
mkbox(float x0, float y0, float x1, float y1)
{
  box2 b;
  b.min_corner.data[0] = x0 < x1 ? x0 : x1;
  b.min_corner.data[1] = y0 < y1 ? y0 : y1;
  b.max_corner.data[0] = x0 < x1 ? x1 : x0;
  b.max_corner.data[1] = y0 < y1 ? y1 : y0;
  return b;
}

bool
bx_intersect(const box2 &a, const box2 &b) noexcept
{
  for ( int d = 0; d < 2; ++d ) {
    if ( a.min_corner.data[d] > b.max_corner.data[d] ) return false;
    if ( a.max_corner.data[d] < b.min_corner.data[d] ) return false;
  }
  return true;
}

float
mindist2_pt(float px, float py, const box2 &b) noexcept
{
  float s = 0.f;
  float v0 = px;
  if ( v0 < b.min_corner.data[0] )
    s += (b.min_corner.data[0] - v0) * (b.min_corner.data[0] - v0);
  else if ( v0 > b.max_corner.data[0] )
    s += (v0 - b.max_corner.data[0]) * (v0 - b.max_corner.data[0]);
  float v1 = py;
  if ( v1 < b.min_corner.data[1] )
    s += (b.min_corner.data[1] - v1) * (b.min_corner.data[1] - v1);
  else if ( v1 > b.max_corner.data[1] )
    s += (v1 - b.max_corner.data[1]) * (v1 - b.max_corner.data[1]);
  return s;
}

struct rec {
  box2 b;
  int val;
};

}      // namespace

int
main(void)
{
  sb::print("=== RTREE TESTS ===");

  sb::test_case("construction - empty");
  {
    micron::rtree<int, float, 2> t;
    sb::require(t.empty());
    sb::require(t.size() == 0ULL);
    int hits = 0;
    t.query(mkbox(0, 0, 100, 100), [&](const box2 &, const int &) { ++hits; });
    sb::require(hits == 0);
  }
  sb::end_test_case();

  sb::test_case("single insert + query hit/miss");
  {
    micron::rtree<int, float, 2> t;
    t.insert(mkbox(10, 10, 20, 20), 1);
    sb::require(t.size() == 1ULL);
    int hits = 0, val = 0;
    t.query(mkbox(15, 15, 16, 16), [&](const box2 &, const int &v) {
      ++hits;
      val = v;
    });
    sb::require(hits == 1 && val == 1);
    hits = 0;
    t.query(mkbox(100, 100, 110, 110), [&](const box2 &, const int &) { ++hits; });
    sb::require(hits == 0);
  }
  sb::end_test_case();

  sb::test_case("range query vs brute-force oracle (M=4)");
  {
    micron::rtree<int, float, 2, 4> t;
    std::vector<rec> all;
    u64 rng = 0x1357ULL;
    const int N = 1500;
    for ( int i = 0; i < N; ++i ) {
      float x = static_cast<float>(splitmix64(rng++) % 1000);
      float y = static_cast<float>(splitmix64(rng++) % 1000);
      float w = static_cast<float>(1 + splitmix64(rng++) % 30);
      float h = static_cast<float>(1 + splitmix64(rng++) % 30);
      box2 b = mkbox(x, y, x + w, y + h);
      t.insert(b, i);
      all.push_back(rec{ b, i });
    }
    sb::require(t.size() == static_cast<usize>(N));

    bool ok = true;
    for ( int q = 0; q < 250 && ok; ++q ) {
      float x = static_cast<float>(splitmix64(rng++) % 1000);
      float y = static_cast<float>(splitmix64(rng++) % 1000);
      float w = static_cast<float>(5 + splitmix64(rng++) % 200);
      float h = static_cast<float>(5 + splitmix64(rng++) % 200);
      box2 qb = mkbox(x, y, x + w, y + h);

      std::vector<int> got;
      t.query(qb, [&](const box2 &, const int &v) { got.push_back(v); });
      std::sort(got.begin(), got.end());

      std::vector<int> want;
      for ( auto &r : all )
        if ( bx_intersect(r.b, qb) ) want.push_back(r.val);
      std::sort(want.begin(), want.end());

      if ( got != want ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("k-NN vs brute-force oracle");
  {
    micron::rtree<int, float, 2, 4> t;
    std::vector<rec> all;
    u64 rng = 0x2468ULL;
    const int N = 1200;

    for ( int i = 0; i < N; ++i ) {
      float x = static_cast<float>(i % 200) * 5.f;
      float y = static_cast<float>(i / 200) * 5.f + static_cast<float>(i % 7);
      box2 b = mkbox(x, y, x, y);
      t.insert(b, i);
      all.push_back(rec{ b, i });
    }
    bool ok = true;
    for ( int q = 0; q < 200 && ok; ++q ) {
      float px = static_cast<float>(splitmix64(rng++) % 1000);
      float py = static_cast<float>(splitmix64(rng++) % 1000);
      const usize k = 1 + (splitmix64(rng++) % 12);

      std::vector<float> got_d;
      float lastd = -1.f;
      bool asc = true;
      t.nearest(micron::math::vec<float, 2>{ px, py }, k, [&](const box2 &, const int &, float d) {
        if ( d + 1e-3f < lastd ) asc = false;
        lastd = d;
        got_d.push_back(d);
      });
      if ( !asc ) {
        ok = false;
        break;
      }

      std::vector<float> scored;
      for ( auto &r : all ) scored.push_back(mindist2_pt(px, py, r.b));
      std::sort(scored.begin(), scored.end());
      const usize kk = k < scored.size() ? k : scored.size();
      if ( got_d.size() != kk ) {
        ok = false;
        break;
      }
      for ( usize i = 0; i < kk; ++i ) {
        float diff = got_d[i] - scored[i];
        if ( diff < 0.f ) diff = -diff;
        if ( diff > 1e-2f * (1.f + scored[i]) ) ok = false;
      }
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("erase + re-query (M=4)");
  {
    micron::rtree<int, float, 2, 4> t;
    std::vector<rec> all;
    u64 rng = 0x99AAULL;
    const int N = 1200;
    for ( int i = 0; i < N; ++i ) {
      float x = static_cast<float>(splitmix64(rng++) % 800);
      float y = static_cast<float>(splitmix64(rng++) % 800);
      box2 b = mkbox(x, y, x + 4, y + 4);
      t.insert(b, i);
      all.push_back(rec{ b, i });
    }

    int removed = 0;
    for ( int i = 0; i < N; i += 3 ) {
      sb::require(t.erase(all[i].b, all[i].val));
      ++removed;
    }
    sb::require(t.size() == static_cast<usize>(N - removed));

    sb::require(!t.erase(all[0].b, 999999));

    std::vector<int> got;
    t.for_each([&](const box2 &, const int &v) { got.push_back(v); });
    std::sort(got.begin(), got.end());
    std::vector<int> want;
    for ( int i = 0; i < N; ++i )
      if ( i % 3 != 0 ) want.push_back(i);
    std::sort(want.begin(), want.end());
    sb::require(got == want);

    bool ok = true;
    for ( int q = 0; q < 100 && ok; ++q ) {
      float x = static_cast<float>(splitmix64(rng++) % 800);
      float y = static_cast<float>(splitmix64(rng++) % 800);
      box2 qb = mkbox(x, y, x + 80, y + 80);
      std::vector<int> g;
      t.query(qb, [&](const box2 &, const int &v) { g.push_back(v); });
      std::sort(g.begin(), g.end());
      std::vector<int> w;
      for ( int i = 0; i < N; ++i )
        if ( i % 3 != 0 && bx_intersect(all[i].b, qb) ) w.push_back(i);
      std::sort(w.begin(), w.end());
      if ( g != w ) ok = false;
    }
    sb::require(ok);
  }
  sb::end_test_case();

  sb::test_case("coincident + heavy overlap");
  {
    micron::rtree<int, float, 2, 4> t;
    for ( int i = 0; i < 400; ++i ) t.insert(mkbox(0, 0, 10, 10), i);
    sb::require(t.size() == 400ULL);
    int hits = 0;
    t.query(mkbox(5, 5, 6, 6), [&](const box2 &, const int &) { ++hits; });
    sb::require(hits == 400);

    for ( int i = 0; i < 400; ++i ) sb::require(t.erase(mkbox(0, 0, 10, 10), i));
    sb::require(t.empty());
  }
  sb::end_test_case();

  sb::test_case("3D instantiation + query");
  {
    using box3 = micron::math::geometry::aligned_box<float, 3>;
    micron::rtree<int, float, 3, 8> t;
    for ( int i = 0; i < 500; ++i ) {
      box3 b;
      float x = static_cast<float>(i % 10) * 10.f, y = static_cast<float>((i / 10) % 10) * 10.f, z = static_cast<float>(i / 100) * 10.f;
      b.min_corner = micron::math::vec<float, 3>{ x, y, z };
      b.max_corner = micron::math::vec<float, 3>{ x + 2, y + 2, z + 2 };
      t.insert(b, i);
    }
    sb::require(t.size() == 500ULL);
    box3 q;
    q.min_corner = micron::math::vec<float, 3>{ -1, -1, -1 };
    q.max_corner = micron::math::vec<float, 3>{ 21, 21, 5 };
    int hits = 0;
    t.query(q, [&](const box3 &, const int &) { ++hits; });

    int want = 0;
    for ( int i = 0; i < 500; ++i ) {
      float x = static_cast<float>(i % 10) * 10.f, y = static_cast<float>((i / 10) % 10) * 10.f, z = static_cast<float>(i / 100) * 10.f;
      bool hit = !(x > 21 || x + 2 < -1 || y > 21 || y + 2 < -1 || z > 5 || z + 2 < -1);
      if ( hit ) ++want;
    }
    sb::require(hits == want);
  }
  sb::end_test_case();

  sb::test_case("move ctor / assign");
  {
    micron::rtree<int, float, 2> a;
    for ( int i = 0; i < 1000; ++i ) a.insert(mkbox(static_cast<float>(i), 0, static_cast<float>(i) + 1, 1), i);
    micron::rtree<int, float, 2> b(micron::move(a));
    sb::require(b.size() == 1000ULL);
    sb::require(a.size() == 0ULL);
    int hits = 0;
    b.query(mkbox(500, 0, 501, 1), [&](const box2 &, const int &) { ++hits; });
    sb::require(hits >= 1);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
