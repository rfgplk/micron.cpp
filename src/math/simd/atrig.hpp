//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../bits/__arch.hpp"
#include "../../types.hpp"
#include "../__asm/hw.hpp"
#include "../bits/coeff/atan_f64.hpp"
#include "../bits/manip.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "_dispatch.hpp"

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)
#include "../../simd/aliases/avx.hpp"
#include "../../simd/aliases/fma.hpp"
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
// TODO: add NEON packed forms
#include "../../simd/aliases/neon.hpp"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Inverse trigonometric fns
//
// branchless scalar + packed simd forms
//
//
// WARNING: these are not bit identical to mkbits::trig_ns::atan_f64

// NOTE: we __must__ disable fast-math and associated opts since reordering/collapsing fp operations will yield *wrong* results
__micron_push_options
__micron_optimize_no_fast_math
namespace micron
{
namespace math
{
namespace mk
{

namespace __atrig
{

inline constexpr f64 pio2 = 0x1.921fb54442d18p+0;

// region boundaries, and the numerator/denominator rows they select
inline constexpr f64 bnd0 = 0.4375;
inline constexpr f64 bnd1 = 0.6875;
inline constexpr f64 bnd2 = 1.1875;
inline constexpr f64 bnd3 = 2.4375;

inline constexpr f64 c_na[5] = { 1.0, 2.0, 1.0, 1.0, 0.0 };
inline constexpr f64 c_nb[5] = { 0.0, -1.0, -1.0, -1.5, -1.0 };
inline constexpr f64 c_da[5] = { 0.0, 1.0, 1.0, 1.5, 1.0 };
inline constexpr f64 c_db[5] = { 1.0, 2.0, 1.0, 1.0, 0.0 };

// atan(ax) for ax finite in [2^-29, 2^61)
[[nodiscard, gnu::always_inline]] inline constexpr f64
atan_core(f64 ax) noexcept
{
  using namespace mkbits::coeff::atan_f64_data;
  const u32 idx = static_cast<u32>(ax >= bnd0) + static_cast<u32>(ax >= bnd1) + static_cast<u32>(ax >= bnd2) + static_cast<u32>(ax >= bnd3);

  const f64 t = hw::fmadd_sd(c_na[idx], ax, c_nb[idx]) / hw::fmadd_sd(c_da[idx], ax, c_db[idx]);
  const f64 z = t * t;
  const f64 w = z * z;

  f64 s1 = hw::fmadd_sd(aT[10], w, aT[8]);
  s1 = hw::fmadd_sd(s1, w, aT[6]);
  s1 = hw::fmadd_sd(s1, w, aT[4]);
  s1 = hw::fmadd_sd(s1, w, aT[2]);
  s1 = hw::fmadd_sd(s1, w, aT[0]);

  f64 s2 = hw::fmadd_sd(aT[9], w, aT[7]);
  s2 = hw::fmadd_sd(s2, w, aT[5]);
  s2 = hw::fmadd_sd(s2, w, aT[3]);
  s2 = hw::fmadd_sd(s2, w, aT[1]);

  const f64 P = hw::fmadd_sd(z, s2, s1);
  const f64 tz = t * z;
  const f64 at = hw::fmadd_sd(-tz, P, t);      // t - t*z*P
  // atan_lo[0] is +0.0 and `at` is strictly positive here, so the idx == 0 case needs no branch
  return f64(atan_lo[idx] + at);
}

};      // namespace __atrig

[[nodiscard, gnu::flatten]] inline constexpr f64
atan_bl(f64 x) noexcept
{
  if ( ieee::is_nan(x) ) [[unlikely]]
    return x;
  const f64 ax = mkbits::manip::fabs(x);
  if ( ax >= 0x1.0p+61 ) [[unlikely]]
    return mkbits::manip::copysign<f64>(__atrig::pio2, x);
  if ( ax < 0x1.0p-29 ) [[unlikely]]
    return x;
  return mkbits::manip::copysign<f64>(__atrig::atan_core(ax), x);
}

[[nodiscard, gnu::flatten]] inline constexpr f64
atan2_pos(f64 y, f64 x) noexcept
{
  if ( y == 0.0 ) [[unlikely]]
    return 0.0;
  if ( x == 0.0 ) [[unlikely]]
    return __atrig::pio2;
  const f64 q = y / x;
  if ( q >= 0x1.0p+61 ) [[unlikely]]
    return __atrig::pio2;
  if ( q < 0x1.0p-29 ) [[unlikely]]
    return q;
  return __atrig::atan_core(q);
}

// full-quadrant atan2
[[nodiscard, gnu::flatten]] inline constexpr f64
atan2_bl(f64 y, f64 x) noexcept
{
  if ( ieee::is_nan(x) || ieee::is_nan(y) ) [[unlikely]]
    return ieee::qnan_v<f64>();

  constexpr f64 pi = 0x1.921fb54442d18p+1;
  constexpr f64 pi_quarter = 0x1.921fb54442d18p-1;
  constexpr f64 three_pi_q = 0x1.2d97c7f3321d2p+1;

  const bool y_neg = mkbits::manip::signbit(y);
  const bool x_neg = mkbits::manip::signbit(x);

  if ( y == 0 ) [[unlikely]] {
    if ( !x_neg ) return mkbits::manip::copysign<f64>(0.0, y);
    return y_neg ? -pi : pi;
  }
  if ( x == 0 ) [[unlikely]]
    return y_neg ? -__atrig::pio2 : __atrig::pio2;
  if ( ieee::is_inf(x) ) [[unlikely]] {
    if ( ieee::is_inf(y) ) {
      if ( !x_neg ) return y_neg ? -pi_quarter : pi_quarter;
      return y_neg ? -three_pi_q : three_pi_q;
    }
    if ( !x_neg ) return mkbits::manip::copysign<f64>(0.0, y);
    return y_neg ? -pi : pi;
  }
  if ( ieee::is_inf(y) ) [[unlikely]]
    return y_neg ? -__atrig::pio2 : __atrig::pio2;

  const f64 z = atan_bl(y / x);
  const f64 adj = x_neg ? mkbits::manip::copysign<f64>(pi, y) : f64(0.0);
  return x_neg ? f64(z + adj) : z;
}

// %%%%%%%%%%%%%%%%%%%%%%%%
// packed

[[gnu::always_inline]] inline constexpr void
__atan2_bl_x4_scalar(const f64 *y, const f64 *x, f64 *r) noexcept
{
  r[0] = atan2_bl(y[0], x[0]);
  r[1] = atan2_bl(y[1], x[1]);
  r[2] = atan2_bl(y[2], x[2]);
  r[3] = atan2_bl(y[3], x[3]);
}

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

inline constexpr bool have_atan2_packed = true;

namespace __atrig
{

inline constexpr f64 sign_mask_f64 = -0.0;      // 0x8000000000000000 as a double

[[gnu::always_inline, gnu::target("avx2,fma")]] inline simd::d256
atan_core_p(simd::d256 ax) noexcept
{
  namespace avx = micron::simd::avx;
  namespace vfma = micron::simd::fma;
  using namespace mkbits::coeff::atan_f64_data;

  const simd::d256 m0 = avx::cmp_f64<_CMP_GE_OQ>(ax, avx::splat_f64(bnd0));
  const simd::d256 m1 = avx::cmp_f64<_CMP_GE_OQ>(ax, avx::splat_f64(bnd1));
  const simd::d256 m2 = avx::cmp_f64<_CMP_GE_OQ>(ax, avx::splat_f64(bnd2));
  const simd::d256 m3 = avx::cmp_f64<_CMP_GE_OQ>(ax, avx::splat_f64(bnd3));

  simd::d256 na = avx::splat_f64(c_na[0]);
  na = avx::blendv_f64(na, avx::splat_f64(c_na[1]), m0);
  na = avx::blendv_f64(na, avx::splat_f64(c_na[2]), m1);
  na = avx::blendv_f64(na, avx::splat_f64(c_na[3]), m2);
  na = avx::blendv_f64(na, avx::splat_f64(c_na[4]), m3);

  simd::d256 nb = avx::splat_f64(c_nb[0]);
  nb = avx::blendv_f64(nb, avx::splat_f64(c_nb[1]), m0);
  nb = avx::blendv_f64(nb, avx::splat_f64(c_nb[2]), m1);
  nb = avx::blendv_f64(nb, avx::splat_f64(c_nb[3]), m2);
  nb = avx::blendv_f64(nb, avx::splat_f64(c_nb[4]), m3);

  simd::d256 da = avx::splat_f64(c_da[0]);
  da = avx::blendv_f64(da, avx::splat_f64(c_da[1]), m0);
  da = avx::blendv_f64(da, avx::splat_f64(c_da[2]), m1);
  da = avx::blendv_f64(da, avx::splat_f64(c_da[3]), m2);
  da = avx::blendv_f64(da, avx::splat_f64(c_da[4]), m3);

  simd::d256 db = avx::splat_f64(c_db[0]);
  db = avx::blendv_f64(db, avx::splat_f64(c_db[1]), m0);
  db = avx::blendv_f64(db, avx::splat_f64(c_db[2]), m1);
  db = avx::blendv_f64(db, avx::splat_f64(c_db[3]), m2);
  db = avx::blendv_f64(db, avx::splat_f64(c_db[4]), m3);

  const simd::d256 t = avx::div_f64(vfma::fma_f64(na, ax, nb), vfma::fma_f64(da, ax, db));
  const simd::d256 z = avx::mul_f64(t, t);
  const simd::d256 w = avx::mul_f64(z, z);

  simd::d256 s1 = vfma::fma_f64(avx::splat_f64(aT[10]), w, avx::splat_f64(aT[8]));
  s1 = vfma::fma_f64(s1, w, avx::splat_f64(aT[6]));
  s1 = vfma::fma_f64(s1, w, avx::splat_f64(aT[4]));
  s1 = vfma::fma_f64(s1, w, avx::splat_f64(aT[2]));
  s1 = vfma::fma_f64(s1, w, avx::splat_f64(aT[0]));

  simd::d256 s2 = vfma::fma_f64(avx::splat_f64(aT[9]), w, avx::splat_f64(aT[7]));
  s2 = vfma::fma_f64(s2, w, avx::splat_f64(aT[5]));
  s2 = vfma::fma_f64(s2, w, avx::splat_f64(aT[3]));
  s2 = vfma::fma_f64(s2, w, avx::splat_f64(aT[1]));

  const simd::d256 P = vfma::fma_f64(z, s2, s1);
  const simd::d256 tz = avx::mul_f64(t, z);
  // t - t*z*P, as fma(-(t*z), P, t) so it matches the scalar core's fused form exactly
  const simd::d256 at = vfma::fma_f64(avx::xor_f64(tz, avx::splat_f64(sign_mask_f64)), P, t);

  simd::d256 lo = avx::splat_f64(atan_lo[0]);
  lo = avx::blendv_f64(lo, avx::splat_f64(atan_lo[1]), m0);
  lo = avx::blendv_f64(lo, avx::splat_f64(atan_lo[2]), m1);
  lo = avx::blendv_f64(lo, avx::splat_f64(atan_lo[3]), m2);
  lo = avx::blendv_f64(lo, avx::splat_f64(atan_lo[4]), m3);
  return avx::add_f64(lo, at);
}

};      // namespace __atrig

[[gnu::flatten]] inline void
atan2_bl_x4(const f64 *y, const f64 *x, f64 *r) noexcept
{
  namespace avx = micron::simd::avx;
  // f64 is _Float64 on amd64 gcc
  const simd::d256 vy = avx::loadu_f64(reinterpret_cast<const double *>(y));
  const simd::d256 vx = avx::loadu_f64(reinterpret_cast<const double *>(x));
  const simd::d256 sgn = avx::splat_f64(__atrig::sign_mask_f64);

  const simd::d256 q = avx::div_f64(vy, vx);
  const simd::d256 aq = avx::andnot_f64(sgn, q);      // |q|

  const simd::d256 ok
      = avx::and_f64(avx::cmp_f64<_CMP_GE_OQ>(aq, avx::splat_f64(0x1.0p-29)), avx::cmp_f64<_CMP_LT_OQ>(aq, avx::splat_f64(0x1.0p+61)));
  if ( avx::movemask_f64(ok) != 0xF ) [[unlikely]] {
    __atan2_bl_x4_scalar(y, x, r);
    return;
  }

  const simd::d256 z = avx::or_f64(__atrig::atan_core_p(aq), avx::and_f64(q, sgn));      // copysign(core, q)
  const simd::d256 xneg = avx::cmp_f64<_CMP_LT_OQ>(vx, avx::splat_f64(0.0));
  const simd::d256 adj = avx::and_f64(avx::or_f64(avx::splat_f64(0x1.921fb54442d18p+1), avx::and_f64(vy, sgn)), xneg);
  avx::storeu_f64(reinterpret_cast<double *>(r), avx::add_f64(z, adj));
}

#else

inline constexpr bool have_atan2_packed = false;

[[gnu::flatten]] inline void
atan2_bl_x4(const f64 *y, const f64 *x, f64 *r) noexcept
{
  __atan2_bl_x4_scalar(y, x, r);
}

#endif

};      // namespace mk
};      // namespace math
};      // namespace micron

__micron_pop_options