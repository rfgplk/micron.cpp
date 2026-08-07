//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div_mu.hpp"
#include "gcd.hpp"
#include "hgcd2.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "mul.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// extended Euclid
//
// cofactors accumulate as non negative magnitudes under a checkerboard convention
//
// ..r_{-1} = m, r_0 = a mod m               t_{-1} = 0, t_0 = 1
// ..r_{i+1} = r_{i-1} - q_i * r_i           t_{i+1} = t_{i-1} + q_i * t_i
// ..(-1)^i * t_i * a  ==  r_i  (mod m)      for every i

namespace micron
{
namespace math
{
namespace mpn
{

[[nodiscard, gnu::flatten]] inline constexpr usize
invmod_itch(usize an, usize mn) noexcept
{
  const usize s = mn + 3u;
  usize w = divrem_itch(2u * s, s);
  const usize m = mul_itch(s, s);
  if ( m > w ) w = m;
  if ( an > mn ) {
    const usize h = (an - mn + 1u) + divrem_itch(an, mn);
    if ( h > w ) w = h;
  }
  // r0 r1 r2 t0 t1 t2 q, the q*t product, the Lehmer path's second staging slot, then the tail
  return 7u * s + 2u * s + s + w;
}

[[nodiscard]] inline constexpr bool
invmod(limb_t *rp, usize &rn, const limb_t *ap, usize an, const limb_t *mp, usize mn, limb_t *scratch) noexcept
{
  mn = normalize(mp, mn);
  an = normalize(ap, an);
  rn = 0;
  if ( mn == 0 ) return false;

  if ( mn == 1 && mp[0] == 1 ) return true;

  const usize s = mn + 3u;
  limb_t *r0 = scratch;                  // s
  limb_t *r1 = r0 + s;                   // s
  limb_t *r2 = r1 + s;                   // s
  limb_t *t0 = r2 + s;                   // s
  limb_t *t1 = t0 + s;                   // s
  limb_t *t2 = t1 + s;                   // s
  limb_t *const q = t2 + s;              // s
  limb_t *const prod = q + s;            // 2s
  limb_t *const w2 = prod + 2u * s;      // s
  limb_t *const work = w2 + s;

  usize r0n = mn, r1n = 0, t0n = 0, t1n = 1;
  copyi(r0, mp, mn);
  t0[0] = 0;
  t1[0] = 1;

  // r_0 = a mod m
  if ( an == 0 ) {
    r1n = 0;
  } else if ( an < mn || cmp_var(ap, an, mp, mn) < 0 ) {
    copyi(r1, ap, an);
    r1n = an;
  } else {
    limb_t *const rq = work;
    limb_t *const rw = rq + (an - mn + 1u);
    divrem(rq, r1, ap, an, mp, mn, rw);
    r1n = normalize(r1, mn);
  }

  // k counts the index of r1
  usize k = 0;
  while ( r1n != 0 ) {
    // Lehmer
    if ( r0n == r1n && r0n >= 2u && r0n >= threshold::gcd_lehmer ) {
      const usize sh = limb_clz(r0[r0n - 1u]);
      limb_t A = r0[r0n - 1u];
      limb_t B = r1[r0n - 1u];
      if ( sh != 0 ) {
        A = static_cast<limb_t>((A << sh) | (r0[r0n - 2u] >> (limb_bits - sh)));
        B = static_cast<limb_t>((B << sh) | (r1[r0n - 2u] >> (limb_bits - sh)));
      }

      mat1 mm{}, nn{};
      const usize steps = hgcd2_ext(mm, nn, A, B);
      if ( steps != 0 ) {
        usize pn = 0;
        if ( mat1_mul_inverse_vector(mm, r2, q, r0, r1, r0n, pn) ) {
          // length is of the cofactors, not the operands
          usize tn = t0n > t1n ? t0n : t1n;
          if ( tn == 0 ) tn = 1u;
          if ( t0n < tn ) zero(t0 + t0n, tn - t0n);
          if ( t1n < tn ) zero(t1 + t1n, tn - t1n);
          (void)mat1_mul_vector(nn, prod, w2, t0, t1, tn);

          const usize a0 = normalize(r2, pn);
          const usize a1 = normalize(q, pn);
          const usize c0 = normalize(prod, tn + 2u);
          const usize c1 = normalize(w2, tn + 2u);

          const bool odd = (steps & 1u) != 0;
          copyi(r0, odd ? q : r2, odd ? a1 : a0);
          copyi(r1, odd ? r2 : q, odd ? a0 : a1);
          copyi(t0, odd ? w2 : prod, odd ? c1 : c0);
          copyi(t1, odd ? prod : w2, odd ? c0 : c1);
          r0n = odd ? a1 : a0;
          r1n = odd ? a0 : a1;
          t0n = odd ? c1 : c0;
          t1n = odd ? c0 : c1;
          k += steps;
          continue;
        }
        // high limbs disagreed; the matrix was not valid for the full operands
      }
    }

    divrem(q, r2, r0, r0n, r1, r1n, work);
    const usize qn = normalize(q, r0n - r1n + 1u);
    const usize r2n = normalize(r2, r1n);

    // t2 = t0 + q*t1
    usize t2n = 0;
    if ( qn == 0 || t1n == 0 ) {
      copyi(t2, t0, t0n);
      t2n = t0n;
    } else {
      const usize pn = qn + t1n;
      if ( qn >= t1n )
        mul(prod, q, qn, t1, t1n, work);
      else
        mul(prod, t1, t1n, q, qn, work);
      const usize pnn = normalize(prod, pn);
      if ( pnn >= t0n ) {
        const limb_t cy = add(t2, prod, pnn, t0, t0n);
        t2[pnn] = cy;
        t2n = normalize(t2, pnn + 1u);
      } else {
        const limb_t cy = add(t2, t0, t0n, prod, pnn);
        t2[t0n] = cy;
        t2n = normalize(t2, t0n + 1u);
      }
    }

    limb_t *sw = r0;
    r0 = r1;
    r1 = r2;
    r2 = sw;
    r0n = r1n;
    r1n = r2n;

    sw = t0;
    t0 = t1;
    t1 = t2;
    t2 = sw;
    t0n = t1n;
    t1n = t2n;
    ++k;
  }

  // g == r0
  if ( r0n != 1 || r0[0] != 1 ) return false;

  // (-1)^(k-1) * t0
  const bool negative = ((k - 1u) & 1u) != 0;
  if ( !negative || t0n == 0 ) {
    copyi(rp, t0, t0n);
    rn = t0n;
    return true;
  }
  (void)sub(rp, mp, mn, t0, t0n);
  rn = normalize(rp, mn);
  return true;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
