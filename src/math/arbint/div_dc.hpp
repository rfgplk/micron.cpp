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
#include "mul.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// divide and conquer division
// (Burnikel-Ziegler)

namespace micron
{
namespace math
{
namespace mpn
{

[[nodiscard, gnu::always_inline]] inline constexpr usize
dc_div_itch(usize dn) noexcept
{
  return dn + mul_itch_forced(dn, dn);
}

// 2n limbs by n limbs, n quotient limbs
inline constexpr limb_t
dc_div_qr_n(limb_t *qp, limb_t *np, const limb_t *dp, usize n, limb_t dinv, limb_t *tp, limb_t *mulsc) noexcept
{
  const usize lo = n >> 1;
  const usize hi = n - lo;      // hi >= lo, so every mul below already has an >= bn

  limb_t qh = (hi < threshold::div_dc) ? sbpi1_div_qr(qp + lo, np + 2u * lo, 2u * hi, dp + lo, hi, dinv)
                                       : dc_div_qr_n(qp + lo, np + 2u * lo, dp + lo, hi, dinv, tp, mulsc);

  mul(tp, qp + lo, hi, dp, lo, mulsc);

  limb_t cy = sub_n(np + lo, np + lo, tp, n);
  if ( qh != 0 ) cy = static_cast<limb_t>(cy + sub_n(np + n, np + n, dp, lo));

  while ( cy != 0 ) {
    qh = static_cast<limb_t>(qh - sub_1(qp + lo, qp + lo, hi, 1));
    cy = static_cast<limb_t>(cy - add_n(np + lo, np + lo, dp, n));
  }

  const limb_t ql = (lo < threshold::div_dc) ? sbpi1_div_qr(qp, np + hi, 2u * lo, dp + hi, lo, dinv)
                                             : dc_div_qr_n(qp, np + hi, dp + hi, lo, dinv, tp, mulsc);

  mul(tp, dp, hi, qp, lo, mulsc);

  cy = sub_n(np, np, tp, n);
  if ( ql != 0 ) cy = static_cast<limb_t>(cy + sub_n(np + lo, np + lo, dp, hi));

  while ( cy != 0 ) {
    (void)sub_1(qp, qp, lo, 1);
    cy = static_cast<limb_t>(cy - add_n(np, np, dp, n));
  }
  return qh;
}

inline constexpr limb_t
dc_div_qr(limb_t *qp, limb_t *np, usize nn, const limb_t *dp, usize dn, limb_t dinv, limb_t *scratch) noexcept
{
  limb_t *const tp = scratch;
  limb_t *const mulsc = tp + dn;

  const usize qn = nn - dn;

  const limb_t qh = static_cast<limb_t>(cmp(np + qn, dp, dn) >= 0);
  if ( qh != 0 ) (void)sub_n(np + qn, np + qn, dp, dn);

  const usize r = qn % dn;
  for ( usize j = qn - r; j >= dn; ) {
    j -= dn;
    (void)dc_div_qr_n(qp + r + j, np + r + j, dp, dn, dinv, tp, mulsc);
  }
  if ( r != 0 ) (void)sbpi1_div_qr(qp, np, r + dn, dp, dn, dinv);

  return qh;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
div_wants_dc(usize nn, usize dn) noexcept
{
  return static_cast<u8>(clamp_to(pick_div(nn, dn), div_tier_cap)) >= static_cast<u8>(divalgo::dc) && dn >= 6u && nn > dn;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
