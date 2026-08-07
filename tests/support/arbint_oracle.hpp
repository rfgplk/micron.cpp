//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../src/types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// a deliberately stupid bignum, to disagree with micron::math::arbint
//
// fixed-width u32 limbs, schoolbook multiply, and a division that is literally shift-one-bit-and-
// subtract. no reciprocals, no accumulators, no tiers, no shared code with src/ -- if the oracle and
// the implementation agree it is because they are both right, not because they share a mistake.
// this is the same bargain tests/support/ryu_oracle.hpp makes.

namespace mtest
{
namespace arbint_oracle
{

inline constexpr usize obn_limbs = 256;
inline constexpr usize obn_bits = obn_limbs * 32u;

struct obn {
  u32 l[obn_limbs];
};

inline void
obn_zero(obn &a) noexcept
{
  for ( usize i = 0; i < obn_limbs; ++i ) a.l[i] = 0;
}

inline bool
obn_is_zero(const obn &a) noexcept
{
  for ( usize i = 0; i < obn_limbs; ++i )
    if ( a.l[i] != 0 ) return false;
  return true;
}

inline int
obn_cmp(const obn &a, const obn &b) noexcept
{
  for ( usize i = obn_limbs; i-- > 0; )
    if ( a.l[i] != b.l[i] ) return a.l[i] < b.l[i] ? -1 : 1;
  return 0;
}

inline usize
obn_bitlen(const obn &a) noexcept
{
  for ( usize i = obn_limbs; i-- > 0; ) {
    if ( a.l[i] == 0 ) continue;
    usize b = 32;
    while ( b > 0 && ((a.l[i] >> (b - 1u)) & 1u) == 0u ) --b;
    return i * 32u + b;
  }
  return 0;
}

inline bool
obn_bit(const obn &a, usize i) noexcept
{
  if ( i >= obn_bits ) return false;
  return ((a.l[i / 32u] >> (i % 32u)) & 1u) != 0u;
}

inline void
obn_setbit(obn &a, usize i) noexcept
{
  if ( i < obn_bits ) a.l[i / 32u] |= (1u << (i % 32u));
}

inline void
obn_from_u64(obn &a, u64 v) noexcept
{
  obn_zero(a);
  a.l[0] = static_cast<u32>(v);
  a.l[1] = static_cast<u32>(v >> 32);
}

template<typename L>
inline bool
obn_from_limbs(obn &a, const L *p, usize n) noexcept
{
  obn_zero(a);
  constexpr usize per = sizeof(L) / sizeof(u32);
  if ( n * per > obn_limbs ) return false;
  for ( usize i = 0; i < n; ++i )
    for ( usize k = 0; k < per; ++k ) a.l[i * per + k] = static_cast<u32>(p[i] >> (32u * k));
  return true;
}

inline bool
obn_fits_product(const obn &a, const obn &b) noexcept
{
  return obn_bitlen(a) + obn_bitlen(b) <= obn_bits;
}

inline u32
obn_add(obn &r, const obn &a, const obn &b) noexcept
{
  u64 cy = 0;
  for ( usize i = 0; i < obn_limbs; ++i ) {
    const u64 t = static_cast<u64>(a.l[i]) + b.l[i] + cy;
    r.l[i] = static_cast<u32>(t);
    cy = t >> 32;
  }
  return static_cast<u32>(cy);
}

inline u32
obn_sub(obn &r, const obn &a, const obn &b) noexcept
{
  u64 bw = 0;
  for ( usize i = 0; i < obn_limbs; ++i ) {
    const u64 t = static_cast<u64>(a.l[i]) - b.l[i] - bw;
    r.l[i] = static_cast<u32>(t);
    bw = (t >> 32) ? 1u : 0u;
  }
  return static_cast<u32>(bw);
}

inline void
obn_mul(obn &r, const obn &a, const obn &b) noexcept
{
  obn t;
  obn_zero(t);
  for ( usize i = 0; i < obn_limbs; ++i ) {
    if ( a.l[i] == 0 ) continue;
    u64 cy = 0;
    for ( usize j = 0; i + j < obn_limbs; ++j ) {
      const u64 p = static_cast<u64>(a.l[i]) * b.l[j] + t.l[i + j] + cy;
      t.l[i + j] = static_cast<u32>(p);
      cy = p >> 32;
    }
  }
  r = t;
}

inline void
obn_shl1(obn &a) noexcept
{
  u32 carry = 0;
  for ( usize i = 0; i < obn_limbs; ++i ) {
    const u32 w = a.l[i];
    a.l[i] = (w << 1) | carry;
    carry = w >> 31;
  }
}

inline void
obn_shl(obn &r, const obn &a, usize k) noexcept
{
  obn t;
  obn_zero(t);
  const usize bl = obn_bitlen(a);
  for ( usize i = 0; i < bl; ++i )
    if ( obn_bit(a, i) ) obn_setbit(t, i + k);
  r = t;
}

inline void
obn_shr(obn &r, const obn &a, usize k) noexcept
{
  obn t;
  obn_zero(t);
  const usize bl = obn_bitlen(a);
  for ( usize i = k; i < bl; ++i )
    if ( obn_bit(a, i) ) obn_setbit(t, i - k);
  r = t;
}

inline void
obn_divmod(obn &q, obn &rem, const obn &n, const obn &d) noexcept
{
  obn oq, orr;
  obn_zero(oq);
  obn_zero(orr);
  if ( obn_is_zero(d) ) {
    q = oq;
    rem = orr;
    return;
  }
  const usize bl = obn_bitlen(n);
  for ( usize i = bl; i-- > 0; ) {
    obn_shl1(orr);
    if ( obn_bit(n, i) ) orr.l[0] |= 1u;
    if ( obn_cmp(orr, d) >= 0 ) {
      obn t;
      (void)obn_sub(t, orr, d);
      orr = t;
      obn_setbit(oq, i);
    }
  }
  q = oq;
  rem = orr;
}

inline void
obn_and(obn &r, const obn &a, const obn &b) noexcept
{
  for ( usize i = 0; i < obn_limbs; ++i ) r.l[i] = a.l[i] & b.l[i];
}

inline void
obn_ior(obn &r, const obn &a, const obn &b) noexcept
{
  for ( usize i = 0; i < obn_limbs; ++i ) r.l[i] = a.l[i] | b.l[i];
}

inline void
obn_xor(obn &r, const obn &a, const obn &b) noexcept
{
  for ( usize i = 0; i < obn_limbs; ++i ) r.l[i] = a.l[i] ^ b.l[i];
}

inline usize
obn_popcount(const obn &a) noexcept
{
  usize c = 0;
  for ( usize i = 0; i < obn_limbs; ++i )
    for ( usize b = 0; b < 32; ++b )
      if ( ((a.l[i] >> b) & 1u) != 0u ) ++c;
  return c;
}

template<typename L>
inline bool
obn_equals_limbs(const obn &a, const L *p, usize n) noexcept
{
  obn b;
  if ( !obn_from_limbs(b, p, n) ) return false;
  return obn_cmp(a, b) == 0;
}

inline bool
obn_fits_modexp(const obn &m) noexcept
{
  return 2u * obn_bitlen(m) <= obn_bits;
}

inline void
obn_addmod(obn &r, const obn &a, const obn &b, const obn &m) noexcept
{
  obn s, q;
  (void)obn_add(s, a, b);
  obn_divmod(q, r, s, m);
}

inline void
obn_submod(obn &r, const obn &a, const obn &b, const obn &m) noexcept
{
  obn t;
  if ( obn_cmp(a, b) >= 0 ) {
    (void)obn_sub(t, a, b);
    r = t;
    return;
  }
  obn s;
  (void)obn_add(s, a, m);
  (void)obn_sub(t, s, b);
  r = t;
}

inline void
obn_mulmod(obn &r, const obn &a, const obn &b, const obn &m) noexcept
{
  obn p, q;
  obn_mul(p, a, b);
  obn_divmod(q, r, p, m);
}

inline bool
obn_powmod(obn &r, const obn &a, const obn &e, const obn &m) noexcept
{
  if ( !obn_fits_modexp(m) ) return false;

  obn one;
  obn_from_u64(one, 1u);
  if ( obn_cmp(m, one) == 0 ) {
    obn_zero(r);
    return true;
  }

  obn acc, base, q;
  obn_from_u64(acc, 1u);
  obn_divmod(q, base, a, m);

  const usize bl = obn_bitlen(e);
  for ( usize i = 0; i < bl; ++i ) {
    if ( obn_bit(e, i) ) {
      obn t;
      obn_mulmod(t, acc, base, m);
      acc = t;
    }
    if ( i + 1u < bl ) {
      obn t;
      obn_mulmod(t, base, base, m);
      base = t;
    }
  }
  r = acc;
  return true;
}

inline void
obn_gcd(obn &g, const obn &a, const obn &b) noexcept
{
  obn x = a, y = b;
  while ( !obn_is_zero(y) ) {
    obn q, r;
    obn_divmod(q, r, x, y);
    x = y;
    y = r;
  }
  g = x;
}

inline int
obn_invmod(obn &r, const obn &a, const obn &m) noexcept
{
  if ( !obn_fits_modexp(m) ) return -1;

  obn one;
  obn_from_u64(one, 1u);
  if ( obn_cmp(m, one) == 0 ) {
    obn_zero(r);
    return 1;
  }

  obn r0 = m, r1, t0, t1, q;
  obn_zero(t0);
  obn_from_u64(t1, 1u);
  obn_divmod(q, r1, a, m);

  while ( !obn_is_zero(r1) ) {
    obn qq, r2, prod, t2;
    obn_divmod(qq, r2, r0, r1);
    obn_mulmod(prod, qq, t1, m);
    obn_submod(t2, t0, prod, m);
    r0 = r1;
    r1 = r2;
    t0 = t1;
    t1 = t2;
  }
  if ( obn_cmp(r0, one) != 0 ) return 0;
  r = t0;
  return 1;
}

};      // namespace arbint_oracle
};      // namespace mtest
