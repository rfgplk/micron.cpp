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
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Karatsuba
// three half-size products instead of four, O(n^1.585)
//
// split a = a0 + B^k a1 and b = b0 + B^k b1, then
//     a*b = z0 + B^k (z0 + z2 - (a0-a1)(b0-b1)) + B^2k z2, z0 = a0 b0, z2 = a1 b1
//
// the middle term is written as a difference of halves rather than the textbook (a0+a1)(b0+b1)
//
// NOTE: rp is distinct from ap and bp and holds an + bn limbs
// an >= bn >= 1

namespace micron
{
namespace math
{
namespace mpn
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scratch accounting
//
// per level: |a0-a1| (k) + |b0-b1| (k) + vm1 (2k) + mid (2k+1)

inline constexpr usize karatsuba_force = 2u;

[[nodiscard, gnu::flatten]] inline constexpr usize
karatsuba_itch(usize n, usize cutoff = threshold::mul_karatsuba) noexcept
{
  if ( cutoff < 2u ) cutoff = 2u;
  usize total = 0;
  while ( n >= cutoff && n >= 2u ) {
    const usize k = (n + 1u) / 2u;
    total += 6u * k + 1u;
    n = k;
  }
  return total;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
unbalanced_stage_itch(usize an, usize bn, usize cutoff) noexcept
{
  if ( cutoff < 2u ) cutoff = 2u;
  usize total = 0;
  while ( an > bn && bn >= cutoff && bn >= 2u ) {
    total += 2u * bn;
    const usize take = an % bn;
    if ( take == 0 ) break;
    an = bn;
    bn = take;
  }
  return total;
}

inline constexpr void __mul_rec(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn,
                                limb_t *scratch, usize cutoff) noexcept;

inline constexpr void
mul_karatsuba_n(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, const limb_t *__restrict__ bp, usize n, limb_t *scratch,
                usize cutoff) noexcept
{
  const usize k = (n + 1u) / 2u;
  const usize h = n - k;

  limb_t *const asm1 = scratch;          // k
  limb_t *const bsm1 = asm1 + k;         // k
  limb_t *const vm1 = bsm1 + k;          // 2k
  limb_t *const mid = vm1 + 2u * k;      // 2k + 1
  limb_t *const rec = mid + 2u * k + 1u;

  // |a0 - a1|, and the same for b
  copyi(asm1, ap + k, h);
  zero(asm1 + h, k - h);
  const bool asign = cmp(ap, asm1, k) < 0;
  if ( asign )
    (void)sub_n(asm1, asm1, ap, k);
  else
    (void)sub_n(asm1, ap, asm1, k);

  copyi(bsm1, bp + k, h);
  zero(bsm1 + h, k - h);
  const bool bsign = cmp(bp, bsm1, k) < 0;
  if ( bsign )
    (void)sub_n(bsm1, bsm1, bp, k);
  else
    (void)sub_n(bsm1, bp, bsm1, k);

  __mul_rec(rp, ap, k, bp, k, rec, cutoff);                       // z0 -> rp[0, 2k)
  __mul_rec(rp + 2u * k, ap + k, h, bp + k, h, rec, cutoff);      // z2 -> rp[2k, 2n)
  __mul_rec(vm1, asm1, k, bsm1, k, rec, cutoff);                  // |a0-a1| * |b0-b1|

  mid[2u * k] = add(mid, rp, 2u * k, rp + 2u * k, 2u * h);
  if ( asign == bsign )
    (void)sub(mid, mid, 2u * k + 1u, vm1, 2u * k);
  else
    (void)add(mid, mid, 2u * k + 1u, vm1, 2u * k);

  const limb_t cy = add(rp + k, rp + k, 2u * n - k, mid, n + 1u);
  (void)cy;
}

inline constexpr void
mul_karatsuba_unbalanced(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn,
                         limb_t *scratch, usize cutoff) noexcept
{
  limb_t *const block = scratch;      // 2*bn
  limb_t *const rec = block + 2u * bn;

  usize done = bn < an ? bn : an;
  __mul_rec(rp, ap, done, bp, bn, rec, cutoff);
  zero(rp + done + bn, an - done);

  while ( done < an ) {
    const usize take = (an - done) < bn ? (an - done) : bn;
    __mul_rec(block, bp, bn, ap + done, take, rec, cutoff);
    const limb_t cy = add(rp + done, rp + done, an + bn - done, block, take + bn);
    (void)cy;
    done += take;
  }
}

inline constexpr void
__mul_rec(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn, limb_t *scratch,
          usize cutoff) noexcept
{
  if ( bn == 1 ) {
    rp[an] = mul_1(rp, ap, an, bp[0]);
    return;
  }
  if ( bn < cutoff ) {
    if ( bn < threshold::mul_comba )
      mul_basecase(rp, ap, an, bp, bn);
    else
      mul_comba(rp, ap, an, bp, bn);
    return;
  }
  if ( an == bn )
    mul_karatsuba_n(rp, ap, bp, an, scratch, cutoff);
  else
    mul_karatsuba_unbalanced(rp, ap, an, bp, bn, scratch, cutoff);
}

inline constexpr void
mul_karatsuba(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn, limb_t *scratch,
              usize cutoff = threshold::mul_karatsuba) noexcept
{
  __mul_rec(rp, ap, an, bp, bn, scratch, cutoff < 2u ? 2u : cutoff);
}

inline constexpr void
mul_karatsuba_top(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn,
                  limb_t *scratch) noexcept
{
  if ( bn < 2u ) {
    __mul_rec(rp, ap, an, bp, bn, scratch, threshold::mul_karatsuba);
    return;
  }
  if ( an == bn )
    mul_karatsuba_n(rp, ap, bp, an, scratch, threshold::mul_karatsuba);
  else
    mul_karatsuba_unbalanced(rp, ap, an, bp, bn, scratch, threshold::mul_karatsuba);
}

inline constexpr void sqr_karatsuba(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch,
                                    usize cutoff) noexcept;

inline constexpr void
sqr_karatsuba_n(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch, usize cutoff) noexcept
{
  const usize k = (n + 1u) / 2u;
  const usize h = n - k;

  limb_t *const asm1 = scratch;
  limb_t *const vm1 = asm1 + k;
  limb_t *const mid = vm1 + 2u * k;
  limb_t *const rec = mid + 2u * k + 1u;

  copyi(asm1, ap + k, h);
  zero(asm1 + h, k - h);
  if ( cmp(ap, asm1, k) < 0 )
    (void)sub_n(asm1, asm1, ap, k);
  else
    (void)sub_n(asm1, ap, asm1, k);

  sqr_karatsuba(rp, ap, k, rec, cutoff);
  sqr_karatsuba(rp + 2u * k, ap + k, h, rec, cutoff);
  sqr_karatsuba(vm1, asm1, k, rec, cutoff);

  mid[2u * k] = add(mid, rp, 2u * k, rp + 2u * k, 2u * h);
  (void)sub(mid, mid, 2u * k + 1u, vm1, 2u * k);      // (a0-a1)^2 is never negative
  (void)add(rp + k, rp + k, 2u * n - k, mid, n + 1u);
}

inline constexpr void
sqr_karatsuba(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch,
              usize cutoff = threshold::sqr_karatsuba) noexcept
{
  if ( cutoff < 2u ) cutoff = 2u;
  if ( n < cutoff || n < 2u ) {
    sqr_basecase(rp, ap, n);
    return;
  }
  sqr_karatsuba_n(rp, ap, n, scratch, cutoff);
}

inline constexpr void
sqr_karatsuba_top(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch) noexcept
{
  if ( n < 2u ) {
    sqr_basecase(rp, ap, n);
    return;
  }
  sqr_karatsuba_n(rp, ap, n, scratch, threshold::sqr_karatsuba);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_karatsuba_itch(usize n, usize cutoff = threshold::sqr_karatsuba) noexcept
{
  if ( cutoff < 2u ) cutoff = 2u;
  usize total = 0;
  while ( n >= cutoff && n >= 2u ) {
    const usize k = (n + 1u) / 2u;
    total += 5u * k + 1u;
    n = k;
  }
  return total;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
