//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// fuzz_algo_data — fuzzing algorithm/data.hpp (heaps, merge, rotate) and the typed block
// primitives in algorithm/memory.hpp.
//
// The heap family has no oracle worth writing -- a second heap implementation would just be a
// second chance to be wrong -- so it is tested by invariant instead:
//
//     make_heap                 => is_heap
//     make_heap ; sort_heap     => ascending, and a permutation of the input
//     push_heap after an append => is_heap still holds, incrementally
//     pop_heap                  => the max lands at the end, the prefix is still a heap
//     rotate / cycle_rotate     => same answer, and both are permutations
//     merge of two sorted runs  => sorted, and a permutation of the concatenation
//
// One documented surprise this pins: the CONTAINER overload of merge() does not merge. It is
// `out = a; out.insert(out.end(), b)`, i.e. plain concatenation, and is identical to concat().
// Only the five-iterator form actually merges. That is asserted below rather than assumed, so
// the day it changes a test says so.
//
// memory.hpp's _n family does NOT have one count convention, which is why this suite exercises
// most of it only at T = u8. simd::memcpy256 / memcmp256 / memset256 all compute
// `bytes = count * sizeof(T)` (ELEMENTS), while micron::memset and bytecmp take BYTES -- so
// zero_n, set_n, compare_n and equal_n mean ELEMENTS when the count divides by 16 or 32 and BYTES
// otherwise. The meaning of the argument depends on its value. See ISSUES.md; it is invisible
// today because callers pass byte pointers, where the two coincide.
//
// At T = u8 the conventions are identical, so the whole family is covered there. For wider T only
// copy_n (elements throughout) and swap_n (bytes throughout) are self-consistent, so only those
// two are asserted.

#include "../snowball/snowball_fuzz.hpp"
#include "../support/algo_fuzz.hpp"

#include "../../src/algorithm/data.hpp"
#include "../../src/algorithm/memory.hpp"
#include "../../src/sort/sort.hpp"

namespace sb = snowball;
namespace sbf = snowball::fuzzing;

using mtest::fuzz::sweep;
using mtest::prng;
namespace fref = mtest::fuzz::fref;

namespace
{

constexpr usize MAXN = 512;

template<typename T>
micron::vector<T>
as_vec(const T *p, usize n)
{
  micron::vector<T> v;
  v.resize(n);
  for ( usize i = 0; i < n; ++i ) v[i] = p[i];
  return v;
}

// ─────────────────────────────────────────────────────────────────────────
// heap invariants
// ─────────────────────────────────────────────────────────────────────────
template<typename T>
void
heap_suite(const char *tag, u64 seed)
{
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &) {
        // -- make_heap establishes the invariant ------------------------------
        micron::vector<T> h = as_vec(buf, n);
        micron::make_heap(h);
        FUZZ_FAIL_IF(!micron::is_heap(h), "make_heap did not produce a heap");
        FUZZ_FAIL_IF(!fref::naive_is_permutation(h.begin(), h.size(), buf, n), "make_heap is not a permutation");
        // the root of a max-heap is the maximum
        if ( n ) FUZZ_FAIL_IF(!(h[0] == mtest::rigor::ref::naive_max(buf, n)), "the heap root is not the maximum");

        // -- make_heap ; sort_heap is an ascending sort -----------------------
        micron::vector<T> s = as_vec(buf, n);
        micron::make_heap(s);
        micron::sort_heap(s);
        for ( usize i = 1; i < n; ++i ) FUZZ_FAIL_IF(s[i] < s[i - 1], "sort_heap did not produce ascending order");
        FUZZ_FAIL_IF(!fref::naive_is_permutation(s.begin(), s.size(), buf, n), "sort_heap is not a permutation");

        // -- pop_heap moves the max to the end, prefix stays a heap ----------
        if ( n >= 2 ) {
          micron::vector<T> p = as_vec(buf, n);
          micron::make_heap(p);
          const T root = p[0];
          micron::pop_heap(p);
          FUZZ_FAIL_IF(!(p[n - 1] == root), "pop_heap did not move the root to the end");
          // the shrunk prefix must still satisfy the heap property
          for ( usize i = 1; i + 1 < n; ++i ) FUZZ_FAIL_IF(p[(i - 1) >> 1] < p[i], "pop_heap left the prefix non-heap");
        }

        // -- push_heap maintains the invariant incrementally ------------------
        {
          micron::vector<T> g;
          for ( usize i = 0; i < n && i < 64; ++i ) {
            g.push_back(buf[i]);
            micron::push_heap(g);
            FUZZ_FAIL_IF(!micron::is_heap(g), "push_heap broke the heap invariant");
          }
          // and the root is still the max of everything pushed
          if ( n ) {
            const usize k = (n < 64) ? n : 64;
            FUZZ_FAIL_IF(!(g[0] == mtest::rigor::ref::naive_max(buf, k)), "the incremental heap root is not the maximum");
          }
        }

        // -- is_heap rejects a deliberately broken heap ----------------------
        // a fuzz suite that only ever feeds VALID input never learns whether the predicate can
        // say no, so break the invariant on purpose
        if ( n >= 2 ) {
          micron::vector<T> bad = as_vec(buf, n);
          micron::make_heap(bad);
          const T mx = mtest::rigor::ref::naive_max(buf, n);
          const T mn = mtest::rigor::ref::naive_min(buf, n);
          if ( mn < mx ) {
            bad[0] = mn;      // a root smaller than a child cannot be a max-heap
            bool any_greater = false;
            for ( usize i = 1; i < n; ++i )
              if ( bad[0] < bad[i] ) any_greater = true;
            if ( any_greater ) FUZZ_FAIL_IF(micron::is_heap(bad), "is_heap accepted a broken heap");
          }
        }
      },
      seed, 2);
}

