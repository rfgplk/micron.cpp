//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "mul_basecase.hpp"
#include "mul_karatsuba.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Toom-Cook 3
// five 1/3-size products instead of nine, O(n^1.465)
//
//  ..points        0     1    -1     2    inf
//  ..A(x)B(x)     v0    v1   vm1    v2   vinf
//
// step ordereding, writing (c4 c3 c2 c1 c0):
//     v2  -= vm1        (16 8 4 2 1) - (1 -1 1 -1 1) = (15 9 3 3 0)
//     v2  /= 3                                       = ( 5 3 1 1 0)
//     vm1  = (v1-vm1)/2                              = ( 0 1 0 1 0)
//     v1  -= v0                                      = ( 1 1 1 1 0)
//     v2   = (v2-v1)/2                               = ( 2 1 0 0 0)
//     v2  -= 2*vinf                                  = ( 0 1 0 0 0)  == c3
//     v1  -= vm1 + vinf                              = ( 0 0 1 0 0)  == c2
//     vm1 -= v2                                      = ( 0 0 0 1 0)  == c1
//
// NOTE: rp distinct from ap and bp, holds an + bn limbs
// balanced only; an == bn == n

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// exact division by 3

inline constexpr limb_t inv3 = static_cast<limb_t>(limb_bits == 64 ? 0xAAAAAAAAAAAAAAABull : 0xAAAAAAABull);
inline constexpr limb_t third_max = static_cast<limb_t>(limb_max / 3u);

