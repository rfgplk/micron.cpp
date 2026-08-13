//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// algo_fuzz.hpp — the shared kit behind tests/rigor/fuzz_algo_*.cpp.
//
// algo_rigor.hpp already supplies naive oracles, 13 pattern generators and the
// adversarial size table. This adds the three things a FUZZ suite needs on top of that:
//
//   * a shape corpus -- one enum over every input distribution worth trying, so a body
//     is driven over all of them instead of the single pat_random that prop_buffer_size
//     uses. The shapes that matter most are the ones a naive loop handles by brute force
//     and a clever one can get wrong: two_valued and periodic (partial matches everywhere,
//     which is what a KMP failure table is for), run_heavy (search_n), and the saturated
//     boundary fills.
//   * sweep() -- shapes x adversarial sizes x trials, so every body sees 0, 1, and each
//     side of every SIMD width under every distribution.
//   * oracles for the fp:: tier and the few eager entry points ref:: does not cover.
//
// Failure convention matches tests/rigor/rigor_snowball_fuzz.cpp: throw a const char*.
// snowball::fuzzing::check_property catches it and prints the seed and iteration, which is
// what makes a fuzz failure reproducible. FUZZ_FAIL_IF adds the entry point's own name.

#include "algo_rigor.hpp"

#include "../../src/algorithm/fp.hpp"
#include "../../src/vector/fvector.hpp"

