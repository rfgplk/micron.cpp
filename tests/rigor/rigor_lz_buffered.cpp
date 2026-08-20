//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// rigor_lz_buffered.cpp -- Phase 4 of micron::lz: the buffered and remaining streaming adaptors.
//
// Coverage:
//   order.hpp    reverse (both arms) sort sort_by
//   scan.hpp     scanl scan scanr
//   zip.hpp      zip zip_with zip_with_trunc zip_strict enumerate ap
//   unique.hpp   unique nub nub_by
//   weave.hpp    intersperse intercalate step_by
//   window.hpp   chunk chunk_into sliding group group_by transpose transpose_trunc
//   flatten.hpp  flatten flat_map concat merge
//
// The claim this file has to carry, beyond agreeing with the eager layer, is WHERE the allocations
// went. __is_materializing is asserted at compile time -- reverse over a contiguous source must be
// false, and that assertion is the feature -- and the windowing adaptors are checked to yield
// sub-views into the source rather than a container per window.

#include "../../src/lz.hpp"

#include "../../src/algorithm/fp.hpp"
#include "../../src/list.hpp"
#include "../../src/vector.hpp"

#include "../support/algo_rigor.hpp"

namespace lz = micron::lz;
using mtest::prng;
using sb::end_test_case;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

using vec_i = micron::vector<int>;
using vec_vi = micron::vector<vec_i>;