[[gnu::flatten]] inline constexpr void
divexact_by3(limb_t *rp, const limb_t *up, usize n) noexcept
{
  limb_t c = 0;
  for ( usize i = 0; i < n; ++i ) {
    limb_t l = 0;
    const limb_t b = subb(up[i], c, 0, l);
    const limb_t q = static_cast<limb_t>(l * inv3);
    rp[i] = q;
    c = static_cast<limb_t>(b + (q > third_max) + (q > static_cast<limb_t>(third_max * 2u)));
  }
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
toom3_split_ok(usize n) noexcept
{
  return n >= 3u && n > 2u * ((n + 2u) / 3u);
}

// %%%%%%%%%%%%%%%%%%%%%%%%
// scratch

[[nodiscard, gnu::flatten]] inline constexpr usize
toom3_itch(usize n, bool force_top = false) noexcept
{
  usize total = 0;
  if ( force_top && n >= 3u && n < threshold::mul_toom3 ) {
    const usize k = (n + 2u) / 3u;
    total += 6u * (k + 1u) + 5u * (2u * k + 2u);
    n = k + 1u;
  }
  while ( n >= threshold::mul_toom3 && n >= 3u ) {
    const usize k = (n + 2u) / 3u;
    total += 6u * (k + 1u) + 5u * (2u * k + 2u);
    n = k + 1u;
  }
  return total + karatsuba_itch(n, karatsuba_force);
}

inline constexpr void __mul_rec3(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn,
                                 limb_t *scratch, usize cutoff) noexcept;

// %%%%%%%%%%%%%%%%%%%%%%%%%%
// evaluation
//
// a0 + a1 + a2 and a0 + 2a1 + 4a2 are at most 3 and 7 times a limb block; both go in k+1 limbs

// s = a0 + a1 + a2, k+1 limbs
inline constexpr void
toom3_eval1(limb_t *s, const limb_t *a0, const limb_t *a1, const limb_t *a2, usize k, usize t) noexcept
{
  copyi(s, a0, k);
  s[k] = add_n(s, s, a1, k);
  s[k] = static_cast<limb_t>(s[k] + add(s, s, k, a2, t));
}

// s = a0 + 2*a1 + 4*a2 by Horner, k+1 limbs
inline constexpr void
toom3_eval2(limb_t *s, const limb_t *a0, const limb_t *a1, const limb_t *a2, usize k, usize t) noexcept
{
  copyi(s, a2, t);
  zero(s + t, k + 1u - t);
  (void)lshift(s, s, k + 1u, 1);                              // 2*a2, top limb still tiny
  s[k] = static_cast<limb_t>(s[k] + add_n(s, s, a1, k));      // + a1
  (void)lshift(s, s, k + 1u, 1);                              // *2
  s[k] = static_cast<limb_t>(s[k] + add_n(s, s, a0, k));      // + a0
}

// s = |a0 - a1 + a2| in k+1 limbs
[[nodiscard]] inline constexpr bool
toom3_evalm1(limb_t *s, limb_t *tmp, const limb_t *a0, const limb_t *a1, const limb_t *a2, usize k, usize t) noexcept
{
  copyi(tmp, a0, k);
  tmp[k] = add(tmp, tmp, k, a2, t);      // a0 + a2, at most 2*B^k so the top limb is 0 or 1

  if ( tmp[k] != 0 || cmp(tmp, a1, k) >= 0 ) {
    copyi(s, tmp, k + 1u);
    s[k] = static_cast<limb_t>(s[k] - sub_n(s, s, a1, k));
    return false;
  }
  (void)sub_n(s, a1, tmp, k);
  s[k] = 0;
  return true;
}

inline constexpr void
toom_interpolate_5pts(limb_t *__restrict__ rp, limb_t *w0, limb_t *w1, limb_t *wm1, limb_t *w2, limb_t *winf, usize k, usize n, usize t,
                      usize kk, bool sm) noexcept
{
  if ( sm )
    (void)add_n(w2, w2, wm1, kk);
  else
    (void)sub_n(w2, w2, wm1, kk);
  divexact_by3(w2, w2, kk);

  if ( sm )
    (void)add_n(wm1, w1, wm1, kk);
  else
    (void)sub_n(wm1, w1, wm1, kk);
  (void)rshift(wm1, wm1, kk, 1);      // exact: (v1 -/+ vm1) is always even

  (void)sub_n(w1, w1, w0, kk);
  (void)sub_n(w2, w2, w1, kk);
  (void)rshift(w2, w2, kk, 1);      // exact again

  // w2 -= 2*vinf
  (void)sub_n(w2, w2, winf, kk);
  (void)sub_n(w2, w2, winf, kk);

  (void)sub_n(w1, w1, wm1, kk);
  (void)sub_n(w1, w1, winf, kk);
  (void)sub_n(wm1, wm1, w2, kk);

  const usize rn = 2u * n;
  zero(rp, rn);
  copyi(rp, w0, 2u * k);
  (void)add(rp + k, rp + k, rn - k, wm1, normalize(wm1, kk));
  (void)add(rp + 2u * k, rp + 2u * k, rn - 2u * k, w1, normalize(w1, kk));
  (void)add(rp + 3u * k, rp + 3u * k, rn - 3u * k, w2, normalize(w2, kk));
  (void)add(rp + 4u * k, rp + 4u * k, rn - 4u * k, winf, normalize(winf, 2u * t));
}

inline constexpr void
mul_toom3(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, const limb_t *__restrict__ bp, usize n, limb_t *scratch,
          usize cutoff) noexcept
{
  const usize k = (n + 2u) / 3u;
  const usize t = n - 2u * k;
  const usize kk = 2u * k + 2u;

  limb_t *const as1 = scratch;
  limb_t *const as2 = as1 + (k + 1u);
  limb_t *const asm1 = as2 + (k + 1u);
  limb_t *const bs1 = asm1 + (k + 1u);
  limb_t *const bs2 = bs1 + (k + 1u);
  limb_t *const bsm1 = bs2 + (k + 1u);
  limb_t *const w0 = bsm1 + (k + 1u);
  limb_t *const w1 = w0 + kk;
  limb_t *const wm1 = w1 + kk;
  limb_t *const w2 = wm1 + kk;
  limb_t *const winf = w2 + kk;
  limb_t *const rec = winf + kk;

  const limb_t *const a0 = ap;
  const limb_t *const a1 = ap + k;
  const limb_t *const a2 = ap + 2u * k;
  const limb_t *const b0 = bp;
  const limb_t *const b1 = bp + k;
  const limb_t *const b2 = bp + 2u * k;

  toom3_eval1(as1, a0, a1, a2, k, t);
  toom3_eval1(bs1, b0, b1, b2, k, t);
  toom3_eval2(as2, a0, a1, a2, k, t);
  toom3_eval2(bs2, b0, b1, b2, k, t);
  const bool sa = toom3_evalm1(asm1, w0, a0, a1, a2, k, t);      // w0 is free scratch until v0 lands
  const bool sb = toom3_evalm1(bsm1, w1, b0, b1, b2, k, t);
  const bool sm = sa != sb;

  __mul_rec3(w1, as1, k + 1u, bs1, k + 1u, rec, cutoff);
  __mul_rec3(wm1, asm1, k + 1u, bsm1, k + 1u, rec, cutoff);
  __mul_rec3(w2, as2, k + 1u, bs2, k + 1u, rec, cutoff);
  __mul_rec3(w0, a0, k, b0, k, rec, cutoff);
  zero(w0 + 2u * k, kk - 2u * k);
  __mul_rec3(winf, a2, t, b2, t, rec, cutoff);
  zero(winf + 2u * t, kk - 2u * t);

  toom_interpolate_5pts(rp, w0, w1, wm1, w2, winf, k, n, t, kk, sm);
}

inline constexpr void
__mul_rec3(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn, limb_t *scratch,
           usize cutoff) noexcept
{
  if ( an == bn && an >= threshold::mul_toom3 && toom3_split_ok(an) )
    mul_toom3(rp, ap, bp, an, scratch, cutoff);
  else
    __mul_rec(rp, ap, an, bp, bn, scratch, cutoff);
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
toom3_applies(usize an, usize bn) noexcept
{
  return an == bn && toom3_split_ok(an);
}

// %%%%%%%%%%%%%%%%%%%%%%
// squaring

[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_toom3_itch(usize n, bool force_top = false) noexcept
{
  usize total = 0;
  if ( force_top && n >= 3u && n < threshold::sqr_toom3 ) {
    const usize k = (n + 2u) / 3u;
    total += 3u * (k + 1u) + 5u * (2u * k + 2u);
    n = k + 1u;
  }
  while ( n >= threshold::sqr_toom3 && n >= 3u ) {
    const usize k = (n + 2u) / 3u;
    total += 3u * (k + 1u) + 5u * (2u * k + 2u);
    n = k + 1u;
  }
  return total + sqr_karatsuba_itch(n, karatsuba_force);
}

inline constexpr void sqr_toom3(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch, usize cutoff) noexcept;

// a sub-square of a Toom-3 split can be another Toom-3 split rather than dropping a tier
inline constexpr void
__sqr_rec3(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch, usize cutoff) noexcept
{
  if ( n >= threshold::sqr_toom3 && toom3_split_ok(n) )
    sqr_toom3(rp, ap, n, scratch, cutoff);
  else
    sqr_karatsuba(rp, ap, n, scratch, cutoff);
}

inline constexpr void
sqr_toom3(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch, usize cutoff) noexcept
{
  const usize k = (n + 2u) / 3u;
  const usize t = n - 2u * k;
  const usize kk = 2u * k + 2u;

  limb_t *const as1 = scratch;
  limb_t *const as2 = as1 + (k + 1u);
  limb_t *const asm1 = as2 + (k + 1u);
  limb_t *const w0 = asm1 + (k + 1u);
  limb_t *const w1 = w0 + kk;
  limb_t *const wm1 = w1 + kk;
  limb_t *const w2 = wm1 + kk;
  limb_t *const winf = w2 + kk;
  limb_t *const rec = winf + kk;

  const limb_t *const a0 = ap;
  const limb_t *const a1 = ap + k;
  const limb_t *const a2 = ap + 2u * k;

  toom3_eval1(as1, a0, a1, a2, k, t);
  toom3_eval2(as2, a0, a1, a2, k, t);
  (void)toom3_evalm1(asm1, w0, a0, a1, a2, k, t);

  __sqr_rec3(w1, as1, k + 1u, rec, cutoff);
  __sqr_rec3(wm1, asm1, k + 1u, rec, cutoff);
  __sqr_rec3(w2, as2, k + 1u, rec, cutoff);
  __sqr_rec3(w0, a0, k, rec, cutoff);
  zero(w0 + 2u * k, kk - 2u * k);
  __sqr_rec3(winf, a2, t, rec, cutoff);
  zero(winf + 2u * t, kk - 2u * t);

  toom_interpolate_5pts(rp, w0, w1, wm1, w2, winf, k, n, t, kk, false);
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
sqr_toom3_applies(usize n) noexcept
{
  return toom3_split_ok(n);
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
