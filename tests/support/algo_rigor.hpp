//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// algo_rigor.hpp — shared infrastructure for the rigor_algo_* test suite.
//
// Provides:
//   * floating-point tolerance comparators (near / near_rel)
//   * 13 adversarial pattern generators that fill [out, out+n)
//   * kAdversarialSizes — 24 size boundaries covering cache-line / SIMD /
//     SBO edges
//   * mtest::rigor::ref:: naive reference implementations against which
//     property tests diff
//   * container factories that fan out a test body across
//     {vector, svector, array, carray, iarray} via type-tag dispatch
//   * prop_buffer_size — wraps snowball::property_test for the common
//     "fill a buffer, run the body" shape
//
// Every helper here is header-only, allocates only what micron itself
// allocates (no libc, no STL), and is designed to be safe to include from
// any /tests/rigor/rigor_algo_*.cpp file.

#include "../../src/array/array.hpp"
#include "../../src/array/carray.hpp"
#include "../../src/array/iarray.hpp"
#include "../../src/concepts.hpp"
#include "../../src/std.hpp"
#include "../../src/type_traits.hpp"
#include "../../src/types.hpp"
#include "../../src/vector/svector.hpp"
#include "../../src/vector/vector.hpp"

#include "../snowball/snowball.hpp"
#include "../snowball/snowball_ext.hpp"

#include "oracles.hpp"

namespace mtest::rigor
{

// ─────────────────────────────────────────────────────────────────────────
// Floating-point tolerance helpers
// ─────────────────────────────────────────────────────────────────────────

template<typename T>
constexpr T
eps_default(void) noexcept
{
  if constexpr ( micron::is_same_v<T, f32> )
    return T(1e-4);
  else if constexpr ( micron::is_same_v<T, f64> )
    return T(1e-9);
  else
    return T(1e-12);
}

template<typename T>
constexpr bool
near(T a, T b, T eps = eps_default<T>()) noexcept
{
  T d = a - b;
  return (d < T(0) ? -d : d) <= eps;
}

template<typename T>
constexpr bool
near_rel(T a, T b, T rel = T(1e-6)) noexcept
{
  T m = (a < T(0) ? -a : a);
  T n = (b < T(0) ? -b : b);
  T tol = (m > n ? m : n) * rel + eps_default<T>();
  T d = a - b;
  return (d < T(0) ? -d : d) <= tol;
}

// Runtime NaN / Inf via bit-cast. Built under -Ofast (which implies
// -ffast-math / -ffinite-math-only), so the usual `0.0/0.0` and `x != x`
// tricks are folded away by the optimizer. Using bit patterns is the only
// reliable way to materialize these values; the IEEE encodings are
// universal so this works on any conforming platform.
template<typename T>
inline T
runtime_nan(void) noexcept
{
  if constexpr ( sizeof(T) == 4 ) {
    u32 bits = 0x7fc00000u;
    T x;
    __builtin_memcpy(&x, &bits, 4);
    return x;
  } else {
    u64 bits = 0x7ff8000000000000ULL;
    T x;
    __builtin_memcpy(&x, &bits, 8);
    return x;
  }
}

template<typename T>
inline T
runtime_pinf(void) noexcept
{
  if constexpr ( sizeof(T) == 4 ) {
    u32 bits = 0x7f800000u;
    T x;
    __builtin_memcpy(&x, &bits, 4);
    return x;
  } else {
    u64 bits = 0x7ff0000000000000ULL;
    T x;
    __builtin_memcpy(&x, &bits, 8);
    return x;
  }
}

template<typename T>
inline T
runtime_ninf(void) noexcept
{
  if constexpr ( sizeof(T) == 4 ) {
    u32 bits = 0xff800000u;
    T x;
    __builtin_memcpy(&x, &bits, 4);
    return x;
  } else {
    u64 bits = 0xfff0000000000000ULL;
    T x;
    __builtin_memcpy(&x, &bits, 8);
    return x;
  }
}

// is_nan via bit pattern — `x != x` is constant-folded under -ffast-math.
template<typename T>
inline bool
is_nan(T x) noexcept
{
  if constexpr ( sizeof(T) == 4 ) {
    u32 bits;
    __builtin_memcpy(&bits, &x, 4);
    return ((bits >> 23) & 0xffu) == 0xffu && (bits & 0x7fffffu) != 0u;
  } else {
    u64 bits;
    __builtin_memcpy(&bits, &x, 8);
    return ((bits >> 52) & 0x7ffULL) == 0x7ffULL && (bits & 0xfffffffffffffULL) != 0ULL;
  }
}

template<typename T>
inline bool
is_inf(T x) noexcept
{
  if constexpr ( sizeof(T) == 4 ) {
    u32 bits;
    __builtin_memcpy(&bits, &x, 4);
    return ((bits >> 23) & 0xffu) == 0xffu && (bits & 0x7fffffu) == 0u;
  } else {
    u64 bits;
    __builtin_memcpy(&bits, &x, 8);
    return ((bits >> 52) & 0x7ffULL) == 0x7ffULL && (bits & 0xfffffffffffffULL) == 0ULL;
  }
}

// ─────────────────────────────────────────────────────────────────────────
// Adversarial pattern generators (write [out, out+n))
// ─────────────────────────────────────────────────────────────────────────

template<typename T>
inline void
pat_sorted(T *out, usize n, T base = T{}) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(base + static_cast<T>(i));
}

