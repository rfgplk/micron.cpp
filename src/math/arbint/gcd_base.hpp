//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// gcd leaves
//
// semantics match micron::math::gcd<T>
//   gcd(0, 0) == 0        gcd(0, x) == x        gcd(x, 0) == x        gcd(x, x) == x
//
// WARNING: mpn::rshift and lshift are strictly 0 < cnt < limb_bits

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arbitrary distance shifts

// {p, n} >>= z in place, returning the normalized new length.
[[nodiscard, gnu::flatten]] inline constexpr usize
__rshift_bits(limb_t *p, usize n, usize z) noexcept
{
  const usize w = z / limb_bits;
  const usize b = z % limb_bits;
  if ( w >= n ) return 0;
  if ( w != 0 ) {
    copyi(p, p + w, n - w);      // ascending
    n -= w;
  }
  if ( b != 0 ) (void)rshift(p, p, n, b);
  return normalize(p, n);
}

// {rp, ...} = {ap, n} << z
[[nodiscard, gnu::flatten]] inline constexpr usize
__lshift_bits(limb_t *rp, const limb_t *ap, usize n, usize z) noexcept
{
  if ( n == 0 ) return 0;
  const usize w = z / limb_bits;
  const usize b = z % limb_bits;
  usize len = n + w;
  if ( b != 0 ) {
    const limb_t hi = lshift(rp + w, ap, n, b);
    if ( hi != 0 ) {
      rp[len] = hi;
      ++len;
    }
  } else {
    copyd(rp + w, ap, n);      // descending
  }
  if ( w != 0 ) zero(rp, w);
  return normalize(rp, len);
}

