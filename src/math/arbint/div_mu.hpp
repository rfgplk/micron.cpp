//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div.hpp"
#include "div_dc.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "mul.hpp"
#include "tags.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Barrett division
// (precomputed n-limb reciprocal into a multiply)
//
// ..invert_limb(d)       one limb      floor((B^2 - 1)/d) - B
// ..invert_pi1(d1, d0)   two limbs     3-by-2 estimator
// ..invert_n(ip, dp, n)  n limbs       floor((B^2n - 1)/D) - B^n
//
// dc_div_qr_n's recurrence D(n) = 2*D(n/2) + 2*M(n/2) solves to about 4.4*M(n) for n quotient limbs against an n-limb divisor
//
// TODO: convert estimate via mullo/low-half mul
//
// WARNING: never gnu::flatten on anything calling mul(), sqr() or divrem(); comptimes _explode_

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// n-limb reciprocal

inline constexpr void
invert_n_basecase(limb_t *ip, const limb_t *dp, usize n, limb_t *scratch) noexcept
{
  if ( n == 1 ) {
    ip[0] = invert_limb(dp[0]);
    return;
  }

  limb_t *const np = scratch;      // 2n
  for ( usize i = 0; i < 2u * n; ++i ) np[i] = limb_max;

  (void)sbpi1_div_qr(ip, np, 2u * n, dp, n, invert_pi1(dp[n - 1], dp[n - 2]));
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
invert_n_basecase_itch(usize n) noexcept
{
  return n == 1 ? 0u : 2u * n;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the correction audit

#if defined(MICRON_ARBINT_MU_AUDIT)
inline usize mu_corrections_down = 0;
inline usize mu_corrections_up = 0;
inline usize mu_saturations = 0;
inline usize inv_fixup_down = 0;
inline usize inv_fixup_up = 0;
#define __micron_arbint_mu_audit(x) (++(x))
#else
#define __micron_arbint_mu_audit(x) ((void)0)
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the exact fixup
//
// {pp, 2n} and {rp, 2n} are scratch; ip is corrected in place

inline constexpr void
__invert_n_fixup(limb_t *ip, const limb_t *dp, usize n, limb_t *pp, limb_t *rp, limb_t *work) noexcept
{
  // P = D * (B^n + V) = D*V + D*B^n
  mul(pp, dp, n, ip, n, work);
  limb_t cy = add_n(pp + n, pp + n, dp, n);

  // P at or above B^2n means V overshot
  while ( cy != 0 ) {
    __micron_arbint_mu_audit(inv_fixup_down);
    (void)sub_1(ip, ip, n, 1);
    cy = static_cast<limb_t>(cy - sub(pp, pp, 2u * n, dp, n));
  }

  // R = B^2n - P
  (void)neg(rp, pp, 2u * n);
  for ( ;; ) {
    if ( is_zero(rp + n, n) && cmp(rp, dp, n) <= 0 ) break;
    __micron_arbint_mu_audit(inv_fixup_up);
    (void)add_1(ip, ip, n, 1);
    (void)sub(rp, rp, 2u * n, dp, n);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Newton
//
// one step costs roughly
// T(n) = T(n/2) + O(M(n)) = O(M(n))
//
// with h = ceil(n/2), l = n - h and D_h the top h limbs of D:
//
// B^2n / D  ~=  B^l * (B^2h / D_h)

inline constexpr void
invert_n(limb_t *ip, const limb_t *dp, usize n, limb_t *scratch) noexcept
{
  if ( n < threshold::inv_newton ) {
    invert_n_basecase(ip, dp, n, scratch);
    return;
  }

  const usize h = (n + 1u) / 2u;
  const usize l = n - h;

  invert_n(ip + l, dp + l, h, scratch);
  zero(ip, l);

  limb_t *const ep = scratch;               // 2n + 1
  limb_t *const wp = ep + 2u * n + 1u;      // n + h
  limb_t *const xp = wp + n + h;            // n + 1
  limb_t *const pp = xp + n + 1u;           // 3n + 2
  limb_t *const work = pp + 3u * n + 2u;

  mul(wp, dp, n, ip + l, h, work);      // W = D * V_h; n >= h, so the order is legal

  zero(ep, n);
  (void)neg(ep + n, dp, n);      // C = B^n - D, in the top half
  const limb_t bw = sub_n(ep + l, ep + l, wp, n + h);
  const bool eneg = (bw != 0);
  if ( eneg ) (void)neg(ep, ep, 2u * n);
  const usize en = normalize(ep, 2u * n);

  if ( en != 0 ) {
    copyi(xp, ip, n);
    xp[n] = 1;      // X_0 = B^n + V, n + 1 limbs
    const usize xn = n + 1u;
    if ( xn >= en )
      mul(pp, xp, xn, ep, en, work);
    else
      mul(pp, ep, en, xp, xn, work);

    const usize pn = xn + en;
    if ( pn > 2u * n ) {
      usize dl = pn - 2u * n;
      dl = normalize(pp + 2u * n, dl);
      if ( dl > n ) dl = n;
      if ( dl != 0 ) {
        if ( eneg )
          (void)sub(ip, ip, n, pp + 2u * n, dl);
        else
          (void)add(ip, ip, n, pp + 2u * n, dl);
      }
    }
  }

  __invert_n_fixup(ip, dp, n, pp, ep, work);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
invert_n_itch(usize n) noexcept
{
  if ( n < 2u ) return 0;
  usize best = 0;
  usize m = n;
  while ( m >= threshold::inv_newton ) {
    const usize h = (m + 1u) / 2u;
    const usize lev = (2u * m + 1u) + (m + h) + (m + 1u) + (3u * m + 2u);
    usize w = mul_itch(m, h);
    const usize w2 = mul_itch(2u * m + 1u, m + 1u);
    if ( w2 > w ) w = w2;
    const usize w3 = mul_itch(m, m);
    if ( w3 > w ) w = w3;
    if ( lev + w > best ) best = lev + w;
    m = h;
  }
  const usize base = invert_n_basecase_itch(m);
  return base > best ? base : best;
}

// %%%%%%%%%%%%%%%%%%%%%%
// block size

[[nodiscard, gnu::always_inline]] inline constexpr usize
mu_block_size(usize qn, usize dn) noexcept
{
  if ( qn == 0 || dn < 2u ) return 2u;
  const usize blocks = (qn - 1u) / dn + 1u;      // ceil(qn / dn)
  usize in = (qn - 1u) / blocks + 1u;            // ceil(qn / blocks), which is <= dn
  if ( in < 2u ) in = 2u;
  if ( in > dn ) in = dn;
  return in;
}

// %%%%%%%%%%%%%%%%%%%%
// one block

inline constexpr void
mu_div_qr_block(limb_t *qp, limb_t *np, const limb_t *dp, usize dn, const limb_t *ip, usize in, limb_t *tp, limb_t *work) noexcept
{
  const limb_t *const u1 = np + dn;

  mul(tp, u1, in, ip, in, work);
  limb_t cy = add_n(qp, tp + in, u1, in);
  cy = static_cast<limb_t>(cy + add_1(qp, qp, in, 1));

  // q^ ran past B^in - 1
  if ( cy != 0 ) [[unlikely]] {
    __micron_arbint_mu_audit(mu_saturations);
    for ( usize i = 0; i < in; ++i ) qp[i] = limb_max;
  }

  mul(tp, dp, dn, qp, in, work);
  cy = sub_n(np, np, tp, dn + in);

  // q^ was too big
  while ( cy != 0 ) {
    __micron_arbint_mu_audit(mu_corrections_down);
    (void)sub_1(qp, qp, in, 1);
    cy = static_cast<limb_t>(cy - add(np, np, dn + in, dp, dn));
  }

  // q^ was too small
  for ( ;; ) {
    if ( is_zero(np + dn, in) && cmp(np, dp, dn) < 0 ) break;
    __micron_arbint_mu_audit(mu_corrections_up);
    (void)add_1(qp, qp, in, 1);
    (void)sub(np, np, dn + in, dp, dn);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// the tier

[[nodiscard]] inline constexpr limb_t
mu_div_qr(limb_t *qp, limb_t *np, usize nn, const limb_t *dp, usize dn, limb_t *scratch) noexcept
{
  const usize qn = nn - dn;
  const usize in = mu_block_size(qn, dn);

  limb_t *const ip = scratch;             // in
  limb_t *const tp = ip + in;             // dn + in
  limb_t *const work = tp + dn + in;      // max(invert_n_itch(in), mul_itch(dn, in), dc_div_itch(dn))

  invert_n(ip, dp + dn - in, in, work);

  const limb_t qh = static_cast<limb_t>(cmp(np + qn, dp, dn) >= 0);
  if ( qh != 0 ) (void)sub_n(np + qn, np + qn, dp, dn);

  const usize r = qn % in;
  for ( usize j = qn - r; j >= in; ) {
    j -= in;
    mu_div_qr_block(qp + r + j, np + r + j, dp, dn, ip, in, tp, work);
  }

  if ( r != 0 ) {
    const limb_t dinv = invert_pi1(dp[dn - 1], dp[dn - 2]);
    if ( div_wants_dc(r + dn, dn) )
      (void)dc_div_qr(qp, np, r + dn, dp, dn, dinv, work);
    else
      (void)sbpi1_div_qr(qp, np, r + dn, dp, dn, dinv);
  }
  return qh;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
mu_div_qr_itch(usize nn, usize dn) noexcept
{
  if ( nn <= dn || dn < 2u ) return 0;
  const usize qn = nn - dn;
  const usize in = mu_block_size(qn, dn);

  usize w = invert_n_itch(in);
  const usize m = mul_itch(dn, in);
  if ( m > w ) w = m;
  const usize e = mul_itch(in, in);
  if ( e > w ) w = e;
  if ( (qn % in) != 0 && div_wants_dc(qn % in + dn, dn) ) {
    const usize t = dc_div_itch(dn);
    if ( t > w ) w = t;
  }
  return in + (dn + in) + w;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
div_wants_mu(usize nn, usize dn) noexcept
{
  return static_cast<u8>(clamp_to(pick_div(nn, dn), div_tier_cap)) >= static_cast<u8>(divalgo::mu) && dn >= 2u && nn > dn;
}

inline constexpr divalgo div_tiers_built = divalgo::mu;

static_assert(static_cast<u8>(div_tier_cap) <= static_cast<u8>(div_tiers_built),
              "arbint: MICRON_ARBINT_DIV_TIER_CAP names a tier above mpn::div_tiers_built");

[[nodiscard, gnu::flatten]] inline constexpr usize
divrem_itch(usize nn, usize dn) noexcept
{
  if ( dn <= 1 ) return 0;
  usize t = nn + 1u + dn;
  if ( div_wants_mu(nn + 1u, dn) )
    t += mu_div_qr_itch(nn + 1u, dn);
  else if ( div_wants_dc(nn + 1u, dn) )
    t += dc_div_itch(dn);
  return t;
}

// qp: nn - dn + 1 limbs, rp: dn limbs, scratch: divrem_itch(nn, dn) limbs
// requires nn >= dn >= 1 and {dp, dn} normalized in the sense that dp[dn-1] != 0
inline constexpr void
divrem(limb_t *qp, limb_t *rp, const limb_t *np, usize nn, const limb_t *dp, usize dn, limb_t *scratch) noexcept
{
  if ( dn == 1 ) {
    rp[0] = divrem_1(qp, np, nn, dp[0]);
    return;
  }

  const usize sh = limb_clz(dp[dn - 1]);
  limb_t *const dwork = scratch;
  limb_t *const nwork = scratch + dn;

  if ( sh != 0 ) {
    (void)lshift(dwork, dp, dn, sh);
    nwork[nn] = lshift(nwork, np, nn, sh);
  } else {
    copyi(dwork, dp, dn);
    copyi(nwork, np, nn);
    nwork[nn] = 0;
  }

  if ( div_wants_mu(nn + 1u, dn) ) {
    (void)mu_div_qr(qp, nwork, nn + 1u, dwork, dn, nwork + nn + 1u);
  } else {
    const limb_t dinv = invert_pi1(dwork[dn - 1], dwork[dn - 2]);
    if ( div_wants_dc(nn + 1u, dn) )
      (void)dc_div_qr(qp, nwork, nn + 1u, dwork, dn, dinv, nwork + nn + 1u);
    else
      (void)sbpi1_div_qr(qp, nwork, nn + 1u, dwork, dn, dinv);
  }

  if ( sh != 0 )
    (void)rshift(rp, nwork, dn, sh);
  else
    copyi(rp, nwork, dn);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pinned tiers

template<divalgo K>
inline constexpr void
divrem_with(limb_t *qp, limb_t *rp, const limb_t *np, usize nn, const limb_t *dp, usize dn, limb_t *scratch) noexcept
{
  static_assert(static_cast<u8>(K) <= static_cast<u8>(div_tiers_built), "arbint: this division tier is not implemented yet");
  if ( dn == 1 ) {
    rp[0] = divrem_1(qp, np, nn, dp[0]);
    return;
  }

  const usize sh = limb_clz(dp[dn - 1]);
  limb_t *const dwork = scratch;
  limb_t *const nwork = scratch + dn;

  if ( sh != 0 ) {
    (void)lshift(dwork, dp, dn, sh);
    nwork[nn] = lshift(nwork, np, nn, sh);
  } else {
    copyi(dwork, dp, dn);
    copyi(nwork, np, nn);
    nwork[nn] = 0;
  }

  limb_t *const tier = nwork + nn + 1u;
  bool done = false;
  if constexpr ( K == divalgo::mu ) {
    if ( nn + 1u > dn ) {
      (void)mu_div_qr(qp, nwork, nn + 1u, dwork, dn, tier);
      done = true;
    }
  }
  if ( !done ) {
    const limb_t dinv = invert_pi1(dwork[dn - 1], dwork[dn - 2]);
    if constexpr ( K != divalgo::sbpi1 ) {
      if ( dn >= 6u && nn + 1u > dn ) {
        (void)dc_div_qr(qp, nwork, nn + 1u, dwork, dn, dinv, tier);
        done = true;
      }
    }
    if ( !done ) (void)sbpi1_div_qr(qp, nwork, nn + 1u, dwork, dn, dinv);
  }

  if ( sh != 0 )
    (void)rshift(rp, nwork, dn, sh);
  else
    copyi(rp, nwork, dn);
}

template<divalgo K>
[[nodiscard, gnu::flatten]] inline constexpr usize
div_itch_with(usize nn, usize dn) noexcept
{
  if ( dn <= 1 ) return 0;
  usize t = nn + 1u + dn;
  if constexpr ( K == divalgo::mu ) {
    if ( nn + 1u > dn ) return t + mu_div_qr_itch(nn + 1u, dn);
  }
  if constexpr ( K != divalgo::sbpi1 ) {
    if ( dn >= 6u && nn + 1u > dn ) return t + dc_div_itch(dn);
  }
  return t;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
