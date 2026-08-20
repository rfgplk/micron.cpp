//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// fuzz_lz.cpp -- the algebraic laws, restated for lazy pipelines.
//
// The laws fuzz_algo_fp.cpp checks for the eager layer, checked again for lz::, over the same
// adversarial shapes and sizes:
//
//     take k    ++ drop k          == input
//     take_while p ++ drop_while p == input
//     filter p  ++ reject p        is a permutation of the input
//     flatten . chunk k            == input
//     concat of group_by           == input
//     nub . nub                    == nub
//     scanl f z            ends at fold_left f z
//     scanr f z            starts at fold_right f z
//     transpose . transpose        == input          (rectangular only)
//
// Plus the FUSION identities, which only the lazy layer can express and which are the cheapest way
// to catch an off-by-one in an adaptor's own bookkeeping:
//
//     take(a) | take(b)     == take(min(a, b))
//     drop(a) | drop(b)     == drop(a + b)
//     fmap(f) | fmap(g)     == fmap(g . f)
//     filter(p) | filter(q) == filter(p && q)
//     reverse | reverse     == id
//     concat(a, empty)      == a
//     init ++ [last]        == input
//     sort                  is a sorted permutation
//
// Seeds are fixed hex literals, one per suite, leetspeak-mnemonic like the other fuzz files.

#include "../../src/lz.hpp"

#include "../../src/algorithm/fp.hpp"
#include "../../src/vector.hpp"

#include "../support/algo_fuzz.hpp"

namespace sb = snowball;
namespace lz = micron::lz;
using mtest::fuzz::sweep;
using mtest::prng;

using vec_i = micron::vector<i32>;
using vec_vi = micron::vector<vec_i>;

static constexpr usize kMaxN = 4096;

static vec_i
as_vec(const i32 *p, usize n)
{
  vec_i v;
  v.reserve(n);
  for ( usize i = 0; i < n; ++i ) v.push_back(p[i]);
  return v;
}

static bool
same_seq(const vec_i &a, const vec_i &b) noexcept
{
  if ( a.size() != b.size() ) return false;
  for ( usize i = 0; i < a.size(); ++i )
    if ( a[i] != b[i] ) return false;
  return true;
}

// a multiset comparison, for the laws that only promise a permutation
static bool
same_multiset(vec_i a, vec_i b)
{
  if ( a.size() != b.size() ) return false;
  micron::sort::sort(a);
  micron::sort::sort(b);
  return same_seq(a, b);
}

// the generic-lambda trap: an unconstrained one IS invocable with const i32 *, which silently binds
// the eager pointer arm and compares POINTERS. constraining it away makes the intent load-bearing.
constexpr auto v_pos = []<typename U>(U x)
  requires(!micron::is_pointer_v<U>)
{
  return x > U(0);
};

constexpr auto v_even = []<typename U>(U x)
  requires(!micron::is_pointer_v<U>)
{
  return (x & U(1)) == U(0);
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// slicing: take/drop, take_while/drop_while, filter/reject
static void
slice_suite(u64 seed)
{
  sweep<i32, kMaxN>(
      "lz: take k ++ drop k == input",
      [](const i32 *p, usize n, prng &rng) {
        const vec_i v = as_vec(p, n);
        const usize k = n ? rng.next_in(n + 1) : 0;
        auto head = v | lz::take(k) | lz::collect<vec_i>();
        auto tail = v | lz::drop(k) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(head.size() + tail.size() != n, "take k ++ drop k changed the length");
        auto joined = head | lz::concat(tail) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(joined, v), "take k ++ drop k != input");
      },
      seed);

  sweep<i32, kMaxN>(
      "lz: take_while p ++ drop_while p == input",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto head = v | lz::take_while(v_pos) | lz::collect<vec_i>();
        auto tail = v | lz::drop_while(v_pos) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(head.size() + tail.size() != n, "take_while ++ drop_while changed the length");
        auto joined = head | lz::concat(tail) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(joined, v), "take_while p ++ drop_while p != input");
      },
      seed + 1);

  sweep<i32, kMaxN>(
      "lz: filter p ++ reject p is a permutation of the input",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto yes = v | lz::filter(v_pos) | lz::collect<vec_i>();
        auto no = v | lz::reject(v_pos) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(yes.size() + no.size() != n, "filter ++ reject changed the length");
        auto joined = yes | lz::concat(no) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_multiset(joined, v), "filter p ++ reject p is not a permutation");
        // and partition is the same split in one pass
        auto pr = v | lz::partition<vec_i>(v_pos);
        FUZZ_FAIL_IF(!same_seq(micron::get<0>(pr), yes), "partition's first half != filter");
        FUZZ_FAIL_IF(!same_seq(micron::get<1>(pr), no), "partition's second half != reject");
      },
      seed + 2);

  sweep<i32, kMaxN>(
      "lz: init ++ [last] == input",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto ini = v | lz::init() | lz::collect<vec_i>();
        auto lst = v | lz::last();
        if ( n == 0 ) {
          FUZZ_FAIL_IF(ini.size() != 0, "init of an empty range is not empty");
          FUZZ_FAIL_IF(lst.is_first(), "last of an empty range reported a value");
          return;
        }
        FUZZ_FAIL_IF(ini.size() + 1 != n, "init dropped the wrong number of elements");
        FUZZ_FAIL_IF(!lst.is_first(), "last of a non-empty range reported no value");
        ini.push_back(lst.cast<i32>());
        FUZZ_FAIL_IF(!same_seq(ini, v), "init ++ [last] != input");
        // and tail is drop(1)
        auto tl = v | lz::tail() | lz::collect<vec_i>();
        auto d1 = v | lz::drop(1) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(tl, d1), "tail != drop(1)");
      },
      seed + 3);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the fusion identities -- only lz:: can state these
