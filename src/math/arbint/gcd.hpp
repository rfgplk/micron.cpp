//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "div_mu.hpp"
#include "gcd_base.hpp"
#include "gcd_lehmer.hpp"
#include "hgcd.hpp"
#include "hgcd2.hpp"
#include "limb.hpp"
#include "mpn_core.hpp"
#include "tags.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// the gcd ladder
//
// THREE RUNGS AND FOUR THRESHOLDS, and that mismatch is the one thing to get right here.
// threshold::gcd_hgcd is NOT a rung -- it is the half-gcd recursion's own cutoff, the size below
// which hgcd iterates instead of recursing, exactly the way `cutoff` is internal to the Karatsuba
// recursion and never appears in pick_mul. gcd_dc is where the ladder starts calling hgcd at all.
// somebody will otherwise write a fourth case and wonder why it is dead.
//
// WHAT IS BUILT is gcd_tiers_built, and the ladder saturates there rather than pretending, the same
// way mul() clamps to mpn::tiers_built. gcd_tier_cap is the separate question of what the automatic
// path may SELECT, which is a measurement result and not a capability.
//
// there is no solver:: tag for a gcd tier and there should not be: the Solver template parameter
// names a MULTIPLICATION tier and means nothing here. pinning a gcd tier is gcd_with<>, which is
// what the differential and the crossover bench call, and the class always takes the ladder.

