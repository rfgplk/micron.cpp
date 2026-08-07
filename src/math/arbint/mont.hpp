//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div.hpp"
#include "div_mu.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "mul.hpp"
#include "mul_toom4.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Montgomery arithmetic
// (multiplication mod m with no division in the loop)
//
// representation is a*R mod m with R = B^n

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// context

// preconditions:
// ..mp[0] must be ODD
// ..mp[n-1] must be NONZERO
struct mont_ctx {
  const limb_t *mp;      // n limbs, borrowed, must outlive the context
  usize n;
  limb_t minv;      // -m^-1 mod B
};

[[nodiscard, gnu::always_inline]] inline constexpr mont_ctx
mont_make(const limb_t *mp, usize n) noexcept
{
  return mont_ctx{ mp, n, static_cast<limb_t>(static_cast<limb_t>(0) - binv_odd(mp[0])) };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// REDC
//
// rp = T * R^-1 mod m, for {tp, 2n} holding T < m*R

[[gnu::flatten]] inline constexpr void
redc(limb_t *rp, limb_t *tp, const mont_ctx &c) noexcept
{
  const usize n = c.n;
  limb_t carry = 0;
  for ( usize i = 0; i < n; ++i ) {
    const limb_t u = static_cast<limb_t>(tp[i] * c.minv);
    const limb_t cy = addmul_1(tp + i, c.mp, n, u);
    carry = static_cast<limb_t>(carry + add_1(tp + i + n, tp + i + n, n - i, cy));
  }
  const limb_t ge = static_cast<limb_t>(carry != 0 || cmp(tp + n, c.mp, n) >= 0);
  (void)cnd_sub_n(ge, rp, tp + n, c.mp, n);
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
redc_itch(usize) noexcept
{
  return 0;
}

inline constexpr void
mont_mul(limb_t *rp, const limb_t *ap, const limb_t *bp, const mont_ctx &c, limb_t *scratch) noexcept
{
  limb_t *const prod = scratch;              // 2n
  limb_t *const work = prod + 2u * c.n;      // mul_itch(n, n)
  mul(prod, ap, c.n, bp, c.n, work);
  redc(rp, prod, c);
}

inline constexpr void
mont_sqr(limb_t *rp, const limb_t *ap, const mont_ctx &c, limb_t *scratch) noexcept
{
  limb_t *const prod = scratch;              // 2n
  limb_t *const work = prod + 2u * c.n;      // sqr_itch(n)
  sqr(prod, ap, c.n, work);
  redc(rp, prod, c);
}

// rp = a * R mod m
inline constexpr void
to_mont(limb_t *rp, const limb_t *ap, usize an, const mont_ctx &c, limb_t *scratch) noexcept
{
  const usize n = c.n;
  an = normalize(ap, an);
  if ( an == 0 ) {
    zero(rp, n);
    return;
  }
  const usize nn = an + n;
  limb_t *const np = scratch;             // an + n
  limb_t *const qp = np + nn;             // an + 1
  limb_t *const work = qp + an + 1u;      // divrem_itch(an + n, n)

  zero(np, n);
  copyi(np + n, ap, an);
  divrem(qp, rp, np, nn, c.mp, n, work);
}

// rp = a * R^-1 mod m, the inverse of to_mont
inline constexpr void
from_mont(limb_t *rp, const limb_t *ap, const mont_ctx &c, limb_t *scratch) noexcept
{
  limb_t *const tp = scratch;      // 2n
  copyi(tp, ap, c.n);
  zero(tp + c.n, c.n);
  redc(rp, tp, c);
}

inline constexpr void
mont_one(limb_t *rp, const mont_ctx &c, limb_t *scratch) noexcept
{
  const limb_t one = 1;
  to_mont(rp, &one, 1, c, scratch);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
mont_mul_itch(usize n) noexcept
{
  return 2u * n + mul_itch(n, n);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
mont_sqr_itch(usize n) noexcept
{
  return 2u * n + sqr_itch(n);
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
from_mont_itch(usize n) noexcept
{
  return 2u * n;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
to_mont_itch(usize an, usize n) noexcept
{
  if ( an == 0 ) an = 1;
  return (an + n) + (an + 1u) + divrem_itch(an + n, n);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
mont_op_itch(usize n) noexcept
{
  usize t = mont_mul_itch(n);
  const usize s = mont_sqr_itch(n);
  if ( s > t ) t = s;
  const usize f = from_mont_itch(n);
  if ( f > t ) t = f;
  const usize o = to_mont_itch(n, n);
  if ( o > t ) t = o;
  return t;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
