//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// fuzz_algo_fp — fuzzing the fp:: tier (fpalgorithm / fpdata / fpfilter / fparith).
//
// The fp tier is where algebraic laws are the strongest available oracle, because most of these
// combinators come in pairs that must compose back to the identity:
//
//     take k    ++ drop k          == input
//     take_while p ++ drop_while p == input          (and span p is exactly that pair)
//     filter p  ++ reject p        is a permutation of the input
//     flatten . chunk_into k       == input
//     concat of group_by           == input
//     nub . nub                    == nub            (idempotent)
//     scanl f z            ends at fold_left f z
//     scanr f z            starts at fold_right f z
//     transpose . transpose        == input          (rectangular only)
//
// nub gets particular attention: it dispatches on whether micron::hash accepts the element type,
// taking a hash set when it does and a quadratic rescan when it does not, and the two paths must
// agree element for element. Opaque below deliberately has == and nothing else, so it is the
// only way to reach the fallback.
//
// option<T,E> convention: success is is_first(), NOT has_value(), and the payload comes out of
// cast<U>().

#include "../snowball/snowball_fuzz.hpp"
#include "../support/algo_fuzz.hpp"

#include "../../src/algorithm/fp.hpp"

namespace sb = snowball;
namespace sbf = snowball::fuzzing;

using mtest::fuzz::sweep;
using mtest::prng;
namespace fref = mtest::fuzz::fref;

