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
#include "mul_nussbaumer.hpp"
#include "mul_toom.hpp"
#include "mul_toom4.hpp"
#include "tags.hpp"
#include "thresholds.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// multiplication ladder
//
namespace micron
{
namespace math
{
namespace mpn
{

inline constexpr algo tiers_built = algo::nussbaumer;

inline constexpr algo mul_tier_cap = static_cast<algo>(MICRON_ARBINT_MUL_TIER_CAP);
inline constexpr algo sqr_tier_cap = static_cast<algo>(MICRON_ARBINT_SQR_TIER_CAP);

static_assert(static_cast<u8>(mul_tier_cap) <= static_cast<u8>(tiers_built),
              "arbint: MICRON_ARBINT_MUL_TIER_CAP names a tier above mpn::tiers_built");
static_assert(static_cast<u8>(sqr_tier_cap) <= static_cast<u8>(tiers_built),
              "arbint: MICRON_ARBINT_SQR_TIER_CAP names a tier above mpn::tiers_built");

[[nodiscard, gnu::always_inline]] inline constexpr algo
clamp_algo(algo k) noexcept
{
  return clamp_to(k, mul_tier_cap);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scratch requirement

[[nodiscard, gnu::flatten]] inline constexpr usize
mul_itch(usize an, usize bn, usize cutoff = threshold::mul_karatsuba) noexcept
{
  const usize n = an < bn ? an : bn;
  if ( n < cutoff ) return 0;
  if ( clamp_algo(pick_mul(an, bn)) == algo::nussbaumer ) return nussbaumer_itch(an, bn);
  usize base = karatsuba_itch(n, cutoff);
  if ( an == bn && n >= threshold::mul_toom3 ) {
    const usize t3 = toom3_itch(n);
    if ( t3 > base ) base = t3;
  }
  if ( an == bn && n >= threshold::mul_toom4 ) {
    const usize t4 = toom4_itch(n);
    if ( t4 > base ) base = t4;
  }
  return (an == bn) ? base : (unbalanced_stage_itch(an, bn, cutoff) + base);
}

[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_itch(usize n, usize cutoff = threshold::sqr_karatsuba) noexcept
{
  if ( clamp_to(pick_sqr(n), sqr_tier_cap) == algo::nussbaumer ) return sqr_nussbaumer_itch(n);
  usize base = sqr_karatsuba_itch(n, cutoff);
  if ( n >= threshold::sqr_toom3 ) {
    const usize t3 = sqr_toom3_itch(n);
    if ( t3 > base ) base = t3;
  }
  if ( n >= threshold::sqr_toom4 ) {
    const usize t4 = sqr_toom4_itch(n);
    if ( t4 > base ) base = t4;
  }
  return base;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
mul_itch_forced(usize an, usize bn) noexcept
{
  usize base = mul_itch(an, bn, karatsuba_force);
  // mul_with<toom3> runs the Toom kernel wherever toom3_applies() allows
  if ( an == bn && an >= 3u ) {
    const usize t3 = toom3_itch(an, true);
    if ( t3 > base ) base = t3;
    const usize t4 = toom4_itch(an, true);
    if ( t4 > base ) base = t4;
  }
  const usize ns = nussbaumer_itch(an, bn);
  if ( ns > base ) base = ns;
  return base;
}

[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_itch_forced(usize n) noexcept
{
  usize base = sqr_itch(n, karatsuba_force);
  if ( sqr_toom3_applies(n) ) {
    const usize t3 = sqr_toom3_itch(n, true);
    if ( t3 > base ) base = t3;
  }
  const usize t4 = sqr_toom4_itch(n, true);
  if ( t4 > base ) base = t4;
  const usize ns = sqr_nussbaumer_itch(n);
  if ( ns > base ) base = ns;
  return base;
}

template<algo K>
[[nodiscard, gnu::flatten]] inline constexpr usize
mul_itch_with(usize an, usize bn) noexcept
{
  if constexpr ( K == algo::basecase || K == algo::comba )
    return 0u;
  else if constexpr ( K == algo::karatsuba ) {
    if ( bn < 2u ) return 0u;
    if ( an != bn ) {
      usize stage = unbalanced_stage_itch(an, bn, threshold::mul_karatsuba);
      if ( stage == 0u ) stage = 2u * bn;
      return stage + karatsuba_itch(bn);
    }
    const usize k = (an + 1u) / 2u;
    return 6u * k + 1u + karatsuba_itch(k);
  } else if constexpr ( K == algo::toom3 ) {
    if ( toom3_applies(an, bn) ) return toom3_itch(an, true);
    return mul_itch_with<algo::karatsuba>(an, bn);
  } else if constexpr ( K == algo::toom4 ) {
    if ( toom4_applies(an, bn) ) return toom4_itch(an, true);
    if ( toom3_applies(an, bn) ) return toom3_itch(an, true);
    return mul_itch_with<algo::karatsuba>(an, bn);
  } else if constexpr ( K == algo::nussbaumer )
    return nussbaumer_itch(an, bn);
}

template<algo K>
[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_itch_with(usize n) noexcept
{
  if constexpr ( K == algo::basecase || K == algo::comba )
    return 0u;
  else if constexpr ( K == algo::karatsuba ) {
    if ( n < 2u ) return 0u;
    const usize k = (n + 1u) / 2u;
    return 5u * k + 1u + sqr_karatsuba_itch(k);
  } else if constexpr ( K == algo::toom3 ) {
    if ( sqr_toom3_applies(n) ) return sqr_toom3_itch(n, true);
    return sqr_itch_with<algo::karatsuba>(n);
  } else if constexpr ( K == algo::toom4 ) {
    if ( sqr_toom4_applies(n) ) return sqr_toom4_itch(n, true);
    if ( sqr_toom3_applies(n) ) return sqr_toom3_itch(n, true);
    return sqr_itch_with<algo::karatsuba>(n);
  } else if constexpr ( K == algo::nussbaumer )
    return sqr_nussbaumer_itch(n);
}

template<arb_solver S>
[[nodiscard, gnu::flatten]] inline constexpr usize
mul_solver_itch(usize an, usize bn) noexcept
{
  if constexpr ( micron::is_same_v<S, solver::automatic> )
    return mul_itch(an, bn);
  else
    return mul_itch_with<pinned_algo<S>()>(an, bn);
}

template<arb_solver S>
[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_solver_itch(usize n) noexcept
{
  if constexpr ( micron::is_same_v<S, solver::automatic> )
    return sqr_itch(n);
  else
    return sqr_itch_with<pinned_algo<S>()>(n);
}

[[nodiscard, gnu::always_inline]] inline constexpr usize
__mul_itch_max(usize a, usize b) noexcept
{
  return a > b ? a : b;
}

template<arb_solver S>
[[nodiscard, gnu::flatten]] inline constexpr usize
mul_solver_cap_itch(usize cap) noexcept
{
  if constexpr ( micron::is_same_v<S, solver::basecase> || micron::is_same_v<S, solver::comba> ) {
    return 0u;
  } else if constexpr ( micron::is_same_v<S, solver::nussbaumer> ) {
    return nussbaumer_itch(cap, cap);
  } else {
    // Every unbalanced Karatsuba stage follows an Euclidean remainder chain.  The sum of that
    // chain is below 3*cap and each stage reserves 2*bn, so this bounds every an,bn <= cap.
    usize out = 0u;
    if constexpr ( !micron::is_same_v<S, solver::automatic> )
      out = __nuss_size_add(__nuss_size_mul(6u, cap), karatsuba_itch(cap));
    else if ( cap >= threshold::mul_karatsuba )
      out = __nuss_size_add(__nuss_size_mul(6u, cap), karatsuba_itch(cap));
    if constexpr ( micron::is_same_v<S, solver::toom> ) {
      out = __mul_itch_max(out, toom3_itch(cap, true));
    } else if constexpr ( micron::is_same_v<S, solver::automatic> ) {
      if constexpr ( static_cast<u8>(mul_tier_cap) >= static_cast<u8>(algo::toom3) )
        if ( cap >= threshold::mul_toom3 ) out = __mul_itch_max(out, toom3_itch(cap));
      if constexpr ( static_cast<u8>(mul_tier_cap) >= static_cast<u8>(algo::toom4) )
        if ( cap >= threshold::mul_toom4 ) out = __mul_itch_max(out, toom4_itch(cap));
      if constexpr ( static_cast<u8>(mul_tier_cap) >= static_cast<u8>(algo::nussbaumer) )
        if ( cap >= threshold::mul_nussbaumer ) out = __mul_itch_max(out, nussbaumer_itch(cap, cap));
    }
    return out;
  }
}

template<arb_solver S>
[[nodiscard, gnu::flatten]] inline constexpr usize
sqr_solver_cap_itch(usize cap) noexcept
{
  if constexpr ( micron::is_same_v<S, solver::automatic> )
    return sqr_itch(cap);
  else
    return sqr_itch_with<pinned_algo<S>()>(cap);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// pinned tiers
template<algo K>
inline constexpr void
mul_with(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn,
         limb_t *scratch) noexcept
{
  static_assert(static_cast<u8>(K) <= static_cast<u8>(tiers_built),
                "arbint: this multiplication tier is not implemented yet -- see mpn::tiers_built");
  if constexpr ( K == algo::basecase )
    mul_basecase(rp, ap, an, bp, bn);
  else if constexpr ( K == algo::comba )
    mul_comba(rp, ap, an, bp, bn);
  else if constexpr ( K == algo::karatsuba )
    mul_karatsuba_top(rp, ap, an, bp, bn, scratch);
  else if constexpr ( K == algo::toom3 ) {
    if ( toom3_applies(an, bn) )
      mul_toom3(rp, ap, bp, an, scratch, threshold::mul_karatsuba);
    else
      mul_karatsuba_top(rp, ap, an, bp, bn, scratch);
  } else if constexpr ( K == algo::toom4 ) {
    if ( toom4_applies(an, bn) )
      mul_toom4(rp, ap, bp, an, scratch, threshold::mul_karatsuba);
    else if ( toom3_applies(an, bn) )
      mul_toom3(rp, ap, bp, an, scratch, threshold::mul_karatsuba);
    else
      mul_karatsuba_top(rp, ap, an, bp, bn, scratch);
  } else {
    mul_nussbaumer(rp, ap, an, bp, bn, scratch);
  }
}

template<algo K>
inline constexpr void
sqr_with(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch) noexcept
{
  static_assert(static_cast<u8>(K) <= static_cast<u8>(tiers_built),
                "arbint: this squaring tier is not implemented yet -- see mpn::tiers_built");
  if constexpr ( K == algo::basecase )
    sqr_basecase(rp, ap, n);
  else if constexpr ( K == algo::comba )
    sqr_comba(rp, ap, n);
  else if constexpr ( K == algo::karatsuba )
    sqr_karatsuba_top(rp, ap, n, scratch);
  else if constexpr ( K == algo::toom3 ) {
    if ( sqr_toom3_applies(n) )
      sqr_toom3(rp, ap, n, scratch, threshold::sqr_karatsuba);
    else
      sqr_karatsuba_top(rp, ap, n, scratch);
  } else if constexpr ( K == algo::toom4 ) {
    if ( sqr_toom4_applies(n) )
      sqr_toom4(rp, ap, n, scratch, threshold::sqr_karatsuba);
    else if ( sqr_toom3_applies(n) )
      sqr_toom3(rp, ap, n, scratch, threshold::sqr_karatsuba);
    else
      sqr_karatsuba_top(rp, ap, n, scratch);
  } else {
    sqr_nussbaumer(rp, ap, n, scratch);
  }
}

// %%%%%%%%%%%%%%%%%%%%%
// automatic

inline constexpr void
mul(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn, limb_t *scratch) noexcept
{
  if ( bn == 1 ) {
    rp[an] = mul_1(rp, ap, an, bp[0]);
    return;
  }
  switch ( clamp_algo(pick_mul(an, bn)) ) {
  case algo::basecase:
    mul_basecase(rp, ap, an, bp, bn);
    return;
  case algo::comba:
    mul_comba(rp, ap, an, bp, bn);
    return;
  case algo::karatsuba:
    mul_karatsuba(rp, ap, an, bp, bn, scratch);
    return;
  case algo::toom3:
    if ( toom3_applies(an, bn) )
      mul_toom3(rp, ap, bp, an, scratch, threshold::mul_karatsuba);
    else
      mul_karatsuba(rp, ap, an, bp, bn, scratch);
    return;
  case algo::toom4:
    if ( toom4_applies(an, bn) )
      mul_toom4(rp, ap, bp, an, scratch, threshold::mul_karatsuba);
    else if ( toom3_applies(an, bn) )
      mul_toom3(rp, ap, bp, an, scratch, threshold::mul_karatsuba);
    else
      mul_karatsuba(rp, ap, an, bp, bn, scratch);
    return;
  case algo::nussbaumer:
    mul_nussbaumer(rp, ap, an, bp, bn, scratch);
    return;
  }
}

inline constexpr void
sqr(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch) noexcept
{
  if ( n == 1 ) {
    mul_wide(ap[0], ap[0], rp[0], rp[1]);
    return;
  }
  switch ( clamp_to(pick_sqr(n), sqr_tier_cap) ) {
  case algo::basecase:
    sqr_basecase(rp, ap, n);
    return;
  case algo::comba:
    sqr_comba(rp, ap, n);
    return;
  case algo::karatsuba:
    sqr_karatsuba(rp, ap, n, scratch);
    return;
  case algo::toom3:
    if ( sqr_toom3_applies(n) )
      sqr_toom3(rp, ap, n, scratch, threshold::sqr_karatsuba);
    else
      sqr_karatsuba(rp, ap, n, scratch);
    return;
  case algo::toom4:
    if ( sqr_toom4_applies(n) )
      sqr_toom4(rp, ap, n, scratch, threshold::sqr_karatsuba);
    else if ( sqr_toom3_applies(n) )
      sqr_toom3(rp, ap, n, scratch, threshold::sqr_karatsuba);
    else
      sqr_karatsuba(rp, ap, n, scratch);
    return;
  case algo::nussbaumer:
    sqr_nussbaumer(rp, ap, n, scratch);
    return;
  }
}

// %%%%%%%%%%%%%%%%%%%%%
// bounded entry

template<usize AN, usize BN, arb_solver S>
[[gnu::always_inline]] inline constexpr void
mul_fixed(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize an, const limb_t *__restrict__ bp, usize bn,
          limb_t *scratch) noexcept
{
  if constexpr ( !micron::is_same_v<S, solver::automatic> ) {
    mul_with<pinned_algo<S>()>(rp, ap, an, bp, bn, scratch);
  } else if constexpr ( AN <= 16 && BN <= 16 && AN + BN <= 24 ) {
    // small enough that the branch to pick a shape costs more than always taking the unrolled one
    if ( an == AN && bn == BN )
      mul_comba_fixed<AN, BN>(rp, ap, bp);
    else
      mul(rp, ap, an, bp, bn, scratch);
  } else {
    mul(rp, ap, an, bp, bn, scratch);
  }
}

template<usize N, arb_solver S>
[[gnu::always_inline]] inline constexpr void
sqr_fixed(limb_t *__restrict__ rp, const limb_t *__restrict__ ap, usize n, limb_t *scratch) noexcept
{
  if constexpr ( !micron::is_same_v<S, solver::automatic> ) {
    sqr_with<pinned_algo<S>()>(rp, ap, n, scratch);
  } else if constexpr ( N <= 16 ) {
    if ( n == N )
      sqr_comba_fixed<N>(rp, ap);
    else
      sqr(rp, ap, n, scratch);
  } else {
    sqr(rp, ap, n, scratch);
  }
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