// ─────────────────────────────────────────────────────────────────────────
// rotate / cycle_rotate / merge / concat
// ─────────────────────────────────────────────────────────────────────────
template<typename T>
void
reshape_suite(const char *tag, u64 seed)
{
  static T ra[MAXN];
  static T rb[MAXN];
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &rng) {
        const usize k = n ? static_cast<usize>(rng.next() % n) : 0;

        // -- rotate(first, n_first, last) puts n_first at the front -----------
        for ( usize i = 0; i < n; ++i ) ra[i] = buf[i];
        for ( usize i = 0; i < n; ++i ) rb[i] = buf[i];
        micron::rotate(ra, ra + k, ra + n);
        micron::cycle_rotate(rb, rb + k, rb + n);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(ra, rb, n), "rotate and cycle_rotate disagree");
        // the element that was at k is now at the front
        if ( n && k ) FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&ra[0], &buf[k], 1), "rotate did not bring n_first to the front");
        FUZZ_FAIL_IF(!mtest::fuzz::has_nan(buf, n) && !fref::naive_is_permutation(ra, n, buf, n), "rotate is not a permutation");

        // -- merge over two SORTED runs is sorted and covers both -------------
        {
          const usize half = n / 2;
          micron::vector<T> a = as_vec(buf, half);
          micron::vector<T> b = as_vec(buf + half, n - half);
          if ( !mtest::fuzz::has_nan(buf, n) ) {
            micron::sort::sort(a);
            micron::sort::sort(b);
            micron::vector<T> out;
            out.resize(n);
            micron::merge(a.begin(), a.end(), b.begin(), b.end(), out.begin());
            for ( usize i = 1; i < n; ++i ) FUZZ_FAIL_IF(out[i] < out[i - 1], "merge of two sorted runs is not sorted");
            FUZZ_FAIL_IF(!fref::naive_is_permutation(out.begin(), n, buf, n), "merge lost or invented elements");
          }
        }

        // -- the CONTAINER merge is concatenation, not a merge ----------------
        {
          micron::vector<T> a = as_vec(buf, n);
          micron::vector<T> b = as_vec(buf, n);
          auto m = micron::merge(a, b);
          auto c = micron::concat(a, b);
          FUZZ_FAIL_IF(m.size() != 2 * n, "container merge did not produce both inputs");
          FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(m.begin(), c.begin(), 2 * n), "container merge and concat differ");
          for ( usize i = 0; i < n; ++i ) {
            FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&m[i], &buf[i], 1), "container merge reordered the first input");
            FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(&m[n + i], &buf[i], 1), "container merge reordered the second input");
          }
        }
      },
      seed, 2);
}