static void
fusion_suite(u64 seed)
{
  sweep<i32, kMaxN>(
      "lz: take(a)|take(b) == take(min(a,b)) and drop(a)|drop(b) == drop(a+b)",
      [](const i32 *p, usize n, prng &rng) {
        const vec_i v = as_vec(p, n);
        const usize a = rng.next_in(n + 2);
        const usize b = rng.next_in(n + 2);
        auto twice = v | lz::take(a) | lz::take(b) | lz::collect<vec_i>();
        auto once = v | lz::take(a < b ? a : b) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(twice, once), "take(a)|take(b) != take(min(a,b))");

        auto dtwice = v | lz::drop(a) | lz::drop(b) | lz::collect<vec_i>();
        auto donce = v | lz::drop(a + b) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(dtwice, donce), "drop(a)|drop(b) != drop(a+b)");
      },
      seed);

  sweep<i32, kMaxN>(
      "lz: fmap(f)|fmap(g) == fmap(g.f), filter(p)|filter(q) == filter(p&&q)",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto f = [](i32 x) { return x + 3; };
        auto g = [](i32 x) { return x * 2; };
        auto twice = v | lz::fmap(f) | lz::fmap(g) | lz::collect<vec_i>();
        auto once = v | lz::fmap([](i32 x) { return (x + 3) * 2; }) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(twice, once), "fmap(f)|fmap(g) != fmap(g.f)");

        auto ftwice = v | lz::filter(v_pos) | lz::filter(v_even) | lz::collect<vec_i>();
        auto fonce = v | lz::filter([](i32 x) { return x > 0 && (x & 1) == 0; }) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(ftwice, fonce), "filter(p)|filter(q) != filter(p&&q)");
      },
      seed + 1);

  sweep<i32, kMaxN>(
      "lz: reverse|reverse == id, concat(a, empty) == a",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto rr = v | lz::reverse() | lz::reverse() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(rr, v), "reverse|reverse != id");
        // the buffered arm has to agree with the streaming one
        auto br = v | lz::filter([](i32) { return true; }) | lz::reverse() | lz::collect<vec_i>();
        auto sr = v | lz::reverse() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(br, sr), "buffered reverse != streaming reverse");

        vec_i empty;
        auto ce = v | lz::concat(empty) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(ce, v), "concat(a, empty) != a");
        auto ec = empty | lz::concat(v) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(ec, v), "concat(empty, a) != a");
      },
      seed + 2);

  sweep<i32, kMaxN>(
      "lz: sort is a sorted permutation, and is idempotent",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto s = v | lz::sort() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(s.size() != n, "sort changed the length");
        for ( usize i = 1; i < n; ++i ) FUZZ_FAIL_IF(s[i] < s[i - 1], "sort left an inversion");
        FUZZ_FAIL_IF(!same_multiset(s, v), "sort is not a permutation of the input");
        auto ss = v | lz::sort() | lz::sort() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(ss, s), "sort is not idempotent");
      },
      seed + 3);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reshaping: chunk, group, transpose
