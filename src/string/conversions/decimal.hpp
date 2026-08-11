//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// exact base-10 shift-and-round bignum

namespace micron
{
namespace __impl
{
namespace __dec
{

// %%%%%%%%%%%%%%%%%%%%%%%%%
// capacity
//
// | format    | p   | e2_min | exact significant digits | max integer digits |
// |-----------|-----|--------|--------------------------|--------------------|
// | binary64  |  53 |  -1074 | 15.95 +   750.7 =    767 |                309 |
// | x87-80    |  64 | -16445 | 19.27 + 11494.6 =  11514 |               4933 |
// | binary128 | 113 | -16494 | 34.02 + 11528.9 =  11563 |               4933 |
constexpr static const u32 __dec_cap = 800;
constexpr static const u32 __dec_shift_max = 32;

// value == 0.d[0]d[1].. * 10^point
template<u32 Cap> struct __decimal_t {
  u32 ndigits = 0;
  i32 point = 0;
  bool truncated = false;
  u8 d[Cap] = {};
};

using __decimal = __decimal_t<__dec_cap>;

template<u32 Cap>
inline constexpr void
__dec_trim(__decimal_t<Cap> &v) noexcept
{
  while ( v.ndigits > 0 && v.d[v.ndigits - 1] == 0 ) --v.ndigits;
  if ( v.ndigits == 0 ) v.point = 0;
}

// + 64 slack is correct for any Cap so long as __dec_shift_max == 32
template<u32 Cap>
inline constexpr void
__dec_shift_left(__decimal_t<Cap> &v, u32 k) noexcept
{
  if ( v.ndigits == 0 ) return;
  u8 scratch[Cap + 64];
  if consteval {
    for ( u32 i = 0; i < static_cast<u32>(sizeof(scratch)); ++i ) scratch[i] = 0;
  }
  u32 w = static_cast<u32>(sizeof(scratch));
  u64 n = 0;
  for ( u32 r = v.ndigits; r-- > 0; ) {
    n += static_cast<u64>(v.d[r]) << k;
    u64 quo = n / 10;
    scratch[--w] = static_cast<u8>(n - quo * 10);
    n = quo;
  }
  while ( n > 0 ) {
    u64 quo = n / 10;
    scratch[--w] = static_cast<u8>(n - quo * 10);
    n = quo;
  }
  u32 nd = static_cast<u32>(sizeof(scratch)) - w;
  v.point += static_cast<i32>(nd) - static_cast<i32>(v.ndigits);
  if ( nd > Cap ) {
    for ( u32 i = Cap; i < nd; ++i )
      if ( scratch[w + i] != 0 ) {
        v.truncated = true;
        break;
      }
    nd = Cap;
  }
  for ( u32 i = 0; i < nd; ++i ) v.d[i] = scratch[w + i];
  v.ndigits = nd;
  __dec_trim(v);
}

template<u32 Cap>
inline constexpr void
__dec_shift_right(__decimal_t<Cap> &v, u32 k) noexcept
{
  u32 r = 0;
  u32 w = 0;
  u64 n = 0;

  for ( ; (n >> k) == 0; ++r ) {
    if ( r >= v.ndigits ) {
      if ( n == 0 ) {
        v.ndigits = 0;
        v.point = 0;
        return;
      }
      while ( (n >> k) == 0 ) {
        n *= 10;
        ++r;
      }
      break;
    }
    n = n * 10 + v.d[r];
  }
  v.point -= static_cast<i32>(r) - 1;

  const u64 mask = (1ull << k) - 1;
  for ( ; r < v.ndigits; ++r ) {
    u64 dig = n >> k;
    n &= mask;
    v.d[w++] = static_cast<u8>(dig);
    n = n * 10 + v.d[r];
  }
  while ( n > 0 ) {
    u64 dig = n >> k;
    n &= mask;
    if ( w < Cap )
      v.d[w++] = static_cast<u8>(dig);
    else if ( dig > 0 )
      v.truncated = true;
    n *= 10;
  }
  v.ndigits = w;
  __dec_trim(v);
}

template<u32 Cap>
inline constexpr void
__dec_shift(__decimal_t<Cap> &v, i32 k) noexcept
{
  while ( k > 0 ) {
    u32 s = (static_cast<u32>(k) > __dec_shift_max) ? __dec_shift_max : static_cast<u32>(k);
    __dec_shift_left(v, s);
    k -= static_cast<i32>(s);
  }
  while ( k < 0 ) {
    u32 s = (static_cast<u32>(-k) > __dec_shift_max) ? __dec_shift_max : static_cast<u32>(-k);
    __dec_shift_right(v, s);
    k += static_cast<i32>(s);
  }
}

// round-to-nearest, ties-to-even, decided on the digit string itself
template<u32 Cap>
inline constexpr bool
__dec_round_up(const __decimal_t<Cap> &v, u32 nd) noexcept
{
  if ( nd >= v.ndigits ) return false;
  if ( v.d[nd] == 5 && nd + 1 == v.ndigits ) {
    if ( v.truncated ) return true;
    return nd > 0 && (v.d[nd - 1] & 1) != 0;
  }
  return v.d[nd] >= 5;
}

template<u32 Cap>
inline constexpr void
__dec_load_u64(__decimal_t<Cap> &v, u64 m) noexcept
{
  v.ndigits = 0;
  v.point = 0;
  v.truncated = false;
  if ( m == 0 ) return;

  u8 tmp[20] = {};
  u32 n = 0;
  while ( m > 0 ) {
    u64 quo = m / 10;
    tmp[n++] = static_cast<u8>(m - quo * 10);
    m = quo;
  }
  for ( u32 i = 0; i < n; ++i ) v.d[i] = tmp[n - 1 - i];
  v.ndigits = n;
  v.point = static_cast<i32>(n);
  __dec_trim(v);
}

};      // namespace __dec
};      // namespace __impl
};      // namespace micron
