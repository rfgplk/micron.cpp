//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// fuzz_algo_search — differential fuzzing of src/algorithm/find.hpp.
//
// Every entry point is diffed against a naive oracle over the full shape corpus at every
// adversarial size. Two things this catches that an example-based test cannot:
//
//   * the ELEMENT TYPE selects the strategy. find/count/find_last dispatch to a vector lane
//     scan for integral, enum and pointer elements and keep the scalar loop for everything
//     else, so this sweeps eight integer widths (signed and unsigned) plus f64 -- which must
//     NOT take the lane path, since a bitwise scan reports +0.0 == -0.0 and NaN == NaN
//     backwards. The signed_zero and with_nan shapes exist to prove it does not.
//   * the PATTERN WIDTH selects the strategy too. search and find_end skip-scan below 8, run
//     KMP off a stack failure table up to 256, and skip-scan again above that, so needle
//     widths straddle both boundaries.
//
// Predicates are deliberately total (x > 0) rather than arithmetic on the value: casting a
// 1e30 f64 through i64 is undefined, and a fuzz suite must not itself be the bug.
//
// Failures throw; snowball_fuzz catches and reports the seed and iteration.

#include "../snowball/snowball_fuzz.hpp"
#include "../support/algo_fuzz.hpp"

#include "../../src/algorithm/find.hpp"

namespace sb = snowball;
namespace sbf = snowball::fuzzing;

using mtest::fuzz::sweep;
using mtest::fuzz::sweep2;
using mtest::prng;
namespace ref = mtest::rigor::ref;
namespace fref = mtest::fuzz::fref;