static void
reshape_suite(u64 seed)
{
  sweep<i32, 1024>(
      "lz: flatten . chunk k == input",
      [](const i32 *p, usize n, prng &rng) {
        const vec_i v = as_vec(p, n);
        const usize k = 1 + rng.next_in(17);
        auto back = v | lz::chunk(k) | lz::flatten() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(back, v), "flatten . chunk != input");
        const usize expect = (n + k - 1) / k;
        FUZZ_FAIL_IF((v | lz::chunk(k) | lz::count()) != expect, "chunk produced the wrong number of chunks");
        // chunk_into agrees with the eager one
        auto ci = v | lz::chunk_into<vec_vi>(k);
        auto ei = micron::fp::chunk_into<vec_vi>(v, k);
        FUZZ_FAIL_IF(ci.size() != ei.size(), "chunk_into disagrees with fp::chunk_into on count");
        for ( usize i = 0; i < ci.size(); ++i ) FUZZ_FAIL_IF(!same_seq(ci[i], ei[i]), "chunk_into disagrees with fp::chunk_into");
      },
      seed);

  sweep<i32, 1024>(
      "lz: concat of group_by == input, and the run lengths match fp::group_by",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto back = v | lz::group() | lz::flatten() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(back, v), "concat of group != input");

        // the comparison is against the PREVIOUS INPUT element (fpdata.hpp), not the run's first.
        // an ascending-run relation is non-transitive, so it separates the two readings.
        auto asc = [](i32 a, i32 b) { return a < b; };
        auto eg = micron::fp::group_by<vec_vi>(v, asc);
        usize gi = 0;
        bool bad = false;
        v | lz::group_by(asc) | lz::for_each([&](auto w) {
          if ( gi >= eg.size() || w.size() != eg[gi].size() ) bad = true;
          ++gi;
        });
        FUZZ_FAIL_IF(bad || gi != eg.size(), "group_by's runs disagree with fp::group_by");

        auto gback = v | lz::group_by(asc) | lz::flatten() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(gback, v), "concat of group_by != input");
      },
      seed + 1);

  sweep<i32, 256>(
      "lz: transpose . transpose == input (rectangular)",
      [](const i32 *p, usize n, prng &rng) {
        if ( n < 2 ) return;
        const usize rows = 1 + rng.next_in(8);
        const usize cols = n / rows;
        if ( cols == 0 ) return;
        vec_vi mat;
        for ( usize r = 0; r < rows; ++r ) {
          vec_i row;
          for ( usize c = 0; c < cols; ++c ) row.push_back(p[r * cols + c]);
          mat.push_back(micron::move(row));
        }
        auto t = mat | lz::transpose<vec_vi>();
        FUZZ_FAIL_IF(!t.is_first(), "transpose rejected a rectangular matrix");
        auto tt = t.cast<vec_vi>() | lz::transpose<vec_vi>();
        FUZZ_FAIL_IF(!tt.is_first(), "transpose . transpose rejected a rectangular matrix");
        const auto &b = tt.cast<vec_vi>();
        FUZZ_FAIL_IF(b.size() != rows, "transpose . transpose changed the row count");
        for ( usize r = 0; r < rows; ++r ) FUZZ_FAIL_IF(!same_seq(b[r], mat[r]), "transpose . transpose != input");
        // and it agrees with the eager one
        auto et = micron::fp::transpose(mat);
        FUZZ_FAIL_IF(!et.is_first(), "fp::transpose rejected a rectangular matrix");
        const auto &ev = et.cast<vec_vi>();
        const auto &lv = t.cast<vec_vi>();
        FUZZ_FAIL_IF(ev.size() != lv.size(), "transpose disagrees with fp::transpose on shape");
        for ( usize c = 0; c < ev.size(); ++c ) FUZZ_FAIL_IF(!same_seq(lv[c], ev[c]), "transpose disagrees with fp::transpose");
      },
      seed + 2);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dedup and the scans
