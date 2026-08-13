//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../simd/strings.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scan kernels

// all fns return an index

namespace micron
{
namespace __impl
{

template<typename T>
concept lane_scannable = (micron::is_integral_v<micron::remove_cv_t<T>> or micron::is_enum_v<micron::remove_cv_t<T>>
                          or micron::is_pointer_v<micron::remove_cv_t<T>>)
                         and (sizeof(T) == 1 or sizeof(T) == 2 or sizeof(T) == 4 or sizeof(T) == 8);

// %%%%%%%%%%%%%%%%%%%%%%%
// element scans

template<typename T>
[[gnu::always_inline]] constexpr usize
scan_find(const T *p, usize n, const T &v) noexcept
{
  if constexpr ( lane_scannable<T> ) {
    if ( !__builtin_is_constant_evaluated() ) return micron::simd::find_first_elem(p, n, v);
  }
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == v ) return i;
  return n;
}

template<typename T>
[[gnu::always_inline]] constexpr usize
scan_rfind(const T *p, usize n, const T &v) noexcept
{
  if constexpr ( lane_scannable<T> ) {
    if ( !__builtin_is_constant_evaluated() ) return micron::simd::find_last_elem(p, n, v);
  }
  for ( usize i = n; i-- > 0; )
    if ( p[i] == v ) return i;
  return n;
}

template<typename T>
[[gnu::always_inline]] constexpr usize
scan_find_not(const T *p, usize n, const T &v) noexcept
{
  if constexpr ( lane_scannable<T> ) {
    if ( !__builtin_is_constant_evaluated() ) return micron::simd::find_first_ne_elem(p, n, v);
  }
  for ( usize i = 0; i < n; ++i )
    if ( !(p[i] == v) ) return i;
  return n;
}

template<typename T>
[[gnu::always_inline]] constexpr usize
scan_count(const T *p, usize n, const T &v) noexcept
{
  if constexpr ( lane_scannable<T> ) {
    if ( !__builtin_is_constant_evaluated() ) return micron::simd::count_elem(p, n, v);
  }
  usize c = 0;
  for ( usize i = 0; i < n; ++i )
    if ( p[i] == v ) ++c;
  return c;
}

template<typename T>
[[gnu::always_inline]] constexpr usize
scan_mismatch(const T *a, const T *b, usize n) noexcept
{
  usize i = 0;
  for ( ; i != n && a[i] == b[i]; ++i );
  return i;
}

template<typename T>
[[gnu::always_inline]] constexpr bool
scan_equal(const T *a, const T *b, usize n) noexcept
{
  usize i = 0;
  if constexpr ( lane_scannable<T> ) {
    if ( !__builtin_is_constant_evaluated() ) {
      for ( ; i + 8 <= n; i += 8 ) {
        bool same = true;
        for ( usize k = 0; k < 8; ++k ) same &= (a[i + k] == b[i + k]);
        if ( !same ) return false;
      }
    }
  }
  for ( ; i < n; ++i )
    if ( !(a[i] == b[i]) ) return false;
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%
// set membership

struct byte_set {
  u64 w[4];

  constexpr byte_set() noexcept : w{ 0, 0, 0, 0 } { }

  constexpr void
  add(u8 c) noexcept
  {
    w[c >> 6] |= (u64(1) << (c & 63));
  }

  constexpr bool
  has(u8 c) const noexcept
  {
    return (w[c >> 6] >> (c & 63)) & 1;
  }
};

template<typename T>
concept byte_wide = sizeof(T) == 1 and (micron::is_integral_v<micron::remove_cv_t<T>> or micron::is_enum_v<micron::remove_cv_t<T>>);

template<typename T, typename P>
[[gnu::always_inline]] constexpr usize
scan_find_first_of(const T *p, usize n, const P *set, usize k) noexcept
{
  if ( k == 0 ) return n;

  if constexpr ( byte_wide<T> ) {
    byte_set bs;
    for ( usize c = 0; c < k; ++c ) bs.add(static_cast<u8>(static_cast<T>(set[c])));
    for ( usize i = 0; i < n; ++i )
      if ( bs.has(static_cast<u8>(p[i])) ) return i;
    return n;
  } else if constexpr ( lane_scannable<T> and micron::is_same_v<micron::remove_cv_t<T>, micron::remove_cv_t<P>> ) {
    if ( !__builtin_is_constant_evaluated() ) return micron::simd::find_first_of_elem(p, n, set, k, 0);
  }

  for ( usize i = 0; i < n; ++i )
    for ( usize c = 0; c < k; ++c )
      if ( p[i] == static_cast<T>(set[c]) ) return i;
  return n;
}

// %%%%%%%%%%%%%%%%%%%
// subsequence search

inline constexpr usize kmp_stack_max = 256;
using kmp_slot = u16;

inline constexpr usize kmp_min_width = 8;

template<typename T, typename P>
constexpr void
kmp_table(const P *pat, usize m, kmp_slot *tbl) noexcept
{
  if ( m == 0 ) return;
  tbl[0] = 0;
  usize k = 0;
  for ( usize i = 1; i < m; ) {
    if ( static_cast<T>(pat[i]) == static_cast<T>(pat[k]) ) {
      tbl[i] = static_cast<kmp_slot>(k + 1);
      ++i;
      ++k;
    } else if ( k ) {
      k = tbl[k - 1];
    } else {
      tbl[i] = 0;
      ++i;
    }
  }
}

template<typename T, typename P>
constexpr void
kmp_table_rev(const P *pat, usize m, kmp_slot *tbl) noexcept
{
  if ( m == 0 ) return;
  tbl[0] = 0;
  usize k = 0;
  for ( usize i = 1; i < m; ) {
    if ( static_cast<T>(pat[m - 1 - i]) == static_cast<T>(pat[m - 1 - k]) ) {
      tbl[i] = static_cast<kmp_slot>(k + 1);
      ++i;
      ++k;
    } else if ( k ) {
      k = tbl[k - 1];
    } else {
      tbl[i] = 0;
      ++i;
    }
  }
}

template<typename T, typename P>
constexpr usize
kmp_scan(const T *hay, usize n, const P *pat, usize m, const kmp_slot *tbl) noexcept
{
  if ( m == 0 ) return 0;
  if ( m > n ) return n;
  usize k = 0;
  for ( usize i = 0; i < n; ) {
    if ( hay[i] == static_cast<T>(pat[k]) ) {
      ++k;
      ++i;
      if ( k == m ) return i - k;
    } else if ( k ) {
      k = tbl[k - 1];
    } else {
      ++i;
    }
  }
  return n;
}

template<typename T, typename P>
constexpr usize
kmp_rscan(const T *hay, usize n, const P *pat, usize m, const kmp_slot *tbl) noexcept
{
  if ( m == 0 or m > n ) return n;
  usize k = 0;
  for ( usize i = 0; i < n; ) {
    if ( hay[n - 1 - i] == static_cast<T>(pat[m - 1 - k]) ) {
      ++k;
      ++i;
      if ( k == m ) return n - i;
    } else if ( k ) {
      k = tbl[k - 1];
    } else {
      ++i;
    }
  }
  return n;
}

template<typename T, typename P>
constexpr usize
skip_scan(const T *hay, usize n, const P *pat, usize m) noexcept
{
  if ( m == 0 ) return 0;
  if ( m > n ) return n;
  const T lead = static_cast<T>(pat[0]);
  usize i = 0;
  while ( i + m <= n ) {
    const usize hop = scan_find(hay + i, n - i, lead);
    if ( hop == n - i ) return n;
    i += hop;
    if ( i + m > n ) return n;
    usize j = 1;
    for ( ; j < m; ++j )
      if ( !(hay[i + j] == static_cast<T>(pat[j])) ) break;
    if ( j == m ) return i;
    ++i;
  }
  return n;
}

template<typename T, typename P>
constexpr usize
skip_rscan(const T *hay, usize n, const P *pat, usize m) noexcept
{
  if ( m == 0 or m > n ) return n;
  for ( usize i = n - m + 1; i-- > 0; ) {
    usize j = 0;
    for ( ; j < m; ++j )
      if ( !(hay[i + j] == static_cast<T>(pat[j])) ) break;
    if ( j == m ) return i;
  }
  return n;
}

};      // namespace __impl
};      // namespace micron