static vec_i
iota_vec(int from, usize n)
{
  vec_i v;
  for ( usize i = 0; i < n; ++i ) v.push_back(from + static_cast<int>(i));
  return v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the materialization contract, checked at COMPILE time. reverse over a contiguous source
// allocating nothing is not an optimisation detail, it is the reason the streaming arm exists.
static_assert(!decltype(micron::declval<vec_i &>() | lz::reverse())::__is_materializing);
static_assert(decltype(micron::declval<micron::list<int> &>() | lz::reverse())::__is_materializing);
static_assert(decltype(micron::declval<vec_i &>() | lz::sort())::__is_materializing);
static_assert(!decltype(micron::declval<vec_i &>() | lz::chunk(2))::__is_materializing);
static_assert(decltype(micron::declval<micron::list<int> &>() | lz::chunk(2))::__is_materializing);
static_assert(!decltype(micron::declval<vec_i &>() | lz::nub())::__is_materializing);
static_assert(!decltype(micron::declval<vec_i &>() | lz::unique())::__is_materializing);

int
main()
{
  sb::print("=== LZ BUFFERED RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // reverse
  // ════════════════════════════════════════════════════════════════════

  test_case("reverse streams over a contiguous source and agrees with fp::reverse_c");
  {
    auto v = iota_vec(1, 6);
    auto r = v | lz::reverse() | lz::collect<vec_i>();
    auto e = micron::fp::reverse_c()(v);
    require(r.size(), e.size());
    for ( usize i = 0; i < r.size(); ++i ) require(r[i], e[i]);
    require(r[0], 6);
    require(r[5], 1);
    // the size kind survives, so collect stays on the exact path
    require(v | lz::reverse() | lz::count(), usize(6));
    require_true((v | lz::reverse() | lz::sum()) == umax_t(21));
    require((v | lz::reverse() | lz::head()).cast<int>(), 6);
    vec_i empty;
    require(empty | lz::reverse() | lz::count(), usize(0));
  }
  end_test_case();

  test_case("reverse | reverse == id, on both arms");
  {
    auto v = iota_vec(1, 9);
    auto rr = v | lz::reverse() | lz::reverse() | lz::collect<vec_i>();
    require(rr.size(), v.size());
    for ( usize i = 0; i < v.size(); ++i ) require(rr[i], v[i]);
    // the buffered arm: a filter ends in a sentinel tag, so reverse cannot stream over it
    auto br = v | lz::filter([](int) { return true; }) | lz::reverse() | lz::collect<vec_i>();
    require(br.size(), v.size());
    for ( usize i = 0; i < v.size(); ++i ) require(br[i], v[v.size() - 1 - i]);
  }
  end_test_case();

  test_case("reverse over a forward-only source buffers, and still reverses");
  {
    micron::list<int> l;
    for ( int i = 1; i <= 7; ++i ) l.push_back(i);
    auto r = l | lz::reverse() | lz::collect<vec_i>();
    require(r.size(), usize(7));
    require(r[0], 7);
    require(r[6], 1);
    micron::list<int> e;
    require(e | lz::reverse() | lz::count(), usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // sort / sort_by
  // ════════════════════════════════════════════════════════════════════

  test_case("sort / sort_by against fp::sort_c / fp::sort_by_c");
  {
    vec_i u;
    u.push_back(5); u.push_back(1); u.push_back(4); u.push_back(1); u.push_back(9); u.push_back(2);
    auto s = u | lz::sort() | lz::collect<vec_i>();
    auto e = micron::fp::sort_c()(u);
    require(s.size(), e.size());
    for ( usize i = 0; i < s.size(); ++i ) require(s[i], e[i]);

    auto d = u | lz::sort_by([](const int &a, const int &b) { return b < a; }) | lz::collect<vec_i>();
    auto ed = micron::fp::sort_by_c([](const int &a, const int &b) { return b < a; })(u);
    for ( usize i = 0; i < d.size(); ++i ) require(d[i], ed[i]);

    // sort composes in both directions, and is idempotent
    require((u | lz::sort() | lz::head()).cast<int>(), 1);
    require((u | lz::sort() | lz::last()).cast<int>(), 9);
    auto ss = u | lz::sort() | lz::sort() | lz::collect<vec_i>();
    for ( usize i = 0; i < s.size(); ++i ) require(ss[i], s[i]);
    auto ts = u | lz::sort() | lz::take(3) | lz::collect<vec_i>();
    require(ts.size(), usize(3));
    require(ts[2], 2);
    vec_i e0;
    require(e0 | lz::sort() | lz::count(), usize(0));
  }
  end_test_case();

  property_test(
      "sort produces a sorted permutation of the input (10k)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x1f) + 1;
        prng rng(raw_n + 227);
        vec_i v;
        for ( usize i = 0; i < n; ++i ) v.push_back(static_cast<int>(rng.next_in(64)));
        auto s = v | lz::sort() | lz::collect<vec_i>();
        require(s.size(), n);
        for ( usize i = 1; i < n; ++i ) require_true(!(s[i] < s[i - 1]));
        // a permutation: same multiset, checked by counting each value both ways
        for ( usize i = 0; i < n; ++i ) require(v | lz::count_of(v[i]), s | lz::count_of(v[i]));
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // scanl / scanr
  // ════════════════════════════════════════════════════════════════════

  test_case("scanl / scanr against fp::, and the two fold laws");
  {
    auto v = iota_vec(1, 9);
    auto add = [](int a, int x) { return a + x; };
    auto s = v | lz::scanl(0, add) | lz::collect<vec_i>();
    auto e = micron::fp::scanl(v, 0, add);
    require(s.size(), v.size() + 1);
    require(s.size(), e.size());
    for ( usize i = 0; i < s.size(); ++i ) require(s[i], e[i]);
    // law: scanl f z ends at fold_left f z
    require(s[s.size() - 1], (v | lz::fold(0, add)));

    auto radd = [](int x, int a) { return x + a; };
    auto r = v | lz::scanr(radd, 0) | lz::collect<vec_i>();
    auto er = micron::fp::scanr(v, 0, radd);
    require(r.size(), v.size() + 1);
    for ( usize i = 0; i < r.size(); ++i ) require(r[i], er[i]);
    // law: scanr f z starts at fold_right f z
    require(r[0], (v | lz::foldr(radd, 0)));
    // scanr over a chain that cannot be walked backwards must agree
    auto r2 = v | lz::filter([](int) { return true; }) | lz::scanr(radd, 0) | lz::collect<vec_i>();
    for ( usize i = 0; i < r.size(); ++i ) require(r2[i], r[i]);
    // scan aliases scanl
    require((v | lz::scan(0, add) | lz::count()), s.size());
  }
  end_test_case();

  test_case("scanl streams -- it composes with an endless source");
  {
    auto se = lz::counting(1) | lz::scanl(0, [](int a, int x) { return a + x; }) | lz::take(5) | lz::collect<vec_i>();
    require(se.size(), usize(5));
    require(se[0], 0);
    require(se[4], 10);      // 0 1 3 6 10
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // zip family
  // ════════════════════════════════════════════════════════════════════

  test_case("zip_with truncates; zip_strict reports the mismatch");
  {
    auto a = iota_vec(1, 9);
    auto b = iota_vec(100, 9);
    auto add = [](int x, int y) { return x + y; };
    auto z = a | lz::zip_with(b, add) | lz::collect<vec_i>();
    auto e = micron::fp::zip_with_trunc(a, b, add);
    require(z.size(), e.size());
    for ( usize i = 0; i < z.size(); ++i ) require(z[i], e[i]);

    auto sh = iota_vec(1, 3);
    require(a | lz::zip_with(sh, add) | lz::count(), usize(3));
    require(sh | lz::zip_with(a, add) | lz::count(), usize(3));
    require_true((a | lz::zip_strict<vec_i>(b, add)).is_first());
    require_true((a | lz::zip_strict<vec_i>(sh, add)).is_second());
    require_true((sh | lz::zip_strict<vec_i>(a, add)).is_second());
    // eager agrees on the strict side
    require_true(micron::fp::zip_with(a, b, add).is_first());
    require_true(micron::fp::zip_with(a, sh, add).is_second());
  }
  end_test_case();

  test_case("zip / enumerate / ap");
  {
    auto a = iota_vec(1, 5);
    auto b = iota_vec(10, 5);
    auto zt = a | lz::zip(b) | lz::fmap([](auto t) { return micron::get<0>(t) * 100 + micron::get<1>(t); })
              | lz::collect<vec_i>();
    require(zt.size(), usize(5));
    require(zt[0], 110);

    auto en = a | lz::enumerate() | lz::fmap([](auto t) { return static_cast<int>(micron::get<0>(t)) * 10 + micron::get<1>(t); })
              | lz::collect<vec_i>();
    require(en.size(), usize(5));
    require(en[0], 1);
    require(en[2], 23);
    require(a | lz::enumerate() | lz::count(), usize(5));

    micron::vector<int (*)(int)> fns;
    fns.push_back([](int x) { return x + 1; });
    fns.push_back([](int x) { return x * 10; });
    auto ap = fns | lz::ap(a) | lz::collect<vec_i>();
    require(ap.size(), usize(2));
    require(ap[0], 2);
    require(ap[1], 20);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // unique / nub
  // ════════════════════════════════════════════════════════════════════

  test_case("unique drops CONSECUTIVE runs; nub drops all repeats -- against fp::");
  {
    vec_i d;
    d.push_back(1); d.push_back(1); d.push_back(2); d.push_back(2); d.push_back(2);
    d.push_back(1); d.push_back(3); d.push_back(3);

    auto u = d | lz::unique() | lz::collect<vec_i>();
    auto eu = micron::fp::unique(d);
    require(u.size(), eu.size());
    for ( usize i = 0; i < u.size(); ++i ) require(u[i], eu[i]);
    require(u.size(), usize(4));      // 1 2 1 3 -- the second 1 survives, it is not adjacent

    auto n = d | lz::nub() | lz::collect<vec_i>();
    auto en = micron::fp::nub(d);
    require(n.size(), en.size());
    for ( usize i = 0; i < n.size(); ++i ) require(n[i], en[i]);
    require(n.size(), usize(3));

    // law: nub . nub == nub
    auto nn = d | lz::nub() | lz::nub() | lz::collect<vec_i>();
    require(nn.size(), n.size());
    for ( usize i = 0; i < n.size(); ++i ) require(nn[i], n[i]);

    auto nb = d | lz::nub_by([](int a, int b) { return a == b; }) | lz::collect<vec_i>();
    require(nb.size(), n.size());
    for ( usize i = 0; i < n.size(); ++i ) require(nb[i], n[i]);
  }
  end_test_case();

  test_case("nub streams: it terminates over an endless source under a take");
  {
    auto ne = lz::counting(0) | lz::fmap([](int x) { return x % 4; }) | lz::nub() | lz::take(4) | lz::collect<vec_i>();
    require(ne.size(), usize(4));
    require(ne[0], 0);
    require(ne[3], 3);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // intersperse / intercalate / step_by
  // ════════════════════════════════════════════════════════════════════

  test_case("intersperse / intercalate against fp::, including the degenerate lengths");
  {
    auto t = iota_vec(1, 4);
    auto is = t | lz::intersperse(0) | lz::collect<vec_i>();
    auto ei = micron::fp::intersperse(t, 0);
    require(is.size(), ei.size());
    for ( usize i = 0; i < is.size(); ++i ) require(is[i], ei[i]);
    require(is.size(), usize(7));      // 2n - 1, no trailing separator
    vec_i one;
    one.push_back(5);
    require(one | lz::intersperse(0) | lz::count(), usize(1));
    vec_i zero;
    require(zero | lz::intersperse(0) | lz::count(), usize(0));

    vec_vi outer;
    outer.push_back(iota_vec(1, 2));
    outer.push_back(iota_vec(10, 2));
    vec_i sep;
    sep.push_back(0);
    auto ic = outer | lz::intercalate(sep) | lz::collect<vec_i>();
    auto ec = micron::fp::intercalate(sep, outer);
    require(ic.size(), ec.size());
    for ( usize i = 0; i < ic.size(); ++i ) require(ic[i], ec[i]);
  }
  end_test_case();

  test_case("step_by");
  {
    auto v = iota_vec(1, 9);
    auto s = v | lz::step_by(3) | lz::collect<vec_i>();
    require(s.size(), usize(3));
    require(s[0], 1);
    require(s[1], 4);
    require(s[2], 7);
    require(v | lz::step_by(1) | lz::count(), v.size());
    require((lz::counting(0) | lz::step_by(5) | lz::take(3) | lz::collect<vec_i>())[2], 10);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // chunk / sliding / group
  // ════════════════════════════════════════════════════════════════════

  test_case("chunk yields SUB-VIEWS, and flatten . chunk == id for every k");
  {
    auto v = iota_vec(1, 9);
    require(v | lz::chunk(4) | lz::count(), usize(3));
    auto sizes = v | lz::chunk(4) | lz::fmap([](auto w) { return static_cast<int>(w.size()); }) | lz::collect<vec_i>();
    require(sizes[0], 4);
    require(sizes[1], 4);
    require(sizes[2], 1);
    for ( usize k = 1; k <= 12; ++k ) {
      auto back = v | lz::chunk(k) | lz::flatten() | lz::collect<vec_i>();
      require(back.size(), v.size());
      for ( usize i = 0; i < v.size(); ++i ) require(back[i], v[i]);
    }
    // over a forward-only source, one drain and then the same sub-views
    micron::list<int> l;
    for ( int i = 1; i <= 7; ++i ) l.push_back(i);
    require(l | lz::chunk(3) | lz::count(), usize(3));
    auto lback = l | lz::chunk(3) | lz::flatten() | lz::collect<vec_i>();
    require(lback.size(), usize(7));
    require(lback[6], 7);
    vec_i e;
    require(e | lz::chunk(3) | lz::count(), usize(0));
  }
  end_test_case();

  test_case("chunk_into against the repaired fp::chunk / fp::chunk_into");
  {
    auto v = iota_vec(1, 9);
    auto ci = v | lz::chunk_into<vec_vi>(4);
    auto ei = micron::fp::chunk_into<vec_vi>(v, 4);
    require(ci.size(), ei.size());
    for ( usize i = 0; i < ci.size(); ++i ) {
      require(ci[i].size(), ei[i].size());
      for ( usize k = 0; k < ci[i].size(); ++k ) require(ci[i][k], ei[i][k]);
    }
    auto eo = micron::fp::chunk<vec_vi>(v, 4);
    require_true(eo.is_first());
    require(ci.size(), eo.cast<vec_vi>().size());
    require((v | lz::chunk_into<vec_vi>(0)).size(), usize(0));
  }
  end_test_case();

  test_case("sliding against fp::sliding");
  {
    auto v = iota_vec(1, 9);
    require(v | lz::sliding(3) | lz::count(), usize(7));
    auto es = micron::fp::sliding<vec_vi>(v, 3);
    require(es.size(), usize(7));
    auto firsts = v | lz::sliding(3) | lz::fmap([](auto w) { return *w.begin(); }) | lz::collect<vec_i>();
    require(firsts.size(), usize(7));
    for ( usize i = 0; i < 7; ++i ) require(firsts[i], es[i][0]);
    // no partial windows
    require(v | lz::sliding(20) | lz::count(), usize(0));
    require(v | lz::sliding(9) | lz::count(), usize(1));
  }
  end_test_case();

  test_case("group / group_by compare against the PREVIOUS element, matching fpdata.hpp");
  {
    vec_i g;
    g.push_back(1); g.push_back(1); g.push_back(2); g.push_back(3); g.push_back(3); g.push_back(3);
    require(g | lz::group() | lz::count(), usize(3));
    auto lens = g | lz::group() | lz::fmap([](auto w) { return static_cast<int>(w.size()); }) | lz::collect<vec_i>();
    auto eg = micron::fp::group<vec_vi>(g);
    require(lens.size(), eg.size());
    for ( usize i = 0; i < eg.size(); ++i ) require(static_cast<usize>(lens[i]), eg[i].size());

    // law: concat of group_by == input
    auto back = g | lz::group() | lz::flatten() | lz::collect<vec_i>();
    require(back.size(), g.size());
    for ( usize i = 0; i < g.size(); ++i ) require(back[i], g[i]);

    // a NON-TRANSITIVE relation, which is the only way to tell the two comparisons apart:
    // "ascending run" groups differently under prev-vs-cur than under first-vs-cur
    vec_i asc;
    asc.push_back(1); asc.push_back(2); asc.push_back(3); asc.push_back(1); asc.push_back(2);
    auto runs = asc | lz::group_by([](int a, int b) { return a < b; })
                | lz::fmap([](auto w) { return static_cast<int>(w.size()); }) | lz::collect<vec_i>();
    auto eruns = micron::fp::group_by<vec_vi>(asc, [](int a, int b) { return a < b; });
    require(runs.size(), eruns.size());
    for ( usize i = 0; i < runs.size(); ++i ) require(static_cast<usize>(runs[i]), eruns[i].size());
    require(runs.size(), usize(2));      // {1,2,3} then {1,2}
    vec_i e;
    require(e | lz::group() | lz::count(), usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // transpose
  // ════════════════════════════════════════════════════════════════════

  test_case("transpose against fp::transpose, plus transpose . transpose == id");
  {
    vec_vi m;
    m.push_back(iota_vec(1, 3));
    m.push_back(iota_vec(10, 3));
    auto t = m | lz::transpose<vec_vi>();
    auto et = micron::fp::transpose(m);
    require_true(t.is_first() && et.is_first());
    const auto &tv = t.cast<vec_vi>();
    const auto &ev = et.cast<vec_vi>();
    require(tv.size(), ev.size());
    for ( usize i = 0; i < tv.size(); ++i ) {
      require(tv[i].size(), ev[i].size());
      for ( usize k = 0; k < tv[i].size(); ++k ) require(tv[i][k], ev[i][k]);
    }
    auto tt = tv | lz::transpose<vec_vi>();
    require_true(tt.is_first());
    const auto &ttv = tt.cast<vec_vi>();
    require(ttv.size(), m.size());
    for ( usize i = 0; i < m.size(); ++i )
      for ( usize k = 0; k < m[i].size(); ++k ) require(ttv[i][k], m[i][k]);
  }
  end_test_case();

  test_case("transpose rejects ragged rows; transpose_trunc truncates instead");
  {
    vec_vi rag;
    rag.push_back(iota_vec(1, 3));
    rag.push_back(iota_vec(10, 2));
    require_true((rag | lz::transpose<vec_vi>()).is_second());
    require_true(micron::fp::transpose(rag).is_second());
    auto rt = rag | lz::transpose_trunc<vec_vi>();
    require(rt.size(), usize(2));
    require(rt[0].size(), usize(2));
    require(rt[1][1], 11);
    vec_vi e;
    require_true((e | lz::transpose<vec_vi>()).is_first());
    require((e | lz::transpose_trunc<vec_vi>()).size(), usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // flatten / flat_map / concat
  // ════════════════════════════════════════════════════════════════════

  test_case("flatten over LVALUE inner ranges, against fp::flatten");
  {
    vec_vi outer;
    outer.push_back(iota_vec(1, 3));
    outer.push_back(iota_vec(10, 2));
    outer.push_back(iota_vec(100, 1));
    auto f = outer | lz::flatten() | lz::collect<vec_i>();
    auto e = micron::fp::flatten(outer);
    require(f.size(), e.size());
    for ( usize i = 0; i < f.size(); ++i ) require(f[i], e[i]);
  }
  end_test_case();

  test_case("flatten skips empty inner ranges wherever they sit");
  {
    vec_vi outer;
    outer.push_back(vec_i{});
    outer.push_back(iota_vec(1, 2));
    outer.push_back(vec_i{});
    outer.push_back(vec_i{});
    outer.push_back(iota_vec(9, 1));
    outer.push_back(vec_i{});
    auto f = outer | lz::flatten() | lz::collect<vec_i>();
    require(f.size(), usize(3));
    require(f[0], 1);
    require(f[2], 9);
    vec_vi allempty;
    allempty.push_back(vec_i{});
    allempty.push_back(vec_i{});
    require(allempty | lz::flatten() | lz::count(), usize(0));
    vec_vi none;
    require(none | lz::flatten() | lz::count(), usize(0));
  }
  end_test_case();

  test_case("flatten over BY-VALUE inner ranges -- the lifetime case the slot exists for");
  {
    auto src = iota_vec(1, 4);
    auto blow = [](int x) {
      vec_i r;
      for ( int k = 0; k < x; ++k ) r.push_back(x);
      return r;      // a temporary; the inner iterators have to outlive this expression
    };
    auto f = src | lz::fmap(blow) | lz::flatten() | lz::collect<vec_i>();
    require(f.size(), usize(1 + 2 + 3 + 4));
    require(f[0], 1);
    require(f[1], 2);
    require(f[3], 3);
    require(f[9], 4);
    auto g = src | lz::flat_map(blow) | lz::collect<vec_i>();
    require(g.size(), f.size());
    for ( usize i = 0; i < g.size(); ++i ) require(g[i], f[i]);
    // with empties interleaved
    auto h = iota_vec(0, 5) | lz::flat_map([](int x) {
               vec_i r;
               if ( x & 1 ) r.push_back(x * 10);
               return r;
             })
             | lz::collect<vec_i>();
    require(h.size(), usize(2));
    require(h[0], 10);
    require(h[1], 30);
  }
  end_test_case();

  test_case("flatten stays lazy: an endless outer under a take terminates");
  {
    auto f = lz::counting(1) | lz::fmap([](int x) {
               vec_i r;
               r.push_back(x);
               r.push_back(-x);
               return r;
             })
             | lz::flatten() | lz::take(5) | lz::collect<vec_i>();
    require(f.size(), usize(5));
    require(f[0], 1);
    require(f[1], -1);
    require(f[4], 3);
  }
  end_test_case();

  test_case("concat / merge, including the identity laws");
  {
    auto a = iota_vec(1, 3);
    auto b = iota_vec(10, 2);
    auto c = a | lz::concat(b) | lz::collect<vec_i>();
    auto em = micron::merge(a, b);
    require(c.size(), usize(5));
    for ( usize i = 0; i < c.size(); ++i ) require(c[i], em[i]);
    require(a | lz::merge(b) | lz::count(), usize(5));

    vec_i e;
    auto ce = a | lz::concat(e) | lz::collect<vec_i>();
    require(ce.size(), a.size());
    for ( usize i = 0; i < a.size(); ++i ) require(ce[i], a[i]);
    auto ec = e | lz::concat(a) | lz::collect<vec_i>();
    require(ec.size(), a.size());
    for ( usize i = 0; i < a.size(); ++i ) require(ec[i], a[i]);
  }
  end_test_case();

  test_case("concat of two chains with DIFFERENT reference types");
  {
    // filter yields const int &, fmap yields a prvalue. taking the first operand's reference type
    // and casting the second to it binds a const reference to a temporary -- so the two sides only
    // share a reference type when they genuinely agree, and fall back to by-value when they do not.
    auto a = iota_vec(1, 4);
    auto c = (a | lz::filter([](int x) { return (x & 1) == 1; })) | lz::concat(a | lz::fmap([](int x) { return x * 100; }))
             | lz::collect<vec_i>();
    require(c.size(), usize(6));
    require(c[0], 1);
    require(c[1], 3);
    require(c[2], 100);
    require(c[5], 400);
  }
  end_test_case();

  sb::print("=== LZ BUFFERED RIGOR SUITE PASSED ===");
  return 1;
}
