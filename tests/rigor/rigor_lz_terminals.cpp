//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// rigor_lz_terminals.cpp -- Phase 3 of micron::lz: the terminals and the arithmetic mirror.
//
// Coverage:
//   reduce.hpp     fold foldl foldr for_each count count_if count_of sum safe_sum min max
//                  safe_min safe_max mean safe_mean geomean harmonicmean inner_product
//                  safe_inner_product
//   search.hpp     all_of any_of none_of find_first find_last find_index find_of elem at nth
//                  head last span_at sbreak
//   partition.hpp  partition unzip traverse sequence sequence_check uncons
//   arith.hpp      add subtract multiply divide pow negate abs clamp_each *_zip safe_divide
//                  safe_divide_zip divide_zip_each
//
// The oracle is the eager layer wherever the eager layer is sound. Where it is not -- fp::unzip and
// fp::chunk were both broken until 2026-08-19, see ISSUES.md -- the eager side is now fixed and IS
// the oracle again; only micron::for_each has no sequence-container counterpart at all, so its
// oracle is a hand-written loop.
//
// Seeds are fixed hex literals. property_test seeds ITSELF from the cycle counter
// (snowball_ext.hpp), so every property here derives a local prng from the generated argument
// instead, the way rigor_algo_fp_core.cpp does.

#include "../../src/lz.hpp"

#include "../../src/algorithm/find.hpp"
#include "../../src/algorithm/fold.hpp"
#include "../../src/algorithm/fp.hpp"
#include "../../src/list.hpp"
#include "../../src/maps/swiss.hpp"
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
using vec_d = micron::vector<f64>;
using opt_i = micron::option<int, micron::fp::empty_container_error>;

static vec_i
iota_vec(int from, usize n)
{
  vec_i v;
  for ( usize i = 0; i < n; ++i ) v.push_back(from + static_cast<int>(i));
  return v;
}