namespace
{

constexpr usize MAXN = 512;

using vec = micron::vector<i32>;
using vecvec = micron::vector<micron::vector<i32>>;

// hashable == takes the hash-set path in nub; Opaque has only ==, so it takes the fallback
struct Opaque {
  i32 v;
  bool
  operator==(const Opaque &o) const
  {
    return v == o.v;
  }
};

static_assert(micron::fp::__impl::hash_dedupable<i32>, "i32 must reach nub's hash path");
static_assert(!micron::fp::__impl::hash_dedupable<Opaque>, "Opaque must reach nub's quadratic path");

constexpr auto v_pos = []<typename U>(U x)
  requires(!micron::is_pointer_v<U>)
{
  return x > U(0);
};

template<typename T>
micron::vector<T>
as_vec(const T *p, usize n)
{
  micron::vector<T> v;
  v.resize(n);
  for ( usize i = 0; i < n; ++i ) v[i] = p[i];
  return v;
}

template<typename A, typename B>
bool
same_seq(const A &a, const B &b)
{
  if ( a.size() != b.size() ) return false;
  for ( usize i = 0; i < a.size(); ++i )
    if ( !(a[i] == b[i]) ) return false;
  return true;
}

// ─────────────────────────────────────────────────────────────────────────
// slicing laws: take / drop / take_while / drop_while / span / sbreak
// ─────────────────────────────────────────────────────────────────────────
void
slice_suite(u64 seed)
{
  sweep<i32, MAXN>(
      "fp slicing laws",
      [](i32 *buf, usize n, prng &rng) {
        vec v = as_vec(buf, n);
        const usize k = static_cast<usize>(rng.next() % (n + 2));

        // take k ++ drop k == input
        auto t = micron::fp::take(v, k);
        auto d = micron::fp::drop(v, k);
        FUZZ_FAIL_IF(t.size() != (k < n ? k : n), "take returned the wrong length");
        FUZZ_FAIL_IF(t.size() + d.size() != n, "take and drop do not partition the input");
        for ( usize i = 0; i < t.size(); ++i ) FUZZ_FAIL_IF(!(t[i] == buf[i]), "take kept the wrong elements");
        for ( usize i = 0; i < d.size(); ++i ) FUZZ_FAIL_IF(!(d[i] == buf[t.size() + i]), "drop kept the wrong elements");

        // take_while p ++ drop_while p == input, and span p is exactly that pair
        const usize tw = fref::naive_take_while(buf, n, v_pos);
        auto a = micron::fp::take_while(v, v_pos);
        auto b = micron::fp::drop_while(v, v_pos);
        FUZZ_FAIL_IF(a.size() != tw, "take_while stopped at the wrong element");
        FUZZ_FAIL_IF(a.size() + b.size() != n, "take_while and drop_while do not partition the input");
        for ( usize i = 0; i < a.size(); ++i ) FUZZ_FAIL_IF(!(a[i] == buf[i]), "take_while kept the wrong elements");
        for ( usize i = 0; i < b.size(); ++i ) FUZZ_FAIL_IF(!(b[i] == buf[tw + i]), "drop_while kept the wrong elements");

        auto sp = micron::fp::span(v, v_pos);
        FUZZ_FAIL_IF(!same_seq(micron::get<0>(sp), a), "span's first half is not take_while");
        FUZZ_FAIL_IF(!same_seq(micron::get<1>(sp), b), "span's second half is not drop_while");

        // sbreak is span over the negated predicate
        auto br = micron::fp::sbreak(v, v_pos);
        auto na = micron::fp::take_while(v, [](i32 x) { return !(x > 0); });
        FUZZ_FAIL_IF(!same_seq(micron::get<0>(br), na), "sbreak is not span over the negated predicate");
      },
      seed);
}

// ─────────────────────────────────────────────────────────────────────────
// dedup: unique / nub, both dispatch paths
// ─────────────────────────────────────────────────────────────────────────
void
dedup_suite(u64 seed)
{
  sweep<i32, MAXN>(
      "fp dedup (nub hash path vs quadratic path)",
      [](i32 *buf, usize n, prng &) {
        vec v = as_vec(buf, n);

        // unique collapses only CONSECUTIVE duplicates
        auto u = micron::fp::unique(v);
        auto wu = fref::naive_unique(buf, n);
        FUZZ_FAIL_IF(u.size() != wu.size(), "unique kept the wrong number of elements");
        for ( usize i = 0; i < u.size(); ++i ) FUZZ_FAIL_IF(!(u[i] == wu[i]), "unique kept the wrong elements");
        for ( usize i = 1; i < u.size(); ++i ) FUZZ_FAIL_IF(u[i] == u[i - 1], "unique left an adjacent duplicate");

        // nub keeps the first occurrence of every distinct value
        auto nb = micron::fp::nub(v);
        auto wn = fref::naive_nub(buf, n);
        FUZZ_FAIL_IF(nb.size() != wn.size(), "nub kept the wrong number of elements");
        for ( usize i = 0; i < nb.size(); ++i ) FUZZ_FAIL_IF(!(nb[i] == wn[i]), "nub kept the wrong elements or order");

        // every element is distinct, and nub is idempotent
        for ( usize i = 0; i < nb.size(); ++i )
          for ( usize j = i + 1; j < nb.size(); ++j ) FUZZ_FAIL_IF(nb[i] == nb[j], "nub left a duplicate");
        auto nb2 = micron::fp::nub(nb);
        FUZZ_FAIL_IF(!same_seq(nb, nb2), "nub is not idempotent");

        // the SAME input through the quadratic path must give the same answer
        micron::vector<Opaque> ov;
        ov.resize(n);
        for ( usize i = 0; i < n; ++i ) ov[i] = Opaque{ buf[i] };
        auto onb = micron::fp::nub(ov);
        FUZZ_FAIL_IF(onb.size() != nb.size(), "nub's two dispatch paths disagree on size");
        for ( usize i = 0; i < onb.size(); ++i ) FUZZ_FAIL_IF(!(onb[i].v == nb[i]), "nub's two dispatch paths disagree on contents");
      },
      seed, 2);
}

// ─────────────────────────────────────────────────────────────────────────
// partition / reject / filter agreement
// ─────────────────────────────────────────────────────────────────────────
void
partition_suite(u64 seed)
{
  sweep<i32, MAXN>(
      "fp partition / reject",
      [](i32 *buf, usize n, prng &) {
        vec v = as_vec(buf, n);

        auto pr = micron::fp::partition(v, v_pos);
        const auto &yes = micron::get<0>(pr);
        const auto &no = micron::get<1>(pr);
        FUZZ_FAIL_IF(yes.size() + no.size() != n, "partition does not cover the input");
        for ( usize i = 0; i < yes.size(); ++i ) FUZZ_FAIL_IF(!(yes[i] > 0), "partition put a failing element in the yes half");
        for ( usize i = 0; i < no.size(); ++i ) FUZZ_FAIL_IF(no[i] > 0, "partition put a passing element in the no half");

        // reject is filter over the negated predicate
        auto rj = micron::fp::reject(v, v_pos);
        FUZZ_FAIL_IF(!same_seq(rj, no), "reject disagrees with partition's no half");

        // yes ++ no is a permutation of the input
        micron::vector<i32> cat = yes;
        for ( usize i = 0; i < no.size(); ++i ) cat.push_back(no[i]);
        FUZZ_FAIL_IF(!fref::naive_is_permutation(cat.begin(), cat.size(), buf, n), "partition's halves are not a permutation of the input");
      },
      seed);
}

// ─────────────────────────────────────────────────────────────────────────
// scans: scanl / scanr against the folds they generalize
// ─────────────────────────────────────────────────────────────────────────
void
scan_suite(u64 seed)
{
  sweep<i32, MAXN>(
      "fp scanl / scanr",
      [](i32 *buf, usize n, prng &) {
        // keep the accumulator small so nothing overflows and the oracle stays exact
        for ( usize i = 0; i < n; ++i ) buf[i] = static_cast<i32>(buf[i] % 8);
        vec v = as_vec(buf, n);
        auto add = [](i32 a, i32 x) { return static_cast<i32>(a + x); };
        auto radd = [](i32 x, i32 a) { return static_cast<i32>(a + x); };

        auto l = micron::fp::scanl(v, i32(0), add);
        auto wl = fref::naive_scanl(buf, n, i32(0), add);
        FUZZ_FAIL_IF(l.size() != n + 1, "scanl did not produce n+1 elements");
        for ( usize i = 0; i <= n; ++i ) FUZZ_FAIL_IF(!(l[i] == wl[i]), "scanl disagrees with oracle");
        // the final prefix IS the fold
        i32 fold = 0;
        for ( usize i = 0; i < n; ++i ) fold = add(fold, buf[i]);
        FUZZ_FAIL_IF(!(l[n] == fold), "scanl does not end at fold_left");

        auto r = micron::fp::scanr(v, i32(0), radd);
        auto wr = fref::naive_scanr(buf, n, i32(0), radd);
        FUZZ_FAIL_IF(r.size() != n + 1, "scanr did not produce n+1 elements");
        for ( usize i = 0; i <= n; ++i ) FUZZ_FAIL_IF(!(r[i] == wr[i]), "scanr disagrees with oracle");
        FUZZ_FAIL_IF(!(r[0] == fold), "scanr does not start at fold_right (over a commutative op)");
      },
      seed);
}

// ─────────────────────────────────────────────────────────────────────────
// reshaping: chunk_into / sliding / flatten / group / intersperse
// ─────────────────────────────────────────────────────────────────────────
void
reshape_suite(u64 seed)
{
  sweep<i32, 128>(
      "fp reshaping laws",
      [](i32 *buf, usize n, prng &rng) {
        vec v = as_vec(buf, n);
        const usize k = 1 + static_cast<usize>(rng.next() % 8u);

        // flatten . chunk_into k == identity
        vecvec ch = micron::fp::chunk_into<vecvec>(v, k);
        usize total = 0;
        for ( usize i = 0; i < ch.size(); ++i ) {
          FUZZ_FAIL_IF(ch[i].size() == 0, "chunk_into produced an empty chunk");
          FUZZ_FAIL_IF(ch[i].size() > k, "chunk_into produced an oversized chunk");
          // only the last chunk may be short
          if ( i + 1 < ch.size() ) FUZZ_FAIL_IF(ch[i].size() != k, "chunk_into produced a short interior chunk");
          total += ch[i].size();
        }
        FUZZ_FAIL_IF(total != n, "chunk_into lost or duplicated elements");
        auto flat = micron::fp::flatten(ch);
        FUZZ_FAIL_IF(!same_seq(flat, v), "flatten . chunk_into is not the identity");

        // sliding: total - k + 1 windows, each of width k
        if ( n >= k ) {
          vecvec sl = micron::fp::sliding<vecvec>(v, k);
          FUZZ_FAIL_IF(sl.size() != n - k + 1, "sliding produced the wrong number of windows");
          for ( usize i = 0; i < sl.size(); ++i ) {
            FUZZ_FAIL_IF(sl[i].size() != k, "sliding produced a window of the wrong width");
            for ( usize j = 0; j < k; ++j ) FUZZ_FAIL_IF(!(sl[i][j] == buf[i + j]), "sliding window contents are wrong");
          }
        }

        // group: the concatenation of the runs is the input, and adjacent runs differ
        if ( n ) {
          vecvec g = micron::fp::group<vecvec>(v);
          auto want = fref::naive_group_by(buf, n, [](i32 a, i32 b) { return a == b; });
          FUZZ_FAIL_IF(g.size() != want.size(), "group produced the wrong number of runs");
          for ( usize i = 0; i < g.size(); ++i ) {
            FUZZ_FAIL_IF(g[i].size() != want[i].b, "group run has the wrong length");
            for ( usize j = 0; j < g[i].size(); ++j ) FUZZ_FAIL_IF(!(g[i][j] == buf[want[i].a + j]), "group run contents are wrong");
          }
          auto gflat = micron::fp::flatten(g);
          FUZZ_FAIL_IF(!same_seq(gflat, v), "flatten . group is not the identity");
        }

        // intersperse: 2n-1 elements for n >= 2, original values at even indices
        {
          const i32 sep = -12345;
          auto is = micron::fp::intersperse(v, sep);
          if ( n < 2 ) {
            FUZZ_FAIL_IF(!same_seq(is, v), "intersperse altered a range shorter than 2");
          } else {
            FUZZ_FAIL_IF(is.size() != 2 * n - 1, "intersperse produced the wrong length");
            for ( usize i = 0; i < is.size(); ++i ) {
              if ( i & 1u )
                FUZZ_FAIL_IF(!(is[i] == sep), "intersperse did not put the separator at an odd index");
              else
                FUZZ_FAIL_IF(!(is[i] == buf[i / 2]), "intersperse moved an original element");
            }
          }
        }
      },
      seed, 2);
}

// ─────────────────────────────────────────────────────────────────────────
// element access + safe aggregates
// ─────────────────────────────────────────────────────────────────────────
void
access_suite(u64 seed)
{
  sweep<i32, MAXN>(
      "fp element access / safe aggregates",
      [](i32 *buf, usize n, prng &rng) {
        vec v = as_vec(buf, n);

        // head / last / tail / init, and the empty-range error branch
        auto hd = micron::fp::head(v);
        auto lt = micron::fp::last(v);
        FUZZ_FAIL_IF(hd.is_first() != (n != 0), "head disagrees with emptiness");
        FUZZ_FAIL_IF(lt.is_first() != (n != 0), "last disagrees with emptiness");
        if ( n ) {
          FUZZ_FAIL_IF(!(hd.template cast<i32>() == buf[0]), "head is not element 0");
          FUZZ_FAIL_IF(!(lt.template cast<i32>() == buf[n - 1]), "last is not the final element");

          auto tl = micron::fp::tail(v);
          auto in = micron::fp::init(v);
          FUZZ_FAIL_IF(!tl.is_first() || !in.is_first(), "tail/init reported an error on a non-empty range");
          auto tlv = tl.template cast<vec>();
          auto inv = in.template cast<vec>();
          FUZZ_FAIL_IF(tlv.size() != n - 1 || inv.size() != n - 1, "tail/init produced the wrong length");
          for ( usize i = 0; i + 1 < n; ++i ) {
            FUZZ_FAIL_IF(!(tlv[i] == buf[i + 1]), "tail dropped the wrong end");
            FUZZ_FAIL_IF(!(inv[i] == buf[i]), "init dropped the wrong end");
          }
        }

        // at() is bounds-checked
        {
          const usize i = static_cast<usize>(rng.next() % (n + 2));
          auto e = micron::fp::at(v, i);
          FUZZ_FAIL_IF(e.is_first() != (i < n), "at disagrees with the bounds");
          if ( i < n ) FUZZ_FAIL_IF(!(e.template cast<i32>() == buf[i]), "at returned the wrong element");
        }

        // elem mirrors contains
        {
          const i32 needle = n ? buf[rng.next() % n] : i32(7);
          FUZZ_FAIL_IF(micron::fp::elem(v, needle) != (micron::find(v.begin(), v.end(), needle) != nullptr),
                       "elem disagrees with find");
        }

        // find_first / find_index agree with each other and with find_if
        {
          auto ff = micron::fp::find_first(v, v_pos);
          auto fi = micron::fp::find_index(v, v_pos);
          const i32 *w = mtest::rigor::ref::naive_find_if(buf, n, v_pos);
          FUZZ_FAIL_IF(ff.is_first() != (w != nullptr), "find_first disagrees with find_if on presence");
          FUZZ_FAIL_IF(fi.is_first() != (w != nullptr), "find_index disagrees with find_if on presence");
          if ( w ) {
            FUZZ_FAIL_IF(!(ff.template cast<i32>() == *w), "find_first returned the wrong element");
            FUZZ_FAIL_IF(fi.template cast<usize>() != static_cast<usize>(w - buf), "find_index returned the wrong index");
          }
        }

        // safe aggregates report the empty case instead of reading element 0
        {
          auto mx = micron::fp::safe_max(v);
          auto mn = micron::fp::safe_min(v);
          auto sm = micron::fp::safe_sum(v);
          FUZZ_FAIL_IF(mx.is_first() != (n != 0), "safe_max disagrees with emptiness");
          FUZZ_FAIL_IF(mn.is_first() != (n != 0), "safe_min disagrees with emptiness");
          FUZZ_FAIL_IF(sm.is_first() != (n != 0), "safe_sum disagrees with emptiness");
          if ( n ) {
            FUZZ_FAIL_IF(!(mx.template cast<i32>() == mtest::rigor::ref::naive_max(buf, n)), "safe_max disagrees with oracle");
            FUZZ_FAIL_IF(!(mn.template cast<i32>() == mtest::rigor::ref::naive_min(buf, n)), "safe_min disagrees with oracle");
          }
        }

        // enumerate pairs every element with its index
        {
          using pair_t = micron::tuple<usize, i32>;
          micron::vector<pair_t> en = micron::fp::enumerate<micron::vector<pair_t>>(v);
          FUZZ_FAIL_IF(en.size() != n, "enumerate changed the length");
          for ( usize i = 0; i < n; ++i ) {
            FUZZ_FAIL_IF(micron::get<0>(en[i]) != i, "enumerate produced the wrong index");
            FUZZ_FAIL_IF(!(micron::get<1>(en[i]) == buf[i]), "enumerate produced the wrong element");
          }
        }
      },
      seed, 2);
}

// ─────────────────────────────────────────────────────────────────────────
// fmap / zip_with
// ─────────────────────────────────────────────────────────────────────────
void
map_suite(u64 seed)
{
  sweep<i32, MAXN>(
      "fp fmap / zip_with",
      [](i32 *buf, usize n, prng &) {
        vec v = as_vec(buf, n);

        auto inc = [](i32 x) { return static_cast<i32>(x + 1); };
        auto m = micron::fp::fmap(inc, v);
        FUZZ_FAIL_IF(m.size() != n, "fmap changed the length");
        for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(m[i] == static_cast<i32>(buf[i] + 1)), "fmap disagrees elementwise");
        // fmap does not touch the source
        for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(v[i] == buf[i]), "fmap mutated its argument");

        // zip_with over equal lengths, and the mismatched-length error branch
        auto z = micron::fp::zip_with(v, v, [](i32 a, i32 b) { return static_cast<i32>(a + b); });
        FUZZ_FAIL_IF(!z.is_first(), "zip_with reported an error for equal lengths");
        auto zv = z.template cast<vec>();
        for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(!(zv[i] == static_cast<i32>(buf[i] + buf[i])), "zip_with disagrees elementwise");

        if ( n ) {
          vec shorter = micron::fp::take(v, n - 1);
          auto bad = micron::fp::zip_with(v, shorter, [](i32 a, i32 b) { return static_cast<i32>(a + b); });
          FUZZ_FAIL_IF(bad.is_first(), "zip_with accepted mismatched lengths");
          // ...but the truncating form takes the shorter length
          auto tr = micron::fp::zip_with_trunc(v, shorter, [](i32 a, i32 b) { return static_cast<i32>(a + b); });
          FUZZ_FAIL_IF(tr.size() != n - 1, "zip_with_trunc did not truncate to the shorter length");
        }
      },
      seed);
}

}      // namespace

