//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "limb.hpp"
#include "tags.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// tier crossovers
// (in limbs)
//
// measured on amd64 (zen+, kernel 7.1.5, -O3 -march=native -funroll-loops, taskset-pinned, median of 5)
//   MUL_KARATSUBA  20   karatsuba loses at 8..16 and wins from 20 on: 252 vs 309 cyc/n^2 at 20 limbs,
//                       257 vs 325 at 24, 177 vs 283 at 64, 99 vs 286 at 1024 and still falling.
//   SQR_KARATSUBA  32   the sqr crossover sits higher, because sqr_basecase already halves the
//                       multiply count via the a_i*a_j symmetry and karatsuba has more to beat:
//                       156 vs 224 cyc/n^2 at 32 limbs, 143 vs 197 at 64, 119 vs 141 at 1024.
//   DIV_DC         96   the recursive divider loses below it and wins above: 281 vs 340 cyc/n^2 at
//                       96 limbs, 231 vs 308 at 160, 156 vs 299 at 512
//   GET_STR_DC    128   the divide-and-conquer formatter loses below it and wins above: 178k vs 196k
//                       cycles at 128 limbs, 411k vs 639k at 256
//   SET_STR_DC  10000  in digits. the divide-and-conquer parser loses badly below it, 2.2x at 4096
//                       bits, 1.7x at 8192, breaks even around 32768, and wins 2.4x at 65536 bits
//                       (534k vs 1263k cycles)
//   *_COMBA             UNSELECTED
//
// TODO: we eventually want to dispatch tiers according to arch, rough references below
// relative refs per arch
//  karatsuba    12 .. 28      (haswell 20, skylake 26, zen 16, zen3 20)
//  toom-3       53 .. 125     (skylake 73, zen 107, zen3 89)
//  toom-4       93 .. 324     (skylake 208, zen3 130)
//  nussbaumer  180000          (Haswell, AVX2 SoA transform; final 2^20 seam wins by 21.2% mul / 23.4% sqr)
//  DC_DIV       19 .. 76      (skylake 55)
//  MU_DIV      855 .. 1899    (skylake 1528)

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mul
#if defined(__micron_arch_width_64)

#ifndef MICRON_ARBINT_MUL_COMBA_THRESHOLD
#define MICRON_ARBINT_MUL_COMBA_THRESHOLD MICRON_ARBINT_MUL_KARATSUBA_THRESHOLD
#endif
#ifndef MICRON_ARBINT_MUL_KARATSUBA_THRESHOLD
#define MICRON_ARBINT_MUL_KARATSUBA_THRESHOLD 20
#endif
#ifndef MICRON_ARBINT_MUL_TOOM3_THRESHOLD
#define MICRON_ARBINT_MUL_TOOM3_THRESHOLD MICRON_ARBINT_MUL_TOOM4_THRESHOLD
#endif
#ifndef MICRON_ARBINT_MUL_TOOM4_THRESHOLD
#define MICRON_ARBINT_MUL_TOOM4_THRESHOLD 320
#endif
#ifndef MICRON_ARBINT_MUL_NUSSBAUMER_THRESHOLD
#define MICRON_ARBINT_MUL_NUSSBAUMER_THRESHOLD 180000
#endif

#ifndef MICRON_ARBINT_SQR_COMBA_THRESHOLD
#define MICRON_ARBINT_SQR_COMBA_THRESHOLD MICRON_ARBINT_SQR_KARATSUBA_THRESHOLD
#endif
#ifndef MICRON_ARBINT_SQR_KARATSUBA_THRESHOLD
#define MICRON_ARBINT_SQR_KARATSUBA_THRESHOLD 32
#endif
#ifndef MICRON_ARBINT_SQR_TOOM3_THRESHOLD
#define MICRON_ARBINT_SQR_TOOM3_THRESHOLD 512
#endif
#ifndef MICRON_ARBINT_SQR_TOOM4_THRESHOLD
#define MICRON_ARBINT_SQR_TOOM4_THRESHOLD 2048
#endif
#ifndef MICRON_ARBINT_SQR_NUSSBAUMER_THRESHOLD
#define MICRON_ARBINT_SQR_NUSSBAUMER_THRESHOLD 180000
#endif