int
main()
{
  sb::print("=== LZ TERMINALS RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // fold / foldl / foldr
  // ════════════════════════════════════════════════════════════════════

  test_case("fold takes the callable by VALUE, unlike micron::fold_left's pointer form");
  {
    auto v = iota_vec(1, 10);
    require(v | lz::fold(0, [](int a, int x) { return a + x; }), 55);
    require(v | lz::foldl(1, [](int a, int x) { return a * x; }), 3628800);
    // the eager equivalent, written with the pointer predicate it insists on
    require(v | lz::fold(0, [](int a, int x) { return a + x; }),
            micron::fold_left(v, 0, [](int a, const int *x) { return a + *x; }));
  }
  end_test_case();

  test_case("foldr agrees on both arms -- reversible walk and buffered fallback");
  {
    auto v = iota_vec(1, 8);
    // a non-commutative fold, so direction is actually observable
    auto f = [](int x, int a) { return a * 3 + x; };
    const int rev = v | lz::foldr(f, 0);
    // a filter ends in a sentinel tag, so this chain cannot be walked backwards and buffers instead
    const int buf = v | lz::filter([](int) { return true; }) | lz::foldr(f, 0);
    require(rev, buf);
    require(rev, micron::fold_right(v, [](const int *x, int a) { return a * 3 + *x; }, 0));
    vec_i e;
    require(e | lz::foldr(f, 7), 7);
  }
  end_test_case();

  property_test(
      "fold over a lazy chain == fold over the eager result (10k)",
      [](u32 raw_n) {
        usize n = (raw_n & 0x1f) + 1;
        prng rng(raw_n + 211);
        vec_i v;
        for ( usize i = 0; i < n; ++i ) v.push_back(static_cast<int>(rng.next_in(200)) - 100);

        const int lazy = v | lz::fmap([](int x) { return x * 2; }) | lz::filter([](int x) { return x > 0; })
                         | lz::fold(0, [](int a, int x) { return a + x; });
        int eager = 0;
        for ( usize i = 0; i < n; ++i ) {
          const int y = v[i] * 2;
          if ( y > 0 ) eager += y;
        }
        require(lazy, eager);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // count family
  // ════════════════════════════════════════════════════════════════════

  test_case("count / count_if / count_of");
  {
    auto v = iota_vec(1, 10);
    require(v | lz::count(), usize(10));
    require(v | lz::filter([](int x) { return (x & 1) == 0; }) | lz::count(), usize(5));
    require(v | lz::count_if([](int x) { return x > 7; }), usize(3));
    require(static_cast<umax_t>(v | lz::count_if([](int x) { return x > 7; })),
            micron::count_if(v, [](const int *x) { return *x > 7; }));
    require(v | lz::count_of(4), usize(1));
    require(static_cast<umax_t>(v | lz::count_of(4)), micron::count(v, 4));
    vec_i e;
    require(e | lz::count(), usize(0));
  }
  end_test_case();

  test_case("count_of takes the lane-scan path on a flat chain and the pull path after an adaptor");
  {
    auto v = iota_vec(0, 64);
    for ( int probe = 0; probe < 64; ++probe ) require(v | lz::count_of(probe), usize(1));
    require(v | lz::count_of(100), usize(0));
    // after fmap the source is no longer flat, so this exercises the other arm on the same data
    require(v | lz::fmap([](int x) { return x; }) | lz::count_of(37), usize(1));
    require(v | lz::fmap([](int x) { return x / 2; }) | lz::count_of(3), usize(2));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // sum -- must equal the eager kernel BIT FOR BIT on a flat chain
  // ════════════════════════════════════════════════════════════════════

  test_case("sum over a flat chain is byte-identical to micron::sum, integral and floating");
  {
    auto v = iota_vec(1, 100);
    require_true((v | lz::sum()) == micron::sum(v));
    vec_d d;
    for ( int i = 1; i <= 100; ++i ) d.push_back(static_cast<f64>(i) * 0.1);
    require_true((d | lz::sum()) == micron::sum(d));
  }
  end_test_case();

  test_case("the STREAMING sum reproduces the eager lane assignment exactly");
  {
    // an identity fmap makes the chain non-flat without changing a single value, so any difference
    // here is the lane assignment and nothing else. the four-element staging buffer in
    // __sum_lanes_stream exists precisely so this holds.
    for ( usize n = 0; n <= 40; ++n ) {
      vec_d d;
      for ( usize i = 0; i < n; ++i ) d.push_back(static_cast<f64>(i) * 0.1 + 0.03);
      const f128 flat = d | lz::sum();
      const f128 streamed = d | lz::fmap([](f64 x) { return x; }) | lz::sum();
      require_true(flat == streamed);
      require_true(flat == micron::sum(d));

      vec_i v;
      for ( usize i = 0; i < n; ++i ) v.push_back(static_cast<int>(i) * 7 - 3);
      require_true((v | lz::sum()) == (v | lz::fmap([](int x) { return x; }) | lz::sum()));
      require_true((v | lz::sum()) == micron::sum(v));
    }
  }
  end_test_case();

  test_case("sum is Neumaier-compensated, not a naive fold");
  {
    // 1 + 1e100 + 1 + -1e100: a naive left fold gives 0, a compensated one gives 2
    vec_d d;
    d.push_back(1.0);
    d.push_back(1e100);
    d.push_back(1.0);
    d.push_back(-1e100);
    require_true((d | lz::sum()) == micron::sum(d));
    require_true(static_cast<f64>(d | lz::sum()) == 2.0);
    require_true(static_cast<f64>(d | lz::fmap([](f64 x) { return x; }) | lz::sum()) == 2.0);
  }
  end_test_case();

  test_case("safe_sum guards the empty case; fp::safe_sum is the oracle");
  {
    auto v = iota_vec(1, 5);
    auto ls = v | lz::safe_sum();
    auto es = micron::fp::safe_sum(v);
    require_true(ls.is_first() && es.is_first());
    require_true(ls.cast<umax_t>() == es.cast<umax_t>());
    vec_i e;
    require_true((e | lz::safe_sum()).is_second());
    require_true(micron::fp::safe_sum(e).is_second());
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // min / max / mean / geomean / harmonicmean
  // ════════════════════════════════════════════════════════════════════

  test_case("min / max / safe_min / safe_max against fp::safe_min / fp::safe_max");
  {
    vec_i v;
    v.push_back(5); v.push_back(-3); v.push_back(9); v.push_back(0);
    require(v | lz::min(), -3);
    require(v | lz::max(), 9);
    require((v | lz::safe_min()).cast<int>(), micron::fp::safe_min(v).cast<int>());
    require((v | lz::safe_max()).cast<int>(), micron::fp::safe_max(v).cast<int>());
    vec_i e;
    require_true((e | lz::safe_min()).is_second());
    require_true((e | lz::safe_max()).is_second());
    // over a lazy chain, where there is no eager equivalent
    require(v | lz::fmap([](int x) { return x * x; }) | lz::max(), 81);
    require(v | lz::filter([](int x) { return x > 0; }) | lz::min(), 5);
  }
  end_test_case();

  test_case("mean / safe_mean / geomean / harmonicmean against eager");
  {
    auto v = iota_vec(1, 10);
    require_true((v | lz::mean<f64>()) == micron::mean<f64>(v));
    require_true((v | lz::safe_mean<f64>()).cast<f64>() == micron::fp::safe_mean<f64>(v).cast<f64>());
    vec_i e;
    require_true((e | lz::safe_mean<f64>()).is_second());
    require_true((e | lz::safe_geomean<f64>()).is_second());
    require_true((e | lz::safe_harmonicmean<f64>()).is_second());
    // the streaming arm of mean, where the length is not known up front
    const f64 filtered = v | lz::filter([](int x) { return x <= 4; }) | lz::mean<f64>();
    require_true(filtered == 2.5);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // inner_product
  // ════════════════════════════════════════════════════════════════════

  test_case("inner_product truncates to the shorter operand; safe_inner_product reports the mismatch");
  {
    auto v = iota_vec(1, 6);
    const f64 e = micron::fp::inner_product<vec_i, f64>(v, v, 0.0);
    require_true((v | lz::inner_product<f64>(v)) == e);
    require_true((v | lz::safe_inner_product<f64>(v)).cast<f64>() == e);

    vec_i sh;
    sh.push_back(1); sh.push_back(2);
    require_true((v | lz::inner_product<f64>(sh)) == 5.0);      // 1*1 + 2*2
    require_true((v | lz::safe_inner_product<f64>(sh)).is_second());
    require_true((sh | lz::safe_inner_product<f64>(v)).is_second());
    // and across operands of DIFFERENT type, which eager cannot express at all
    vec_d d;
    for ( int i = 0; i < 6; ++i ) d.push_back(2.0);
    require_true((v | lz::inner_product<f64>(d)) == 42.0);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // for_each -- no eager sequence counterpart exists; the oracle is a loop
  // ════════════════════════════════════════════════════════════════════

  test_case("for_each visits every element exactly once, in order");
  {
    auto v = iota_vec(1, 20);
    int acc = 0;
    usize seen = 0;
    int last = 0;
    v | lz::for_each([&](int x) {
      acc += x;
      ++seen;
      require_true(x > last);
      last = x;
    });
    require(acc, 210);
    require(seen, usize(20));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // predicates and finders
  // ════════════════════════════════════════════════════════════════════

  test_case("all_of / any_of / none_of against fp::*_c");
  {
    auto v = iota_vec(1, 10);
    require_true(v | lz::all_of([](int x) { return x > 0; }));
    require_false(v | lz::all_of([](int x) { return x > 5; }));
    require_true(v | lz::any_of([](int x) { return x == 7; }));
    require_false(v | lz::any_of([](int x) { return x == 99; }));
    require_true(v | lz::none_of([](int x) { return x > 100; }));
    require_false(v | lz::none_of([](int x) { return x == 3; }));
    require((v | lz::all_of([](int x) { return x > 0; })), micron::fp::all_of_c([](int x) { return x > 0; })(v));
    require((v | lz::any_of([](int x) { return x > 9; })), micron::fp::any_of_c([](int x) { return x > 9; })(v));
    require((v | lz::none_of([](int x) { return x > 9; })), micron::fp::none_of_c([](int x) { return x > 9; })(v));
    // the empty conventions
    vec_i e;
    require_true(e | lz::all_of([](int) { return false; }));
    require_false(e | lz::any_of([](int) { return true; }));
    require_true(e | lz::none_of([](int) { return true; }));
  }
  end_test_case();

  test_case("find_first / find_last / find_index / find_of / elem against fp::");
  {
    vec_i v;
    v.push_back(1); v.push_back(4); v.push_back(2); v.push_back(4); v.push_back(3);
    auto pf = [](int x) { return x == 4; };

    require((v | lz::find_first(pf)).cast<int>(), micron::fp::find_first(v, pf).cast<int>());
    require((v | lz::find_last(pf)).cast<int>(), micron::fp::find_last(v, pf).cast<int>());
    require((v | lz::find_index(pf)).cast<usize>(), micron::fp::find_index(v, pf).cast<usize>());
    require((v | lz::find_of(2)).cast<usize>(), usize(2));
    require((v | lz::elem(4)), micron::fp::elem(v, 4));
    require((v | lz::elem(99)), micron::fp::elem(v, 99));
    require_true((v | lz::find_first([](int x) { return x > 100; })).is_second());
    require_true((v | lz::find_last([](int x) { return x > 100; })).is_second());
    require_true((v | lz::find_of(100)).is_second());
  }
  end_test_case();

  test_case("find_last agrees on the reversible and the forward-scan arm");
  {
    auto v = iota_vec(1, 30);
    auto p = [](int x) { return (x % 7) == 0; };
    const auto rev = v | lz::find_last(p);              // walks backwards
    const auto fwd = v | lz::filter([](int) { return true; }) | lz::find_last(p);      // full forward scan
    require_true(rev.is_first() && fwd.is_first());
    require(rev.cast<int>(), fwd.cast<int>());
    require(rev.cast<int>(), 28);
    // and over a forward-only source
    micron::list<int> l;
    for ( int i = 1; i <= 30; ++i ) l.push_back(i);
    require((l | lz::find_last(p)).cast<int>(), 28);
  }
  end_test_case();

  test_case("at / nth / head / last against fp::");
  {
    auto v = iota_vec(10, 5);
    require((v | lz::at(0)).cast<int>(), micron::fp::at(v, 0).cast<int>());
    require((v | lz::at(4)).cast<int>(), micron::fp::at(v, 4).cast<int>());
    require_true((v | lz::at(5)).is_second());
    require_true(micron::fp::at(v, 5).is_second());
    require((v | lz::nth(2)).cast<int>(), 12);
    require((v | lz::head()).cast<int>(), micron::fp::head(v).cast<int>());
    require((v | lz::last()).cast<int>(), micron::fp::last(v).cast<int>());
    // last over a chain that cannot be walked backwards
    require((v | lz::filter([](int) { return true; }) | lz::last()).cast<int>(), 14);
    vec_i e;
    require_true((e | lz::head()).is_second());
    require_true((e | lz::last()).is_second());
    require_true((e | lz::at(0)).is_second());
  }
  end_test_case();

  test_case("span_at / sbreak against fp::span / fp::sbreak");
  {
    auto v = iota_vec(1, 8);
    auto p = [](int x) { return x < 4; };
    auto ls = v | lz::span_at<vec_i>(p);
    auto es = micron::fp::span(v, p);
    require(micron::get<0>(ls).size(), micron::get<0>(es).size());
    require(micron::get<1>(ls).size(), micron::get<1>(es).size());
    for ( usize i = 0; i < micron::get<0>(ls).size(); ++i ) require(micron::get<0>(ls)[i], micron::get<0>(es)[i]);
    for ( usize i = 0; i < micron::get<1>(ls).size(); ++i ) require(micron::get<1>(ls)[i], micron::get<1>(es)[i]);

    auto q = [](int x) { return x > 4; };
    auto lb = v | lz::sbreak<vec_i>(q);
    auto eb = micron::fp::sbreak(v, q);
    require(micron::get<0>(lb).size(), micron::get<0>(eb).size());
    require(micron::get<1>(lb).size(), micron::get<1>(eb).size());
    // law: the two halves concatenate back to the input
    require(micron::get<0>(ls).size() + micron::get<1>(ls).size(), v.size());
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // partition / unzip / traverse / sequence / uncons
  // ════════════════════════════════════════════════════════════════════

  test_case("partition returns (matching, non-matching), matching fp::partition's order");
  {
    auto v = iota_vec(1, 10);
    auto p = [](int x) { return (x & 1) == 0; };
    auto lp = v | lz::partition<vec_i>(p);
    auto ep = micron::fp::partition(v, p);
    require(micron::get<0>(lp).size(), micron::get<0>(ep).size());
    require(micron::get<1>(lp).size(), micron::get<1>(ep).size());
    for ( usize i = 0; i < micron::get<0>(lp).size(); ++i ) require(micron::get<0>(lp)[i], micron::get<0>(ep)[i]);
    for ( usize i = 0; i < micron::get<1>(lp).size(); ++i ) require(micron::get<1>(lp)[i], micron::get<1>(ep)[i]);
    // law: filter p ++ reject p is a permutation of the input
    require(micron::get<0>(lp).size() + micron::get<1>(lp).size(), v.size());
  }
  end_test_case();

  test_case("unzip against the repaired fp::unzip, plus the round-trip law");
  {
    micron::vector<micron::tuple<int, f64>> z;
    for ( int i = 0; i < 7; ++i ) z.push_back(micron::make_tuple(i, static_cast<f64>(i) * 1.5));
    auto lu = z | lz::unzip<vec_i, vec_d>();
    auto eu = micron::fp::unzip<vec_i, vec_d>(z);
    require(micron::get<0>(lu).size(), micron::get<0>(eu).size());
    for ( usize i = 0; i < 7; ++i ) {
      require(micron::get<0>(lu)[i], micron::get<0>(eu)[i]);
      require_true(micron::get<1>(lu)[i] == micron::get<1>(eu)[i]);
      // zip(unzip(z).a, unzip(z).b) == z
      require(micron::get<0>(lu)[i], micron::get<0>(z[i]));
      require_true(micron::get<1>(lu)[i] == micron::get<1>(z[i]));
    }
    // unzip over a chain that never was a container
    auto lz2 = iota_vec(1, 5) | lz::fmap([](int x) { return micron::make_tuple(x, x * x); }) | lz::unzip<vec_i, vec_i>();
    require(micron::get<1>(lz2)[3], 16);
  }
  end_test_case();

  test_case("traverse short-circuits, and does so EARLIER than fp::traverse");
  {
    auto v = iota_vec(1, 10);
    usize calls = 0;
    auto ok = v | lz::traverse<vec_i>([&](int x) {
      ++calls;
      return opt_i{ x * 2 };
    });
    require_true(ok.is_first());
    require(ok.cast<vec_i>().size(), usize(10));
    require(ok.cast<vec_i>()[4], 10);
    require(calls, usize(10));

    calls = 0;
    auto bad = v | lz::traverse<vec_i>([&](int x) {
      ++calls;
      return x >= 3 ? opt_i{ micron::fp::empty_container_error{} } : opt_i{ x };
    });
    require_true(bad.is_second());
    // stopped ON the failure and pulled nothing after it. fp::traverse has already resized its
    // output to c.size() before it can do this.
    require(calls, usize(3));
  }
  end_test_case();

  test_case("sequence unwraps (fp::sequence_extract's semantics) and sequence_check agrees with fp::");
  {
    micron::vector<opt_i> good;
    for ( int i = 0; i < 5; ++i ) good.push_back(opt_i{ i });
    auto s = good | lz::sequence<vec_i>();
    require_true(s.is_first());
    require(s.cast<vec_i>().size(), usize(5));
    require(s.cast<vec_i>()[3], 3);
    require((good | lz::sequence_check()), micron::fp::sequence_check(good));

    micron::vector<opt_i> bad;
    bad.push_back(opt_i{ 1 });
    bad.push_back(opt_i{ micron::fp::empty_container_error{} });
    bad.push_back(opt_i{ 3 });
    require_true((bad | lz::sequence<vec_i>()).is_second());
    require((bad | lz::sequence_check()), micron::fp::sequence_check(bad));
    require_false(bad | lz::sequence_check());
  }
  end_test_case();

  test_case("uncons against fp::uncons");
  {
    auto v = iota_vec(1, 6);
    auto u = v | lz::uncons<vec_i>();
    require_true(u.is_first());
    const auto &t = u.cast<micron::tuple<int, vec_i>>();
    require(micron::get<0>(t), 1);
    require(micron::get<1>(t).size(), usize(5));
    require(micron::get<1>(t)[0], 2);
    vec_i e;
    require_true((e | lz::uncons<vec_i>()).is_second());
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // the arithmetic mirror
  // ════════════════════════════════════════════════════════════════════

  test_case("curried scalars against fp::*_c");
  {
    auto v = iota_vec(1, 8);
    auto chk = [&](const vec_i &l, const vec_i &e) {
      require(l.size(), e.size());
      for ( usize i = 0; i < l.size(); ++i ) require(l[i], e[i]);
    };
    chk(v | lz::add(5) | lz::collect<vec_i>(), micron::fp::add_c(5)(v));
    chk(v | lz::subtract(2) | lz::collect<vec_i>(), micron::fp::subtract_c(2)(v));
    chk(v | lz::multiply(3) | lz::collect<vec_i>(), micron::fp::multiply_c(3)(v));
    chk(v | lz::divide(2) | lz::collect<vec_i>(), micron::fp::divide_c(2)(v));
    chk(v | lz::pow(2) | lz::collect<vec_i>(), micron::fp::pow_c(2)(v));
    // the _c aliases and the direct two-arg spelling
    chk(v | lz::add_c(5) | lz::collect<vec_i>(), micron::fp::add_c(5)(v));
    chk(lz::add(v, 5) | lz::collect<vec_i>(), micron::fp::add_c(5)(v));
  }
  end_test_case();

  test_case("negate / abs / clamp_each");
  {
    vec_i v;
    for ( int i = -4; i <= 4; ++i ) v.push_back(i);
    auto ln = v | lz::negate() | lz::collect<vec_i>();
    auto en = micron::fp::negate(v);
    for ( usize i = 0; i < ln.size(); ++i ) require(ln[i], en[i]);
    auto la = v | lz::abs() | lz::collect<vec_i>();
    auto ea = micron::fp::abs(v);
    for ( usize i = 0; i < la.size(); ++i ) require(la[i], ea[i]);
    auto lc = v | lz::clamp_each(-2, 2) | lz::collect<vec_i>();
    require(lc[0], -2);
    require(lc[4], 0);
    require(lc[8], 2);
  }
  end_test_case();

  test_case("*_zip at equal size matches eager; ragged yields exactly the overlap");
  {
    auto a = iota_vec(1, 8);
    vec_i b;
    for ( int i = 1; i <= 8; ++i ) b.push_back(i * 10);
    auto la = a | lz::add_zip(b) | lz::collect<vec_i>();
    auto ea = micron::fp::add_zip(a, b);
    require(la.size(), usize(8));
    for ( usize i = 0; i < 8; ++i ) require(la[i], ea[i]);
    auto lm = a | lz::multiply_zip(b) | lz::collect<vec_i>();
    auto em = micron::fp::multiply_zip(a, b);
    for ( usize i = 0; i < 8; ++i ) require(lm[i], em[i]);
    auto ls = a | lz::subtract_zip(b) | lz::collect<vec_i>();
    auto es = micron::fp::subtract_zip(a, b);
    for ( usize i = 0; i < 8; ++i ) require(ls[i], es[i]);

    // ragged: lazy yields min(n, m) elements, eager returns all of `a` with the overlap modified
    vec_i sh;
    sh.push_back(100); sh.push_back(200);
    auto lr = a | lz::add_zip(sh) | lz::collect<vec_i>();
    auto er = micron::fp::add_zip(a, sh);
    require(lr.size(), usize(2));
    require(er.size(), usize(8));
    for ( usize i = 0; i < 2; ++i ) require(lr[i], er[i]);
    // and the other way round
    require((sh | lz::add_zip(a) | lz::count()), usize(2));
    // operands of different types, which eager cannot express
    vec_d d;
    for ( int i = 0; i < 8; ++i ) d.push_back(0.5);
    auto lx = a | lz::add_zip(d) | lz::collect<vec_d>();
    require_true(lx[0] == 1.5);
  }
  end_test_case();

  test_case("safe_divide / safe_divide_zip / divide_zip_each");
  {
    auto v = iota_vec(2, 8);
    auto ok = v | lz::safe_divide<vec_i>(2);
    auto eo = micron::fp::safe_divide(v, 2);
    require_true(ok.is_first() && eo.is_first());
    for ( usize i = 0; i < 8; ++i ) require(ok.cast<vec_i>()[i], eo.cast<vec_i>()[i]);
    require_true((v | lz::safe_divide<vec_i>(0)).is_second());
    require_true(micron::fp::safe_divide(v, 0).is_second());

    vec_i two;
    for ( int i = 0; i < 8; ++i ) two.push_back(2);
    require_true((v | lz::safe_divide_zip<vec_i>(two)).is_first());
    vec_i withzero;
    for ( int i = 0; i < 8; ++i ) withzero.push_back(i == 3 ? 0 : 2);
    require_true((v | lz::safe_divide_zip<vec_i>(withzero)).is_second());
    // a zero PAST the overlap is never read
    vec_i shortdiv;
    shortdiv.push_back(2);
    require_true((v | lz::safe_divide_zip<vec_i>(shortdiv)).is_first());

    // the per-element option form, composed with sequence
    require_true((v | lz::divide_zip_each<int, int>(two) | lz::sequence<vec_i>()).is_first());
    require_true((v | lz::divide_zip_each<int, int>(withzero) | lz::sequence<vec_i>()).is_second());
  }
  end_test_case();

  property_test(
      "add_zip on ragged operands yields exactly the overlap (10k)",
      [](u32 raw_n, u32 raw_m) {
        usize n = (raw_n & 0x1f) + 1;
        usize m = (raw_m & 0x1f) + 1;
        const usize ov = n < m ? n : m;
        prng rng(raw_n + 223);
        vec_i a, b;
        for ( usize i = 0; i < n; ++i ) a.push_back(static_cast<int>(rng.next_in(200)) - 100);
        for ( usize i = 0; i < m; ++i ) b.push_back(static_cast<int>(rng.next_in(200)) - 100);
        auto got = a | lz::add_zip(b) | lz::collect<vec_i>();
        require(got.size(), ov);
        for ( usize i = 0; i < ov; ++i ) require(got[i], a[i] + b[i]);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // sources the eager layer never had
  // ════════════════════════════════════════════════════════════════════

  test_case("terminals over node and map sources");
  {
    micron::list<int> l;
    for ( int i = 1; i <= 10; ++i ) l.push_back(i);
    require(l | lz::count(), usize(10));
    require_true((l | lz::sum()) == umax_t(55));
    require(l | lz::max(), 10);
    require_true(l | lz::any_of([](int x) { return x == 7; }));
    require((l | lz::at(3)).cast<int>(), 4);

    micron::stack_swiss_map<int, int, 64> m;
    for ( int i = 1; i <= 6; ++i ) m.insert(i, i * 100);
    require(m | lz::keys() | lz::count(), usize(6));
    require_true((m | lz::keys() | lz::sum()) == umax_t(21));
    require_true((m | lz::values() | lz::sum()) == umax_t(2100));
    require_true(m | lz::keys() | lz::elem(4));
    require_false(m | lz::values() | lz::any_of([](int v) { return v == 7; }));
  }
  end_test_case();

  test_case("once / empty / unfold / tail / init");
  {
    require((lz::once(7) | lz::collect<vec_i>()).size(), usize(1));
    require((lz::once(7) | lz::collect<vec_i>())[0], 7);
    require((lz::empty<int>() | lz::count()), usize(0));

    using op = micron::option<micron::pair<int, int>, micron::fp::empty_container_error>;
    auto u = lz::unfold([](const int &s) { return s < 5 ? op{ micron::pair<int, int>{ s * s, s + 1 } }
                                                        : op{ micron::fp::empty_container_error{} }; },
                        0)
             | lz::collect<vec_i>();
    require(u.size(), usize(5));
    require(u[4], 16);

    auto v = iota_vec(1, 5);
    auto t = v | lz::tail() | lz::collect<vec_i>();
    require(t.size(), usize(4));
    require(t[0], 2);
    auto n = v | lz::init() | lz::collect<vec_i>();
    require(n.size(), usize(4));
    require(n[0], 1);
    require(n[3], 4);
    // law: init ++ [last] == input
    require((v | lz::init() | lz::count()) + usize(1), v.size());
    vec_i one;
    one.push_back(9);
    require((one | lz::init() | lz::count()), usize(0));
    vec_i zero;
    require((zero | lz::init() | lz::count()), usize(0));
    require((zero | lz::tail() | lz::count()), usize(0));
  }
  end_test_case();

  sb::print("=== LZ TERMINALS RIGOR SUITE PASSED ===");
  return 1;
}
