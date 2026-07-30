//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/types.hpp"

namespace mtest::ryu_oracle
{

inline constexpr usize bn_limbs = 44;
inline constexpr u32 bn_pow5_max = 368;

struct bn {
  u32 l[bn_limbs];
};

inline void
bn_zero(bn &a)
{
  for ( usize i = 0; i < bn_limbs; ++i ) a.l[i] = 0;
}

inline void
bn_from_u64(bn &a, u64 v)
{
  bn_zero(a);
  a.l[0] = static_cast<u32>(v);
  a.l[1] = static_cast<u32>(v >> 32);
}

inline i32
bn_cmp(const bn &a, const bn &b)
{
  for ( usize i = bn_limbs; i-- > 0; ) {
    if ( a.l[i] != b.l[i] ) return a.l[i] < b.l[i] ? -1 : 1;
  }
  return 0;
}

inline bool
bn_is_zero(const bn &a)
{
  for ( usize i = 0; i < bn_limbs; ++i )
    if ( a.l[i] ) return false;
  return true;
}

inline u32
bn_bitlen(const bn &a)
{
  for ( usize i = bn_limbs; i-- > 0; ) {
    if ( a.l[i] ) {
      u32 b = 32;
      u32 v = a.l[i];
      while ( !(v & 0x80000000u) ) {
        v <<= 1;
        --b;
      }
      return static_cast<u32>(i) * 32 + b;
    }
  }
  return 0;
}

inline bool
bn_shl(bn &a, u32 s)
{
  u32 limbs = s / 32, bits = s % 32;
  if ( bn_bitlen(a) + s > bn_limbs * 32 ) return false;
  if ( limbs ) {
    for ( usize i = bn_limbs; i-- > limbs; ) a.l[i] = a.l[i - limbs];
    for ( usize i = 0; i < limbs; ++i ) a.l[i] = 0;
  }
  if ( bits ) {
    u32 carry = 0;
    for ( usize i = limbs; i < bn_limbs; ++i ) {
      u32 nc = a.l[i] >> (32 - bits);
      a.l[i] = (a.l[i] << bits) | carry;
      carry = nc;
    }
    if ( carry ) return false;
  }
  return true;
}

inline bool
bn_mul_u32(bn &a, u32 f)
{
  u64 carry = 0;
  for ( usize i = 0; i < bn_limbs; ++i ) {
    u64 p = static_cast<u64>(a.l[i]) * f + carry;
    a.l[i] = static_cast<u32>(p);
    carry = p >> 32;
  }
  return carry == 0;
}

inline bool
bn_addmul_shifted(bn &acc, const bn &d, u32 f, u32 limb_off)
{
  u64 carry = 0;
  for ( usize i = 0; i + limb_off < bn_limbs; ++i ) {
    u64 p = static_cast<u64>(d.l[i]) * f + acc.l[i + limb_off] + carry;
    acc.l[i + limb_off] = static_cast<u32>(p);
    carry = p >> 32;
  }
  if ( carry ) return false;
  for ( usize i = bn_limbs - limb_off; i < bn_limbs; ++i )
    if ( d.l[i] ) return false;
  return true;
}

inline bool
bn_mul_u128(bn &out, const bn &d, u64 lo, u64 hi)
{
  bn_zero(out);
  const u32 piece[4] = { static_cast<u32>(lo), static_cast<u32>(lo >> 32), static_cast<u32>(hi), static_cast<u32>(hi >> 32) };
  for ( u32 k = 0; k < 4; ++k )
    if ( piece[k] && !bn_addmul_shifted(out, d, piece[k], k) ) return false;
  return true;
}

inline bool
bn_sub(bn &a, const bn &b)
{
  u64 borrow = 0;
  for ( usize i = 0; i < bn_limbs; ++i ) {
    u64 d = static_cast<u64>(a.l[i]) - b.l[i] - borrow;
    a.l[i] = static_cast<u32>(d);
    borrow = (d >> 63) & 1;
  }
  return borrow == 0;
}

inline const bn &
bn_pow5(u32 q)
{
  static bn tbl[bn_pow5_max + 1];
  static bool ready = false;
  if ( !ready ) {
    bn_from_u64(tbl[0], 1);
    for ( u32 k = 1; k <= bn_pow5_max; ++k ) {
      tbl[k] = tbl[k - 1];
      bn_mul_u32(tbl[k], 5);
    }
    ready = true;
  }
  return tbl[q > bn_pow5_max ? 0 : q];
}

inline bool
fwd_entry_exact(u32 i, u64 lo, u64 hi)
{
  if ( i > 325 ) return false;
  const bn &p = bn_pow5(i);
  u32 b = bn_bitlen(p);
  if ( b <= 125 ) {
    bn e;
    bn_from_u64(e, lo);
    bn t;
    bn_from_u64(t, hi);
    if ( !bn_shl(t, 64) ) return false;
    for ( usize k = 0; k < bn_limbs; ++k ) e.l[k] |= t.l[k];
    bn q = p;
    if ( !bn_shl(q, 125 - b) ) return false;
    return bn_cmp(e, q) == 0;
  }

  u32 s = b - 125;
  bn one;
  bn_from_u64(one, 1);
  bn E;
  if ( !bn_mul_u128(E, one, lo, hi) || !bn_shl(E, s) ) return false;
  if ( bn_cmp(E, p) > 0 ) return false;
  u64 lo1 = lo + 1, hi1 = hi + (lo1 == 0 ? 1 : 0);
  bn E1;
  if ( !bn_mul_u128(E1, one, lo1, hi1) || !bn_shl(E1, s) ) return false;
  return bn_cmp(p, E1) < 0;
}

inline bool
inv_entry_exact(u32 q, u64 lo, u64 hi)
{
  if ( q > 341 ) return false;
  const bn &p = bn_pow5(q);
  u32 b = bn_bitlen(p);
  bn N;
  bn_from_u64(N, 1);
  if ( !bn_shl(N, b - 1 + 125) ) return false;

  u64 lom = lo - 1, him = hi - (lo == 0 ? 1 : 0);
  bn A, B;
  if ( !bn_mul_u128(A, p, lom, him) || !bn_mul_u128(B, p, lo, hi) ) return false;
  return bn_cmp(A, N) <= 0 && bn_cmp(N, B) < 0;
}

struct fbits {
  u64 mfull;
  i32 ef;
  u32 mm_shift;
  bool even;
  bool neg;
  bool zero;
  bool special;
};

inline fbits
decompose64(u64 bits)
{
  fbits v{};
  u64 M = bits & ((1ull << 52) - 1);
  u32 E = static_cast<u32>((bits >> 52) & 0x7FF);
  v.neg = (bits >> 63) != 0;
  v.special = (E == 0x7FF);
  v.zero = (E == 0 && M == 0);
  v.ef = (E == 0) ? (1 - 1023 - 52) : (static_cast<i32>(E) - 1023 - 52);
  v.mfull = (E == 0) ? M : ((1ull << 52) | M);
  v.mm_shift = (M != 0 || E <= 1) ? 1 : 0;
  v.even = (v.mfull & 1) == 0;
  return v;
}

inline fbits
decompose32(u32 bits)
{
  fbits v{};
  u32 M = bits & ((1u << 23) - 1);
  u32 E = (bits >> 23) & 0xFF;
  v.neg = (bits >> 31) != 0;
  v.special = (E == 0xFF);
  v.zero = (E == 0 && M == 0);
  v.ef = (E == 0) ? (1 - 127 - 23) : (static_cast<i32>(E) - 127 - 23);
  v.mfull = (E == 0) ? M : ((1u << 23) | M);
  v.mm_shift = (M != 0 || E <= 1) ? 1 : 0;
  v.even = (v.mfull & 1) == 0;
  return v;
}

inline i32
__max3(i32 a, i32 b, i32 c)
{
  i32 m = a > b ? a : b;
  return m > c ? m : c;
}

inline bool
__scale_cand(bn &A, u64 m, i32 e10, i32 P5, i32 P2)
{
  if ( e10 + P5 < 0 || static_cast<u32>(e10 + P5) > bn_pow5_max ) return false;
  if ( !bn_mul_u128(A, bn_pow5(static_cast<u32>(e10 + P5)), m, 0) ) return false;
  return bn_shl(A, static_cast<u32>(e10 + P2));
}

inline bool
__scale_bound(bn &B, u64 K, i32 eq, i32 P5, i32 P2)
{
  if ( !bn_mul_u128(B, bn_pow5(static_cast<u32>(P5)), K, 0) ) return false;
  return bn_shl(B, static_cast<u32>(eq + P2));
}

inline bool
decodes_to(u64 m, i32 e10, const fbits &v)
{
  if ( v.special || v.zero || m == 0 ) return false;
  i32 eq = v.ef - 2;
  u64 KL = 4 * v.mfull - 1 - v.mm_shift;
  u64 KH = 4 * v.mfull + 2;
  i32 P5 = e10 < 0 ? -e10 : 0;
  i32 P2 = __max3(0, -e10, -eq);
  bn A, BL, BH;
  if ( !__scale_cand(A, m, e10, P5, P2) ) return false;
  if ( !__scale_bound(BL, KL, eq, P5, P2) ) return false;
  if ( !__scale_bound(BH, KH, eq, P5, P2) ) return false;
  i32 cl = bn_cmp(A, BL), ch = bn_cmp(A, BH);
  return v.even ? (cl >= 0 && ch <= 0) : (cl > 0 && ch < 0);
}

inline bool
is_shortest(u64 m, i32 e10, const fbits &v)
{
  if ( m < 10 ) return true;
  u64 md = m / 10;
  if ( decodes_to(md, e10 + 1, v) ) return false;
  if ( decodes_to(md + 1, e10 + 1, v) ) return false;
  return true;
}

inline bool
is_closest(u64 m, i32 e10, const fbits &v)
{
  if ( v.special || v.zero || m == 0 ) return false;
  i32 eq = v.ef - 2;
  i32 P5 = e10 < 0 ? -e10 : 0;
  i32 P2 = __max3(0, -e10, -eq);
  bn V;
  if ( !__scale_bound(V, 4 * v.mfull, eq, P5, P2) ) return false;
  bn dm;
  {
    bn A;
    if ( !__scale_cand(A, m, e10, P5, P2) ) return false;
    bn t = V;
    if ( bn_cmp(A, V) >= 0 ) {
      bn_sub(A, V);
      dm = A;
    } else {
      bn_sub(t, A);
      dm = t;
    }
  }
  const u64 nb[2] = { m - 1, m + 1 };
  for ( u32 k = 0; k < 2; ++k ) {
    if ( nb[k] == 0 || !decodes_to(nb[k], e10, v) ) continue;
    bn A;
    if ( !__scale_cand(A, nb[k], e10, P5, P2) ) return false;
    bn t = V;
    bn d;
    if ( bn_cmp(A, V) >= 0 ) {
      bn_sub(A, V);
      d = A;
    } else {
      bn_sub(t, A);
      d = t;
    }
    if ( bn_cmp(d, dm) < 0 ) return false;
  }
  return true;
}

struct parsed {
  bool ok;
  bool special;
  bool neg;
  u64 m;
  i32 e10;
  u32 sig_digits;
};

inline parsed
parse_text(const char *s, usize n)
{
  parsed r{};
  usize i = 0;
  if ( i < n && s[i] == '-' ) {
    r.neg = true;
    ++i;
  }
  if ( i < n && (s[i] == 'N' || s[i] == 'I') ) {
    r.ok = true;
    r.special = true;
    return r;
  }
  u64 m = 0;
  u32 digs = 0;
  i32 frac = 0;
  bool seen_digit = false, seen_point = false;
  for ( ; i < n; ++i ) {
    char c = s[i];
    if ( c >= '0' && c <= '9' ) {
      seen_digit = true;
      if ( seen_point ) ++frac;
      if ( m != 0 || c != '0' ) {
        if ( digs >= 19 ) return r;
        m = m * 10 + static_cast<u64>(c - '0');
        ++digs;
      }
    } else if ( c == '.' ) {
      if ( seen_point ) return r;
      seen_point = true;
    } else if ( c == 'e' || c == 'E' ) {
      ++i;
      break;
    } else {
      return r;
    }
  }
  i32 ev = 0;
  bool eneg = false;
  if ( i < n ) {
    if ( s[i] == '-' ) {
      eneg = true;
      ++i;
    } else if ( s[i] == '+' )
      ++i;
    if ( i >= n ) return r;
    for ( ; i < n; ++i ) {
      if ( s[i] < '0' || s[i] > '9' ) return r;
      ev = ev * 10 + (s[i] - '0');
    }
    if ( eneg ) ev = -ev;
  }
  if ( !seen_digit ) return r;
  r.e10 = ev - frac;
  while ( m != 0 && m % 10 == 0 ) {
    m /= 10;
    ++r.e10;
    --digs;
  }
  r.m = m;
  r.sig_digits = digs;
  r.ok = true;
  return r;
}

};      // namespace mtest::ryu_oracle