namespace
{

constexpr usize MAXN = 1024;

// The value-taking predicates MUST be unusable with a pointer argument. micron's overload
// sets choose between Fn(const T) and Fn(const T*) with is_invocable_v, and a bare
// `[](auto x)` satisfies both -- the pointer arm then wins and the body hard-errors outside
// the immediate context, which no amount of SFINAE can recover from.
constexpr auto v_pos = []<typename U>(U x)
  requires(!micron::is_pointer_v<U>)
{
  return x > U(0);
};
constexpr auto p_pos = []<typename U>(const U *x) { return *x > U(0); };
constexpr auto p_eq = []<typename U>(const U *a, const U *b) { return *a == *b; };

// ─────────────────────────────────────────────────────────────────────────
// quantifiers, finds and counts over every shape and size
// ─────────────────────────────────────────────────────────────────────────
template<typename T>
void
scan_suite(const char *tag, u64 seed)
{
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &rng) {
        const T *b = buf;
        const T *e = buf + n;
        // half the needles are drawn from the buffer so hits are common; the rest are
        // arbitrary, so misses are covered too
        const T needle = (n && (rng.next() & 1u)) ? buf[rng.next() % n] : static_cast<T>(rng.next() & 0xffu);
        // see mtest::fuzz::cmp_diff_ok -- under -ffinite-math-only a NaN-bearing range cannot be
        // differentially compared, so the calls still run but the diffs are withheld
        const bool chk = mtest::fuzz::cmp_diff_ok(b, n);

        // -- find / find_last / contains ------------------------------------
        const T *got = micron::find(b, e, needle);
        const T *want = ref::naive_find(b, n, needle);
        FUZZ_FAIL_IF(chk && got != want, "find disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::contains(b, e, needle) != (want != nullptr), "contains disagrees with find");

        const T *gotl = micron::find_last(b, e, needle);
        FUZZ_FAIL_IF(chk && gotl != ref::naive_find_last(b, n, needle), "find_last disagrees with oracle");

        if ( got ) {
          FUZZ_FAIL_IF(chk && !(*got == needle), "find returned a non-matching element");
          FUZZ_FAIL_IF(chk && gotl == nullptr, "find hit but find_last missed");
          FUZZ_FAIL_IF(chk && gotl < got, "find_last precedes find");
          FUZZ_FAIL_IF(chk && !(*gotl == needle), "find_last returned a non-matching element");
        }

        // -- count ----------------------------------------------------------
        const umax_t c = micron::count(b, e, needle);
        FUZZ_FAIL_IF(chk && c != ref::naive_count_eq(b, n, needle), "count disagrees with oracle");
        FUZZ_FAIL_IF(chk && (c > 0) != (want != nullptr), "count and find disagree on presence");
        FUZZ_FAIL_IF(chk && c > n, "count exceeds the range length");

        // -- quantifiers over a value ---------------------------------------
        const bool all_v = micron::all_of(b, e, needle);
        const bool any_v = micron::any_of(b, e, needle);
        FUZZ_FAIL_IF(chk && all_v != ref::naive_all_of_eq(b, n, needle), "all_of[value] disagrees with oracle");
        FUZZ_FAIL_IF(chk && any_v != ref::naive_any_of_eq(b, n, needle), "any_of[value] disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::none_of(b, e, needle) != !any_v, "none_of[value] is not any_of negated");
        FUZZ_FAIL_IF(chk && n && all_v && !any_v, "all_of true but any_of false on a non-empty range");
        FUZZ_FAIL_IF(chk && all_v && c != n, "all_of true but count != length");

        // -- quantifiers over a predicate -----------------------------------
        FUZZ_FAIL_IF(chk && micron::all_of(b, e, v_pos) != ref::naive_all_of_if(b, n, v_pos), "all_of[value-pred] disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::any_of(b, e, v_pos) != ref::naive_any_of_if(b, n, v_pos), "any_of[value-pred] disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::all_of(b, e, p_pos) != ref::naive_all_of_if(b, n, v_pos), "all_of[ptr-pred] disagrees with the value form");
        FUZZ_FAIL_IF(chk && micron::any_of(b, e, p_pos) != ref::naive_any_of_if(b, n, v_pos), "any_of[ptr-pred] disagrees with the value form");
        FUZZ_FAIL_IF(chk && micron::none_of(b, e, p_pos) != !ref::naive_any_of_if(b, n, v_pos), "none_of[ptr-pred] disagrees with the value form");

        // -- find_if / find_if_not, both predicate flavours ------------------
        const T *wif = ref::naive_find_if(b, n, v_pos);
        FUZZ_FAIL_IF(chk && micron::find_if(b, e, v_pos) != wif, "find_if[value-pred] disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::find_if(b, e, p_pos) != wif, "find_if[ptr-pred] disagrees with the value form");

        const T *win = fref::naive_find_if_not(b, n, v_pos);
        FUZZ_FAIL_IF(chk && micron::find_if_not(b, e, v_pos) != win, "find_if_not[value-pred] disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::find_if_not(b, e, p_pos) != win, "find_if_not[ptr-pred] disagrees with the value form");

        // find_if and find_if_not partition the range: element 0 belongs to exactly one
        if ( n ) FUZZ_FAIL_IF(chk && (wif == b) == (win == b), "find_if and find_if_not both claim (or both disclaim) element 0");

        // -- find_last_if / find_last_if_not --------------------------------
        FUZZ_FAIL_IF(chk && micron::find_last_if(b, e, p_pos) != fref::naive_find_last_if(b, n, v_pos), "find_last_if disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::find_last_if_not(b, e, p_pos) != fref::naive_find_last_if_not(b, n, v_pos),
                     "find_last_if_not disagrees with oracle");

        // -- count_if -------------------------------------------------------
        const umax_t cif = micron::count_if(b, e, v_pos);
        FUZZ_FAIL_IF(chk && cif != ref::naive_count_if(b, n, v_pos), "count_if[value-pred] disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::count_if(b, e, p_pos) != cif, "count_if[ptr-pred] disagrees with the value form");
        // a predicate and its negation must partition the range
        FUZZ_FAIL_IF(chk && cif + micron::count_if(b, e, [](const T *x) { return !(*x > T(0)); }) != n,
                     "count_if over a predicate and its negation does not sum to the length");

        // -- adjacent_find ---------------------------------------------------
        const T *ga = micron::adjacent_find(b, e);
        FUZZ_FAIL_IF(chk && ga != ref::naive_adjacent_find(b, n), "adjacent_find disagrees with oracle");
        FUZZ_FAIL_IF(chk && micron::adjacent_find(b, e, p_eq) != fref::naive_adjacent_find_if(b, n, p_eq),
                     "adjacent_find[Fn] disagrees with oracle");
        if ( ga ) FUZZ_FAIL_IF(chk && !(ga[0] == ga[1]), "adjacent_find hit is not an adjacent equal pair");
      },
      seed);
}

// ─────────────────────────────────────────────────────────────────────────
// two-range: mismatch / equal
// ─────────────────────────────────────────────────────────────────────────
template<typename T>
void
pair_suite(const char *tag, u64 seed)
{
  sweep2<T, MAXN>(
      tag,
      [](T *a, T *b, usize n, prng &) {
        const bool chk = mtest::fuzz::cmp_diff_ok(a, n) && mtest::fuzz::cmp_diff_ok(b, n);
        const usize widx = ref::naive_mismatch_idx(a, b, n);
        auto m = micron::mismatch(static_cast<const T *>(a), a + n, static_cast<const T *>(b));
        FUZZ_FAIL_IF(chk && m.a != a + widx, "mismatch first pointer disagrees with oracle");
        FUZZ_FAIL_IF(chk && m.b != b + widx, "mismatch second pointer disagrees with oracle");

        const bool eq = micron::equal(static_cast<const T *>(a), a + n, static_cast<const T *>(b));
        FUZZ_FAIL_IF(chk && eq != ref::naive_equal(a, b, n), "equal disagrees with oracle");
        FUZZ_FAIL_IF(chk && eq != (widx == n), "equal and mismatch disagree");

        // Reflexivity holds only WITHOUT a NaN: equal(x, x) is legitimately false for a range
        // containing one, because NaN != NaN. Symmetry holds either way.
        // reflexivity fails for a NaN-bearing range by IEEE, independently of the build
        if ( !mtest::fuzz::has_nan(a, n) )
          FUZZ_FAIL_IF(chk && !micron::equal(static_cast<const T *>(a), a + n, static_cast<const T *>(a)), "equal is not reflexive");
        FUZZ_FAIL_IF(chk && micron::equal(static_cast<const T *>(b), b + n, static_cast<const T *>(a)) != eq, "equal is not symmetric");

        // the 4-pointer form additionally compares lengths
        FUZZ_FAIL_IF(chk && micron::equal(static_cast<const T *>(a), a + n, static_cast<const T *>(b), b + n) != eq,
                     "equal[4-ptr] disagrees with the 3-ptr form at equal length");
        if ( n ) FUZZ_FAIL_IF(chk && micron::equal(static_cast<const T *>(a), a + n, static_cast<const T *>(b), b + n - 1),
                              "equal[4-ptr] ignored a length difference");
      },
      seed);
}

// ─────────────────────────────────────────────────────────────────────────
// subsequence family
// ─────────────────────────────────────────────────────────────────────────
template<typename T>
void
subseq_suite(const char *tag, u64 seed)
{
  static T pat[600];
  sweep<T, MAXN>(
      tag,
      [](T *hay, usize n, prng &rng) {
        const T *h = hay;
        const T *he = hay + n;
        const bool chk = mtest::fuzz::cmp_diff_ok(h, n);
        // widths straddling kmp_min_width (8) and kmp_stack_max (256)
        static constexpr usize widths[] = { 0, 1, 2, 3, 7, 8, 9, 16, 33, 64, 255, 256, 257, 400 };
        const usize m = widths[rng.next() % (sizeof(widths) / sizeof(widths[0]))];

        for ( usize i = 0; i < m; ++i ) pat[i] = static_cast<T>(rng.next() & 3u);
        // plant the needle half the time, biased to both extremes as well as the interior
        if ( m && m <= n && (rng.next() & 1u) ) {
          const u64 pick = rng.next() % 3u;
          const usize at = (pick == 0) ? 0 : ((pick == 1) ? n - m : rng.next() % (n - m + 1));
          for ( usize i = 0; i < m; ++i ) hay[at + i] = pat[i];
        }
        const T *p = pat;
        const T *pe = pat + m;

        // -- search / find_end -----------------------------------------------
        // micron answers nullptr for an empty pattern over an EMPTY range (the old outer
        // loop had to step once before it could report), where the oracle answers `h`.
        const T *gs = micron::search(h, he, p, pe);
        const T *xs = (m == 0 && n == 0) ? nullptr : ref::naive_search(h, n, p, m);
        FUZZ_FAIL_IF(chk && gs != xs, "search disagrees with oracle");

        const T *ge = micron::find_end(h, he, p, pe);
        FUZZ_FAIL_IF(chk && ge != ref::naive_search_end(h, n, p, m), "find_end disagrees with oracle");

        if ( m && gs ) {
          for ( usize i = 0; i < m; ++i ) FUZZ_FAIL_IF(chk && !(gs[i] == p[i]), "search hit is not the pattern");
          FUZZ_FAIL_IF(chk && ge == nullptr, "search hit but find_end missed");
          FUZZ_FAIL_IF(chk && ge < gs, "find_end precedes search");
          for ( usize i = 0; i < m; ++i ) FUZZ_FAIL_IF(chk && !(ge[i] == p[i]), "find_end hit is not the pattern");
        }
        if ( m ) FUZZ_FAIL_IF(chk && (gs == nullptr) != (ge == nullptr), "search and find_end disagree on presence");

        // -- contains_subrange mirrors search ---------------------------------
        if ( m ) FUZZ_FAIL_IF(chk && micron::contains_subrange(h, he, p, pe) != (gs != nullptr), "contains_subrange disagrees with search");

        // -- starts_with / ends_with ------------------------------------------
        if ( m <= n ) {
          const bool sw = micron::starts_with(h, he, p, pe);
          const bool ew = micron::ends_with(h, he, p, pe);
          FUZZ_FAIL_IF(chk && sw != ref::naive_starts_with(h, n, p, m), "starts_with disagrees with oracle");
          FUZZ_FAIL_IF(chk && ew != ref::naive_ends_with(h, n, p, m), "ends_with disagrees with oracle");
          if ( m && sw ) FUZZ_FAIL_IF(chk && gs != h, "starts_with true but search did not answer position 0");
          if ( m && ew ) FUZZ_FAIL_IF(chk && ge != he - m, "ends_with true but find_end did not answer the tail");
        }

        // -- search_n ----------------------------------------------------------
        {
          const usize k = 1 + static_cast<usize>(rng.next() % 12u);
          const T v = static_cast<T>(rng.next() & 3u);
          const T *gn = micron::search_n(h, he, k, v);
          FUZZ_FAIL_IF(chk && gn != ref::naive_search_n(h, n, k, v), "search_n disagrees with oracle");
          if ( gn ) {
            for ( usize i = 0; i < k; ++i ) FUZZ_FAIL_IF(chk && !(gn[i] == v), "search_n hit is not a run of the value");
            // a run of k implies a run of every shorter length
            if ( k > 1 ) FUZZ_FAIL_IF(chk && micron::search_n(h, he, k - 1, v) == nullptr, "search_n found a run of k but not of k-1");
          }
        }

        // -- find_first_of ------------------------------------------------------
        {
          const usize k = static_cast<usize>(rng.next() % 9u);
          T set_[8];
          for ( usize i = 0; i < k; ++i ) set_[i] = static_cast<T>(rng.next() & 7u);
          const T *gf = micron::find_first_of(h, he, static_cast<const T *>(set_), set_ + k);
          FUZZ_FAIL_IF(chk && gf != ref::naive_find_first_of(h, n, static_cast<const T *>(set_), k), "find_first_of disagrees with oracle");
          if ( gf ) {
            bool in = false;
            for ( usize i = 0; i < k; ++i )
              if ( *gf == set_[i] ) in = true;
            FUZZ_FAIL_IF(chk && !in, "find_first_of hit is not a member of the set");
          }
          // a single-element set is exactly find
          if ( k >= 1 ) {
            T one[1] = { set_[0] };
            FUZZ_FAIL_IF(chk && micron::find_first_of(h, he, static_cast<const T *>(one), one + 1) != micron::find(h, he, set_[0]),
                         "find_first_of over a 1-element set disagrees with find");
          }
        }
      },
      seed, 2);
}

}      // namespace

int
main(void)
{
  sb::print("=== ALGO/SEARCH FUZZ SUITE ===");

  // every lane width the vector scan handles, signed and unsigned, plus a float type that
  // must stay on the scalar path
  scan_suite<u8>("scan u8", 0xF117D00711ULL);
  scan_suite<i8>("scan i8", 0xF117D00722ULL);
  scan_suite<u16>("scan u16", 0xF117D00733ULL);
  scan_suite<i16>("scan i16", 0xF117D00744ULL);
  scan_suite<u32>("scan u32", 0xF117D00755ULL);
  scan_suite<i32>("scan i32", 0xF117D00766ULL);
  scan_suite<u64>("scan u64", 0xF117D00777ULL);
  scan_suite<i64>("scan i64", 0xF117D00788ULL);
  scan_suite<f64>("scan f64 (must NOT take the lane path)", 0xF117D00799ULL);

  pair_suite<u8>("pair u8", 0xBEEF900101ULL);
  pair_suite<u16>("pair u16", 0xBEEF900102ULL);
  pair_suite<i32>("pair i32", 0xBEEF900103ULL);
  pair_suite<u64>("pair u64", 0xBEEF900104ULL);
  pair_suite<f64>("pair f64", 0xBEEF900105ULL);

  subseq_suite<u8>("subseq u8", 0x5EA2C411D0ULL);
  subseq_suite<u16>("subseq u16", 0x5EA2C411D1ULL);
  subseq_suite<i32>("subseq i32", 0x5EA2C411D2ULL);
  subseq_suite<u64>("subseq u64", 0x5EA2C411D3ULL);

  // ────────────────────────────────────────────────────────────────────
  // generator-driven properties over whole fuzzed containers
  // ────────────────────────────────────────────────────────────────────

  sbf::check_property(
      "find/count/contains agree on a fuzzed vector",
      [](micron::vector<i32> v, i32 needle) {
        const i32 *b = v.begin();
        const usize n = v.size();
        const i32 *f = micron::find(b, v.end(), needle);
        const umax_t c = micron::count(b, v.end(), needle);
        if ( f != ref::naive_find(b, n, needle) ) FUZZ_FAIL("find disagrees with oracle");
        if ( c != ref::naive_count_eq(b, n, needle) ) FUZZ_FAIL("count disagrees with oracle");
        if ( (f != nullptr) != (c > 0) ) FUZZ_FAIL("find and count disagree on presence");
      },
      { .seed = 0xA11CE0F0, .count = 20000 }, sbf::vector_of(sbf::spec<i32>{}).len(0, 64), sbf::special_ints<i32>());

  sbf::check_property(
      "search is consistent with starts_with over fuzzed vectors",
      [](micron::vector<u8> hay, micron::vector<u8> pat) {
        const u8 *h = hay.begin();
        const u8 *p = pat.begin();
        const usize n = hay.size(), m = pat.size();
        const u8 *hit = micron::search(h, hay.end(), p, pat.end());
        const u8 *expect = (m == 0 && n == 0) ? nullptr : ref::naive_search(h, n, p, m);
        if ( hit != expect ) FUZZ_FAIL("search disagrees with oracle on fuzzed vectors");
        if ( m && m <= n && ref::naive_starts_with(h, n, p, m) && hit != h ) FUZZ_FAIL("prefix match did not answer position 0");
      },
      { .seed = 0xC0DE5EA2, .count = 20000 }, sbf::vector_of(sbf::range<u8>(0, 3)).len(0, 96),
      sbf::vector_of(sbf::range<u8>(0, 3)).len(0, 12));

  sbf::check_property(
      "per-value counts partition the range",
      [](micron::vector<u8> v) {
        // summing count() over every value a byte can take must total exactly size()
        umax_t total = 0;
        for ( u32 b = 0; b < 256; ++b ) total += micron::count(v.begin(), v.end(), static_cast<u8>(b));
        if ( total != v.size() ) FUZZ_FAIL("per-value counts do not sum to the range length");
      },
      { .seed = 0x50117A27, .count = 3000 }, sbf::vector_of(sbf::spec<u8>{}).len(0, 128));

  sbf::check_property(
      "find_first_of over a union of two sets is the earlier of the two",
      [](micron::vector<u8> v, micron::vector<u8> s1, micron::vector<u8> s2) {
        const u8 *b = v.begin();
        const u8 *e = v.end();
        const u8 *h1 = micron::find_first_of(b, e, s1.begin(), s1.end());
        const u8 *h2 = micron::find_first_of(b, e, s2.begin(), s2.end());
        micron::vector<u8> u = s1;
        for ( usize i = 0; i < s2.size(); ++i ) u.push_back(s2[i]);
        const u8 *hu = micron::find_first_of(b, e, u.begin(), u.end());
        const u8 *want = (h1 == nullptr) ? h2 : ((h2 == nullptr) ? h1 : (h1 < h2 ? h1 : h2));
        if ( hu != want ) FUZZ_FAIL("find_first_of over a union is not the earlier of the parts");
      },
      { .seed = 0x0710077, .count = 8000 }, sbf::vector_of(sbf::range<u8>(0, 15)).len(0, 64),
      sbf::vector_of(sbf::range<u8>(0, 15)).len(0, 6), sbf::vector_of(sbf::range<u8>(0, 15)).len(0, 6));

  sb::print("=== ALGO/SEARCH FUZZ SUITE PASSED ===");
  return 1;
}
