//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "bits/carry.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// division: Moller-Granlund reciprocal -> schoolbook
// O(qn * dn)
//
// we get every quotient digit from a multiply by a precomputed reciprocal rather than via hardware divides
//
// for portability, aarch64 has no 128/64 divide instruction

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// double-word add / subtract, add_ssaaaa / sub_ddmmss

[[gnu::always_inline]] inline constexpr void
add2(limb_t &sh, limb_t &sl, limb_t ah, limb_t al, limb_t bh, limb_t bl) noexcept
{
  limb_t lo = 0;
  const limb_t c = addc(al, bl, 0, lo);
  sl = lo;
  sh = static_cast<limb_t>(ah + bh + c);
}

[[gnu::always_inline]] inline constexpr void
sub2(limb_t &sh, limb_t &sl, limb_t ah, limb_t al, limb_t bh, limb_t bl) noexcept
{
  limb_t lo = 0;
  const limb_t bw = subb(al, bl, 0, lo);
  sl = lo;
  sh = static_cast<limb_t>(ah - bh - bw);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// reciprocals
//
// (B - 1 - d) * B + (B - 1) / d == floor((B^2 - 1)/d) - B

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
invert_limb(limb_t d) noexcept
{
  return lo_half(join(static_cast<limb_t>(~d), limb_max) / static_cast<dlimb_t>(d));
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
invert_pi1(limb_t d1, limb_t d0) noexcept
{
  limb_t v = invert_limb(d1);
  limb_t p = static_cast<limb_t>(static_cast<limb_t>(d1 * v) + d0);
  if ( p < d0 ) {
    --v;
    const limb_t mask = static_cast<limb_t>(0) - static_cast<limb_t>(p >= d1);
    p = static_cast<limb_t>(p - d1);
    v = static_cast<limb_t>(v + mask);
    p = static_cast<limb_t>(p - (mask & d1));
  }
  limb_t t1 = 0, t0 = 0;
  mul_wide(d0, v, t0, t1);
  p = static_cast<limb_t>(p + t1);
  if ( p < t1 ) {
    --v;
    if ( p >= d1 && (p > d1 || t0 >= d0) ) --v;
  }
  return v;
}

// {nh, nl} / d with nh < d. dinv == invert_limb(d), d normalized
[[gnu::always_inline]] inline constexpr void
divrem_2by1(limb_t &q, limb_t &r, limb_t nh, limb_t nl, limb_t d, limb_t dinv) noexcept
{
  limb_t qh = 0, ql = 0;
  mul_wide(nh, dinv, ql, qh);
  add2(qh, ql, qh, ql, static_cast<limb_t>(nh + 1u), nl);

  limb_t rr = static_cast<limb_t>(nl - static_cast<limb_t>(qh * d));
  const limb_t mask = static_cast<limb_t>(0) - static_cast<limb_t>(rr > ql);
  qh = static_cast<limb_t>(qh + mask);
  rr = static_cast<limb_t>(rr + (mask & d));
  if ( rr >= d ) [[unlikely]] {
    rr = static_cast<limb_t>(rr - d);
    ++qh;
  }
  q = qh;
  r = rr;
}

// {n2, n1, n0} / {d1, d0} with {n2,n1} < {d1,d0}. dinv == invert_pi1(d1, d0), d1 normalized
[[gnu::flatten]] inline constexpr void
divrem_3by2(limb_t &q, limb_t &r1, limb_t &r0, limb_t n2, limb_t n1, limb_t n0, limb_t d1, limb_t d0, limb_t dinv) noexcept
{
  limb_t qh = 0, q0 = 0;
  mul_wide(n2, dinv, q0, qh);
  add2(qh, q0, qh, q0, n2, n1);

  limb_t s1 = static_cast<limb_t>(n1 - static_cast<limb_t>(d1 * qh));
  limb_t s0 = 0;
  sub2(s1, s0, s1, n0, d1, d0);

  limb_t t1 = 0, t0 = 0;
  mul_wide(d0, qh, t0, t1);
  sub2(s1, s0, s1, s0, t1, t0);
  ++qh;

  const limb_t mask = static_cast<limb_t>(0) - static_cast<limb_t>(s1 >= q0);
  qh = static_cast<limb_t>(qh + mask);
  add2(s1, s0, s1, s0, static_cast<limb_t>(mask & d1), static_cast<limb_t>(mask & d0));

  if ( s1 >= d1 ) [[unlikely]] {
    if ( s1 > d1 || s0 >= d0 ) {
      ++qh;
      sub2(s1, s0, s1, s0, d1, d0);
    }
  }
  q = qh;
  r1 = s1;
  r0 = s0;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// single-limb divisor

[[gnu::flatten]] inline constexpr limb_t
divrem_1(limb_t *qp, const limb_t *up, usize un, limb_t d) noexcept
{
  const usize sh = limb_clz(d);
  const limb_t dnorm = static_cast<limb_t>(d << sh);
  const limb_t dinv = invert_limb(dnorm);
  const usize rsh = limb_bits - sh;

  limb_t r = (sh != 0) ? static_cast<limb_t>(up[un - 1] >> rsh) : limb_t{ 0 };
  for ( usize i = un; i-- > 0; ) {
    limb_t nl = static_cast<limb_t>(up[i] << sh);
    if ( sh != 0 && i > 0 ) nl |= static_cast<limb_t>(up[i - 1] >> rsh);
    limb_t q = 0;
    divrem_2by1(q, r, r, nl, dnorm, dinv);
    qp[i] = q;
  }
  return static_cast<limb_t>(r >> sh);
}

[[nodiscard, gnu::flatten]] inline constexpr limb_t
mod_1(const limb_t *up, usize un, limb_t d) noexcept
{
  const usize sh = limb_clz(d);
  const limb_t dnorm = static_cast<limb_t>(d << sh);
  const limb_t dinv = invert_limb(dnorm);
  const usize rsh = limb_bits - sh;

  limb_t r = (sh != 0) ? static_cast<limb_t>(up[un - 1] >> rsh) : limb_t{ 0 };
  for ( usize i = un; i-- > 0; ) {
    limb_t nl = static_cast<limb_t>(up[i] << sh);
    if ( sh != 0 && i > 0 ) nl |= static_cast<limb_t>(up[i - 1] >> rsh);
    limb_t q = 0;
    divrem_2by1(q, r, r, nl, dnorm, dinv);
  }
  return static_cast<limb_t>(r >> sh);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// schoolbook, divisor >= 2 limbs
//
// {dp, dn} normalized (top bit of dp[dn-1] set), dn >= 2, nn >= dn

[[nodiscard, gnu::flatten]] inline constexpr limb_t
sbpi1_div_qr(limb_t *qp, limb_t *np, usize nn, const limb_t *dp, usize dn, limb_t dinv) noexcept
{
  const limb_t qh = static_cast<limb_t>(cmp(np + nn - dn, dp, dn) >= 0);
  if ( qh != 0 ) (void)sub_n(np + nn - dn, np + nn - dn, dp, dn);

  const limb_t d1 = dp[dn - 1];
  const limb_t d0 = dp[dn - 2];
  limb_t n1 = np[nn - 1];

  for ( usize j = nn - dn; j-- > 0; ) {
    limb_t q = 0;
    if ( n1 == d1 && np[j + dn - 1] == d0 ) [[unlikely]] {
      q = limb_max;
      (void)submul_1(np + j, dp, dn, q);
      n1 = np[j + dn - 1];
    } else {
      limb_t n0 = 0;
      divrem_3by2(q, n1, n0, n1, np[j + dn - 1], np[j + dn - 2], d1, d0, dinv);

      const limb_t cy = submul_1(np + j, dp, dn - 2, q);
      const limb_t cy1 = static_cast<limb_t>(n0 < cy);
      n0 = static_cast<limb_t>(n0 - cy);
      const limb_t cy2 = static_cast<limb_t>(n1 < cy1);
      n1 = static_cast<limb_t>(n1 - cy1);
      np[j + dn - 2] = n0;

      if ( cy2 != 0 ) [[unlikely]] {
        n1 = static_cast<limb_t>(n1 + d1 + add_n(np + j, np + j, dp, dn - 1));
        --q;
      }
    }
    qp[j] = q;
  }
  np[dn - 1] = n1;
  return qh;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
