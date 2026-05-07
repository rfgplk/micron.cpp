//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// numpy style numerical fns

#include "../concepts.hpp"
#include "../types.hpp"
#include "generic.hpp"
#include "mk.hpp"

namespace micron
{
namespace math
{

// c[0] constant term, c[N-1] highest order
template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
polyval(const F *coeffs, usize n, F x) noexcept
{
  if ( n == 0 ) return F(0);
  F r = coeffs[n - 1];
  for ( usize i = n - 1; i-- > 0; ) r = math::fma<F>(r, x, coeffs[i]);
  return r;
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
horner(const F *coeffs, usize n, F x) noexcept
{
  return polyval<F>(coeffs, n, x);
}

template <is_iterable_container C, ieee754_floating F = typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr F
polyval(const C &coeffs, F x) noexcept
{
  return polyval<F>(coeffs.cbegin(), coeffs.size(), x);
}

// out[i] = in[i+1] - in[i]
template <typename T>
[[gnu::flatten]] inline constexpr void
diff(const T *__restrict__ in, usize n, T *__restrict__ out) noexcept
{
  if ( n < 2 ) return;
  for ( usize i = 0; i + 1 < n; ++i ) out[i] = T(in[i + 1] - in[i]);
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
diff(const C &in, C &out) noexcept
{
  diff<typename C::value_type>(in.cbegin(), in.size(), out.begin());
}

template <ieee754_floating F>
[[gnu::flatten]] inline constexpr void
gradient(const F *__restrict__ y, usize n, F *__restrict__ g) noexcept
{
  if ( n == 0 ) return;
  if ( n == 1 ) {
    g[0] = F(0);
    return;
  }
  g[0] = y[1] - y[0];
  for ( usize i = 1; i + 1 < n; ++i ) g[i] = F(0.5) * (y[i + 1] - y[i - 1]);
  g[n - 1] = y[n - 1] - y[n - 2];
}

template <ieee754_floating F>
[[gnu::flatten]] inline void
gradient(const F *__restrict__ x, const F *__restrict__ y, usize n, F *__restrict__ g) noexcept
{
  if ( n == 0 ) return;
  if ( n == 1 ) {
    g[0] = F(0);
    return;
  }
  g[0] = (y[1] - y[0]) / (x[1] - x[0]);
  for ( usize i = 1; i + 1 < n; ++i ) g[i] = (y[i + 1] - y[i - 1]) / (x[i + 1] - x[i - 1]);
  g[n - 1] = (y[n - 1] - y[n - 2]) / (x[n - 1] - x[n - 2]);
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[gnu::flatten]] inline void
gradient(const C &y, C &g) noexcept
{
  gradient<typename C::value_type>(y.cbegin(), y.size(), g.begin());
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
interp(F xq, const F *x, const F *y, usize n) noexcept
{
  if ( n == 0 ) return F(0);
  if ( n == 1 || xq <= x[0] ) return y[0];
  if ( xq >= x[n - 1] ) return y[n - 1];
  usize lo = 0, hi = n - 1;
  while ( hi - lo > 1 ) {
    const usize mid = lo + (hi - lo) / 2;
    if ( x[mid] <= xq )
      lo = mid;
    else
      hi = mid;
  }
  const F t = (xq - x[lo]) / (x[lo + 1] - x[lo]);
  return math::fma<F>(t, y[lo + 1] - y[lo], y[lo]);
}

template <is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline typename C::value_type
interp(typename C::value_type xq, const C &x, const C &y) noexcept
{
  return interp<typename C::value_type>(xq, x.cbegin(), y.cbegin(), x.size());
}

// non FFT based
template <typename T>
[[gnu::flatten]] inline constexpr void
convolve(const T *__restrict__ a, usize n, const T *__restrict__ b, usize k, T *__restrict__ out) noexcept
{
  for ( usize i = 0; i + 0 < n + k - 1; ++i ) {
    T s{ 0 };
    const usize j_lo = (i + 1 > k) ? (i + 1 - k) : 0;
    const usize j_hi = (i + 1 < n) ? (i + 1) : n;
    for ( usize j = j_lo; j < j_hi; ++j ) s = s + a[j] * b[i - j];
    out[i] = s;
  }
}

template <typename T>
[[gnu::flatten]] inline constexpr void
correlate(const T *__restrict__ a, usize n, const T *__restrict__ b, usize k, T *__restrict__ out) noexcept
{
  for ( usize i = 0; i + 0 < n + k - 1; ++i ) {
    T s{ 0 };
    const usize j_lo = (i + 1 > k) ? (i + 1 - k) : 0;
    const usize j_hi = (i + 1 < n) ? (i + 1) : n;
    for ( usize j = j_lo; j < j_hi; ++j ) s = s + a[j] * b[k - 1 - (i - j)];
    out[i] = s;
  }
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
convolve(const C &a, const C &b, C &out) noexcept
{
  convolve<typename C::value_type>(a.cbegin(), a.size(), b.cbegin(), b.size(), out.begin());
}

template <is_iterable_container C>
[[gnu::flatten]] inline void
correlate(const C &a, const C &b, C &out) noexcept
{
  correlate<typename C::value_type>(a.cbegin(), a.size(), b.cbegin(), b.size(), out.begin());
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr T
lcm(T a, T b) noexcept
{
  if ( a == T(0) || b == T(0) ) return T(0);
  const T g = math::gcd<T>(a, b);
  T av = (a < T(0)) ? T(-a) : a;
  T bv = (b < T(0)) ? T(-b) : b;
  return T((av / g) * bv);
}

template <typename T>
  requires(micron::is_integral_v<T>)
struct divmod_result {
  T quot;
  T rem;
};

template <typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr divmod_result<T>
divmod(T a, T b) noexcept
{
  T q = a / b;
  T r = a - q * b;
  if constexpr ( micron::is_signed_v<T> ) {
    if ( (r != T(0)) && ((r < T(0)) != (b < T(0))) ) {
      q = q - T(1);
      r = r + b;
    }
  }
  return { q, r };
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%
// factorials

template <typename T = u64>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr T
factorial(int n) noexcept
{
  T r{ 1 };
  for ( int i = 2; i <= n; ++i ) r = r * T(i);
  return r;
}

template <typename T = u64>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline constexpr T
double_factorial(int n) noexcept
{
  T r{ 1 };
  for ( int i = n; i >= 2; i -= 2 ) r = r * T(i);
  return r;
}

template <typename T = u64>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::flatten]] inline constexpr T
comb(int n, int k) noexcept
{
  if ( k < 0 || k > n ) return T(0);
  if ( k > n - k ) k = n - k;
  T r{ 1 };
  for ( int i = 0; i < k; ++i ) {
    r = r * T(n - i);
    r = r / T(i + 1);
  }
  return r;
}

template <typename T = u64>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::flatten]] inline constexpr T
perm(int n, int k) noexcept
{
  if ( k < 0 || k > n ) return T(0);
  T r{ 1 };
  for ( int i = 0; i < k; ++i ) r = r * T(n - i);
  return r;
}

template <typename T>
  requires(micron::is_integral_v<T>)
[[gnu::flatten]] inline void
bincount(const T *__restrict__ x, usize n_x, u64 *__restrict__ counts, usize n_bins) noexcept
{
  for ( usize i = 0; i < n_bins; ++i ) counts[i] = 0;
  for ( usize i = 0; i < n_x; ++i ) {
    const T v = x[i];
    if ( v >= T(0) && usize(v) < n_bins ) ++counts[usize(v)];
  }
}

template <ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr usize
digitize(F x, const F *edges, usize n_edges) noexcept
{
  if ( n_edges == 0 ) return 0;
  if ( x < edges[0] ) return 0;
  if ( x >= edges[n_edges - 1] ) return n_edges;
  usize lo = 0, hi = n_edges - 1;
  while ( hi - lo > 1 ) {
    const usize mid = lo + (hi - lo) / 2;
    if ( edges[mid] <= x )
      lo = mid;
    else
      hi = mid;
  }
  return lo + 1;
}

template <ieee754_floating F>
[[gnu::flatten]] inline void
histogram(const F *__restrict__ x, usize n_x, F lo, F hi, u64 *__restrict__ out_counts, usize n_bins) noexcept
{
  for ( usize i = 0; i < n_bins; ++i ) out_counts[i] = 0;
  if ( n_bins == 0 || hi <= lo ) return;
  const F inv_w = F(n_bins) / (hi - lo);
  for ( usize i = 0; i < n_x; ++i ) {
    const F v = x[i];
    if ( v < lo || v >= hi ) continue;
    usize b = usize((v - lo) * inv_w);
    if ( b >= n_bins ) b = n_bins - 1;
    ++out_counts[b];
  }
}

};     // namespace math
};     // namespace micron