template<typename T>
inline void
pat_reverse_sorted(T *out, usize n, T base = T{}) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(base + static_cast<T>(n - 1 - i));
}

template<typename T>
inline void
pat_all_equal(T *out, usize n, T v = T{ 1 }) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = v;
}

template<typename T>
inline void
pat_alternating(T *out, usize n, T a = T{ 1 }, T b = T{ static_cast<T>(-1) }) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = (i & 1u) ? b : a;
}

template<typename T>
inline void
pat_sawtooth(T *out, usize n, usize period = 8) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(i % period);
}

template<typename T>
inline void
pat_zeros(T *out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = T{};
}

template<typename T>
inline void
pat_single_spike(T *out, usize n, usize at, T spike) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = T{};
  if ( at < n ) out[at] = spike;
}

template<typename T>
inline void
pat_near_overflow_max(T *out, usize n) noexcept
{
  if constexpr ( micron::is_integral_v<T> ) {
    if constexpr ( micron::is_signed_v<T> ) {
      T big = static_cast<T>(static_cast<T>(~static_cast<T>(0)) ^ (static_cast<T>(1) << (sizeof(T) * 8 - 1)));
      for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(big - static_cast<T>(i));
    } else {
      T big = static_cast<T>(~static_cast<T>(0));
      for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(big - static_cast<T>(i));
    }
  } else {
    for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(1e30) - static_cast<T>(i);
  }
}

template<typename T>
inline void
pat_near_overflow_min(T *out, usize n) noexcept
{
  if constexpr ( micron::is_integral_v<T> ) {
    if constexpr ( micron::is_signed_v<T> ) {
      T small_ = static_cast<T>(static_cast<T>(1) << (sizeof(T) * 8 - 1));
      for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(small_ + static_cast<T>(i));
    } else {
      for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(i);
    }
  } else {
    for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(-1e30) + static_cast<T>(i);
  }
}

template<typename T>
  requires micron::is_floating_point_v<T>
inline void
pat_with_nan(T *out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(i);
  if ( n > 2 ) out[n / 2] = runtime_nan<T>();
}

template<typename T>
  requires micron::is_floating_point_v<T>
inline void
pat_with_inf(T *out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = static_cast<T>(i);
  if ( n > 3 ) {
    out[n / 3] = runtime_pinf<T>();
    out[(2 * n) / 3] = runtime_ninf<T>();
  }
}

template<typename T>
  requires micron::is_floating_point_v<T>
inline void
pat_signed_zero(T *out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) out[i] = (i & 1u) ? T(0) : -T(0);
}

template<typename T>
inline void
pat_random(T *out, usize n, prng &rng) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    u64 r = rng.next();
    if constexpr ( micron::is_integral_v<T> ) {
      out[i] = static_cast<T>(r);
    } else if constexpr ( micron::is_floating_point_v<T> ) {
      out[i] = static_cast<T>(static_cast<i64>(r)) / static_cast<T>(static_cast<i64>(1) << 32);
    } else {
      out[i] = T{};
    }
  }
}

