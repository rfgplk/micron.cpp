//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div_mu.hpp"
#include "gcd_base.hpp"
#include "hgcd2.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// Lehmer (accelerated Euclid)
//
// Lehmer simulates a whole run of Euclidean steps on the leading limbs;
// each step costs a single limb divide, then applies the accumulated 2x2 matrix to the full operands
// run of ten steps costs ten limb divides plus one pass

namespace micron
{
namespace math
{
namespace mpn
{

[[nodiscard, gnu::flatten]] inline constexpr usize
gcd_lehmer_itch(usize un, usize vn) noexcept
{
  if ( vn > un ) {
    const usize t = un;
    un = vn;
    vn = t;
  }
  if ( vn == 0 ) return 0;
  return 4u * un + (un + 1u) + divrem_itch(un, vn);
}

// un >= vn >= 1
[[nodiscard]] inline constexpr usize
gcd_lehmer(limb_t *gp, const limb_t *up, usize un, const limb_t *vp, usize vn, limb_t *scratch) noexcept
{
  limb_t *a = scratch;           // un
  limb_t *b = a + un;            // un
  limb_t *t = b + un;            // un
  limb_t *s = t + un;            // un
  limb_t *const q = s + un;      // un + 1
  limb_t *const work = q + un + 1u;

  usize an = un, bn = vn;
  copyi(a, up, un);
  copyi(b, vp, vn);

  for ( ;; ) {
    if ( bn == 0 ) break;
    if ( an <= 2 && bn <= 2 ) {
      limb_t g1 = 0, g0 = 0;
      gcd_22_any(g1, g0, an > 1 ? a[1] : 0, a[0], bn > 1 ? b[1] : 0, b[0]);
      a[0] = g0;
      a[1] = g1;
      an = normalize(a, 2);
      bn = 0;
      break;
    }
    if ( bn == 1 ) {
      a[0] = gcd_1(a, an, b[0]);
      an = a[0] != 0 ? 1u : 0u;
      bn = 0;
      break;
    }

    if ( an == bn ) {
      const usize sh = limb_clz(a[an - 1u]);
      limb_t A = 0, A0 = 0, B = 0, B0 = 0;
      __window2(A, A0, a, an, sh);
      __window2(B, B0, b, an, sh);

      mat1 m{};
      if ( hgcd2_sel(m, A, A0, B, B0) ) {
        usize n2 = 0;
        if ( mat1_mul_inverse_vector(m, t, s, a, b, an, n2) ) {
          limb_t *sw = a;
          a = t;
          t = sw;
          sw = b;
          b = s;
          s = sw;
          an = normalize(a, n2);
          bn = normalize(b, n2);
          if ( cmp_var(a, an, b, bn) < 0 ) {
            sw = a;
            a = b;
            b = sw;
            const usize tn = an;
            an = bn;
            bn = tn;
          }
          continue;
        }
        // the matrix was not valid for the full operands
      }
    }

    // no run could be validated
    divrem(q, t, a, an, b, bn, work);
    const usize tn = normalize(t, bn);
    limb_t *sw = a;
    a = b;
    b = t;
    t = sw;
    an = bn;
    bn = tn;
  }

  if ( an == 0 ) return 0;
  copyi(gp, a, an);
  return an;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