#ifndef MICRON_ARBINT_DIV_DC_THRESHOLD
#define MICRON_ARBINT_DIV_DC_THRESHOLD 320
#endif
#ifndef MICRON_ARBINT_DIV_MU_THRESHOLD
#define MICRON_ARBINT_DIV_MU_THRESHOLD 1536
#endif

#ifndef MICRON_ARBINT_INV_NEWTON_THRESHOLD
#define MICRON_ARBINT_INV_NEWTON_THRESHOLD 48
#endif
#ifndef MICRON_ARBINT_POWM_TABLE_LIMBS
#define MICRON_ARBINT_POWM_TABLE_LIMBS 1024
#endif

#ifndef MICRON_ARBINT_GCD_LEHMER_THRESHOLD
#define MICRON_ARBINT_GCD_LEHMER_THRESHOLD 3
#endif
#ifndef MICRON_ARBINT_GCD_HGCD_THRESHOLD
#define MICRON_ARBINT_GCD_HGCD_THRESHOLD 96
#endif
#ifndef MICRON_ARBINT_GCD_DC_THRESHOLD
#define MICRON_ARBINT_GCD_DC_THRESHOLD 512
#endif

#ifndef MICRON_ARBINT_GET_STR_DC_THRESHOLD
#define MICRON_ARBINT_GET_STR_DC_THRESHOLD 128
#endif
#ifndef MICRON_ARBINT_SET_STR_DC_THRESHOLD
#define MICRON_ARBINT_SET_STR_DC_THRESHOLD 10000
#endif

#else      // 32 bits

#ifndef MICRON_ARBINT_MUL_COMBA_THRESHOLD
#define MICRON_ARBINT_MUL_COMBA_THRESHOLD MICRON_ARBINT_MUL_KARATSUBA_THRESHOLD
#endif
#ifndef MICRON_ARBINT_MUL_KARATSUBA_THRESHOLD
#define MICRON_ARBINT_MUL_KARATSUBA_THRESHOLD 40
#endif
#ifndef MICRON_ARBINT_MUL_TOOM3_THRESHOLD
#define MICRON_ARBINT_MUL_TOOM3_THRESHOLD MICRON_ARBINT_MUL_TOOM4_THRESHOLD
#endif
#ifndef MICRON_ARBINT_MUL_TOOM4_THRESHOLD
#define MICRON_ARBINT_MUL_TOOM4_THRESHOLD 640
#endif
// The direct window has local wins, but its 100k transform jump and capped overlap-add prevent a
// monotonic crossover.  Keep the pinned tier, but disable automatic selection for feasible shapes.
#ifndef MICRON_ARBINT_MUL_NUSSBAUMER_THRESHOLD
#define MICRON_ARBINT_MUL_NUSSBAUMER_THRESHOLD 200000000u
#endif

#ifndef MICRON_ARBINT_SQR_COMBA_THRESHOLD
#define MICRON_ARBINT_SQR_COMBA_THRESHOLD MICRON_ARBINT_SQR_KARATSUBA_THRESHOLD
#endif
#ifndef MICRON_ARBINT_SQR_KARATSUBA_THRESHOLD
#define MICRON_ARBINT_SQR_KARATSUBA_THRESHOLD 64
#endif
#ifndef MICRON_ARBINT_SQR_TOOM3_THRESHOLD
#define MICRON_ARBINT_SQR_TOOM3_THRESHOLD 1024
#endif
#ifndef MICRON_ARBINT_SQR_TOOM4_THRESHOLD
#define MICRON_ARBINT_SQR_TOOM4_THRESHOLD 4096
#endif
#ifndef MICRON_ARBINT_SQR_NUSSBAUMER_THRESHOLD
#define MICRON_ARBINT_SQR_NUSSBAUMER_THRESHOLD 200000000u
#endif

#ifndef MICRON_ARBINT_DIV_DC_THRESHOLD
#define MICRON_ARBINT_DIV_DC_THRESHOLD 640
#endif
#ifndef MICRON_ARBINT_DIV_MU_THRESHOLD
#define MICRON_ARBINT_DIV_MU_THRESHOLD 3072
#endif

#ifndef MICRON_ARBINT_INV_NEWTON_THRESHOLD
#define MICRON_ARBINT_INV_NEWTON_THRESHOLD 96
#endif
#ifndef MICRON_ARBINT_POWM_TABLE_LIMBS
#define MICRON_ARBINT_POWM_TABLE_LIMBS 2048
#endif

