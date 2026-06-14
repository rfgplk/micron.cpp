//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../types.hpp"
#include "__asm/hw.hpp"
#include "mk.hpp"

namespace micron
{
namespace math
{

template<typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
sum(const T *first, const T *last) noexcept
{
  T acc{ 0 };
  for ( ; first != last; ++first ) acc = acc + *first;
  return acc;
}

template<typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
prod(const T *first, const T *last) noexcept
{
  T acc{ 1 };
  for ( ; first != last; ++first ) acc = acc * *first;
  return acc;
}

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-associative-math", "no-reciprocal-math", "signed-zeros")

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
kahan_sum(const F *first, const F *last) noexcept
{
  F s = F(0), c = F(0);
  for ( ; first != last; ++first ) {
    const F y = *first - c;
    const F t = s + y;
    c = (t - s) - y;
    s = t;
  }
  return s;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
neumaier_sum(const F *first, const F *last) noexcept
{
  if ( first == last ) return F(0);
  F s = *first++;
  F c = F(0);
  for ( ; first != last; ++first ) {
    const F x = *first;
    const F t = s + x;
    if ( mk::manip::fabs<F>(s) >= mk::manip::fabs<F>(x) ) {
      c = c + ((s - t) + x);
    } else {
      c = c + ((x - t) + s);
    }
    s = t;
  }
  return s + c;
}

#pragma GCC pop_options

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
mean(const F *first, const F *last) noexcept
{
  const usize n = usize(last - first);
  if ( n == 0 ) return F(0);
  return kahan_sum<F>(first, last) / F(n);
}

template<typename T>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::flatten]] inline constexpr f64
mean(const T *first, const T *last) noexcept
{
  const usize n = usize(last - first);
  if ( n == 0 ) return 0.0;
  i64 s = 0;
  for ( ; first != last; ++first ) s += i64(*first);
  return f64(s) / f64(n);
}

template<ieee754_floating F> struct moments {
  F mean;
  F var;
  usize n;
};

// Welford's online algorithm
template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr moments<F>
welford(const F *first, const F *last) noexcept
{
  moments<F> m{ F(0), F(0), 0 };
  for ( ; first != last; ++first ) {
    ++m.n;
    const F delta = *first - m.mean;
    m.mean += delta / F(m.n);
    const F delta2 = *first - m.mean;
    m.var += delta * delta2;
  }
  if ( m.n > 0 ) m.var /= F(m.n);
  return m;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
var(const F *first, const F *last) noexcept
{
  return welford<F>(first, last).var;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr F
std_dev(const F *first, const F *last) noexcept
{
  return mk::pow_ns::sqrt<F>(var<F>(first, last));
}

template<typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
min(const T *first, const T *last) noexcept
{
  if ( first == last ) return T{};
  T m = *first;
  for ( ++first; first != last; ++first )
    if ( *first < m ) m = *first;
  return m;
}

template<typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
max(const T *first, const T *last) noexcept
{
  if ( first == last ) return T{};
  T m = *first;
  for ( ++first; first != last; ++first )
    if ( *first > m ) m = *first;
  return m;
}

template<typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
argmin(const T *first, const T *last) noexcept
{
  if ( first == last ) return usize(0);
  usize idx = 0;
  T m = *first;
  for ( const T *p = first + 1; p != last; ++p ) {
    if ( *p < m ) {
      m = *p;
      idx = usize(p - first);
    }
  }
  return idx;
}

template<typename T>
[[nodiscard, gnu::flatten]] inline constexpr usize
argmax(const T *first, const T *last) noexcept
{
  if ( first == last ) return usize(0);
  usize idx = 0;
  T m = *first;
  for ( const T *p = first + 1; p != last; ++p ) {
    if ( *p > m ) {
      m = *p;
      idx = usize(p - first);
    }
  }
  return idx;
}

template<typename T>
[[nodiscard, gnu::flatten]] inline constexpr T
ptp(const T *first, const T *last) noexcept
{
  if ( first == last ) return T{};
  T mn = *first, mx = *first;
  for ( ++first; first != last; ++first ) {
    if ( *first < mn )
      mn = *first;
    else if ( *first > mx )
      mx = *first;
  }
  return T(mx - mn);
}

namespace __select
{

// modified quickselect O(n)
template<typename T>
inline constexpr void
quickselect(T *lo, T *hi, T *k) noexcept
{
  while ( hi - lo > 1 ) {
    T *mid = lo + (hi - lo) / 2;
    if ( *lo > *mid ) {
      T tmp = *lo;
      *lo = *mid;
      *mid = tmp;
    }
    if ( *mid > *(hi - 1) ) {
      T tmp = *mid;
      *mid = *(hi - 1);
      *(hi - 1) = tmp;
    }
    if ( *lo > *mid ) {
      T tmp = *lo;
      *lo = *mid;
      *mid = tmp;
    }
    const T pivot = *mid;
    T *i = lo, *j = hi - 1;
    while ( i <= j ) {
      while ( *i < pivot ) ++i;
      while ( *j > pivot ) --j;
      if ( i <= j ) {
        T tmp = *i;
        *i = *j;
        *j = tmp;
        ++i;
        --j;
      }
    }
    if ( k <= j )
      hi = j + 1;
    else if ( k >= i )
      lo = i;
    else
      return;
  }
}

};      // namespace __select

template<typename T>
[[nodiscard]] inline T
nth_element(T *first, T *last, usize k) noexcept
{
  if ( k >= usize(last - first) ) return T{};
  __select::quickselect(first, last, first + k);
  return first[k];
}

template<typename T>
[[nodiscard]] inline T
median(T *first, T *last) noexcept
{
  const usize n = usize(last - first);
  if ( n == 0 ) return T{};
  const usize k = n / 2;
  T hi = nth_element(first, last, k);
  if ( n & 1 ) return hi;
  T lo = nth_element(first, first + k, k - 1);
  return T((lo + hi) / T(2));
}

template<ieee754_floating F>
[[nodiscard]] inline F
quantile(F *first, F *last, F q) noexcept
{
  const usize n = usize(last - first);
  if ( n == 0 ) return F(0);
  if ( q <= F(0) ) return min<F>(first, last);
  if ( q >= F(1) ) return max<F>(first, last);
  const F idx = q * F(n - 1);
  const F i_lo_f = mk::round_ns::floor<F>(idx);
  const usize i_lo = usize(i_lo_f);
  const usize i_hi = i_lo + (i_lo + 1 < n ? 1 : 0);
  const F frac = idx - i_lo_f;
  const F a = nth_element(first, last, i_lo);
  const F b = nth_element(first, last, i_hi);
  return a + frac * (b - a);
}

template<typename T>
[[gnu::flatten]] inline constexpr void
cumsum(const T *__restrict__ first, const T *__restrict__ last, T *__restrict__ out) noexcept
{
  T acc{ 0 };
  for ( ; first != last; ++first, ++out ) {
    acc = acc + *first;
    *out = acc;
  }
}

template<typename T>
[[gnu::flatten]] inline constexpr void
cumprod(const T *__restrict__ first, const T *__restrict__ last, T *__restrict__ out) noexcept
{
  T acc{ 1 };
  for ( ; first != last; ++first, ++out ) {
    acc = acc * *first;
    *out = acc;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// container overloads

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
sum(const C &c) noexcept
{
  return sum(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
prod(const C &c) noexcept
{
  return prod(c.cbegin(), c.cend());
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
kahan_sum(const C &c) noexcept
{
  return kahan_sum<typename C::value_type>(c.cbegin(), c.cend());
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
neumaier_sum(const C &c) noexcept
{
  return neumaier_sum<typename C::value_type>(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr auto
mean(const C &c) noexcept
{
  return mean(c.cbegin(), c.cend());
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
var(const C &c) noexcept
{
  return var<typename C::value_type>(c.cbegin(), c.cend());
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
std_dev(const C &c) noexcept
{
  return std_dev<typename C::value_type>(c.cbegin(), c.cend());
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard, gnu::flatten]] inline constexpr moments<typename C::value_type>
welford(const C &c) noexcept
{
  return welford<typename C::value_type>(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
min(const C &c) noexcept
{
  return min(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
max(const C &c) noexcept
{
  return max(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr usize
argmin(const C &c) noexcept
{
  return argmin(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr usize
argmax(const C &c) noexcept
{
  return argmax(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard, gnu::flatten]] inline constexpr typename C::value_type
ptp(const C &c) noexcept
{
  return ptp(c.cbegin(), c.cend());
}

template<is_iterable_container C>
[[nodiscard]] inline typename C::value_type
median(C &c) noexcept
{
  return median(c.begin(), c.end());
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard]] inline typename C::value_type
quantile(C &c, typename C::value_type q) noexcept
{
  return quantile<typename C::value_type>(c.begin(), c.end(), q);
}

template<is_iterable_container C>
[[gnu::flatten]] inline void
cumsum(const C &in, C &out) noexcept
{
  cumsum(in.cbegin(), in.cend(), out.begin());
}

template<is_iterable_container C>
[[gnu::flatten]] inline void
cumprod(const C &in, C &out) noexcept
{
  cumprod(in.cbegin(), in.cend(), out.begin());
}

};      // namespace math
};      // namespace micron
