//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/types.hpp"

// An INDEPENDENT exact-decimal oracle for the float -> text writers.

namespace mtest
{
namespace exact_decimal
{

constexpr static const u32 LIMBS = 128;
constexpr static const u32 DIGCAP = 1200;

struct big {
  u32 l[LIMBS] = {};
  u32 n = 0;
};

inline void
set_u64(big &b, u64 v)
{
  b.n = 0;
  for ( u32 i = 0; i < LIMBS; ++i ) b.l[i] = 0;
  while ( v > 0 ) {
    b.l[b.n++] = static_cast<u32>(v % 1000000000ull);
    v /= 1000000000ull;
  }
}

inline void
mul_small(big &b, u32 m)
{
  u64 carry = 0;
  for ( u32 i = 0; i < b.n; ++i ) {
    const u64 cur = static_cast<u64>(b.l[i]) * m + carry;
    b.l[i] = static_cast<u32>(cur % 1000000000ull);
    carry = cur / 1000000000ull;
  }
  while ( carry > 0 ) {
    b.l[b.n++] = static_cast<u32>(carry % 1000000000ull);
    carry /= 1000000000ull;
  }
}

inline u32
to_digits(const big &b, char *out)
{
  if ( b.n == 0 ) {
    out[0] = '0';
    return 1;
  }
  u32 pos = 0;
  u32 top = b.l[b.n - 1];
  char tmp[10];
  u32 t = 0;
  if ( top == 0 )
    tmp[t++] = '0';
  else
    while ( top > 0 ) {
      tmp[t++] = static_cast<char>('0' + top % 10);
      top /= 10;
    }
  while ( t-- > 0 ) out[pos++] = tmp[t];
  for ( u32 i = b.n - 1; i-- > 0; )
    for ( i32 d = 8; d >= 0; --d ) {
      u32 p = 1;
      for ( i32 q = 0; q < d; ++q ) p *= 10;
      out[pos++] = static_cast<char>('0' + (b.l[i] / p) % 10);
    }
  return pos;
}

inline void
expand(u64 m2, i32 e2, char *digits, u32 &ndig, u32 &pt_from_right)
{
  big b;
  set_u64(b, m2);
  if ( e2 >= 0 ) {
    i32 k = e2;
    while ( k >= 29 ) {
      mul_small(b, 1u << 29);
      k -= 29;
    }
    if ( k > 0 ) mul_small(b, 1u << k);
    pt_from_right = 0;
  } else {
    i32 k = -e2;
    pt_from_right = static_cast<u32>(k);
    while ( k >= 13 ) {
      mul_small(b, 1220703125u);
      k -= 13;
    }
    u32 rem = 1;
    for ( i32 q = 0; q < k; ++q ) rem *= 5u;
    if ( rem > 1 ) mul_small(b, rem);
  }
  ndig = to_digits(b, digits);
}

inline void
round_keep(char *d, u32 &n, u32 keep, bool &carried)
{
  carried = false;
  if ( keep >= n ) return;

  bool up;
  const char first = d[keep];
  if ( first > '5' )
    up = true;
  else if ( first < '5' )
    up = false;
  else {
    bool rest = false;
    for ( u32 i = keep + 1; i < n; ++i )
      if ( d[i] != '0' ) {
        rest = true;
        break;
      }
    if ( rest )
      up = true;
    else
      up = (keep > 0) && (((d[keep - 1] - '0') & 1) != 0);
  }

  n = keep;
  if ( !up ) return;
  for ( u32 i = keep; i-- > 0; ) {
    if ( d[i] != '9' ) {
      ++d[i];
      return;
    }
    d[i] = '0';
  }

  for ( u32 i = n; i > 0; --i ) d[i] = d[i - 1];
  d[0] = '1';
  n = keep + 1;
  carried = true;
}

inline usize
fixed(u64 m2, i32 e2, bool neg, u32 prec, char *out)
{
  char d[DIGCAP];
  u32 n = 0, k = 0;
  usize pos = 0;

  if ( m2 == 0 ) {
    if ( neg ) out[pos++] = '-';
    out[pos++] = '0';
    if ( prec > 0 ) {
      out[pos++] = '.';
      for ( u32 i = 0; i < prec; ++i ) out[pos++] = '0';
    }
    return pos;
  }

  expand(m2, e2, d, n, k);

  if ( k > prec ) {
    const u32 drop = k - prec;
    if ( drop > n ) {

      n = 0;
    } else {
      bool carried = false;
      round_keep(d, n, n - drop, carried);
    }
  } else {
    for ( u32 i = 0; i < prec - k; ++i ) d[n++] = '0';
  }

  if ( neg ) out[pos++] = '-';
  if ( n > prec ) {
    for ( u32 i = 0; i < n - prec; ++i ) out[pos++] = d[i];
  } else {
    out[pos++] = '0';
  }
  if ( prec > 0 ) {
    out[pos++] = '.';
    const u32 have = (n > prec) ? prec : n;
    for ( u32 i = 0; i < prec - have; ++i ) out[pos++] = '0';
    for ( u32 i = n - have; i < n; ++i ) out[pos++] = d[i];
  }
  return pos;
}

inline usize
sci(u64 m2, i32 e2, bool neg, u32 prec, char *out)
{
  char d[DIGCAP];
  u32 n = 0, k = 0;
  usize pos = 0;
  i32 sci_exp = 0;

  if ( m2 == 0 ) {
    if ( neg ) out[pos++] = '-';
    out[pos++] = '0';
    if ( prec > 0 ) {
      out[pos++] = '.';
      for ( u32 i = 0; i < prec; ++i ) out[pos++] = '0';
    }
    out[pos++] = 'e';
    out[pos++] = '+';
    out[pos++] = '0';
    out[pos++] = '0';
    return pos;
  }

  expand(m2, e2, d, n, k);
  sci_exp = static_cast<i32>(n) - static_cast<i32>(k) - 1;

  bool carried = false;
  round_keep(d, n, prec + 1, carried);
  if ( carried ) ++sci_exp;

  if ( neg ) out[pos++] = '-';
  out[pos++] = d[0];
  if ( prec > 0 ) {
    out[pos++] = '.';
    for ( u32 j = 1; j <= prec; ++j ) out[pos++] = (j < n) ? d[j] : '0';
  }
  out[pos++] = 'e';
  out[pos++] = (sci_exp < 0) ? '-' : '+';
  u32 ae = static_cast<u32>(sci_exp < 0 ? -sci_exp : sci_exp);
  if ( ae >= 100 ) out[pos++] = static_cast<char>('0' + ae / 100);
  out[pos++] = static_cast<char>('0' + (ae / 10) % 10);
  out[pos++] = static_cast<char>('0' + ae % 10);
  return pos;
}

};      // namespace exact_decimal
};      // namespace mtest