namespace micron
{
namespace math
{
namespace mpn
{

// ALL THREE TIERS ARE BUILT. what the automatic path may SELECT is gcd_tier_cap, which is a
// measurement result and not a capability -- Lehmer earned its window and sits at 1; the dc tier
// ships built, differentially tested and capped out until the crossover bench finds one for it,
// the same treatment comba and toom-3 have. gcd_with<gcd_algo::dc> and a --def on the cap are how
// it gets exercised meanwhile.
inline constexpr gcd_algo gcd_tiers_built = gcd_algo::dc;

static_assert(static_cast<u8>(gcd_tier_cap) <= static_cast<u8>(gcd_tiers_built),
              "arbint: MICRON_ARBINT_GCD_TIER_CAP names a tier above mpn::gcd_tiers_built -- the ladder cannot select "
              "what is not built");

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the divide-and-conquer tier
//
// reduce the pair by half with hgcd, repeat, and hand the remainder to Lehmer once the operands are
// small enough that the recursion's constant no longer pays. THE MATRIX IS DISCARDED at every
// round: gcd wants only hgcd's side effect on the pair, which is a reduction that preserves the
// gcd. gcdext is the caller that would want the matrix, and it stays on the Lehmer cofactors.

[[nodiscard, gnu::flatten]] inline constexpr usize
gcd_dc_itch(usize un, usize vn) noexcept
{
  if ( vn > un ) {
    const usize t = un;
    un = vn;
    vn = t;
  }
  if ( vn == 0 ) return 0;

  // the two working copies, then whichever of the three consumers is largest -- they are sequential
  usize w = hgcd_mat_itch(un) + hgcd_itch(un);
  const usize d = subdiv_step_itch(un);
  if ( d > w ) w = d;
  // the hand-off may happen at any size the loop exits at, including un itself when the pair is
  // already below the threshold, so Lehmer is charged at full width rather than at the threshold
  const usize l = gcd_lehmer_itch(un, un);
  if ( l > w ) w = l;

  return 2u * un + w;
}

// same contract as gcd_binary and gcd_lehmer: un >= vn >= 1, both normalized and nonzero, gp holds
// vn limbs, the operands survive the call, and the return is the gcd's length.
//
// the operands are copied in because hgcd MUTATES its pair in place -- it has to, the recursion
// works on a sub-slice of the same buffers -- and because the differential runs two tiers back to
// back on the same inputs.
[[nodiscard]] inline constexpr usize
gcd_dc(limb_t *gp, const limb_t *up, usize un, const limb_t *vp, usize vn, limb_t *scratch) noexcept
{
  limb_t *a = scratch;             // un
  limb_t *b = a + un;              // un
  limb_t *const work = b + un;

  copyi(a, up, un);
  copyi(b, vp, vn);
  zero(b + vn, un - vn);

  usize n = un;
  for ( ;; ) {
    usize an = normalize(a, n);
    const usize bn = normalize(b, n);
    if ( bn == 0 ) {
      n = an;
      break;
    }
    if ( an < bn ) {
      limb_t *const t = a;
      a = b;
      b = t;
      an = bn;
    }
    n = an;
    if ( n <= threshold::gcd_dc ) break;

    // hgcd reads n limbs of both, so the shorter one is padded rather than trusted to be clean
    zero(b + normalize(b, n), n - normalize(b, n));

    hgcd_mat M{};
    hgcd_mat_init(M, n, work);
    const usize k = hgcd(a, b, n, M, work + hgcd_mat_itch(n));
    if ( k != 0 ) {
      n = k;
      continue;
    }

    // hgcd could not move the pair -- one genuine Euclidean step, which always can
    __gcd_hook hook{};
    const usize t = __subdiv_step(a, b, n, hook, work);
    if ( t == 0 ) break;
    n = t;
  }

  usize an = normalize(a, n);
  usize bn = normalize(b, n);
  if ( bn == 0 ) {
    if ( an == 0 ) return 0;
    copyi(gp, a, an);
    return an;
  }
  if ( an < bn ) {
    limb_t *const t = a;
    a = b;
    b = t;
    const usize tn = an;
    an = bn;
    bn = tn;
  }
  return gcd_lehmer(gp, a, an, b, bn, work);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scratch

[[nodiscard, gnu::flatten]] inline constexpr usize
gcd_tier_itch_for(gcd_algo k, usize un, usize vn) noexcept
{
  if ( k == gcd_algo::binary ) return gcd_binary_itch(un, vn);
  if ( k == gcd_algo::lehmer ) return gcd_lehmer_itch(un, vn);
  return gcd_dc_itch(un, vn);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
gcd_tier_itch(usize un, usize vn) noexcept
{
  // every tier the LADDER could pick, maxed rather than summed -- it picks one at runtime, and a
  // bounded frame is sized once at instantiation.
  const usize a = gcd_binary_itch(un, vn);
  const usize b = gcd_lehmer_itch(un, vn);
  usize m = a > b ? a : b;

  // the dc tier is charged only where the ladder could actually reach it. BOTH gates are
  // compile-time constants at instantiation -- a --def on the cap or the threshold is visible right
  // here -- and gcd() dispatches on the REDUCED pair, which is never longer than un, so a size that
  // cannot select dc on the way in cannot select it later either. without this, every bounded
  // arbuint would carry hgcd's frame, including the widths that can never reach it.
  if ( static_cast<u8>(gcd_tier_cap) >= static_cast<u8>(gcd_algo::dc) && un >= threshold::gcd_dc ) {
    const usize c = gcd_dc_itch(un, vn);
    if ( c > m ) m = c;
  }
  return m;
}

// the prologue's levelling division, plus whichever tier follows it. the two are SEQUENTIAL and
// share everything past the reduced pair, so this is a max and not a sum.
[[nodiscard, gnu::flatten]] inline constexpr usize
gcd_itch(usize un, usize vn) noexcept
{
  if ( un < vn ) {
    const usize t = un;
    un = vn;
    vn = t;
  }
  if ( vn == 0 ) return 0;
  if ( un == vn ) return gcd_tier_itch(vn, vn);

  const usize pair = vn;      // the remainder the levelling division leaves behind
  const usize head = (un - vn + 1u) + divrem_itch(un, vn);
  const usize tier = gcd_tier_itch(vn, vn);
  return pair + (head > tier ? head : tier);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the friendly entry
//
// gp takes min(un, vn) limbs and the return is the gcd's length, zero only when both operands were
// zero. up and vp survive the call.
//
// the degenerate cases are stripped HERE and never reach a tier, which is what lets every tier
// state "normalized and nonzero" as a precondition instead of re-checking. the answers match
// micron::math::gcd<T> in math/generic.hpp exactly -- gcd(0,0) == 0, gcd(0,x) == x.

[[nodiscard]] inline constexpr usize
gcd(limb_t *gp, const limb_t *up, usize un, const limb_t *vp, usize vn, limb_t *scratch) noexcept
{
  un = normalize(up, un);
  vn = normalize(vp, vn);

  if ( vn == 0 ) {
    copyi(gp, up, un);
    return un;
  }
  if ( un == 0 ) {
    copyi(gp, vp, vn);
    return vn;
  }
  if ( cmp_var(up, un, vp, vn) < 0 ) {
    const limb_t *tp = up;
    up = vp;
    vp = tp;
    const usize tn = un;
    un = vn;
    vn = tn;
  }

  const limb_t *ap = up;
  usize an = un;
  const limb_t *bp = vp;
  usize bn = vn;
  limb_t *work = scratch;

  if ( un > vn ) {
    // one division levels the sizes, so the tier below sees a balanced pair. after it the operands
    // are (v, u mod v), both at most vn limbs, which is what gcd_itch charges the tier for.
    limb_t *const b = scratch;                       // vn
    limb_t *const q = b + vn;                        // un - vn + 1
    limb_t *const dwork = q + (un - vn + 1u);
    divrem(q, b, up, un, vp, vn, dwork);
    bn = normalize(b, vn);
    if ( bn == 0 ) {
      copyi(gp, vp, vn);
      return vn;
    }
    ap = vp;
    an = vn;
    bp = b;
    work = scratch + vn;
  }

  if ( an < bn ) {
    const limb_t *tp = ap;
    ap = bp;
    bp = tp;
    const usize tn = an;
    an = bn;
    bn = tn;
  }

  switch ( clamp_to(pick_gcd(an, bn), gcd_tier_cap) ) {
  case gcd_algo::binary:
    return gcd_binary(gp, ap, an, bp, bn, work);
  case gcd_algo::dc:
    return gcd_dc(gp, ap, an, bp, bn, work);
  default:
    return gcd_lehmer(gp, ap, an, bp, bn, work);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pinned tiers

// the pinned entry may name a tier the ladder would never select, so this CANNOT defer to
// gcd_itch's cap-and-threshold gate -- it charges the named tier outright. deferring is how a
// gcd_with<dc> in the differential or the bench gets a frame sized for Lehmer and overruns it
// without failing a single correctness check.
template<gcd_algo K>
[[nodiscard, gnu::flatten]] inline constexpr usize
gcd_itch_with(usize un, usize vn) noexcept
{
  if ( un < vn ) {
    const usize t = un;
    un = vn;
    vn = t;
  }
  if ( vn == 0 ) return 0;
  if ( un == vn ) return gcd_tier_itch_for(K, vn, vn);

  const usize pair = vn;
  const usize head = (un - vn + 1u) + divrem_itch(un, vn);
  const usize tier = gcd_tier_itch_for(K, vn, vn);
  return pair + (head > tier ? head : tier);
}

// the prologue is shared with gcd() -- the degenerate cases and the levelling division are not the
// tier's business either way -- and only the final dispatch is pinned.
template<gcd_algo K>
[[nodiscard]] inline constexpr usize
gcd_with(limb_t *gp, const limb_t *up, usize un, const limb_t *vp, usize vn, limb_t *scratch) noexcept
{
  static_assert(static_cast<u8>(K) <= static_cast<u8>(gcd_tiers_built),
                "arbint: this gcd tier is not implemented yet -- see mpn::gcd_tiers_built");

  un = normalize(up, un);
  vn = normalize(vp, vn);
  if ( vn == 0 ) {
    copyi(gp, up, un);
    return un;
  }
  if ( un == 0 ) {
    copyi(gp, vp, vn);
    return vn;
  }
  if ( cmp_var(up, un, vp, vn) < 0 ) {
    const limb_t *tp = up;
    up = vp;
    vp = tp;
    const usize tn = un;
    un = vn;
    vn = tn;
  }

  const limb_t *ap = up;
  usize an = un;
  const limb_t *bp = vp;
  usize bn = vn;
  limb_t *work = scratch;

  if ( un > vn ) {
    limb_t *const b = scratch;
    limb_t *const q = b + vn;
    limb_t *const dwork = q + (un - vn + 1u);
    divrem(q, b, up, un, vp, vn, dwork);
    bn = normalize(b, vn);
    if ( bn == 0 ) {
      copyi(gp, vp, vn);
      return vn;
    }
    ap = vp;
    an = vn;
    bp = b;
    work = scratch + vn;
  }

  if ( an < bn ) {
    const limb_t *tp = ap;
    ap = bp;
    bp = tp;
    const usize tn = an;
    an = bn;
    bn = tn;
  }

  if constexpr ( K == gcd_algo::binary )
    return gcd_binary(gp, ap, an, bp, bn, work);
  else if constexpr ( K == gcd_algo::dc )
    return gcd_dc(gp, ap, an, bp, bn, work);
  else
    return gcd_lehmer(gp, ap, an, bp, bn, work);
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
