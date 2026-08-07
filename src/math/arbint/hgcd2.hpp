//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div.hpp"
#include "gcd_base.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// single-limb step
//
//     ( a )   ( u00  u01 ) ( a' )               ( a' )   (  u11  -u01 ) ( a )
//     ( b ) = ( u10  u11 ) ( b' )      and      ( b' ) = ( -u10   u00 ) ( b )
//
// det M == 1 ALWAYS: every step right-multiplies by (1 q; 0 1) or (1 0; q 1)
//
// WARNING: u00 and u11 multiply the operand of the __same__ index, and u01 and u10 cross over and are the ones that get __subbed__
//
//     a' = u11*a - u01*b        b' = u00*b - u10*a
//
// validity test via Knuth 4.5.2 Algorithm L

namespace micron
{
namespace math
{
namespace mpn
{

struct mat1 {
  limb_t u00, u01, u10, u11;
};

[[nodiscard, gnu::always_inline]] inline constexpr mat1
mat1_identity() noexcept
{
  return mat1{ 1, 0, 0, 1 };
}

// simulate as many Euclidean steps as the leading limbs determine
[[nodiscard, gnu::flatten]] inline constexpr bool
hgcd2(mat1 &m, limb_t A, limb_t B) noexcept
{
  m = mat1_identity();
  if ( B == 0 ) return false;

  usize steps = 0;
  for ( ;; ) {
    // reduce A by B
    // (b's lower bound must be strictly positive or nothing can be divided by it)
    if ( B <= m.u10 ) break;

    // WARNING: UPPER BOUNDS CAN CARRY OUT OF A LIMB; SIGFPE
    const limb_t bhi = static_cast<limb_t>(B + m.u00);
    if ( bhi < B ) break;
    const limb_t q1 = static_cast<limb_t>((A - m.u01) / bhi);
    if ( q1 == 0 ) break;
    const limb_t ahi = static_cast<limb_t>(A + m.u11);
    if ( ahi < A ) break;
    if ( q1 != static_cast<limb_t>(ahi / (B - m.u10)) ) break;
    m.u01 = static_cast<limb_t>(m.u01 + q1 * m.u00);
    m.u11 = static_cast<limb_t>(m.u11 + q1 * m.u10);
    A = static_cast<limb_t>(A - q1 * B);
    ++steps;

    // and now reduce B by A, with the roles of the two bound pairs exchanged
    if ( A <= m.u01 ) break;
    const limb_t ahi2 = static_cast<limb_t>(A + m.u11);
    if ( ahi2 < A ) break;
    const limb_t q2 = static_cast<limb_t>((B - m.u10) / ahi2);
    if ( q2 == 0 ) break;
    const limb_t bhi2 = static_cast<limb_t>(B + m.u00);
    if ( bhi2 < B ) break;
    if ( q2 != static_cast<limb_t>(bhi2 / (A - m.u01)) ) break;
    m.u00 = static_cast<limb_t>(m.u00 + q2 * m.u01);
    m.u10 = static_cast<limb_t>(m.u10 + q2 * m.u11);
    B = static_cast<limb_t>(B - q2 * A);
    ++steps;
  }
  return steps != 0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// two-limb window
//
// (on Zen+)
// leaf                          cyc/call   bits/call   cyc/bit
// one limb   (Knuth)               622        28.6       21.8
// two limbs  (Jebelean)           2981        60.6       49.2
//
// validity test via Jebelean's (A double-digit Lehmer-Euclid algorithm for finding the GCD of
// long integers, J. Symbolic Comput. 15, 1993)
[[nodiscard, gnu::flatten]] inline constexpr limb_t
__div2(limb_t &ah, limb_t &al, limb_t bh, limb_t bl) noexcept
{
  const usize ca = limb_clz(ah);
  const usize cb = limb_clz(bh);
  if ( cb < ca ) return 0;
  const usize d = cb - ca;

  limb_t sh = bh, sl = bl;
  if ( d != 0 ) {
    sh = static_cast<limb_t>((bh << d) | (bl >> (limb_bits - d)));
    sl = static_cast<limb_t>(bl << d);
  }

  limb_t q = 0;
  for ( usize i = d;; --i ) {
    limb_t lo = 0, hi = 0;
    const limb_t bw = subb(ah, sh, subb(al, sl, 0, lo), hi);
    const limb_t mask = static_cast<limb_t>(bw - 1u);      // all ones when A >= S, zero when below

    ah = static_cast<limb_t>((hi & mask) | (ah & ~mask));
    al = static_cast<limb_t>((lo & mask) | (al & ~mask));
    q = static_cast<limb_t>(q | (mask & (static_cast<limb_t>(1) << i)));

    if ( i == 0 ) break;
    sl = static_cast<limb_t>((sl >> 1) | (sh << (limb_bits - 1u)));
    sh = static_cast<limb_t>(sh >> 1);
  }
  return q;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
__mat_step(limb_t &out, limb_t cur, limb_t q, limb_t mult) noexcept
{
  limb_t hi = 0, lo = 0;
  mul_wide(q, mult, lo, hi);
  if ( hi != 0 ) return false;
  const limb_t s = static_cast<limb_t>(cur + lo);
  if ( s < cur ) return false;
  out = s;
  return true;
}

// Jebelean's test
[[nodiscard, gnu::always_inline]] inline constexpr bool
__w2_valid(limb_t x1, limb_t x0, limb_t y1, limb_t y0, limb_t bound, limb_t e0, limb_t e1) noexcept
{
  if ( y1 == 0 && y0 < bound ) return false;

  limb_t d1 = 0, d0 = 0;
  sub2(d1, d0, x1, x0, y1, y0);

  const limb_t s0 = static_cast<limb_t>(e0 + e1);
  const limb_t s1 = static_cast<limb_t>(s0 < e0 ? 1u : 0u);
  return d1 > s1 || (d1 == s1 && d0 >= s0);
}

[[nodiscard, gnu::flatten]] inline constexpr bool
hgcd2_w2(mat1 &m, limb_t ah, limb_t al, limb_t bh, limb_t bl) noexcept
{
  m = mat1_identity();

  usize steps = 0;
  for ( ;; ) {
    if ( bh == 0 ) break;
    limb_t ra = ah, ra0 = al;
    const limb_t q1 = __div2(ra, ra0, bh, bl);
    if ( q1 == 0 ) break;

    limb_t u01n = 0, u11n = 0;
    if ( !__mat_step(u01n, m.u01, q1, m.u00) ) break;
    if ( !__mat_step(u11n, m.u11, q1, m.u10) ) break;
    if ( !__w2_valid(bh, bl, ra, ra0, u01n, m.u10, u11n) ) break;

    m.u01 = u01n;
    m.u11 = u11n;
    ah = ra;
    al = ra0;
    ++steps;

    if ( ah == 0 ) break;
    limb_t rb = bh, rb0 = bl;
    const limb_t q2 = __div2(rb, rb0, ah, al);
    if ( q2 == 0 ) break;

    limb_t u00n = 0, u10n = 0;
    if ( !__mat_step(u00n, m.u00, q2, m.u01) ) break;
    if ( !__mat_step(u10n, m.u10, q2, m.u11) ) break;
    if ( !__w2_valid(ah, al, rb, rb0, u10n, u00n, m.u01) ) break;

    m.u00 = u00n;
    m.u10 = u10n;
    bh = rb;
    bl = rb0;
    ++steps;
  }
  return steps != 0;
}

[[gnu::always_inline]] inline constexpr void
__window2(limb_t &hi, limb_t &lo, const limb_t *p, usize n, usize sh) noexcept
{
  if ( sh == 0 ) {
    hi = p[n - 1u];
    lo = p[n - 2u];
    return;
  }
  const usize r = limb_bits - sh;
  hi = static_cast<limb_t>((p[n - 1u] << sh) | (p[n - 2u] >> r));
  lo = static_cast<limb_t>((p[n - 2u] << sh) | (n >= 3u ? (p[n - 3u] >> r) : 0u));
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
hgcd2_sel(mat1 &m, limb_t ah, limb_t al, limb_t bh, limb_t bl) noexcept
{
  if constexpr ( threshold::hgcd2_window == 2u ) {
    return hgcd2_w2(m, ah, al, bh, bl);
  } else {
    (void)al;
    (void)bl;
    return hgcd2(m, ah, bh);
  }
}

[[nodiscard, gnu::flatten]] inline constexpr usize
hgcd2_ext(mat1 &m, mat1 &nm, limb_t A, limb_t B) noexcept
{
  m = mat1_identity();
  nm = mat1_identity();
  if ( B == 0 ) return 0;

  usize steps = 0;
  for ( ;; ) {
    if ( B <= m.u10 ) break;
    const limb_t bhi = static_cast<limb_t>(B + m.u00);
    if ( bhi < B ) break;
    const limb_t q1 = static_cast<limb_t>((A - m.u01) / bhi);
    if ( q1 == 0 ) break;
    const limb_t ahi = static_cast<limb_t>(A + m.u11);
    if ( ahi < A ) break;
    if ( q1 != static_cast<limb_t>(ahi / (B - m.u10)) ) break;
    m.u01 = static_cast<limb_t>(m.u01 + q1 * m.u00);
    m.u11 = static_cast<limb_t>(m.u11 + q1 * m.u10);
    nm.u00 = static_cast<limb_t>(nm.u00 + q1 * nm.u10);
    nm.u01 = static_cast<limb_t>(nm.u01 + q1 * nm.u11);
    A = static_cast<limb_t>(A - q1 * B);
    ++steps;

    if ( A <= m.u01 ) break;
    const limb_t ahi2 = static_cast<limb_t>(A + m.u11);
    if ( ahi2 < A ) break;
    const limb_t q2 = static_cast<limb_t>((B - m.u10) / ahi2);
    if ( q2 == 0 ) break;
    const limb_t bhi2 = static_cast<limb_t>(B + m.u00);
    if ( bhi2 < B ) break;
    if ( q2 != static_cast<limb_t>(bhi2 / (A - m.u01)) ) break;
    m.u00 = static_cast<limb_t>(m.u00 + q2 * m.u01);
    m.u10 = static_cast<limb_t>(m.u10 + q2 * m.u11);
    nm.u10 = static_cast<limb_t>(nm.u10 + q2 * nm.u00);
    nm.u11 = static_cast<limb_t>(nm.u11 + q2 * nm.u01);
    B = static_cast<limb_t>(B - q2 * A);
    ++steps;
  }
  return steps;
}

// (rp; sp) <- (n00*a + n01*b ; n10*a + n11*b) over n limbs
//
// only adds, no subs
// NOTE: rp and sp are n + 2 limbs and must be distinct from ap, bp and each other
[[nodiscard, gnu::flatten]] inline constexpr usize
mat1_mul_vector(const mat1 &nm, limb_t *rp, limb_t *sp, const limb_t *ap, const limb_t *bp, usize n) noexcept
{
  const limb_t h0 = mul_1(rp, ap, n, nm.u00);
  const limb_t c0 = addmul_1(rp, bp, n, nm.u01);
  rp[n] = static_cast<limb_t>(h0 + c0);
  rp[n + 1u] = static_cast<limb_t>(rp[n] < h0 ? 1u : 0u);

  const limb_t h1 = mul_1(sp, ap, n, nm.u10);
  const limb_t c1 = addmul_1(sp, bp, n, nm.u11);
  sp[n] = static_cast<limb_t>(h1 + c1);
  sp[n + 1u] = static_cast<limb_t>(sp[n] < h1 ? 1u : 0u);

  const usize rn = normalize(rp, n + 2u);
  const usize sn = normalize(sp, n + 2u);
  return rn > sn ? rn : sn;
}

// (rp; sp) <- (u11*a - u01*b ; u00*b - u10*a) over n limbs
[[nodiscard, gnu::flatten]] inline constexpr bool
mat1_mul_inverse_vector(const mat1 &m, limb_t *rp, limb_t *sp, const limb_t *ap, const limb_t *bp, usize n, usize &rn) noexcept
{
  const limb_t c0 = mul_1(rp, ap, n, m.u11);
  const limb_t b0 = submul_1(rp, bp, n, m.u01);
  const limb_t c1 = mul_1(sp, bp, n, m.u00);
  const limb_t b1 = submul_1(sp, ap, n, m.u10);
  if ( c0 != b0 || c1 != b1 ) return false;

  const usize an = normalize(rp, n);
  const usize bn = normalize(sp, n);
  rn = an > bn ? an : bn;
  return true;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
