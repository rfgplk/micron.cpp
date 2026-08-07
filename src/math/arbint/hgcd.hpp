//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div_mu.hpp"
#include "hgcd2.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "mul.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// half-gcd
//
//  ..T(n) = 2*T(n/2) + Theta(M(n))
//  ..T(n) = Theta(M(n) log n)
//
//
// (via Moller's formulation [Niels Moller, "On Schoenhage's algorithm and subquadratic integer
// gcd computation", Math. Comp. 77, 2008,)
// matrix convention
//     ( a )   ( u00  u01 ) ( a' )              a' = u11*a - u01*b       b' = u00*b - u10*a
//     ( b ) = ( u10  u11 ) ( b' )

namespace micron
{
namespace math
{
namespace mpn
{

struct hgcd_mat {
  limb_t *p[2][2];
  usize n;
  usize alloc;
};

[[nodiscard, gnu::always_inline]] inline constexpr usize
hgcd_mat_itch(usize n) noexcept
{
  return 4u * (((n + 1u) / 2u) + 2u);
}

[[gnu::flatten]] inline constexpr void
hgcd_mat_init(hgcd_mat &M, usize n, limb_t *space) noexcept
{
  const usize s = ((n + 1u) / 2u) + 2u;
  zero(space, 4u * s);
  M.alloc = s;
  M.n = 1;
  M.p[0][0] = space;
  M.p[0][1] = space + s;
  M.p[1][0] = space + 2u * s;
  M.p[1][1] = space + 3u * s;
  M.p[0][0][0] = 1;
  M.p[1][1][0] = 1;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
hgcd_mat_mul_1_itch(usize mn) noexcept
{
  return 2u * (mn + 2u);
}

// M <- M * m1, for a single-limb matrix m1
[[gnu::flatten]] inline constexpr void
hgcd_mat_mul_1(hgcd_mat &M, const mat1 &m1, limb_t *tp) noexcept
{
  const mat1 t{ m1.u00, m1.u10, m1.u01, m1.u11 };

  const usize w = M.n + 2u;
  limb_t *const r0 = tp;
  limb_t *const r1 = tp + w;

  const usize n0 = mat1_mul_vector(t, r0, r1, M.p[0][0], M.p[0][1], M.n);
  copyi(M.p[0][0], r0, w);
  copyi(M.p[0][1], r1, w);

  const usize n1 = mat1_mul_vector(t, r0, r1, M.p[1][0], M.p[1][1], M.n);
  copyi(M.p[1][0], r0, w);
  copyi(M.p[1][1], r1, w);

  M.n = n0 > n1 ? n0 : n1;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
matrix22_mul_itch(usize rn, usize mn) noexcept
{
  const usize w = rn + mn + 1u;
  const usize a = rn > mn ? rn : mn;
  const usize b = rn > mn ? mn : rn;
  return 4u * w + (rn + mn) + mul_itch(a, b);
}

[[gnu::always_inline]] inline constexpr void
__mul_ord(limb_t *rp, const limb_t *ap, usize an, const limb_t *bp, usize bn, limb_t *sc) noexcept
{
  if ( an >= bn )
    mul(rp, ap, an, bp, bn, sc);
  else
    mul(rp, bp, bn, ap, an, sc);
}

// never flatten this (or anything that invokes mul)
inline constexpr void
matrix22_mul(hgcd_mat &M, const hgcd_mat &M1, limb_t *tp) noexcept
{
  const usize rn = M.n;
  const usize mn = M1.n;
  const usize pn = rn + mn;
  const usize w = pn + 1u;

  limb_t *const o00 = tp;
  limb_t *const o01 = o00 + w;
  limb_t *const o10 = o01 + w;
  limb_t *const o11 = o10 + w;
  limb_t *const prod = o11 + w;
  limb_t *const work = prod + pn;

  limb_t *const out[2][2] = { { o00, o01 }, { o10, o11 } };

  for ( usize i = 0; i < 2u; ++i ) {
    for ( usize j = 0; j < 2u; ++j ) {
      limb_t *const o = out[i][j];
      __mul_ord(o, M.p[i][0], rn, M1.p[0][j], mn, work);
      __mul_ord(prod, M.p[i][1], rn, M1.p[1][j], mn, work);
      o[pn] = add_n(o, o, prod, pn);
    }
  }

  usize nn = 1;
  for ( usize i = 0; i < 2u; ++i ) {
    for ( usize j = 0; j < 2u; ++j ) {
      const usize k = normalize(out[i][j], w);
      if ( k > nn ) nn = k;
    }
  }

  for ( usize i = 0; i < 2u; ++i )
    for ( usize j = 0; j < 2u; ++j ) copyi(M.p[i][j], out[i][j], w);

  M.n = nn;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
hgcd_mat_adjust_itch(usize p, usize mn) noexcept
{
  const usize a = p > mn ? p : mn;
  const usize b = p > mn ? mn : p;
  return 3u * (p + mn) + mul_itch(a, b);
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__adjust_width(usize k, usize mn, usize h) noexcept
{
  usize w = k > mn + 1u ? k : mn + 1u;
  if ( w > h ) w = h;
  return w;
}

[[nodiscard]] inline constexpr usize
hgcd_mat_adjust(const hgcd_mat &M, usize n, limb_t *ap, limb_t *bp, usize p, limb_t *tp) noexcept
{
  const usize mn = M.n;
  const usize w = p + mn;

  limb_t *const t0 = tp;          // u11 * a_lo
  limb_t *const t1 = t0 + w;      // u10 * a_lo
  limb_t *const t2 = t1 + w;      // u01 * b_lo, then u00 * b_lo
  limb_t *const work = t2 + w;

  __mul_ord(t0, ap, p, M.p[1][1], mn, work);
  __mul_ord(t1, ap, p, M.p[1][0], mn, work);
  __mul_ord(t2, bp, p, M.p[0][1], mn, work);

  // a <- a_hi*B^p + t0 - t2
  copyi(ap, t0, p);
  limb_t ah = add(ap + p, ap + p, n - p, t0 + p, mn);
  ah = static_cast<limb_t>(ah - sub(ap, ap, n, t2, w));

  __mul_ord(t2, bp, p, M.p[0][0], mn, work);

  // b <- b_hi*B^p + t2 - t1
  copyi(bp, t2, p);
  limb_t bh = add(bp + p, bp + p, n - p, t2 + p, mn);
  bh = static_cast<limb_t>(bh - sub(bp, bp, n, t1, w));

  (void)ah;
  (void)bh;

  const usize an = normalize(ap, n);
  const usize bn = normalize(bp, n);
  return an > bn ? an : bn;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
hgcd_mat_update_q_itch(usize mn, usize qn) noexcept
{
  const usize a = mn > qn ? mn : qn;
  const usize b = mn > qn ? qn : mn;
  return (mn + qn + 1u) + mul_itch(a, b);
}

// requires M.n + qn + 1 <= M.alloc
inline constexpr void
hgcd_mat_update_q(hgcd_mat &M, const limb_t *qp, usize qn, limb_t *tp) noexcept
{
  const usize mn = M.n;
  const usize pn = mn + qn;
  const usize w = pn + 1u;

  limb_t *const t = tp;
  limb_t *const work = t + w;

  usize nn = 1;
  for ( usize i = 0; i < 2u; ++i ) {
    __mul_ord(t, M.p[i][0], mn, qp, qn, work);
    t[pn] = add(t, t, pn, M.p[i][1], mn);

    copyi(M.p[i][1], M.p[i][0], mn);
    copyi(M.p[i][0], t, w);

    const usize k = normalize(t, w);
    if ( k > nn ) nn = k;
  }
  M.n = nn;
}

[[nodiscard, gnu::always_inline]] inline constexpr bool
__mat1_make_even(mat1 &m) noexcept
{
  if ( m.u00 > m.u01 ) return true;

  const limb_t q = static_cast<limb_t>(m.u01 / m.u00);
  m.u01 = static_cast<limb_t>(m.u01 - q * m.u00);
  m.u11 = static_cast<limb_t>(m.u11 - q * m.u10);

  return !(m.u00 == 1u && m.u01 == 0u && m.u10 == 0u && m.u11 == 1u);
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
subdiv_step_itch(usize n) noexcept
{
  return (n + 1u) + n + divrem_itch(n, n);
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
hgcd_mat_update_q_itch_max(usize n) noexcept
{
  const usize mn = (n + 1u) / 2u + 2u;
  return hgcd_mat_update_q_itch(mn, mn);
}

struct __gcd_hook {
  [[nodiscard]] constexpr bool
  quotient(const limb_t *, usize) noexcept
  {
    return true;
  }
};

template<class Hook>
[[nodiscard]] inline constexpr usize
__subdiv_step(limb_t *ap, limb_t *bp, usize n, Hook &hook, limb_t *tp) noexcept
{
  const usize an = normalize(ap, n);
  const usize bn = normalize(bp, n);
  if ( bn == 0 || an < bn ) return 0;

  limb_t *const qp = tp;                       // an - bn + 1
  limb_t *const rp = qp + (an - bn + 1u);      // bn
  limb_t *const work = rp + bn;

  divrem(qp, rp, ap, an, bp, bn, work);
  const usize qn = normalize(qp, an - bn + 1u);
  const usize rn = normalize(rp, bn);

  // nothing has moved yet, so a refusal costs the division and no correctness
  if ( !hook.quotient(qp, qn) ) return 0;

  copyi(ap, bp, bn);
  zero(ap + bn, n - bn);
  copyi(bp, rp, rn);
  zero(bp + rn, n - rn);
  return bn;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
hgcd_subdiv_pair_itch(usize n) noexcept
{
  const usize d = divrem_itch(n, n);
  const usize u = hgcd_mat_update_q_itch_max(n);
  return (n + 1u) + 3u * n + 2u + (d > u ? d : u);
}

[[nodiscard]] inline constexpr usize
hgcd_subdiv_pair(limb_t *ap, limb_t *bp, usize n, hgcd_mat &M, limb_t *tp) noexcept
{
  const usize an = normalize(ap, n);
  const usize bn = normalize(bp, n);
  if ( bn == 0 || an < bn ) return 0;

  limb_t *const q1 = tp;                       // an - bn + 1
  limb_t *const r1 = q1 + (an - bn + 1u);      // bn
  limb_t *const q2 = r1 + bn;                  // bn + 1
  limb_t *const r2 = q2 + bn + 1u;             // bn
  limb_t *const work = r2 + bn;

  divrem(q1, r1, ap, an, bp, bn, work);
  const usize q1n = normalize(q1, an - bn + 1u);
  const usize r1n = normalize(r1, bn);
  if ( r1n == 0 ) return 0;

  divrem(q2, r2, bp, bn, r1, r1n, work);
  const usize q2n = normalize(q2, bn - r1n + 1u);
  const usize r2n = normalize(r2, r1n);

  if ( M.n + q1n + q2n + 2u > M.alloc ) return 0;

  hgcd_mat_update_q(M, q1, q1n, work);
  hgcd_mat_update_q(M, q2, q2n, work);

  copyi(ap, r1, r1n);
  zero(ap + r1n, n - r1n);
  copyi(bp, r2, r2n);
  zero(bp + r2n, n - r2n);
  return r1n;
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
hgcd_step_itch(usize n) noexcept
{
  const usize win = 2u * n + hgcd_mat_mul_1_itch((n + 1u) / 2u + 2u);
  const usize div = hgcd_subdiv_pair_itch(n);
  return win > div ? win : div;
}

[[nodiscard]] inline constexpr usize
hgcd_step(usize n, limb_t *ap, limb_t *bp, usize s, hgcd_mat &M, limb_t *tp) noexcept
{
  const limb_t top = static_cast<limb_t>(ap[n - 1u] | bp[n - 1u]);
  if ( top != 0 && M.n + 2u <= M.alloc && cmp_var(ap, n, bp, n) > 0 ) {
    const usize sh = limb_clz(top);
    limb_t ah = 0, al = 0, bh = 0, bl = 0;
    __window2(ah, al, ap, n, sh);
    __window2(bh, bl, bp, n, sh);

    mat1 m1{};
    if ( hgcd2_sel(m1, ah, al, bh, bl) && __mat1_make_even(m1) ) {
      limb_t *const t0 = tp;
      limb_t *const t1 = tp + n;
      usize n2 = 0;
      if ( mat1_mul_inverse_vector(m1, t0, t1, ap, bp, n, n2) ) {
        if ( n2 != 0 && cmp_var(t0, n2, t1, n2) > 0 ) {
          const usize an = normalize(t0, n2);
          if ( an > s ) {
            copyi(ap, t0, n);
            copyi(bp, t1, n);
            hgcd_mat_mul_1(M, m1, tp + 2u * n);
            return an;
          }
        }
      }
    }
  }

  return hgcd_subdiv_pair(ap, bp, n, M, tp);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// recursion
//
// reduce (a; b) of n limbs until the pair fits about half that, accumulating the matrix
// Moller's shape; reduce the top half by recursion, take a few steps to trim, reduce again, and finish by iterating
//
//   pre:   n >= 2, a > b, ap[n-1] | bp[n-1] != 0, M the identity, M.alloc >= (n+1)/2 + 2
//   post:  0   -- no progress, and ap, bp and M are untouched
//          nn  -- the reduced pair is in (ap; bp) at nn limbs, det M == 1, and
//                 M * (a'; b') == (a; b) EXACTLY against the pair that came in

[[nodiscard]] inline constexpr usize
hgcd_itch(usize n) noexcept
{
  const usize s = n / 2u + 1u;
  if ( n <= s ) return 0;

  const usize step = hgcd_step_itch(n);
  if ( n < threshold::gcd_hgcd ) return step;

  const usize p = n / 2u;
  const usize h = n - p;                    // first sub-problem
  const usize hn = (h + 1u) / 2u + 2u;      // widest matrix it may hand back

  const usize sub = h > p ? h : p;
  const usize rec = hgcd_itch(sub);

  usize m = rec;
  const usize adj1 = hgcd_mat_adjust_itch(p, hn);
  if ( adj1 > m ) m = adj1;
  if ( step > m ) m = step;

  const usize pn = (p + 1u) / 2u + 2u;
  usize inner = rec;
  const usize adj3 = hgcd_mat_adjust_itch(p, pn);
  if ( adj3 > inner ) inner = adj3;
  const usize mm = matrix22_mul_itch((n + 1u) / 2u + 2u, pn);
  if ( mm > inner ) inner = mm;

  const usize blk = hgcd_mat_itch(p) + inner;
  return blk > m ? blk : m;
}

[[nodiscard]] inline constexpr usize
hgcd(limb_t *ap, limb_t *bp, usize n, hgcd_mat &M, limb_t *tp) noexcept
{
  const usize s = n / 2u + 1u;
  if ( n <= s ) return 0;

  usize nn = n;
  bool success = false;

  if ( n >= threshold::gcd_hgcd ) {
    const usize p = n / 2u;
    {
      const usize h = n - p;
      const usize save = M.alloc;

      usize want = (h + 1u) / 2u + 2u;
      if ( h >= 2u && want > h - 1u ) want = h - 1u;
      M.alloc = want < save ? want : save;

      const usize k = hgcd(ap + p, bp + p, h, M, tp);
      M.alloc = save;
      if ( k > 0 ) {
        nn = hgcd_mat_adjust(M, p + __adjust_width(k, M.n, h), ap, bp, p, tp);
        success = true;
      }
    }

    const usize n2 = (3u * n) / 4u + 1u;
    while ( nn > n2 ) {
      if ( normalize(bp, nn) == 0 ) return success ? nn : 0;
      const usize k = hgcd_step(nn, ap, bp, s, M, tp);
      if ( k == 0 ) return success ? nn : 0;
      nn = k;
      success = true;
    }

    // reduce again, this time folding the result in with a matrix product
    if ( nn > s + 2u ) {
      const usize p2 = 2u * s - nn + 1u;
      const usize m = nn - p2;

      // what M can still absorb
      usize room = M.alloc > M.n + 1u ? M.alloc - M.n - 1u : 0u;
      if ( m >= 2u && room > m - 1u ) room = m - 1u;
      if ( m >= 2u && room >= 3u ) {
        hgcd_mat M1{};
        hgcd_mat_init(M1, m, tp);
        if ( M1.alloc > room ) M1.alloc = room;
        limb_t *const w = tp + hgcd_mat_itch(m);

        const usize k = hgcd(ap + p2, bp + p2, m, M1, w);
        if ( k > 0 ) {
          nn = hgcd_mat_adjust(M1, p2 + __adjust_width(k, M1.n, m), ap, bp, p2, w);
          matrix22_mul(M, M1, w);
          success = true;
        }
      }
    }
  }

  for ( ;; ) {
    if ( nn <= s ) break;
    if ( normalize(bp, nn) == 0 ) break;
    const usize k = hgcd_step(nn, ap, bp, s, M, tp);
    if ( k == 0 ) break;
    nn = k;
    success = true;
  }

  return success ? nn : 0;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