template<typename T>
inline void
pat_random_small(T *out, usize n, prng &rng, T lo = T(-1000), T hi = T(1000)) noexcept
{
  T range = static_cast<T>(hi - lo);
  for ( usize i = 0; i < n; ++i ) {
    u64 r = rng.next();
    if constexpr ( micron::is_integral_v<T> ) {
      if ( range == T(0) ) {
        out[i] = lo;
      } else {
        out[i] = static_cast<T>(lo + static_cast<T>(r % static_cast<u64>(range > 0 ? range : -range)));
      }
    } else {
      T u = static_cast<T>(static_cast<u32>(r) & 0xffffffu) / static_cast<T>(0x1000000);
      out[i] = static_cast<T>(lo + u * range);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────
// Adversarial size table
// ─────────────────────────────────────────────────────────────────────────

inline constexpr usize kAdversarialSizes[]
    = { 0u, 1u, 2u, 3u, 4u, 7u, 8u, 15u, 16u, 17u, 31u, 32u, 33u, 63u, 64u, 65u, 127u, 128u, 129u, 255u, 256u, 257u, 1023u };
inline constexpr usize kAdversarialSizesCount = sizeof(kAdversarialSizes) / sizeof(kAdversarialSizes[0]);

// ─────────────────────────────────────────────────────────────────────────
// Naive reference implementations.
//
// These are intentionally the most obvious linear loops. Use them as the
// oracle inside property_test bodies — if micron::find disagrees with
// ref::naive_find, the failure is in micron, not the oracle.
// ─────────────────────────────────────────────────────────────────────────

namespace ref
{

template<typename T>
inline T
naive_sum(const T *p, usize n) noexcept
{
  T s{};
  for ( usize i = 0; i < n; ++i ) s = static_cast<T>(s + p[i]);
  return s;
}

template<typename T>
inline T
naive_max(const T *p, usize n) noexcept
{
  T m = p[0];
  for ( usize i = 1; i < n; ++i )
    if ( p[i] > m ) m = p[i];
  return m;
}

template<typename T>
inline T
naive_min(const T *p, usize n) noexcept
{
  T m = p[0];
  for ( usize i = 1; i < n; ++i )
    if ( p[i] < m ) m = p[i];
  return m;
}

template<typename T>
inline usize
naive_max_idx(const T *p, usize n) noexcept
{
  usize mi = 0;
  for ( usize i = 1; i < n; ++i )
    if ( p[i] > p[mi] ) mi = i;
  return mi;
}

template<typename T>
inline usize
naive_min_idx(const T *p, usize n) noexcept
{
  usize mi = 0;
  for ( usize i = 1; i < n; ++i )
    if ( p[i] < p[mi] ) mi = i;
  return mi;
}

template<typename T>
inline bool
naive_all_of_eq(const T *p, usize n, const T &v) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !(p[i] == v) ) return false;
  return true;
}

template<typename T>
inline bool
naive_any_of_eq(const T *p, usize n, const T &v) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == v ) return true;
  return false;
}

template<typename T, typename Pred>
inline bool
naive_all_of_if(const T *p, usize n, Pred pred) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !pred(p[i]) ) return false;
  return true;
}

template<typename T, typename Pred>
inline bool
naive_any_of_if(const T *p, usize n, Pred pred) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( pred(p[i]) ) return true;
  return false;
}

template<typename T>
inline usize
naive_count_eq(const T *p, usize n, const T &v) noexcept
{
  usize c = 0;
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == v ) ++c;
  return c;
}

template<typename T, typename Pred>
inline usize
naive_count_if(const T *p, usize n, Pred pred) noexcept
{
  usize c = 0;
  for ( usize i = 0; i < n; ++i )
    if ( pred(p[i]) ) ++c;
  return c;
}

template<typename T>
inline const T *
naive_find(const T *p, usize n, const T &v) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == v ) return p + i;
  return nullptr;
}