// {h, l} >>= c, for 0 < c < limb_bits
[[gnu::always_inline]] inline constexpr void
__rsh2(limb_t &h, limb_t &l, usize c) noexcept
{
  l = static_cast<limb_t>((l >> c) | (h << (limb_bits - c)));
  h = static_cast<limb_t>(h >> c);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// one limb

// gcd of two odd, nonzero limbs; subtractive binary
[[nodiscard, gnu::flatten]] inline constexpr limb_t
gcd_11(limb_t u, limb_t v) noexcept
{
  while ( u != v ) {
    const limb_t d = static_cast<limb_t>(u - v);      // wraps when u < v
    const limb_t mask = static_cast<limb_t>(static_cast<limb_t>(0) - static_cast<limb_t>(u < v));
    v = static_cast<limb_t>(v + (mask & d));         // v <- min(u, v)
    u = static_cast<limb_t>((d ^ mask) - mask);      // u <- |u - v|, nonzero and even
    u = static_cast<limb_t>(u >> limb_ctz(u));       // odd again
  }
  return u;
}

// arbitrary limbs, zero and even
[[nodiscard, gnu::flatten]] inline constexpr limb_t
gcd_11_any(limb_t u, limb_t v) noexcept
{
  if ( u == 0 ) return v;
  if ( v == 0 ) return u;
  const usize zu = limb_ctz(u);
  const usize zv = limb_ctz(v);
  const usize k = zu < zv ? zu : zv;
  return static_cast<limb_t>(gcd_11(static_cast<limb_t>(u >> zu), static_cast<limb_t>(v >> zv)) << k);
}

// gcd of a multilimb value and one limb
[[nodiscard, gnu::flatten]] inline constexpr limb_t
gcd_1(const limb_t *up, usize un, limb_t v) noexcept
{
  un = normalize(up, un);
  if ( un == 0 ) return v;
  return gcd_11_any(v, mod_1(up, un, v));
}

// %%%%%%%%%%%%%%%%%%%%%%
// two limbs

[[gnu::flatten]] inline constexpr void
gcd_22(limb_t &g1, limb_t &g0, limb_t u1, limb_t u0, limb_t v1, limb_t v0) noexcept
{
  while ( (u1 | v1) != 0 ) {
    limb_t t1 = 0, t0 = 0;
    if ( u1 > v1 || (u1 == v1 && u0 >= v0) ) {
      sub2(t1, t0, u1, u0, v1, v0);
    } else {
      sub2(t1, t0, v1, v0, u1, u0);
      v1 = u1;
      v0 = u0;      // v <- min
    }
    if ( (t1 | t0) == 0 ) {
      g1 = v1;
      g0 = v0;
      return;
    }
    if ( t0 == 0 ) {
      t0 = static_cast<limb_t>(t1 >> limb_ctz(t1));
      t1 = 0;
    } else {
      __rsh2(t1, t0, limb_ctz(t0));
    }
    u1 = t1;
    u0 = t0;
  }
  g1 = 0;
  g0 = gcd_11(u0, v0);
}

[[gnu::flatten]] inline constexpr void
gcd_22_any(limb_t &g1, limb_t &g0, limb_t u1, limb_t u0, limb_t v1, limb_t v0) noexcept
{
  if ( (u1 | u0) == 0 ) {
    g1 = v1;
    g0 = v0;
    return;
  }
  if ( (v1 | v0) == 0 ) {
    g1 = u1;
    g0 = u0;
    return;
  }
  const usize zu = (u0 != 0) ? limb_ctz(u0) : limb_bits + limb_ctz(u1);
  const usize zv = (v0 != 0) ? limb_ctz(v0) : limb_bits + limb_ctz(v1);
  const usize k = zu < zv ? zu : zv;

  if ( zu >= limb_bits ) {
    u0 = static_cast<limb_t>(u1 >> (zu - limb_bits));
    u1 = 0;
  } else if ( zu != 0 )
    __rsh2(u1, u0, zu);
  if ( zv >= limb_bits ) {
    v0 = static_cast<limb_t>(v1 >> (zv - limb_bits));
    v1 = 0;
  } else if ( zv != 0 )
    __rsh2(v1, v0, zv);

  gcd_22(g1, g0, u1, u0, v1, v0);

  if ( k >= limb_bits ) {
    g1 = static_cast<limb_t>(g0 << (k - limb_bits));
    g0 = 0;
  } else if ( k != 0 ) {
    g1 = static_cast<limb_t>((g1 << k) | (g0 >> (limb_bits - k)));
    g0 = static_cast<limb_t>(g0 << k);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// binary (Stein)

[[nodiscard, gnu::always_inline]] inline constexpr usize
gcd_binary_itch(usize un, usize vn) noexcept
{
  return un + vn;
}

[[nodiscard]] inline constexpr usize
gcd_binary(limb_t *gp, const limb_t *up, usize un, const limb_t *vp, usize vn, limb_t *scratch) noexcept
{
  limb_t *a = scratch;           // un
  limb_t *b = scratch + un;      // vn
  usize an = un, bn = vn;
  copyi(a, up, un);
  copyi(b, vp, vn);

  const usize za = scan1(a, an);
  const usize zb = scan1(b, bn);
  const usize k = za < zb ? za : zb;
  an = __rshift_bits(a, an, za);
  bn = __rshift_bits(b, bn, zb);

  limb_t g1 = 0, g0 = 0;
  usize gn = 0;
  const limb_t *gsrc = nullptr;

  for ( ;; ) {
    if ( bn == 0 ) {
      gsrc = a;
      gn = an;
      break;
    }
    if ( an == 0 ) {
      gsrc = b;
      gn = bn;
      break;
    }
    if ( an <= 2 && bn <= 2 ) {
      gcd_22_any(g1, g0, an > 1 ? a[1] : 0, a[0], bn > 1 ? b[1] : 0, b[0]);
      a[0] = g0;
      a[1] = g1;
      gsrc = a;
      gn = normalize(a, 2);
      break;
    }
    if ( bn == 1 ) {
      a[0] = gcd_1(a, an, b[0]);
      gsrc = a;
      gn = a[0] != 0 ? 1u : 0u;
      break;
    }
    const int c = cmp_var(a, an, b, bn);
    if ( c == 0 ) {
      gsrc = a;
      gn = an;
      break;
    }
    if ( c < 0 ) {
      limb_t *t = a;
      a = b;
      b = t;
      const usize tn = an;
      an = bn;
      bn = tn;
    }
    (void)sub(a, a, an, b, bn);
    an = normalize(a, an);
    an = __rshift_bits(a, an, scan1(a, an));
  }

  if ( gn == 0 ) return 0;
  return __lshift_bits(gp, gsrc, gn, k);
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
