//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "mul_karatsuba.hpp"
#include "mul_toom.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Toom-Cook 4
// seven 1/4-size products instead of sixteen, O(n^1.404)
//
//     points        0     1    -1     2    -2    1/2    inf
//     A(x)B(x)     w0    w2    w3    w4    w1     w5     w6
//
// NOTE: rp distinct from ap and bp, holds an + bn limbs; balanced only; an == bn == n

namespace micron
{
namespace math
{
namespace mpn
{

[[nodiscard, gnu::always_inline]] inline constexpr limb_t
binv_odd(limb_t d) noexcept
{

  limb_t x = d;
  for ( usize i = 0; i < 6u; ++i ) x = static_cast<limb_t>(x * static_cast<limb_t>(limb_t{ 2 } - static_cast<limb_t>(d * x)));
  return x;
}

inline constexpr limb_t inv9 = binv_odd(9u);
inline constexpr limb_t inv15 = binv_odd(15u);

static_assert(binv_odd(3u) == inv3, "arbint: the general binary inverse must agree with the hand-written 3^-1");
static_assert(static_cast<limb_t>(9u * inv9) == 1u, "arbint: 9 * 9^-1 must be 1 mod 2^limb_bits");
static_assert(static_cast<limb_t>(15u * inv15) == 1u, "arbint: 15 * 15^-1 must be 1 mod 2^limb_bits");

[[gnu::flatten]] inline constexpr void
divexact_1_odd(limb_t *rp, const limb_t *up, usize n, limb_t d, limb_t dinv) noexcept
{
  limb_t cy = 0;
  for ( usize i = 0; i < n; ++i ) {
    limb_t s = 0;
    const limb_t bw = subb(up[i], cy, 0, s);
    const limb_t q = static_cast<limb_t>(s * dinv);
    rp[i] = q;
    limb_t lo = 0, hi = 0;
    mul_wide(q, d, lo, hi);
    cy = static_cast<limb_t>(hi + bw);
  }
}

[[gnu::always_inline]] inline constexpr void
divexact_by9(limb_t *rp, const limb_t *up, usize n) noexcept
{
  divexact_1_odd(rp, up, n, 9u, inv9);
}

[[gnu::always_inline]] inline constexpr void
divexact_by15(limb_t *rp, const limb_t *up, usize n) noexcept
{
  divexact_1_odd(rp, up, n, 15u, inv15);
}

[[gnu::flatten]] inline constexpr void
rsh1add_n(limb_t *rp, const limb_t *up, const limb_t *vp, usize n) noexcept
{
  const limb_t cy = add_n(rp, up, vp, n);
  (void)rshift(rp, rp, n, 1);
  rp[n - 1u] = static_cast<limb_t>(rp[n - 1u] | (cy << (limb_bits - 1u)));
}

[[nodiscard, gnu::flatten]] inline constexpr bool
toom4_eval_pm1(limb_t *xp1, limb_t *xm1, limb_t *tp, const limb_t *x0, const limb_t *x1, const limb_t *x2, const limb_t *x3, usize k,
               usize s) noexcept
{
  xp1[k] = add_n(xp1, x0, x2, k);
  tp[k] = add(tp, x1, k, x3, s);
  const bool neg = cmp(xp1, tp, k + 1u) < 0;
  if ( neg )
    (void)sub_n(xm1, tp, xp1, k + 1u);
  else
    (void)sub_n(xm1, xp1, tp, k + 1u);
  (void)add_n(xp1, xp1, tp, k + 1u);
  return neg;
}

[[nodiscard, gnu::flatten]] inline constexpr bool
toom4_eval_pm2(limb_t *xp2, limb_t *xm2, limb_t *tp, const limb_t *x0, const limb_t *x1, const limb_t *x2, const limb_t *x3, usize k,
               usize s) noexcept
{
  limb_t cy = lshift(xp2, x2, k, 2);
  xp2[k] = static_cast<limb_t>(cy + add_n(xp2, xp2, x0, k));

  zero(tp, k + 1u);
  copyi(tp, x3, s);
  cy = lshift(tp, tp, k, 2);
  tp[k] = static_cast<limb_t>(cy + add_n(tp, tp, x1, k));
  cy = lshift(tp, tp, k, 1);
  tp[k] = static_cast<limb_t>((tp[k] << 1) | cy);

  const bool neg = cmp(xp2, tp, k + 1u) < 0;
  if ( neg )
    (void)sub_n(xm2, tp, xp2, k + 1u);
  else
    (void)sub_n(xm2, xp2, tp, k + 1u);
  (void)add_n(xp2, xp2, tp, k + 1u);
  return neg;
}

[[gnu::flatten]] inline constexpr void
toom4_eval_half(limb_t *xh, const limb_t *x0, const limb_t *x1, const limb_t *x2, const limb_t *x3, usize k, usize s) noexcept
{
  limb_t c = lshift(xh, x0, k, 1);
  xh[k] = static_cast<limb_t>(c + add_n(xh, xh, x1, k));
  c = lshift(xh, xh, k, 1);
  xh[k] = static_cast<limb_t>((xh[k] << 1) | c);
  xh[k] = static_cast<limb_t>(xh[k] + add_n(xh, xh, x2, k));
  c = lshift(xh, xh, k, 1);
  xh[k] = static_cast<limb_t>((xh[k] << 1) | c);
  xh[k] = static_cast<limb_t>(xh[k] + add(xh, xh, k, x3, s));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// seven point interpolation
//
// Bodrato's sequence
//     W5 = W5 + W4          W5 = W5 - W2*65      W1 = W5 - W1
//     W1 = (W4 - W1)/2      W2 = W2 - W6 - W0    W5 = (W5 - W3*8)/9
//     W4 = W4 - W0          W5 = (W5 + W2*45)/2  W3 = W3 - W5
//     W4 = (W4 - W1)/4      W4 = (W4 - W2)/3     W1 = (W1/15 + W5)/2
//          - W6*16          W2 = W2 - W4         W5 = W5 - W1
//     W3 = (W2 - W3)/2
//     W2 = W2 - W3
// on exit the seven buffers hold c0..c6 and rp holds their sum at strides of k limbs
inline constexpr void
toom_interpolate_7pts(limb_t *__restrict__ rp, limb_t *w0, limb_t *w1, limb_t *w2, limb_t *w3, limb_t *w4, limb_t *w5, limb_t *w6,
                      limb_t *tp, usize k, usize n, usize s, usize m, bool w1_neg, bool w3_neg) noexcept
{
  (void)add_n(w5, w5, w4, m);

  if ( w1_neg )
    rsh1add_n(w1, w1, w4, m);
  else {
    (void)sub_n(w1, w4, w1, m);
    (void)rshift(w1, w1, m, 1);
  }

  (void)sub_n(w4, w4, w0, m);
  (void)sub_n(w4, w4, w1, m);
  (void)rshift(w4, w4, m, 2);
  (void)lshift(tp, w6, m, 4);
  (void)sub_n(w4, w4, tp, m);

  if ( w3_neg )
    rsh1add_n(w3, w3, w2, m);
  else {
    (void)sub_n(w3, w2, w3, m);
    (void)rshift(w3, w3, m, 1);
  }

  (void)sub_n(w2, w2, w3, m);

  (void)submul_1(w5, w2, m, 65u);
  (void)sub_n(w2, w2, w6, m);
  (void)sub_n(w2, w2, w0, m);
  (void)addmul_1(w5, w2, m, 45u);
  (void)rshift(w5, w5, m, 1);

  (void)sub_n(w4, w4, w2, m);
  divexact_by3(w4, w4, m);
  (void)sub_n(w2, w2, w4, m);

  (void)sub_n(w1, w5, w1, m);

  (void)lshift(tp, w3, m, 3);
  (void)sub_n(w5, w5, tp, m);
  divexact_by9(w5, w5, m);
  (void)sub_n(w3, w3, w5, m);

  divexact_by15(w1, w1, m);
  rsh1add_n(w1, w1, w5, m);
  w1[m - 1u] = static_cast<limb_t>(w1[m - 1u] & (limb_max >> 1));
  (void)sub_n(w5, w5, w1, m);

  const usize rn = 2u * n;
  zero(rp, rn);
  copyi(rp, w0, 2u * k);
  (void)add(rp + k, rp + k, rn - k, w1, normalize(w1, m));
  (void)add(rp + 2u * k, rp + 2u * k, rn - 2u * k, w2, normalize(w2, m));
  (void)add(rp + 3u * k, rp + 3u * k, rn - 3u * k, w3, normalize(w3, m));
  (void)add(rp + 4u * k, rp + 4u * k, rn - 4u * k, w4, normalize(w4, m));
  (void)add(rp + 5u * k, rp + 5u * k, rn - 5u * k, w5, normalize(w5, m));
  (void)add(rp + 6u * k, rp + 6u * k, rn - 6u * k, w6, normalize(w6, 2u * s));
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
toom4_split_ok(usize n) noexcept
{
  return n >= 4u && n > 3u * ((n + 3u) / 4u);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scratch

[[nodiscard, gnu::flatten]] inline constexpr usize
toom4_itch(usize n, bool force_top = false) noexcept
{
  usize total = 0;
  if ( force_top && n >= 4u && n < threshold::mul_toom4 ) {
    const usize k = (n + 3u) / 4u;
    total += 11u * (k + 1u) + 8u * (2u * k + 2u);
    n = k + 1u;
  }
  while ( n >= threshold::mul_toom4 && n >= 4u ) {
    const usize k = (n + 3u) / 4u;
    total += 11u * (k + 1u) + 8u * (2u * k + 2u);
    n = k + 1u;
  }
  return total + toom3_itch(n);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_toom4_itch(usize n, bool force_top = false) noexcept
{
  usize total = 0;
  if ( force_top && n >= 4u && n < threshold::sqr_toom4 ) {
    const usize k = (n + 3u) / 4u;
    total += 6u * (k + 1u) + 8u * (2u * k + 2u);
    n = k + 1u;
  }
  while ( n >= threshold::sqr_toom4 && n >= 4u ) {
    const usize k = (n + 3u) / 4u;
    total += 6u * (k + 1u) + 8u * (2u * k + 2u);
    n = k + 1u;
  }
  return total + sqr_toom3_itch(n);
}

inline constexpr void mul_toom4(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, const limb_t *__restrict__ bp, usize n,
                                limb_t *scratch, usize cutoff) noexcept;
inline constexpr void sqr_toom4(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch, usize cutoff) noexcept;

inline constexpr void
__mul_rec4(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn, limb_t *scratch,
           usize cutoff) noexcept
{
  if ( an == bn && an >= threshold::mul_toom4 && toom4_split_ok(an) )
    mul_toom4(rp, ap, bp, an, scratch, cutoff);
  else
    __mul_rec3(rp, ap, an, bp, bn, scratch, cutoff);
}

inline constexpr void
__sqr_rec4(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch, usize cutoff) noexcept
{
  if ( n >= threshold::sqr_toom4 && toom4_split_ok(n) )
    sqr_toom4(rp, ap, n, scratch, cutoff);
  else
    __sqr_rec3(rp, ap, n, scratch, cutoff);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// kernels

inline constexpr void
mul_toom4(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, const limb_t *__restrict__ bp, usize n, limb_t *scratch,
          usize cutoff) noexcept
{
  const usize k = (n + 3u) / 4u;
  const usize s = n - 3u * k;      // 1 <= s <= k
  const usize m = 2u * k + 2u;
  const usize e = k + 1u;

  limb_t *const ap1 = scratch;
  limb_t *const am1 = ap1 + e;
  limb_t *const ap2 = am1 + e;
  limb_t *const am2 = ap2 + e;
  limb_t *const ah = am2 + e;
  limb_t *const bp1 = ah + e;
  limb_t *const bm1 = bp1 + e;
  limb_t *const bp2 = bm1 + e;
  limb_t *const bm2 = bp2 + e;
  limb_t *const bh = bm2 + e;
  limb_t *const et = bh + e;
  limb_t *const w0 = et + e;
  limb_t *const w1 = w0 + m;
  limb_t *const w2 = w1 + m;
  limb_t *const w3 = w2 + m;
  limb_t *const w4 = w3 + m;
  limb_t *const w5 = w4 + m;
  limb_t *const w6 = w5 + m;
  limb_t *const tp = w6 + m;
  limb_t *const rec = tp + m;

  const limb_t *const a0 = ap;
  const limb_t *const a1 = ap + k;
  const limb_t *const a2 = ap + 2u * k;
  const limb_t *const a3 = ap + 3u * k;
  const limb_t *const b0 = bp;
  const limb_t *const b1 = bp + k;
  const limb_t *const b2 = bp + 2u * k;
  const limb_t *const b3 = bp + 3u * k;

  const bool an1 = toom4_eval_pm1(ap1, am1, et, a0, a1, a2, a3, k, s);
  const bool bn1 = toom4_eval_pm1(bp1, bm1, et, b0, b1, b2, b3, k, s);
  const bool an2 = toom4_eval_pm2(ap2, am2, et, a0, a1, a2, a3, k, s);
  const bool bn2 = toom4_eval_pm2(bp2, bm2, et, b0, b1, b2, b3, k, s);
  toom4_eval_half(ah, a0, a1, a2, a3, k, s);
  toom4_eval_half(bh, b0, b1, b2, b3, k, s);

  __mul_rec4(w2, ap1, e, bp1, e, rec, cutoff);      // f(1)
  __mul_rec4(w3, am1, e, bm1, e, rec, cutoff);      // |f(-1)|
  __mul_rec4(w4, ap2, e, bp2, e, rec, cutoff);      // f(2)
  __mul_rec4(w1, am2, e, bm2, e, rec, cutoff);      // |f(-2)|
  __mul_rec4(w5, ah, e, bh, e, rec, cutoff);        // 64 * f(1/2)
  __mul_rec4(w0, a0, k, b0, k, rec, cutoff);        // f(0)
  zero(w0 + 2u * k, m - 2u * k);
  __mul_rec4(w6, a3, s, b3, s, rec, cutoff);      // f(inf)
  zero(w6 + 2u * s, m - 2u * s);

  toom_interpolate_7pts(rp, w0, w1, w2, w3, w4, w5, w6, tp, k, n, s, m, an2 != bn2, an1 != bn1);
}

inline constexpr void
sqr_toom4(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch, usize cutoff) noexcept
{
  const usize k = (n + 3u) / 4u;
  const usize s = n - 3u * k;
  const usize m = 2u * k + 2u;
  const usize e = k + 1u;

  limb_t *const ap1 = scratch;
  limb_t *const am1 = ap1 + e;
  limb_t *const ap2 = am1 + e;
  limb_t *const am2 = ap2 + e;
  limb_t *const ah = am2 + e;
  limb_t *const et = ah + e;
  limb_t *const w0 = et + e;
  limb_t *const w1 = w0 + m;
  limb_t *const w2 = w1 + m;
  limb_t *const w3 = w2 + m;
  limb_t *const w4 = w3 + m;
  limb_t *const w5 = w4 + m;
  limb_t *const w6 = w5 + m;
  limb_t *const tp = w6 + m;
  limb_t *const rec = tp + m;

  const limb_t *const a0 = ap;
  const limb_t *const a1 = ap + k;
  const limb_t *const a2 = ap + 2u * k;
  const limb_t *const a3 = ap + 3u * k;

  (void)toom4_eval_pm1(ap1, am1, et, a0, a1, a2, a3, k, s);
  (void)toom4_eval_pm2(ap2, am2, et, a0, a1, a2, a3, k, s);
  toom4_eval_half(ah, a0, a1, a2, a3, k, s);

  __sqr_rec4(w2, ap1, e, rec, cutoff);
  __sqr_rec4(w3, am1, e, rec, cutoff);
  __sqr_rec4(w4, ap2, e, rec, cutoff);
  __sqr_rec4(w1, am2, e, rec, cutoff);
  __sqr_rec4(w5, ah, e, rec, cutoff);
  __sqr_rec4(w0, a0, k, rec, cutoff);
  zero(w0 + 2u * k, m - 2u * k);
  __sqr_rec4(w6, a3, s, rec, cutoff);
  zero(w6 + 2u * s, m - 2u * s);

  toom_interpolate_7pts(rp, w0, w1, w2, w3, w4, w5, w6, tp, k, n, s, m, false, false);
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
toom4_applies(usize an, usize bn) noexcept
{
  return an == bn && toom4_split_ok(an);
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
sqr_toom4_applies(usize n) noexcept
{
  return toom4_split_ok(n);
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