template<typename T>
inline const T *
naive_find_last(const T *p, usize n, const T &v) noexcept
{
  const T *r = nullptr;
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == v ) r = p + i;
  return r;
}

template<typename T, typename Pred>
inline const T *
naive_find_if(const T *p, usize n, Pred pred) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( pred(p[i]) ) return p + i;
  return nullptr;
}

template<typename T>
inline const T *
naive_adjacent_find(const T *p, usize n) noexcept
{
  if ( n < 2 ) return nullptr;
  for ( usize i = 0; i + 1 < n; ++i )
    if ( p[i] == p[i + 1] ) return p + i;
  return nullptr;
}

template<typename T>
inline bool
naive_equal(const T *a, const T *b, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !(a[i] == b[i]) ) return false;
  return true;
}

template<typename T>
inline usize
naive_mismatch_idx(const T *a, const T *b, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !(a[i] == b[i]) ) return i;
  return n;
}

template<typename T>
inline bool
naive_starts_with(const T *p, usize n, const T *q, usize m) noexcept
{
  if ( m > n ) return false;
  for ( usize i = 0; i < m; ++i )
    if ( !(p[i] == q[i]) ) return false;
  return true;
}

template<typename T>
inline bool
naive_ends_with(const T *p, usize n, const T *q, usize m) noexcept
{
  if ( m > n ) return false;
  for ( usize i = 0; i < m; ++i )
    if ( !(p[n - m + i] == q[i]) ) return false;
  return true;
}

template<typename T>
inline const T *
naive_search(const T *p, usize n, const T *q, usize m) noexcept
{
  if ( m == 0 ) return p;
  if ( m > n ) return nullptr;
  for ( usize i = 0; i + m <= n; ++i ) {
    bool ok = true;
    for ( usize j = 0; j < m; ++j )
      if ( !(p[i + j] == q[j]) ) {
        ok = false;
        break;
      }
    if ( ok ) return p + i;
  }
  return nullptr;
}

template<typename T>
inline const T *
naive_search_end(const T *p, usize n, const T *q, usize m) noexcept
{
  if ( m == 0 ) return p + n;
  if ( m > n ) return nullptr;
  const T *r = nullptr;
  for ( usize i = 0; i + m <= n; ++i ) {
    bool ok = true;
    for ( usize j = 0; j < m; ++j )
      if ( !(p[i + j] == q[j]) ) {
        ok = false;
        break;
      }
    if ( ok ) r = p + i;
  }
  return r;
}

template<typename T>
inline const T *
naive_search_n(const T *p, usize n, usize k, const T &v) noexcept
{
  if ( k == 0 ) return p;
  if ( k > n ) return nullptr;
  for ( usize i = 0; i + k <= n; ++i ) {
    bool ok = true;
    for ( usize j = 0; j < k; ++j )
      if ( !(p[i + j] == v) ) {
        ok = false;
        break;
      }
    if ( ok ) return p + i;
  }
  return nullptr;
}

template<typename T>
inline const T *
naive_find_first_of(const T *p, usize n, const T *s, usize m) noexcept
{
  for ( usize i = 0; i < n; ++i )
    for ( usize j = 0; j < m; ++j )
      if ( p[i] == s[j] ) return p + i;
  return nullptr;
}

template<typename T>
inline void
naive_reverse(T *p, usize n) noexcept
{
  if ( n < 2 ) return;
  usize i = 0, j = n - 1;
  while ( i < j ) {
    T t = p[i];
    p[i] = p[j];
    p[j] = t;
    ++i;
    --j;
  }
}

template<typename T>
inline void
naive_rotate_left(T *p, usize n, usize k) noexcept
{
  if ( n == 0 ) return;
  k %= n;
  if ( k == 0 ) return;
  T tmp[1024];
  for ( usize i = 0; i < k; ++i ) tmp[i] = p[i];
  for ( usize i = 0; i + k < n; ++i ) p[i] = p[i + k];
  for ( usize i = 0; i < k; ++i ) p[n - k + i] = tmp[i];
}