// ─────────────────────────────────────────────────────────────────────────
// typed block primitives from algorithm/memory.hpp
//
// Counts are in ELEMENTS of the pointee type. Every buffer here is MAXN wide and every count is
// bounded by n <= MAXN, so nothing can walk off the end.
// ─────────────────────────────────────────────────────────────────────────
template<typename T>
void
block_suite(const char *tag, u64 seed)
{
  static T dst[MAXN];
  static T other[MAXN];
  sweep<T, MAXN>(
      tag,
      [](T *buf, usize n, prng &rng) {
        if ( n == 0 ) return;      // these primitives all reject a null/empty request

        // -- copy_n: ELEMENTS on every branch, so safe for any T --------------
        for ( usize i = 0; i < n; ++i ) dst[i] = T{};
        micron::copy_n(static_cast<const T *>(buf), dst, n);
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(dst, buf, n), "copy_n did not reproduce the source");

        // -- swap_n: BYTES on every branch, and its own inverse ---------------
        for ( usize i = 0; i < n; ++i ) {
          dst[i] = buf[i];
          other[i] = static_cast<T>(static_cast<u64>(i) * 7u + 1u);
        }
        static T save_a[MAXN];
        static T save_b[MAXN];
        for ( usize i = 0; i < n; ++i ) {
          save_a[i] = dst[i];
          save_b[i] = other[i];
        }
        micron::swap_n(dst, other, n * sizeof(T));
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(dst, save_b, n), "swap_n did not bring the second block over");
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(other, save_a, n), "swap_n did not bring the first block over");
        micron::swap_n(dst, other, n * sizeof(T));
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(dst, save_a, n), "swap_n is not its own inverse");
        FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(other, save_b, n), "swap_n is not its own inverse");

        // -- the rest of the family, only where ELEMENTS == BYTES -------------
        if constexpr ( sizeof(T) == 1 ) {
          // zero_n / set_n
          for ( usize i = 0; i < n; ++i ) dst[i] = buf[i];
          micron::zero_n(dst, n);
          for ( usize i = 0; i < n; ++i ) FUZZ_FAIL_IF(dst[i] != T{}, "zero_n left a nonzero element");

          micron::set_n(dst, static_cast<byte>(0xAB), n);
          for ( usize i = 0; i < n; ++i )
            FUZZ_FAIL_IF(reinterpret_cast<const byte *>(dst)[i] != static_cast<byte>(0xAB), "set_n left a byte unwritten");

          // compare_n / equal_n, including a single altered element
          for ( usize i = 0; i < n; ++i ) dst[i] = buf[i];
          FUZZ_FAIL_IF(!micron::equal_n(static_cast<const T *>(buf), static_cast<const T *>(dst), n),
                       "equal_n says an exact copy differs");
          FUZZ_FAIL_IF(micron::compare_n(static_cast<const T *>(buf), static_cast<const T *>(dst), n) != 0,
                       "compare_n is nonzero for an exact copy");
          {
            const usize at = static_cast<usize>(rng.next() % n);
            dst[at] = static_cast<T>(buf[at] ^ static_cast<T>(0xFF));
            if ( dst[at] != buf[at] )
              FUZZ_FAIL_IF(micron::equal_n(static_cast<const T *>(buf), static_cast<const T *>(dst), n),
                           "equal_n missed an altered element");
          }

          // move_n copies the payload; its source-clear half is NOT asserted, because it
          // memcpy's cnt elements and then bytesets cnt bytes (see ISSUES.md)
          {
            static T src_[MAXN];
            for ( usize i = 0; i < n; ++i ) {
              src_[i] = buf[i];
              dst[i] = T{};
            }
            micron::move_n(src_, dst, n);
            FUZZ_FAIL_IF(!mtest::fuzz::bits_equal(dst, buf, n), "move_n did not reproduce the source payload");
          }
        }
      },
      seed, 2);
}

}      // namespace

int
main(void)
{
  sb::print("=== ALGO/DATA FUZZ SUITE ===");

  heap_suite<u8>("heap u8", 0x4EA90001ULL);
  heap_suite<i32>("heap i32", 0x4EA90002ULL);
  heap_suite<u64>("heap u64", 0x4EA90003ULL);

  reshape_suite<u8>("reshape u8", 0x201A7E01ULL);
  reshape_suite<i32>("reshape i32", 0x201A7E02ULL);
  reshape_suite<u64>("reshape u64", 0x201A7E03ULL);
  reshape_suite<f64>("reshape f64", 0x201A7E04ULL);

  block_suite<u8>("block u8", 0xB10C4001ULL);
  block_suite<u16>("block u16", 0xB10C4002ULL);
  block_suite<i32>("block i32", 0xB10C4003ULL);
  block_suite<u64>("block u64", 0xB10C4004ULL);

  // ────────────────────────────────────────────────────────────────────
  // generator-driven laws
  // ────────────────────────────────────────────────────────────────────

  sbf::check_property(
      "make_heap ; sort_heap sorts and preserves the multiset",
      [](micron::vector<i32> v) {
        micron::vector<i32> orig = v;
        micron::make_heap(v);
        if ( !micron::is_heap(v) ) FUZZ_FAIL("make_heap did not produce a heap");
        micron::sort_heap(v);
        for ( usize i = 1; i < v.size(); ++i )
          if ( v[i] < v[i - 1] ) FUZZ_FAIL("sort_heap did not sort ascending");
        if ( !fref::naive_is_permutation(v.begin(), v.size(), orig.begin(), orig.size()) )
          FUZZ_FAIL("heapsort is not a permutation");
      },
      { .seed = 0x4EA95024, .count = 20000 }, sbf::vector_of(sbf::spec<i32>{}).len(0, 64));

  sbf::check_property(
      "rotate by k then by n-k is the identity",
      [](micron::vector<i32> v, u32 raw_k) {
        if ( v.size() == 0 ) return;
        micron::vector<i32> orig = v;
        const usize n = v.size();
        const usize k = static_cast<usize>(raw_k % n);
        micron::rotate(v.begin(), v.begin() + k, v.end());
        micron::rotate(v.begin(), v.begin() + (n - k) % n, v.end());
        for ( usize i = 0; i < n; ++i )
          if ( v[i] != orig[i] ) FUZZ_FAIL("rotate by k then n-k is not the identity");
      },
      { .seed = 0x207A7E10, .count = 20000 }, sbf::vector_of(sbf::spec<i32>{}).len(1, 64), sbf::spec<u32>{});

  sb::print("=== ALGO/DATA FUZZ SUITE PASSED ===");
  return 1;
}