int
main(void)
{
  sb::print("=== ALGO/FP FUZZ SUITE ===");

  slice_suite(0xFA5110E1ULL);
  dedup_suite(0xDED09002ULL);
  partition_suite(0xBA2711003ULL);
  scan_suite(0x5CA41004ULL);
  reshape_suite(0x2E5AA9E5ULL);
  access_suite(0xACCE5506ULL);
  map_suite(0xFA9A9007ULL);

  // ────────────────────────────────────────────────────────────────────
  // generator-driven laws
  // ────────────────────────────────────────────────────────────────────

  sbf::check_property(
      "nub's two dispatch paths agree on a fuzzed vector",
      [](micron::vector<i32> v) {
        micron::vector<Opaque> o;
        o.resize(v.size());
        for ( usize i = 0; i < v.size(); ++i ) o[i] = Opaque{ v[i] };
        auto a = micron::fp::nub(v);
        auto b = micron::fp::nub(o);
        if ( a.size() != b.size() ) FUZZ_FAIL("nub paths disagree on size");
        for ( usize i = 0; i < a.size(); ++i )
          if ( a[i] != b[i].v ) FUZZ_FAIL("nub paths disagree on contents");
      },
      { .seed = 0x0BAD0BAD, .count = 8000 }, sbf::vector_of(sbf::range<i32>(0, 12)).len(0, 64));

  sbf::check_property(
      "take k ++ drop k reconstructs the input for any k",
      [](micron::vector<i32> v, u32 raw_k) {
        const usize k = static_cast<usize>(raw_k % (v.size() + 4));
        auto t = micron::fp::take(v, k);
        auto d = micron::fp::drop(v, k);
        if ( t.size() + d.size() != v.size() ) FUZZ_FAIL("take/drop do not partition");
        for ( usize i = 0; i < t.size(); ++i )
          if ( t[i] != v[i] ) FUZZ_FAIL("take kept the wrong prefix");
        for ( usize i = 0; i < d.size(); ++i )
          if ( d[i] != v[t.size() + i] ) FUZZ_FAIL("drop kept the wrong suffix");
      },
      { .seed = 0x7A4ED209, .count = 20000 }, sbf::vector_of(sbf::spec<i32>{}).len(0, 64), sbf::spec<u32>{});

  sb::print("=== ALGO/FP FUZZ SUITE PASSED ===");
  return 1;
}