template<typename T>
inline void
naive_rotate_right(T *p, usize n, usize k) noexcept
{
  if ( n == 0 ) return;
  k %= n;
  if ( k == 0 ) return;
  naive_rotate_left(p, n, n - k);
}

template<typename T>
inline void
naive_shift_left(T *p, usize n, usize k) noexcept
{
  if ( n == 0 || k == 0 ) return;
  if ( k >= n ) {
    for ( usize i = 0; i < n; ++i ) p[i] = T{};
    return;
  }
  for ( usize i = 0; i + k < n; ++i ) p[i] = p[i + k];
  for ( usize i = n - k; i < n; ++i ) p[i] = T{};
}

template<typename T>
inline void
naive_shift_right(T *p, usize n, usize k) noexcept
{
  if ( n == 0 || k == 0 ) return;
  if ( k >= n ) {
    for ( usize i = 0; i < n; ++i ) p[i] = T{};
    return;
  }
  for ( usize i = n; i-- > k; ) p[i] = p[i - k];
  for ( usize i = 0; i < k; ++i ) p[i] = T{};
}

template<typename T, typename Acc, typename Fn>
inline Acc
naive_fold_left(const T *p, usize n, Acc init, Fn fn) noexcept
{
  Acc acc = init;
  for ( usize i = 0; i < n; ++i ) acc = fn(acc, p[i]);
  return acc;
}

template<typename T, typename Acc, typename Fn>
inline Acc
naive_fold_right(const T *p, usize n, Acc init, Fn fn) noexcept
{
  Acc acc = init;
  for ( usize i = n; i-- > 0; ) acc = fn(p[i], acc);
  return acc;
}

template<typename T, typename Acc>
inline Acc
naive_accumulate(const T *p, usize n, Acc init) noexcept
{
  Acc s = init;
  for ( usize i = 0; i < n; ++i ) s = static_cast<Acc>(s + p[i]);
  return s;
}

template<typename T, typename Acc>
inline Acc
naive_inner_product(const T *a, const T *b, usize n, Acc init) noexcept
{
  Acc s = init;
  for ( usize i = 0; i < n; ++i ) s = static_cast<Acc>(s + a[i] * b[i]);
  return s;
}

template<typename T>
inline bool
naive_is_heap_max(const T *p, usize n) noexcept
{
  for ( usize i = 1; i < n; ++i ) {
    usize parent = (i - 1) / 2;
    if ( p[parent] < p[i] ) return false;
  }
  return true;
}

template<typename T>
inline usize
naive_unique_inplace(T *p, usize n) noexcept
{
  if ( n == 0 ) return 0;
  usize w = 1;
  for ( usize i = 1; i < n; ++i )
    if ( !(p[i] == p[w - 1]) ) p[w++] = p[i];
  return w;
}

template<typename T, typename Pred>
inline usize
naive_partition(T *p, usize n, Pred pred) noexcept
{
  usize w = 0;
  for ( usize i = 0; i < n; ++i ) {
    if ( pred(p[i]) ) {
      T t = p[w];
      p[w] = p[i];
      p[i] = t;
      ++w;
    }
  }
  return w;
}

};      // namespace ref

// ─────────────────────────────────────────────────────────────────────────
// Container factories
// ─────────────────────────────────────────────────────────────────────────

// type tag for dispatch in for_each_*_container helpers
template<typename T> struct type_tag {
  using type = T;
};

// build any of {vector, svector, array, carray} from a raw buffer + size.
// For fixed-size containers (array/carray), N must equal n; the helper
// silently truncates / leaves tail untouched otherwise.
template<typename C, typename T>
inline C
make_from_ptr(const T *p, usize n)
{
  C c{};
  if constexpr ( requires(C x) { x.resize(usize{ 0 }); } ) {
    c.resize(n);
    for ( usize i = 0; i < n; ++i ) c[i] = p[i];
  } else if constexpr ( requires(C x, T v) { x.push_back(v); } ) {
    for ( usize i = 0; i < n; ++i ) c.push_back(p[i]);
  } else {
    usize lim = (n < c.size()) ? n : c.size();
    for ( usize i = 0; i < lim; ++i ) c[i] = p[i];
  }
  return c;
}

