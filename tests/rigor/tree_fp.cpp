//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../src/io/console.hpp"
#include "../src/io/stdout.hpp"

#include "../src/algorithm/fix.hpp"
#include "../src/function.hpp"
#include "../src/trees.hpp"

#include "../src/std.hpp"

#include "../snowball/snowball.hpp"

#include <cstdio>
#include <map>
#include <set>
#include <vector>

using namespace micron;

using vec2 = micron::math::vec<float, 2>;
using box2 = micron::math::geometry::aligned_box<float, 2>;

static box2
mkbox(float x0, float y0, float x1, float y1)
{
  box2 b;
  b.min_corner = vec2{ x0, y0 };
  b.max_corner = vec2{ x1, y1 };
  return b;
}

[[gnu::always_inline]] static inline u64
splitmix64(u64 &x) noexcept
{
  u64 z = (x += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

static_assert(is_tree<rb_tree<int>> && is_set_tree<rb_tree<int>>);
static_assert(!is_tree_map<rb_tree<int>> && !is_spatial_tree<rb_tree<int>> && !is_ordered_tree<rb_tree<int>>);
static_assert(is_tree<art<int, int>> && is_tree_map<art<int, int>> && !is_ordered_tree<art<int, int>>);
static_assert(!is_set_tree<art<int, int>> && !is_spatial_tree<art<int, int>>);
static_assert(is_tree<b_tree<int, int>> && is_tree_map<b_tree<int, int>> && is_ordered_tree<b_tree<int, int>>);
static_assert(!is_set_tree<b_tree<int, int>> && !is_spatial_tree<b_tree<int, int>>);
static_assert(is_tree<quadtree<int>> && is_spatial_tree<quadtree<int>>);
static_assert(is_tree<octree<int>> && is_spatial_tree<octree<int>>);
static_assert(is_tree<rtree<int>> && is_spatial_tree<rtree<int>>);
static_assert(!is_tree_map<quadtree<int>> && !is_set_tree<quadtree<int>> && !is_tree_map<rtree<int>>);

static_assert(!is_map_class<rb_tree<int>> && !is_map_class<b_tree<int, int>>);

int
main(void)
{
  sb::print("=== TREE FUNCTIONAL-PROGRAMMING TESTS ===");

  sb::test_case("function.hpp fix + algorithm/fix.hpp memo_fix");
  {
    auto fac = micron::fix([](auto self, u64 n) -> u64 { return n < 2 ? 1 : n * self(n - 1); });
    sb::require(fac(0) == 1ULL && fac(5) == 120ULL && fac(10) == 3628800ULL);

    auto fib = micron::memo_fix<u64, u64>([](auto self, u64 n) -> u64 { return n < 2 ? n : self(n - 1) + self(n - 2); });
    sb::require(fib(10) == 55ULL && fib(90) == 2880067194370816120ULL);
  }
  sb::end_test_case();

  sb::test_case("rb_tree: fold / predicates / count_if / find_if / filter / erase_if");
  {
    rb_tree<int> t;
    for ( int i = 1; i <= 10; ++i ) t.insert(i);

    sb::require(micron::fold(t, 0, [](int a, const int &e) { return a + e; }) == 55);
    sb::require(micron::accumulate(t, 0) == 55);
    sb::require(micron::count_if(t, [](const int &e) { return (e & 1) == 0; }) == 5ULL);
    sb::require(micron::all_of(t, [](const int &e) { return e > 0; }));
    sb::require(micron::any_of(t, [](const int &e) { return e == 7; }));
    sb::require(micron::none_of(t, [](const int &e) { return e == 99; }));

    const int *p = micron::find_if(t, [](const int &e) { return e == 7; });
    sb::require(p && *p == 7 && micron::find_if(t, [](const int &e) { return e == 99; }) == nullptr);

    rb_tree<int> evens = micron::filter([](const int &e) { return (e & 1) == 0; }, t);
    sb::require(evens.size() == 5ULL && evens.contains(4) && !evens.contains(3));

    usize removed = micron::erase_if(t, [](const int &e) { return (e & 1) == 1; });
    sb::require(removed == 5ULL && t.size() == 5ULL && t.contains(2) && !t.contains(1));
  }
  sb::end_test_case();

  sb::test_case("b_tree: fold / count_if / find_if / filter / erase_if");
  {
    b_tree<int, int> m;
    for ( int i = 1; i <= 10; ++i ) m.insert(i, i * 10);

    sb::require(micron::fold(m, 0, [](int a, const int &, const int &v) { return a + v; }) == 550);
    sb::require(micron::count_if(m, [](const int &, const int &v) { return v > 50; }) == 5ULL);
    const int *pv = micron::find_if(m, [](const int &k, const int &) { return k == 3; });
    sb::require(pv && *pv == 30);

    b_tree<int, int> big = micron::filter([](const int &, const int &v) { return v > 50; }, m);
    sb::require(big.size() == 5ULL && big.contains(6) && !big.contains(5));

    usize removed = micron::erase_if(m, [](const int &k, const int &) { return (k & 1) == 0; });
    sb::require(removed == 5ULL && m.size() == 5ULL && m.contains(1) && !m.contains(2));
  }
  sb::end_test_case();

  sb::test_case("quadtree: fold / count_if / filter / erase_if");
  {
    quadtree<int> q(mkbox(0, 0, 1000, 1000));
    for ( int i = 1; i <= 10; ++i ) q.insert(vec2{ static_cast<float>(i * 10), static_cast<float>(i * 10) }, i);

    sb::require(micron::fold(q, 0, [](int a, const vec2 &, const int &v) { return a + v; }) == 55);
    sb::require(micron::count_if(q, [](const vec2 &, const int &v) { return v > 5; }) == 5ULL);
    quadtree<int> q2 = micron::filter([](const vec2 &, const int &v) { return v > 5; }, q);
    sb::require(q2.size() == 5ULL);
    sb::require(micron::erase_if(q, [](const vec2 &, const int &v) { return v <= 5; }) == 5ULL && q.size() == 5ULL);
  }
  sb::end_test_case();

  sb::test_case("map: new tree, value/type changing + free fmap");
  {
    rb_tree<int> s;
    for ( int i = 1; i <= 5; ++i ) s.insert(i);

    rb_tree<int> dbl = s.map([](const int &e) { return e * 2; });
    sb::require(dbl.size() == 5ULL && dbl.contains(10) && !dbl.contains(5));
    rb_tree<double> asd = s.map([](const int &e) { return static_cast<double>(e) + 0.5; });
    sb::require(asd.size() == 5ULL && asd.contains(1.5));
    rb_tree<int> dbl2 = micron::fmap([](const int &e) { return e * 2; }, s);
    sb::require(dbl2.contains(8));

    b_tree<int, int> m;
    for ( int i = 1; i <= 5; ++i ) m.insert(i, i);
    b_tree<int, int> inc = m.map([](const int &v) { return v + 100; });
    sb::require(*inc.find(3) == 103);
    b_tree<int, int> mi = m.map([](const int &k, const int &v) { return k * 10 + v; });
    sb::require(*mi.find(4) == 44);

    quadtree<int> q(mkbox(0, 0, 1000, 1000));
    for ( int i = 1; i <= 5; ++i ) q.insert(vec2{ static_cast<float>(i * 10), static_cast<float>(i * 10) }, i);
    quadtree<int> qm = q.map([](const int &v) { return v + 1; });
    sb::require(qm.size() == 5ULL && micron::fold(qm, 0, [](int a, const vec2 &, const int &v) { return a + v; }) == (2 + 3 + 4 + 5 + 6));
  }
  sb::end_test_case();

  sb::test_case("cata: rb (binary) / b_tree (B+) / quadtree (PR) / rtree (R*)");
  {
    rb_tree<int> t;
    for ( int i = 1; i <= 7; ++i ) t.insert(i);
    sb::require(static_cast<usize>(t.cata(0, [](int l, const int &, int r) { return l + 1 + r; })) == t.size());
    sb::require(t.cata(0, [](int l, const int &v, int r) { return l + v + r; }) == 28);
    int h = t.cata(0, [](int l, const int &, int r) { return 1 + (l > r ? l : r); });
    sb::require(h >= 3 && h <= 5);
    sb::require(micron::cata(t, 0, [](int l, const int &v, int r) { return l + v + r; }) == 28);

    b_tree<int, int> m;
    for ( int i = 1; i <= 100; ++i ) m.insert(i, i);
    int n = m.cata([](const int *, const int *, u16 c) -> int { return static_cast<int>(c); },
                   [](const int *, u16, const int *kids, u16 nk) -> int {
                     int s = 0;
                     for ( u16 i = 0; i < nk; ++i ) s += kids[i];
                     return s;
                   });
    sb::require(n == 100 && static_cast<usize>(n) == m.size());
    int sv = m.cata(
        [](const int *, const int *vals, u16 c) -> int {
          int s = 0;
          for ( u16 i = 0; i < c; ++i ) s += vals[i];
          return s;
        },
        [](const int *, u16, const int *kids, u16 nk) -> int {
          int s = 0;
          for ( u16 i = 0; i < nk; ++i ) s += kids[i];
          return s;
        });
    sb::require(sv == 5050);

    auto inner_sum = [](const int *kids, usize nc) {
      int s = 0;
      for ( usize i = 0; i < nc; ++i ) s += kids[i];
      return s;
    };
    quadtree<int> q(mkbox(0, 0, 1000, 1000));
    for ( int i = 1; i <= 20; ++i ) q.insert(vec2{ static_cast<float>((i * 37) % 1000), static_cast<float>((i * 53) % 1000) }, i);
    sb::require(static_cast<usize>(q.cata(0, [](int a, const vec2 &, const int &) { return a + 1; }, inner_sum)) == q.size());

    rtree<int> rt;
    for ( int i = 1; i <= 20; ++i ) rt.insert(vec2{ static_cast<float>(i), static_cast<float>(i) }, i);
    int rc = rt.cata(
        0, [](int a, const box2 &, const int &) { return a + 1; },
        [](const int *kids, u16 nc) {
          int s = 0;
          for ( u16 i = 0; i < nc; ++i ) s += kids[i];
          return s;
        });
    sb::require(rc == 20 && static_cast<usize>(rc) == rt.size());
  }
  sb::end_test_case();

  sb::test_case("traverse: orders / stop / prune / free traverse");
  {
    rb_tree<int> t;
    for ( int i = 1; i <= 10; ++i ) t.insert(i);

    micron::fvector<int> got;
    t.traverse([&](const int &e) { got.push_back(e); });
    sb::require(got.size() == 10ULL && got[0] == 1 && got[9] == 10);

    int seen = 0, last = 0;
    walk_ctl r = t.traverse([&](const int &e) -> walk_ctl {
      ++seen;
      last = e;
      return seen >= 4 ? walk_ctl::stop : walk_ctl::continue_;
    });
    sb::require(seen == 4 && last == 4 && r == walk_ctl::stop);

    int cnt = 0;
    t.traverse([&](const int &e) -> bool {
      ++cnt;
      return e < 3;
    });
    sb::require(cnt == 3);

    int firstpre = -1, firstlvl = -1;
    t.traverse<traversal_order::preorder>([&](const int &e) {
      if ( firstpre < 0 ) firstpre = e;
    });
    t.traverse<traversal_order::level>([&](const int &e) {
      if ( firstlvl < 0 ) firstlvl = e;
    });
    sb::require(firstpre > 0 && firstlvl == firstpre);

    int fcnt = 0;
    micron::traverse(t, [&](const int &) { ++fcnt; });
    sb::require(fcnt == 10);

    b_tree<int, int> m;
    for ( int i = 1; i <= 50; ++i ) m.insert(i, i);
    int c = 0;
    m.traverse([&](const int &k, const int &) -> walk_ctl {
      ++c;
      return k >= 10 ? walk_ctl::stop : walk_ctl::continue_;
    });
    sb::require(c == 10);
  }
  sb::end_test_case();

  sb::test_case("lambda ctors / assign / update / insert_or_modify");
  {
    rb_tree<int> g(10, [](usize i) { return static_cast<int>(i * i); });
    sb::require(g.size() == 10ULL && g.contains(81) && g.contains(0));
    int k = 0;
    rb_tree<int> nul(5, [&k]() { return ++k; });
    sb::require(nul.size() == 5ULL && nul.contains(5));
    rb_tree<int> bld([](auto &s) {
      for ( int i = 0; i < 4; ++i ) s.insert(i * 10);
    });
    sb::require(bld.size() == 4ULL && bld.contains(30));
    rb_tree<int> a;
    a.assign(3, [](usize i) { return static_cast<int>(i) + 100; });
    sb::require(a.contains(101));
    a.assign([](auto &s) {
      s.insert(7);
      s.insert(8);
    });
    sb::require(a.size() == 2ULL && a.contains(7) && !a.contains(100));

    b_tree<int, int> m([](auto &t) {
      for ( int i = 1; i <= 5; ++i ) t.insert(i, i * 10);
    });
    m.update(3, [](const int *cur) { return cur ? *cur + 1 : 0; });
    sb::require(*m.find(3) == 31);
    m.update(99, [](const int *cur) { return cur ? *cur + 1 : 7; });
    sb::require(*m.find(99) == 7);
    b_tree<int, int> wc;
    int words[] = { 1, 2, 1, 3, 1, 2 };
    for ( int w : words ) wc.insert_or_modify(w, [] { return 1; }, [](int &c) { ++c; });
    sb::require(*wc.find(1) == 3 && *wc.find(2) == 2 && *wc.find(3) == 1);

    art<int, int> ar([](auto &t) {
      t.insert(1, 10);
      t.insert(2, 20);
    });
    ar.update(1, [](const int *cur) { return cur ? *cur + 5 : 0; });
    sb::require(*ar.find(1) == 15);

    quadtree<int> q(mkbox(0, 0, 100, 100), [](auto &t) {
      for ( int i = 1; i <= 5; ++i ) t.insert(vec2{ static_cast<float>(i), static_cast<float>(i) }, i);
    });
    sb::require(q.size() == 5ULL);
    rtree<int> rt([](auto &t) {
      for ( int i = 1; i <= 5; ++i ) t.insert(vec2{ static_cast<float>(i), static_cast<float>(i) }, i);
    });
    sb::require(rt.size() == 5ULL);
  }
  sb::end_test_case();

  sb::test_case("oracle: rb_tree fold / map / filter vs std::set");
  {
    u64 rng = 0x5DEECE66DULL;
    for ( int round = 0; round < 60; ++round ) {
      rb_tree<int> t;
      std::set<int> oracle;
      int nn = 10 + static_cast<int>(splitmix64(rng) % 90);
      for ( int i = 0; i < nn; ++i ) {
        int x = static_cast<int>(splitmix64(rng) % 1000);
        t.insert(x);
        oracle.insert(x);
      }
      sb::require(t.size() == oracle.size());

      long s = micron::fold(t, 0L, [](long acc, const int &e) { return acc + e; });
      long os = 0;
      for ( int e : oracle ) os += e;
      sb::require(s == os);

      rb_tree<int> d = t.map([](const int &e) { return e * 2; });
      std::set<int> od;
      for ( int e : oracle ) od.insert(e * 2);
      bool ok = d.size() == od.size();
      d.for_each([&](const int &e) {
        if ( od.find(e) == od.end() ) ok = false;
      });
      sb::require(ok);

      rb_tree<int> ev = micron::filter([](const int &e) { return (e & 1) == 0; }, t);
      usize oc = 0;
      for ( int e : oracle )
        if ( (e & 1) == 0 ) ++oc;
      sb::require(ev.size() == oc);
    }
  }
  sb::end_test_case();

  sb::test_case("oracle: b_tree fold / erase_if vs std::map");
  {
    u64 rng = 0x123456789ULL;
    for ( int round = 0; round < 60; ++round ) {
      b_tree<int, int> m;
      std::map<int, int> oracle;
      int nn = 10 + static_cast<int>(splitmix64(rng) % 90);
      for ( int i = 0; i < nn; ++i ) {
        int kk = static_cast<int>(splitmix64(rng) % 500);
        int vv = static_cast<int>(splitmix64(rng) % 1000);
        m.insert(kk, vv);
        oracle.insert({ kk, vv });
      }
      sb::require(m.size() == oracle.size());

      long s = micron::fold(m, 0L, [](long acc, const int &, const int &v) { return acc + v; });
      long os = 0;
      for ( auto &kv : oracle ) os += kv.second;
      sb::require(s == os);

      micron::erase_if(m, [](const int &kk, const int &) { return (kk & 1) == 0; });
      for ( auto it = oracle.begin(); it != oracle.end(); ) {
        if ( (it->first & 1) == 0 )
          it = oracle.erase(it);
        else
          ++it;
      }
      sb::require(m.size() == oracle.size());
      bool ok = true;
      m.for_each([&](const int &kk, const int &v) {
        auto it = oracle.find(kk);
        if ( it == oracle.end() || it->second != v ) ok = false;
      });
      sb::require(ok);
    }
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