static void
scan_dedup_suite(u64 seed)
{
  sweep<i32, kMaxN>(
      "lz: nub . nub == nub, and unique/nub agree with fp::",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto nb = v | lz::nub() | lz::collect<vec_i>();
        auto en = micron::fp::nub(v);
        FUZZ_FAIL_IF(!same_seq(nb, en), "nub disagrees with fp::nub");
        auto nn = v | lz::nub() | lz::nub() | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(nn, nb), "nub . nub != nub");

        auto uq = v | lz::unique() | lz::collect<vec_i>();
        auto eu = micron::fp::unique(v);
        FUZZ_FAIL_IF(!same_seq(uq, eu), "unique disagrees with fp::unique");
        // unique never grows, and nub is at most unique
        FUZZ_FAIL_IF(uq.size() > n, "unique grew the sequence");
        FUZZ_FAIL_IF(nb.size() > uq.size(), "nub kept more than unique");
      },
      seed);

  sweep<i32, kMaxN>(
      "lz: scanl f z ends at fold_left f z; scanr f z starts at fold_right f z",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        auto add = [](i32 a, i32 x) { return a + x; };
        auto radd = [](i32 x, i32 a) { return x + a; };

        auto sl = v | lz::scanl(0, add) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(sl.size() != n + 1, "scanl did not emit n + 1 elements");
        FUZZ_FAIL_IF(sl[0] != 0, "scanl did not start at the seed");
        FUZZ_FAIL_IF(sl[n] != (v | lz::fold(0, add)), "scanl does not end at fold_left");
        auto el = micron::fp::scanl(v, 0, add);
        FUZZ_FAIL_IF(!same_seq(sl, el), "scanl disagrees with fp::scanl");

        auto sr = v | lz::scanr(radd, 0) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(sr.size() != n + 1, "scanr did not emit n + 1 elements");
        FUZZ_FAIL_IF(sr[n] != 0, "scanr did not end at the seed");
        FUZZ_FAIL_IF(sr[0] != (v | lz::foldr(radd, 0)), "scanr does not start at fold_right");
        auto er = micron::fp::scanr(v, 0, radd);
        FUZZ_FAIL_IF(!same_seq(sr, er), "scanr disagrees with fp::scanr");
        // the buffered arm of scanr has to agree with the reversible one
        auto sr2 = v | lz::filter([](i32) { return true; }) | lz::scanr(radd, 0) | lz::collect<vec_i>();
        FUZZ_FAIL_IF(!same_seq(sr2, sr), "buffered scanr != reversible scanr");
      },
      seed + 1);

  sweep<i32, kMaxN>(
      "lz: reductions agree with the eager kernels",
      [](const i32 *p, usize n, prng &) {
        const vec_i v = as_vec(p, n);
        FUZZ_FAIL_IF((v | lz::count()) != n, "count != n");
        if ( n == 0 ) {
          FUZZ_FAIL_IF((v | lz::safe_sum()).is_first(), "safe_sum of an empty range reported a value");
          FUZZ_FAIL_IF((v | lz::safe_max()).is_first(), "safe_max of an empty range reported a value");
          return;
        }
        FUZZ_FAIL_IF((v | lz::sum()) != micron::sum(v), "sum != micron::sum");
        // the streaming arm, reached through an identity fmap, must be bit-identical
        FUZZ_FAIL_IF((v | lz::fmap([](i32 x) { return x; }) | lz::sum()) != micron::sum(v),
                     "the streaming sum diverges from the flat one");
        FUZZ_FAIL_IF((v | lz::safe_max()).cast<i32>() != micron::fp::safe_max(v).cast<i32>(), "max != fp::safe_max");
        FUZZ_FAIL_IF((v | lz::safe_min()).cast<i32>() != micron::fp::safe_min(v).cast<i32>(), "min != fp::safe_min");
        FUZZ_FAIL_IF((v | lz::count_if(v_pos)) != micron::fp::count(v, v_pos), "count_if != fp::count");
        FUZZ_FAIL_IF((v | lz::elem(p[n / 2])) != micron::fp::elem(v, p[n / 2]), "elem != fp::elem");
        FUZZ_FAIL_IF(static_cast<umax_t>(v | lz::count_of(p[0])) != micron::count(v, p[0]), "count_of != micron::count");
      },
      seed + 2);

  sweep<i32, kMaxN>(
      "lz: zip_with truncates to the shorter side, both ways round",
      [](const i32 *p, usize n, prng &rng) {
        const vec_i v = as_vec(p, n);
        const usize m = rng.next_in(n + 2);
        vec_i w;
        for ( usize i = 0; i < m; ++i ) w.push_back(static_cast<i32>(rng.next_in(64)));
        auto add = [](i32 a, i32 b) { return a + b; };
        const usize ov = n < m ? n : m;
        FUZZ_FAIL_IF((v | lz::zip_with(w, add) | lz::count()) != ov, "zip_with did not truncate to the shorter side");
        FUZZ_FAIL_IF((w | lz::zip_with(v, add) | lz::count()) != ov, "zip_with is not symmetric in its truncation");
        auto z = v | lz::zip_with(w, add) | lz::collect<vec_i>();
        for ( usize i = 0; i < ov; ++i ) FUZZ_FAIL_IF(z[i] != v[i] + w[i], "zip_with produced the wrong element");
        const bool strict_ok = (v | lz::zip_strict<vec_i>(w, add)).is_first();
        FUZZ_FAIL_IF(strict_ok != (n == m), "zip_strict disagreed with the actual lengths");
      },
      seed + 3);
}

int
main()
{
  sb::print("=== LZ LAW FUZZ SUITE ===");

  slice_suite(0x51CE1001ULL);
  fusion_suite(0xF05E1002ULL);
  reshape_suite(0x2E5AA003ULL);
  scan_dedup_suite(0x5CADED04ULL);

  sb::print("=== LZ LAW FUZZ SUITE PASSED ===");
  return 1;
}