namespace mtest::fuzz
{

using mtest::rigor::kAdversarialSizes;
using mtest::rigor::kAdversarialSizesCount;
using mtest::prng;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// failure reporting

#define FUZZ_FAIL(msg) throw("fuzz: " msg)
#define FUZZ_FAIL_IF(cond, msg)                                                                                                            \
  do {                                                                                                                                     \
    if ( cond ) throw("fuzz: " msg);                                                                                                       \
  } while ( 0 )

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// input shapes

enum class shape : u8 {
  random,             // full-width random
  random_small,       // small alphabet, so hits are dense
  two_valued,         // {0,1} only -- every window partially matches
  periodic,           // a short cell repeated; the KMP worst case
  run_heavy,          // long runs of one value; the search_n worst case
  sorted,             //
  reverse_sorted,     //
  all_equal,          //
  alternating,        //
  sawtooth,           //
  zeros,              //
  single_spike,       // exactly one distinguished element
  near_max,           // saturated at the type's ceiling
  near_min,           // saturated at the type's floor
  with_nan,           // float only
  with_inf,           // float only
  signed_zero,        // float only: +0 and -0 interleaved, which compare equal but differ in bits
};

inline constexpr shape kShapes[] = { shape::random,     shape::random_small, shape::two_valued,   shape::periodic,
                                    shape::run_heavy,  shape::sorted,       shape::reverse_sorted, shape::all_equal,
                                    shape::alternating, shape::sawtooth,    shape::zeros,        shape::single_spike,
                                    shape::near_max,   shape::near_min,     shape::with_nan,     shape::with_inf,
                                    shape::signed_zero };
inline constexpr usize kShapeCount = sizeof(kShapes) / sizeof(kShapes[0]);

inline const char *
shape_name(shape s) noexcept
{
  switch ( s ) {
  case shape::random:
    return "random";
  case shape::random_small:
    return "random_small";
  case shape::two_valued:
    return "two_valued";
  case shape::periodic:
    return "periodic";
  case shape::run_heavy:
    return "run_heavy";
  case shape::sorted:
    return "sorted";
  case shape::reverse_sorted:
    return "reverse_sorted";
  case shape::all_equal:
    return "all_equal";
  case shape::alternating:
    return "alternating";
  case shape::sawtooth:
    return "sawtooth";
  case shape::zeros:
    return "zeros";
  case shape::single_spike:
    return "single_spike";
  case shape::near_max:
    return "near_max";
  case shape::near_min:
    return "near_min";
  case shape::with_nan:
    return "with_nan";
  case shape::with_inf:
    return "with_inf";
  case shape::signed_zero:
    return "signed_zero";
  }
  return "?";
}

// Does the range hold a NaN?
//
// Read as BITS, deliberately -- `x != x` is exactly the test -ffinite-math-only is allowed to
// fold to false, and micron/math/ieee.hpp classifies on bits for the same reason. A caller
// needs this because several perfectly reasonable-looking identities stop holding once a NaN
// is present: equal(x, x) is FALSE for a range containing one, since NaN != NaN, and that is
// correct IEEE behaviour rather than a bug to assert against.
template<typename T>
inline bool
has_nan(const T *p, usize n) noexcept
{
  if constexpr ( micron::is_same_v<micron::remove_cv_t<T>, f64> ) {
    for ( usize i = 0; i < n; ++i ) {
      const u64 b = __builtin_bit_cast(u64, p[i]);
      if ( (b & 0x7ff0000000000000ull) == 0x7ff0000000000000ull && (b & 0x000fffffffffffffull) != 0 ) return true;
    }
    return false;
  } else if constexpr ( micron::is_same_v<micron::remove_cv_t<T>, f32> ) {
    for ( usize i = 0; i < n; ++i ) {
      const u32 b = __builtin_bit_cast(u32, p[i]);
      if ( (b & 0x7f800000u) == 0x7f800000u && (b & 0x007fffffu) != 0 ) return true;
    }
    return false;
  } else {
    (void)p;
    (void)n;
    return false;
  }
}

// Bytewise equality of two element ranges.
//
// NOT micron::memcmp: that is value-based for any element wider than a byte (its scalar arm is
// `if (src[i] != dest[i])`), so two bit-identical NaN buffers compare UNEQUAL through it. When a
// test wants "did these bits survive the round trip" -- reverse being its own inverse, rotate and
// cycle_rotate agreeing, swap_n undoing itself -- it wants this, which is also stricter about
// +0.0 vs -0.0 and therefore the right tool for an involution check.
template<typename T>
inline bool
bits_equal(const T *a, const T *b, usize n) noexcept
{
  return __builtin_memcmp(a, b, n * sizeof(T)) == 0;
}

// Are value-comparison diffs meaningful for this range?
//
// duck defaults to -Ofast, whose -ffinite-math-only tells gcc that NaN cannot occur. Two
// inlinings of the same comparison may then legally disagree -- measured here, micron::contains
// and the micron::find it is literally defined as gave different answers for a NaN needle.
// Nothing is wrong with micron: the build asked for that licence. But it does mean a NaN-bearing
// range cannot be differentially tested in such a build.
//
// So the algorithms are still CALLED on NaN input either way (crash and UB coverage is
// unaffected), and only the comparison assertions are withheld. tests/rigor/fuzz_algo.duck
// builds one cell at -O2, where __FINITE_MATH_ONLY__ is 0 and the full NaN contract IS asserted
// -- that is the cell which proves the f64 scan is not a bitwise lane compare.
//
// A `#pragma GCC optimize("no-finite-math-only")` does NOT work here; it is applied too late to
// undo a TU-wide -ffast-math, which is why this is a gate rather than an override.
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
inline constexpr bool kIeeeNanSemantics = false;
#else
inline constexpr bool kIeeeNanSemantics = true;
#endif

template<typename T>
inline bool
cmp_diff_ok(const T *p, usize n) noexcept
{
  return kIeeeNanSemantics || !has_nan(p, n);
}

// the three float-only shapes are skipped for integral T rather than silently producing
// something else, so a sweep over ints does not pretend to have covered NaN
template<typename T>
inline constexpr bool
shape_applies(shape s) noexcept
{
  if constexpr ( micron::is_floating_point_v<T> ) {
    return true;
  } else {
    return s != shape::with_nan && s != shape::with_inf && s != shape::signed_zero;
  }
}

template<typename T>
inline void
fill_shape(T *out, usize n, shape s, prng &rng) noexcept
{
  using namespace mtest::rigor;
  switch ( s ) {
  case shape::random:
    pat_random(out, n, rng);
    return;
  case shape::random_small:
    pat_random_small(out, n, rng, T(0), T(7));
    return;
  case shape::two_valued:
    for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(rng.next() & 1u);
    return;
  case shape::periodic: {
    // period is coprime-ish with the vector widths so the repeat does not land on a lane
    // boundary every time
    const usize period = 1 + static_cast<usize>(rng.next() % 7u);
    for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(i % period);
    return;
  }
  case shape::run_heavy: {
    usize i = 0;
    T v = static_cast<T>(rng.next() & 3u);
    while ( i < n ) {
      usize run = 1 + static_cast<usize>(rng.next() % 9u);
      for ( usize k = 0; k < run && i < n; ++k, ++i ) out[i] = v;
      v = static_cast<T>(rng.next() & 3u);
    }
    return;
  }
  case shape::sorted:
    pat_sorted(out, n);
    return;
  case shape::reverse_sorted:
    pat_reverse_sorted(out, n);
    return;
  case shape::all_equal:
    pat_all_equal(out, n, static_cast<T>(rng.next() & 7u));
    return;
  case shape::alternating:
    pat_alternating(out, n);
    return;
  case shape::sawtooth:
    pat_sawtooth(out, n, 1 + static_cast<usize>(rng.next() % 16u));
    return;
  case shape::zeros:
    pat_zeros(out, n);
    return;
  case shape::single_spike:
    pat_single_spike(out, n, n ? static_cast<usize>(rng.next() % n) : 0, static_cast<T>(99));
    return;
  case shape::near_max:
    pat_near_overflow_max(out, n);
    return;
  case shape::near_min:
    pat_near_overflow_min(out, n);
    return;
  case shape::with_nan:
    if constexpr ( micron::is_floating_point_v<T> ) pat_with_nan(out, n);
    return;
  case shape::with_inf:
    if constexpr ( micron::is_floating_point_v<T> ) pat_with_inf(out, n);
    return;
  case shape::signed_zero:
    if constexpr ( micron::is_floating_point_v<T> ) pat_signed_zero(out, n);
    return;
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sweep driver
//
// body(buf, n, rng) is called once per (shape, size, trial). `buf` is scratch -- it is
// refilled before every call, so a body is free to mutate it. Sizes come from
// kAdversarialSizes, which straddles every SIMD width and both ends of the KMP bounds.

// A body signals failure by throwing (see FUZZ_FAIL). Catching it HERE rather than letting it
// reach check_property's handler is the whole point: the shape and the length are what make a
// range-algorithm failure reproducible, and only the driver knows them.
[[noreturn]] inline void
__report(const char *name, const char *what, shape sh, usize n, usize trial, u64 seed)
{
  micron::io::print("\033[34msnowball fuzz failure:\033[0m ");
  micron::io::print(what);
  micron::io::print("\n\r  in: ");
  micron::io::print(name);
  micron::io::print("\n\r  shape=");
  micron::io::print(shape_name(sh));
  micron::io::print(" n=");
  micron::io::print(static_cast<u64>(n));
  micron::io::print(" trial=");
  micron::io::print(static_cast<u64>(trial));
  micron::io::print(" seed=");
  micron::io::print(seed);
  micron::io::print("\n\r");
  snowball::should_print_stack();
  micron::abort(6);
}

template<typename T, usize MaxN, typename Body>
inline void
sweep(const char *name, Body &&body, u64 seed, usize trials = 3)
{
  snowball::test_case(name);
  prng rng(seed);
  static T buf[MaxN];
  for ( usize si = 0; si < kShapeCount; ++si ) {
    const shape sh = kShapes[si];
    if ( !shape_applies<T>(sh) ) continue;
    for ( usize zi = 0; zi < kAdversarialSizesCount; ++zi ) {
      const usize n = kAdversarialSizes[zi];
      if ( n > MaxN ) continue;
      for ( usize t = 0; t < trials; ++t ) {
        fill_shape(buf, n, sh, rng);
#if defined(__cpp_exceptions)
        try {
          body(buf, n, rng);
        } catch ( const char *what ) {
          __report(name, what, sh, n, t, seed);
        }
#else
        body(buf, n, rng);
#endif
      }
    }
  }
  snowball::end_test_case();
}

// two independent buffers of the same length, for the binary entry points
template<typename T, usize MaxN, typename Body>
inline void
sweep2(const char *name, Body &&body, u64 seed, usize trials = 3)
{
  snowball::test_case(name);
  prng rng(seed);
  static T a[MaxN];
  static T b[MaxN];
  for ( usize si = 0; si < kShapeCount; ++si ) {
    const shape sh = kShapes[si];
    if ( !shape_applies<T>(sh) ) continue;
    for ( usize zi = 0; zi < kAdversarialSizesCount; ++zi ) {
      const usize n = kAdversarialSizes[zi];
      if ( n > MaxN ) continue;
      for ( usize t = 0; t < trials; ++t ) {
        fill_shape(a, n, sh, rng);
        // half the time b is a copy, so the all-equal case is hit often instead of
        // essentially never
        if ( rng.next() & 1u ) {
          for ( usize i = 0; i < n; ++i ) b[i] = a[i];
          if ( n && (rng.next() & 1u) ) b[rng.next() % n] = static_cast<T>(rng.next());
        } else {
          fill_shape(b, n, kShapes[rng.next() % kShapeCount], rng);
        }
#if defined(__cpp_exceptions)
        try {
          body(a, b, n, rng);
        } catch ( const char *what ) {
          __report(name, what, sh, n, t, seed);
        }
#else
        body(a, b, n, rng);
#endif
      }
    }
  }
  snowball::end_test_case();
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// oracles
//
// Deliberately the dumbest possible implementations. `ref` in algo_rigor.hpp covers the
// eager entry points; this covers what it does not, plus the whole fp:: tier.

namespace fref
{

template<typename T, typename Pred>
inline const T *
naive_find_if_not(const T *p, usize n, Pred pred) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !pred(p[i]) ) return p + i;
  return nullptr;
}

template<typename T, typename Pred>
inline const T *
naive_find_last_if(const T *p, usize n, Pred pred) noexcept
{
  const T *hit = nullptr;
  for ( usize i = 0; i < n; ++i )
    if ( pred(p[i]) ) hit = p + i;
  return hit;
}

template<typename T, typename Pred>
inline const T *
naive_find_last_if_not(const T *p, usize n, Pred pred) noexcept
{
  const T *hit = nullptr;
  for ( usize i = 0; i < n; ++i )
    if ( !pred(p[i]) ) hit = p + i;
  return hit;
}

template<typename T, typename Fn>
inline const T *
naive_adjacent_find_if(const T *p, usize n, Fn eq) noexcept
{
  if ( n == 0 ) return nullptr;
  for ( usize i = 0; i + 1 < n; ++i )
    if ( eq(p + i, p + i + 1) ) return p + i;
  return nullptr;
}

// keep the FIRST occurrence of each distinct value, original order
template<typename T>
inline micron::fvector<T>
naive_nub(const T *p, usize n)
{
  micron::fvector<T> out;
  for ( usize i = 0; i < n; ++i ) {
    bool seen = false;
    for ( usize j = 0; j < out.size(); ++j )
      if ( out[j] == p[i] ) {
        seen = true;
        break;
      }
    if ( !seen ) out.push_back(p[i]);
  }
  return out;
}

// collapse consecutive duplicates only
template<typename T>
inline micron::fvector<T>
naive_unique(const T *p, usize n)
{
  micron::fvector<T> out;
  for ( usize i = 0; i < n; ++i )
    if ( i == 0 || !(p[i] == p[i - 1]) ) out.push_back(p[i]);
  return out;
}

template<typename T, typename Pred>
inline usize
naive_take_while(const T *p, usize n, Pred pred) noexcept
{
  usize i = 0;
  while ( i < n && pred(p[i]) ) ++i;
  return i;
}

template<typename T, typename Acc, typename Fn>
inline micron::fvector<Acc>
naive_scanl(const T *p, usize n, Acc init, Fn fn)
{
  micron::fvector<Acc> out;
  out.push_back(init);
  Acc a = init;
  for ( usize i = 0; i < n; ++i ) {
    a = fn(a, p[i]);
    out.push_back(a);
  }
  return out;
}

template<typename T, typename Acc, typename Fn>
inline micron::fvector<Acc>
naive_scanr(const T *p, usize n, Acc init, Fn fn)
{
  micron::fvector<Acc> out;
  out.resize(n + 1);
  out[n] = init;
  Acc a = init;
  for ( usize i = n; i-- > 0; ) {
    a = fn(p[i], a);
    out[i] = a;
  }
  return out;
}

// runs of adjacent elements that satisfy eq, as (start,len) pairs
template<typename T, typename Eq>
inline micron::fvector<micron::pair<usize, usize>>
naive_group_by(const T *p, usize n, Eq eq)
{
  micron::fvector<micron::pair<usize, usize>> out;
  usize i = 0;
  while ( i < n ) {
    usize j = i + 1;
    while ( j < n && eq(p[j - 1], p[j]) ) ++j;
    out.push_back(micron::pair<usize, usize>(i, j - i));
    i = j;
  }
  return out;
}

// NOTE: matches by ==, so it is meaningless on a NaN-bearing range -- a NaN matches nothing,
// including itself, and this would report "not a permutation" for a correct permutation. Guard
// call sites with mtest::fuzz::cmp_diff_ok / !has_nan.
template<typename T>
inline bool
naive_is_permutation(const T *a, usize an, const T *b, usize bn)
{
  if ( an != bn ) return false;
  micron::fvector<bool> used;
  used.resize(bn);
  for ( usize i = 0; i < bn; ++i ) used[i] = false;
  for ( usize i = 0; i < an; ++i ) {
    bool matched = false;
    for ( usize j = 0; j < bn; ++j ) {
      if ( !used[j] && b[j] == a[i] ) {
        used[j] = true;
        matched = true;
        break;
      }
    }
    if ( !matched ) return false;
  }
  return true;
}

};      // namespace fref

};      // namespace mtest::fuzz