#ifndef MICRON_ARBINT_GCD_LEHMER_THRESHOLD
#define MICRON_ARBINT_GCD_LEHMER_THRESHOLD 6
#endif
#ifndef MICRON_ARBINT_GCD_HGCD_THRESHOLD
#define MICRON_ARBINT_GCD_HGCD_THRESHOLD 192
#endif
#ifndef MICRON_ARBINT_GCD_DC_THRESHOLD
#define MICRON_ARBINT_GCD_DC_THRESHOLD 1024
#endif

#ifndef MICRON_ARBINT_GET_STR_DC_THRESHOLD
#define MICRON_ARBINT_GET_STR_DC_THRESHOLD 256
#endif
#ifndef MICRON_ARBINT_SET_STR_DC_THRESHOLD
#define MICRON_ARBINT_SET_STR_DC_THRESHOLD 10000
#endif

#endif

#ifndef MICRON_ARBINT_MUL_TIER_CAP
#define MICRON_ARBINT_MUL_TIER_CAP 5
#endif
#ifndef MICRON_ARBINT_SQR_TIER_CAP
#define MICRON_ARBINT_SQR_TIER_CAP 5
#endif

#ifndef MICRON_ARBINT_NUSSBAUMER_LEAF_LOG
#define MICRON_ARBINT_NUSSBAUMER_LEAF_LOG 4
#endif
#ifndef MICRON_ARBINT_NUSSBAUMER_MAX_LOG
#define MICRON_ARBINT_NUSSBAUMER_MAX_LOG 20
#endif
#ifndef MICRON_ARBINT_NUSSBAUMER_CACHE_BLOCK_COEFFS
#define MICRON_ARBINT_NUSSBAUMER_CACHE_BLOCK_COEFFS 8192
#endif

#ifndef MICRON_ARBINT_DIV_TIER_CAP
#define MICRON_ARBINT_DIV_TIER_CAP 1
#endif
#ifndef MICRON_ARBINT_GCD_TIER_CAP
#define MICRON_ARBINT_GCD_TIER_CAP 1
#endif
#ifndef MICRON_ARBINT_HGCD2_WINDOW
#define MICRON_ARBINT_HGCD2_WINDOW 1
#endif

// exponent bit counts
#ifndef MICRON_ARBINT_POWM_WIN3_THRESHOLD
#define MICRON_ARBINT_POWM_WIN3_THRESHOLD 32
#endif
#ifndef MICRON_ARBINT_POWM_WIN4_THRESHOLD
#define MICRON_ARBINT_POWM_WIN4_THRESHOLD 96
#endif
#ifndef MICRON_ARBINT_POWM_WIN5_THRESHOLD
#define MICRON_ARBINT_POWM_WIN5_THRESHOLD 400
#endif
#ifndef MICRON_ARBINT_POWM_WIN6_THRESHOLD
#define MICRON_ARBINT_POWM_WIN6_THRESHOLD 1600
#endif
#ifndef MICRON_ARBINT_POWM_WINDOW_CAP
#define MICRON_ARBINT_POWM_WINDOW_CAP 6
#endif
#ifndef MICRON_ARBINT_POWM_ENGINE
#define MICRON_ARBINT_POWM_ENGINE 0
#endif

