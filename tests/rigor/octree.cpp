//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/trees/octree.hpp"
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

using vec3 = micron::math::vec<float, 3>;
using box3 = micron::math::geometry::aligned_box<float, 3>;

box3
mkbox(float x0, float y0, float z0, float x1, float y1, float z1)
{
  box3 b;
  b.min_corner = vec3{ x0, y0, z0 };
  b.max_corner = vec3{ x1, y1, z1 };
  return b;
}

struct rec {
  vec3 p;
  int val;
};

bool
in_box(const vec3 &p, const box3 &b) noexcept
{
  for ( int d = 0; d < 3; ++d )
    if ( p.data[d] < b.min_corner.data[d] || p.data[d] > b.max_corner.data[d] ) return false;
  return true;
}

float
d2(const vec3 &a, const vec3 &b) noexcept
{
  float s = 0.f;
  for ( int d = 0; d < 3; ++d ) s += (a.data[d] - b.data[d]) * (a.data[d] - b.data[d]);
  return s;
}

}      // namespace

int
main(void)
{
  sb::print("=== OCTREE TESTS ===");

  const box3 uni = mkbox(0, 0, 0, 500, 500, 500);

  sb::test_case("construction + reject out of bounds");
  {
    micron::octree<int> o(uni);
    sb::require(o.empty());
    sb::require(o.insert(vec3{ 10, 10, 10 }, 1));
    sb::require(!o.insert(vec3{ -1, 5, 5 }, 2));
    sb::require(!o.insert(vec3{ 5, 5, 9999 }, 3));
    sb::require(o.size() == 1ULL);
  }
  sb::end_test_case();

  sb::test_case("box query vs brute-force oracle");
  {
    micron::octree<int, float, 4, 10> o(uni);
    std::vector<rec> all;
    u64 rng = 0xCAFEULL;
    const int N = 3000;
    for ( int i = 0; i < N; ++i ) {
      vec3 p{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      o.insert(p, i);
      all.push_back(rec{ p, i });
    }
    sb::require(o.size() == static_cast<usize>(N));
    bool ok = true;
    for ( int t = 0; t < 250 && ok; ++t ) {
      float x = static_cast<float>(splitmix64(rng++) % 500), y = static_cast<float>(splitmix64(rng++) % 500),
            z = static_cast<float>(splitmix64(rng++) % 500);
      float w = static_cast<float>(splitmix64(rng++) % 200), h = static_cast<float>(splitmix64(rng++) % 200),
            l = static_cast<float>(splitmix64(rng++) % 200);
      box3 qb = mkbox(x, y, z, x + w, y + h, z + l);
      std::vector<int> got;
      o.query(qb, [&](const vec3 &, const int &v) { got.push_back(v); });
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
    micron::octree<int, float, 8, 10> o(uni);
    std::vector<rec> all;
    u64 rng = 0xD00DULL;
    const int N = 2500;
    for ( int i = 0; i < N; ++i ) {
      vec3 p{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      o.insert(p, i);
      all.push_back(rec{ p, i });
    }
    bool ok = true;
    for ( int t = 0; t < 150 && ok; ++t ) {
      vec3 c{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      float r = static_cast<float>(10 + splitmix64(rng++) % 80);
      std::vector<int> got;
      o.query_radius(c, r, [&](const vec3 &, const int &v) { got.push_back(v); });
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
    micron::octree<int, float, 4, 10> o(uni);
    std::vector<rec> all;
    u64 rng = 0xBEADULL;
    const int N = 2500;
    for ( int i = 0; i < N; ++i ) {
      vec3 p{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      o.insert(p, i);
      all.push_back(rec{ p, i });
    }
    bool ok = true;
    for ( int t = 0; t < 200 && ok; ++t ) {
      vec3 c{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      usize k = 1 + (splitmix64(rng++) % 16);
      std::vector<float> got;
      float last = -1.f;
      bool asc = true;
      o.nearest(c, k, [&](const vec3 &, const int &, float dd) {
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

  sb::test_case("coincident points - overflow chain guard");
  {
    micron::octree<int, float, 4, 6> o(uni);
    for ( int i = 0; i < 1500; ++i ) o.insert(vec3{ 250.f, 250.f, 250.f }, i);
    sb::require(o.size() == 1500ULL);
    int hits = 0;
    o.query(mkbox(249, 249, 249, 251, 251, 251), [&](const vec3 &, const int &) { ++hits; });
    sb::require(hits == 1500);
    for ( int i = 0; i < 1500; ++i ) sb::require(o.erase(vec3{ 250.f, 250.f, 250.f }, i));
    sb::require(o.empty());
  }
  sb::end_test_case();

  sb::test_case("erase + re-query");
  {
    micron::octree<int, float, 8, 10> o(uni);
    std::vector<rec> all;
    u64 rng = 0x3030ULL;
    const int N = 2000;
    for ( int i = 0; i < N; ++i ) {
      vec3 p{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      o.insert(p, i);
      all.push_back(rec{ p, i });
    }
    int removed = 0;
    for ( int i = 0; i < N; i += 2 ) {
      sb::require(o.erase(all[i].p, all[i].val));
      ++removed;
    }
    sb::require(o.size() == static_cast<usize>(N - removed));
    std::vector<int> got;
    o.for_each([&](const vec3 &, const int &v) { got.push_back(v); });
    std::sort(got.begin(), got.end());
    std::vector<int> want;
    for ( int i = 1; i < N; i += 2 ) want.push_back(i);
    std::sort(want.begin(), want.end());
    sb::require(got == want);
  }
  sb::end_test_case();

  sb::test_case("reserve_nodes: pre-sized arena never grows under insert; clear() retains the slab");
  {
    const box3 uni = mkbox(0, 0, 0, 500, 500, 500);
    micron::octree<int, float, 4, 10> o(uni);
    o.reserve_nodes(256);
    const usize cap0 = o.nodes_reserved();
    sb::require(cap0 >= 256);
    u64 rng = 0x9E3779B97F4A7C15ull;
    for ( int i = 0; i < 200; ++i ) {
      vec3 p{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      o.insert(p, i);
    }
    sb::require(o.nodes_used() <= o.nodes_reserved());
    sb::require(o.nodes_reserved() == cap0);      // never grew past the reservation
    // clear retains the slab: a rebuild allocates ZERO new capacity
    o.clear();
    sb::require(o.size() == 0);
    sb::require(o.nodes_reserved() == cap0);
    rng = 0x9E3779B97F4A7C15ull;
    for ( int i = 0; i < 200; ++i ) {
      vec3 p{ static_cast<float>(splitmix64(rng++) % 500), static_cast<float>(splitmix64(rng++) % 500),
              static_cast<float>(splitmix64(rng++) % 500) };
      o.insert(p, i);
    }
    sb::require(o.nodes_reserved() == cap0);
    sb::require(o.size() == 200);
  }
  sb::end_test_case();

  sb::test_case("erase RECLAIMS nodes - churn must not grow the arena without bound");
  {
    // erase() used to remove the entry and decrement count, and stop there: no emptied leaf
    // freed, no overflow node unlinked, no childless internal node collapsed. nodes_used() was
    // monotonically non-decreasing, so a steady-state insert/erase workload grew forever. The
    // assertion is on nodes_used(), not on correctness -- the old code answered every query right
    // while leaking, which is exactly why no existing test saw it.
    const box3 uni = mkbox(0, 0, 0, 1000, 1000, 1000);
    micron::octree<u32> t(uni);

    constexpr int N = 4000;
    u64 rng = 0x0C7EE501ULL;
    vec3 pts[N];
    for ( int i = 0; i < N; ++i ) pts[i] = vec3{ static_cast<float>(splitmix64(rng++) % 900), static_cast<float>(splitmix64(rng++) % 900),
                     static_cast<float>(splitmix64(rng++) % 900) };

    for ( int i = 0; i < N; ++i ) t.insert(pts[i], static_cast<u32>(i));
    const usize peak = t.nodes_used();
    sb::require(peak > 0ULL);
    sb::require(t.size() == static_cast<usize>(N));

    // drain completely: every node must come back
    for ( int i = 0; i < N; ++i ) sb::require(t.erase(pts[i], static_cast<u32>(i)));
    sb::require(t.size() == 0ULL);
    sb::require(t.nodes_used() == 0ULL);

    // and a steady-state churn must not ratchet the high-water mark upward
    for ( int i = 0; i < N; ++i ) t.insert(pts[i], static_cast<u32>(i));
    const usize after_rebuild = t.nodes_used();
    sb::require(after_rebuild <= peak);
    for ( u32 round = 0; round < 6; ++round ) {
      for ( int i = 0; i < N; ++i ) t.erase(pts[i], static_cast<u32>(i));
      for ( int i = 0; i < N; ++i ) t.insert(pts[i], static_cast<u32>(i));
      sb::require(t.size() == static_cast<usize>(N));
      sb::require(t.nodes_used() <= after_rebuild);
    }

    // NEGATIVE CONTROL: a tree that is never drained must still report nodes in use, so the
    // assertion above is measuring reclaim and not a nodes_used() that always answers zero
    sb::require(t.nodes_used() > 0ULL);

    // and every point is still findable after all that churn
    int found = 0;
    t.for_each([&](const vec3 &, const u32 &) { ++found; });
    sb::require(found == N);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