template<typename T, usize N>
inline micron::iarray<T, N>
make_iarray_from(const T *p) noexcept
{
  micron::array<T, N> tmp;
  for ( usize i = 0; i < N; ++i ) tmp[i] = p[i];
  return micron::iarray<T, N>(micron::move(tmp));
}

// Single-call container builder used in for_each_*_container lambdas.
// Detects iarray (immutable_tag) and routes through its array-conversion
// constructor; everything else goes through make_from_ptr.
template<typename C, typename T>
inline C
build_test_container(const T *buf, usize n)
{
  if constexpr ( requires { typename C::mutability_type; } ) {
    if constexpr ( micron::is_same_v<typename C::mutability_type, micron::immutable_tag> ) {
      constexpr usize N = C::static_size;
      (void)n;
      return make_iarray_from<T, N>(buf);
    } else {
      return make_from_ptr<C, T>(buf, n);
    }
  } else {
    return make_from_ptr<C, T>(buf, n);
  }
}

// ─────────────────────────────────────────────────────────────────────────
// Container fan-out helpers.
//
// Each helper invokes `fn(type_tag<C>{})` once per container kind. The
// lambda receives a typed tag; recover the underlying type with
//
//     using C = typename decltype(tag)::type;
//
// fan-out subsets are deliberately curated to avoid SFINAE failures:
// iarray is immutable (no mutating writes), svector/array/carray lack
// resize() (no container-returning algorithms).
// ─────────────────────────────────────────────────────────────────────────

template<typename T, usize N, typename Fn>
inline void
for_each_mutating_container(Fn &&fn)
{
  fn(type_tag<micron::vector<T>>{});
  fn(type_tag<micron::array<T, N>>{});
  fn(type_tag<micron::svector<T, N>>{});
  fn(type_tag<micron::carray<T, N>>{});
}

// Read-only fan-out. iarray is deliberately EXCLUDED — its begin()/data()
// return const overloads, which makes it fail micron::is_iterable_container
// (the concept requires data() to return T::pointer, not T::const_pointer).
// iarray gets tested separately via pointer overloads on its begin()/end().
template<typename T, usize N, typename Fn>
inline void
for_each_readonly_container(Fn &&fn)
{
  fn(type_tag<micron::vector<T>>{});
  fn(type_tag<micron::array<T, N>>{});
  fn(type_tag<micron::svector<T, N>>{});
  fn(type_tag<micron::carray<T, N>>{});
}

template<typename T, typename Fn>
inline void
for_each_resizable_container(Fn &&fn)
{
  fn(type_tag<micron::vector<T>>{});
}

// ─────────────────────────────────────────────────────────────────────────
// Property loop wrappers
// ─────────────────────────────────────────────────────────────────────────

// Drive 10k random invocations of body(buf, n) over an int-buffer pattern.
// body must not throw; the per-iteration buffer is freshly populated.
template<typename T, usize MaxN, typename Body>
inline void
prop_buffer_size(const char *name, Body &&body, usize count = 10000, u64 seed = 0xc001cafedeadbeefULL)
{
  snowball::test_case(name);
  prng rng(seed);
  T buf[MaxN];
  for ( usize i = 0; i < count; ++i ) {
    usize n = (rng.next() & (MaxN - 1));
    if ( n == 0 ) n = 1;
    pat_random(buf, n, rng);
    body(buf, n);
  }
  snowball::end_test_case();
}

// Like prop_buffer_size but for binary algorithms — generates two buffers.
template<typename T, usize MaxN, typename Body>
inline void
prop_two_buffers(const char *name, Body &&body, usize count = 10000, u64 seed = 0xc001cafedeadbeefULL)
{
  snowball::test_case(name);
  prng rng(seed);
  T a[MaxN];
  T b[MaxN];
  for ( usize i = 0; i < count; ++i ) {
    usize n = (rng.next() & (MaxN - 1));
    if ( n == 0 ) n = 1;
    pat_random(a, n, rng);
    pat_random(b, n, rng);
    body(a, b, n);
  }
  snowball::end_test_case();
}

};      // namespace mtest::rigor