namespace micron
{
namespace math
{
namespace mpn
{
namespace threshold
{

inline constexpr usize mul_comba = MICRON_ARBINT_MUL_COMBA_THRESHOLD;
inline constexpr usize mul_karatsuba = MICRON_ARBINT_MUL_KARATSUBA_THRESHOLD;
inline constexpr usize mul_toom3 = MICRON_ARBINT_MUL_TOOM3_THRESHOLD;
inline constexpr usize mul_toom4 = MICRON_ARBINT_MUL_TOOM4_THRESHOLD;
inline constexpr usize mul_nussbaumer = MICRON_ARBINT_MUL_NUSSBAUMER_THRESHOLD;

inline constexpr usize sqr_comba = MICRON_ARBINT_SQR_COMBA_THRESHOLD;
inline constexpr usize sqr_karatsuba = MICRON_ARBINT_SQR_KARATSUBA_THRESHOLD;
inline constexpr usize sqr_toom3 = MICRON_ARBINT_SQR_TOOM3_THRESHOLD;
inline constexpr usize sqr_toom4 = MICRON_ARBINT_SQR_TOOM4_THRESHOLD;
inline constexpr usize sqr_nussbaumer = MICRON_ARBINT_SQR_NUSSBAUMER_THRESHOLD;

inline constexpr usize nussbaumer_leaf_log = MICRON_ARBINT_NUSSBAUMER_LEAF_LOG;
inline constexpr usize nussbaumer_max_log = MICRON_ARBINT_NUSSBAUMER_MAX_LOG;
inline constexpr usize nussbaumer_cache_block_coeffs = MICRON_ARBINT_NUSSBAUMER_CACHE_BLOCK_COEFFS;

inline constexpr usize div_dc = MICRON_ARBINT_DIV_DC_THRESHOLD;
inline constexpr usize div_mu = MICRON_ARBINT_DIV_MU_THRESHOLD;
inline constexpr usize inv_newton = MICRON_ARBINT_INV_NEWTON_THRESHOLD;

inline constexpr usize gcd_lehmer = MICRON_ARBINT_GCD_LEHMER_THRESHOLD;
inline constexpr usize gcd_hgcd = MICRON_ARBINT_GCD_HGCD_THRESHOLD;
inline constexpr usize gcd_dc = MICRON_ARBINT_GCD_DC_THRESHOLD;

inline constexpr usize powm_win3 = MICRON_ARBINT_POWM_WIN3_THRESHOLD;
inline constexpr usize powm_win4 = MICRON_ARBINT_POWM_WIN4_THRESHOLD;
inline constexpr usize powm_win5 = MICRON_ARBINT_POWM_WIN5_THRESHOLD;
inline constexpr usize powm_win6 = MICRON_ARBINT_POWM_WIN6_THRESHOLD;
inline constexpr usize powm_window_cap = MICRON_ARBINT_POWM_WINDOW_CAP;
inline constexpr usize powm_table_limbs = MICRON_ARBINT_POWM_TABLE_LIMBS;
inline constexpr u32 powm_engine = MICRON_ARBINT_POWM_ENGINE;

inline constexpr u32 hgcd2_window = MICRON_ARBINT_HGCD2_WINDOW;

inline constexpr usize get_str_dc = MICRON_ARBINT_GET_STR_DC_THRESHOLD;
inline constexpr usize set_str_dc = MICRON_ARBINT_SET_STR_DC_THRESHOLD;

static_assert(mul_comba >= 1 && mul_comba <= mul_karatsuba, "arbint: mul thresholds must ascend");
static_assert(mul_karatsuba <= mul_toom3 && mul_toom3 <= mul_toom4 && mul_toom4 <= mul_nussbaumer, "arbint: mul thresholds must ascend");
static_assert(sqr_comba >= 1 && sqr_comba <= sqr_karatsuba, "arbint: sqr thresholds must ascend");
static_assert(sqr_karatsuba <= sqr_toom3 && sqr_toom3 <= sqr_toom4 && sqr_toom4 <= sqr_nussbaumer, "arbint: sqr thresholds must ascend");
static_assert(nussbaumer_leaf_log >= 1u && nussbaumer_leaf_log <= nussbaumer_max_log,
              "arbint: MICRON_ARBINT_NUSSBAUMER_LEAF_LOG must be in [1, MAX_LOG]");
static_assert(nussbaumer_max_log <= 20u, "arbint: one Nussbaumer transform is limited to 2^20 coefficients");
static_assert(nussbaumer_max_log + 2u < limb_bits,
              "arbint: Nussbaumer needs at least one base digit below the signed-coefficient guard bits");
static_assert(nussbaumer_cache_block_coeffs >= (usize{ 1 } << nussbaumer_leaf_log)
                  && (nussbaumer_cache_block_coeffs & (nussbaumer_cache_block_coeffs - 1u)) == 0u,
              "arbint: Nussbaumer cache block must be a power of two no smaller than the leaf");
static_assert(div_dc <= div_mu, "arbint: div thresholds must ascend");
static_assert(gcd_lehmer <= gcd_dc, "arbint: gcd ladder thresholds must ascend");
static_assert(gcd_hgcd >= 2, "arbint: hgcd's recursion cutoff must leave room for a step");
static_assert(inv_newton >= 2, "arbint: the Newton reciprocal's basecase needs two divisor limbs");
static_assert(inv_newton <= div_mu, "arbint: the reciprocal needs a Newton path at the sizes mu enters at");
static_assert(powm_win3 <= powm_win4 && powm_win4 <= powm_win5 && powm_win5 <= powm_win6, "arbint: powm window bands must ascend");
static_assert(powm_window_cap >= 1 && powm_window_cap <= 8, "arbint: powm window cap out of range");
static_assert(powm_table_limbs >= 1, "arbint: the powm window table needs room for at least one residue");
static_assert(powm_engine <= 2, "arbint: MICRON_ARBINT_POWM_ENGINE is 0 auto, 1 montgomery, 2 barrett");
static_assert(hgcd2_window == 1u || hgcd2_window == 2u,
              "arbint: MICRON_ARBINT_HGCD2_WINDOW is 1 (Knuth, one limb) or 2 (Jebelean, two limbs)");

};      // namespace threshold

// %%%%%%%%%%%%%%%%%%%
// ladders

// mul ladder
[[nodiscard, gnu::always_inline]] inline constexpr algo
pick_mul(usize an, usize bn) noexcept
{
  const usize n = an < bn ? an : bn;
  if ( n < threshold::mul_comba ) return algo::basecase;
  if ( n < threshold::mul_karatsuba ) return algo::comba;
  if ( n < threshold::mul_toom3 ) return algo::karatsuba;
  if ( n < threshold::mul_toom4 ) return algo::toom3;
  if ( n < threshold::mul_nussbaumer ) return algo::toom4;
  return algo::nussbaumer;
}

// sqr ladder
[[nodiscard, gnu::always_inline]] inline constexpr algo
pick_sqr(usize n) noexcept
{
  if ( n < threshold::sqr_comba ) return algo::basecase;
  if ( n < threshold::sqr_karatsuba ) return algo::comba;
  if ( n < threshold::sqr_toom3 ) return algo::karatsuba;
  if ( n < threshold::sqr_toom4 ) return algo::toom3;
  if ( n < threshold::sqr_nussbaumer ) return algo::toom4;
  return algo::nussbaumer;
}

// divison ladder
[[nodiscard, gnu::always_inline]] inline constexpr divalgo
pick_div(usize nn, usize dn) noexcept
{
  if ( dn < threshold::div_dc ) return divalgo::sbpi1;
  if ( dn < threshold::div_mu ) return divalgo::dc;
  return (nn >= dn && (nn - dn) >= dn) ? divalgo::mu : divalgo::dc;
}

inline constexpr divalgo div_tier_cap = static_cast<divalgo>(MICRON_ARBINT_DIV_TIER_CAP);

// gcd ladder
[[nodiscard, gnu::always_inline]] inline constexpr gcd_algo
pick_gcd(usize un, usize vn) noexcept
{
  const usize n = un < vn ? un : vn;
  if ( n < threshold::gcd_lehmer ) return gcd_algo::binary;
  if ( n < threshold::gcd_dc ) return gcd_algo::lehmer;
  return gcd_algo::dc;
}

inline constexpr gcd_algo gcd_tier_cap = static_cast<gcd_algo>(MICRON_ARBINT_GCD_TIER_CAP);

[[nodiscard, gnu::always_inline]] inline constexpr usize
powm_window(usize ebits) noexcept
{
  if ( ebits < threshold::powm_win3 ) return 1u;      // plain square-and-multiply, no table at all
  if ( ebits < threshold::powm_win4 ) return 3u;
  if ( ebits < threshold::powm_win5 ) return 4u;
  if ( ebits < threshold::powm_win6 ) return 5u;
  return 6u;
}

template<arb_solver S>
[[nodiscard, gnu::always_inline]] inline consteval algo
pinned_algo() noexcept
{
  if constexpr ( micron::is_same_v<S, solver::basecase> )
    return algo::basecase;
  else if constexpr ( micron::is_same_v<S, solver::comba> )
    return algo::comba;
  else if constexpr ( micron::is_same_v<S, solver::karatsuba> )
    return algo::karatsuba;
  else if constexpr ( micron::is_same_v<S, solver::toom> )
    return algo::toom3;
  else
    return algo::nussbaumer;
}

};      // namespace mpn
};      // namespace math
};      // namespace micron
