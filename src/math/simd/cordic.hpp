//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// per iteration:
//   ys = y * 2^-i          (FP multiply by 2^-i = exponent decrement = shift)
//   xs = x * 2^-i
//   sb = sign_mask(z) & -0.0
//   x -= ys ^ sb           (subtract if z>=0, add if z<0)
//   y += xs ^ sb           (add if z>=0, subtract if z<0)
//   z -= atan_i ^ sb       (subtract if z>=0, add if z<0)

#include "../../simd/aliases.hpp"
#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/cordic.hpp"
#include "_dispatch.hpp"
#include "manip.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wignored-attributes"

namespace micron
{
namespace math
{
namespace mk
{

#if defined(__micron_x86_avx2) && defined(__micron_x86_fma)

namespace __cordic_simd
{
inline constexpr f64 pio2_hi = 0x1.921fb54400000p+0;
inline constexpr f64 pio2_mid = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_lo = 0x1.9880000000000p-77;
inline constexpr f64 inv_pio2 = 0x1.45f306dc9c883p-1;

using mkbits::cordic_ns::ATAN_F32;
using mkbits::cordic_ns::ATAN_F64;
using mkbits::cordic_ns::K_F32;
using mkbits::cordic_ns::K_F64;
using mkbits::cordic_ns::N_FP_F32;
using mkbits::cordic_ns::N_FP_F64;
using mkbits::cordic_ns::SHIFT_POW_F32;
using mkbits::cordic_ns::SHIFT_POW_F64;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// d256 path (4 lanes f64)

[[gnu::always_inline]] inline simd::d256
reduce_pio2_d256(simd::d256 x, simd::i256 *q_out) noexcept
{
  const simd::d256 fN
      = simd::avx::round_f64<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(simd::avx::mul_f64(x, simd::avx::splat_f64(inv_pio2)));
  simd::d256 t = simd::fma::fnma_f64(fN, simd::avx::splat_f64(pio2_hi), x);
  t = simd::fma::fnma_f64(fN, simd::avx::splat_f64(pio2_mid), t);
  t = simd::fma::fnma_f64(fN, simd::avx::splat_f64(pio2_lo), t);
  const simd::i256 N32 = simd::avx::cast_lo128_to_i256(simd::avx::convert_f64_to_i32(fN));
  *q_out = simd::avx2::widen_i32_to_i64(simd::avx::cast_i256_to_lo128(N32));
  return t;
}

[[gnu::flatten]] inline void
sincos_kernel_d256(simd::d256 z, simd::d256 *c_out, simd::d256 *s_out) noexcept
{
  simd::d256 x = simd::avx::splat_f64(K_F64);
  simd::d256 y = simd::avx::zero_f64();
  const simd::d256 neg_zero = simd::avx::splat_f64(-0.0);
  for ( int i = 0; i < N_FP_F64; ++i ) {
    const simd::d256 sm = simd::avx::cmp_f64<_CMP_LT_OQ>(z, simd::avx::zero_f64());
    const simd::d256 sb = simd::avx::and_f64(sm, neg_zero);
    const simd::d256 vshift = simd::avx::splat_f64(SHIFT_POW_F64.v[i]);
    const simd::d256 ys = simd::avx::mul_f64(y, vshift);
    const simd::d256 xs = simd::avx::mul_f64(x, vshift);
    const simd::d256 ai = simd::avx::splat_f64(ATAN_F64.v[i]);
    const simd::d256 ys_s = simd::avx::xor_f64(ys, sb);
    const simd::d256 xs_s = simd::avx::xor_f64(xs, sb);
    const simd::d256 ai_s = simd::avx::xor_f64(ai, sb);
    x = simd::avx::sub_f64(x, ys_s);
    y = simd::avx::add_f64(y, xs_s);
    z = simd::avx::sub_f64(z, ai_s);
  }
  *c_out = x;
  *s_out = y;
}

};      // namespace __cordic_simd

[[gnu::flatten]] inline simd::d256
sin_cordic(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __cordic_simd::reduce_pio2_d256(x, &q);
  simd::d256 c, s;
  __cordic_simd::sincos_kernel_d256(r, &c, &s);
  //   q=0: s, q=1: c, q=2: -s, q=3: -c
  const simd::i256 q_and1 = simd::avx2::and_i256(q, simd::avx::splat_i64(1));
  const simd::d256 m_odd = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(q_and1, simd::avx::splat_i64(1)));
  const simd::d256 m_neg
      = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(simd::avx2::and_i256(q, simd::avx::splat_i64(2)), simd::avx::splat_i64(2)));
  simd::d256 result = simd::avx::blendv_f64(s, c, m_odd);
  result = simd::avx::blendv_f64(result, fneg(result), m_neg);
  return result;
}

[[gnu::flatten]] inline simd::d256
cos_cordic(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __cordic_simd::reduce_pio2_d256(x, &q);
  simd::d256 c, s;
  __cordic_simd::sincos_kernel_d256(r, &c, &s);
  // cos(x) = sin(x + pi/2) effectively shifts the quadrant by +1
  const simd::i256 qs = simd::avx2::add_i64(q, simd::avx::splat_i64(1));
  const simd::i256 q_and1 = simd::avx2::and_i256(qs, simd::avx::splat_i64(1));
  const simd::d256 m_odd = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(q_and1, simd::avx::splat_i64(1)));
  const simd::d256 m_neg
      = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(simd::avx2::and_i256(qs, simd::avx::splat_i64(2)), simd::avx::splat_i64(2)));
  simd::d256 result = simd::avx::blendv_f64(s, c, m_odd);
  result = simd::avx::blendv_f64(result, fneg(result), m_neg);
  return result;
}

[[gnu::flatten]] inline void
sincos_cordic(simd::d256 x, simd::d256 *sn, simd::d256 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::d256
tan_cordic(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __cordic_simd::reduce_pio2_d256(x, &q);
  simd::d256 c, s;
  __cordic_simd::sincos_kernel_d256(r, &c, &s);
  const simd::d256 ratio = simd::avx::div_f64(s, c);
  const simd::d256 cot = simd::avx::div_f64(fneg(c), s);
  const simd::i256 q_and1 = simd::avx2::and_i256(q, simd::avx::splat_i64(1));
  const simd::d256 m_odd = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(q_and1, simd::avx::splat_i64(1)));
  return simd::avx::blendv_f64(ratio, cot, m_odd);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// f256 path (8 lanes f32)
[[gnu::flatten]] inline simd::f256
sin_cordic(simd::f256 x) noexcept
{
  const simd::d256 lo_d = simd::avx::convert_f32_to_f64(simd::avx::cast_f32_to_lo128(x));
  const simd::d256 hi_d = simd::avx::convert_f32_to_f64(simd::avx::extract_f128_f32<1>(x));
  const simd::f128 lo = simd::avx::convert_f64_to_f32(sin_cordic(lo_d));
  const simd::f128 hi = simd::avx::convert_f64_to_f32(sin_cordic(hi_d));
  return simd::avx::insert_f128_f32<1>(simd::avx::cast_lo128_to_f32(lo), hi);
}

[[gnu::flatten]] inline simd::f256
cos_cordic(simd::f256 x) noexcept
{
  const simd::d256 lo_d = simd::avx::convert_f32_to_f64(simd::avx::cast_f32_to_lo128(x));
  const simd::d256 hi_d = simd::avx::convert_f32_to_f64(simd::avx::extract_f128_f32<1>(x));
  const simd::f128 lo = simd::avx::convert_f64_to_f32(cos_cordic(lo_d));
  const simd::f128 hi = simd::avx::convert_f64_to_f32(cos_cordic(hi_d));
  return simd::avx::insert_f128_f32<1>(simd::avx::cast_lo128_to_f32(lo), hi);
}

[[gnu::flatten]] inline simd::f256
tan_cordic(simd::f256 x) noexcept
{
  const simd::d256 lo_d = simd::avx::convert_f32_to_f64(simd::avx::cast_f32_to_lo128(x));
  const simd::d256 hi_d = simd::avx::convert_f32_to_f64(simd::avx::extract_f128_f32<1>(x));
  const simd::f128 lo = simd::avx::convert_f64_to_f32(tan_cordic(lo_d));
  const simd::f128 hi = simd::avx::convert_f64_to_f32(tan_cordic(hi_d));
  return simd::avx::insert_f128_f32<1>(simd::avx::cast_lo128_to_f32(lo), hi);
}

[[gnu::flatten]] inline void
sincos_cordic(simd::f256 x, simd::f256 *sn, simd::f256 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%
// d128 / f128 paths

[[gnu::flatten]] inline simd::d128
sin_cordic(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(sin_cordic(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::d128
cos_cordic(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(cos_cordic(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::d128
tan_cordic(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(tan_cordic(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline void
sincos_cordic(simd::d128 x, simd::d128 *sn, simd::d128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::f128
sin_cordic(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(sin_cordic(simd::avx::cast_lo128_to_f32(x)));
}

[[gnu::flatten]] inline simd::f128
cos_cordic(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(cos_cordic(simd::avx::cast_lo128_to_f32(x)));
}

[[gnu::flatten]] inline simd::f128
tan_cordic(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(tan_cordic(simd::avx::cast_lo128_to_f32(x)));
}

[[gnu::flatten]] inline void
sincos_cordic(simd::f128 x, simd::f128 *sn, simd::f128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

#endif      // AVX2 + FMA

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

namespace __cordic_simd_neon
{
inline constexpr f32 pio2_hi_f = 0x1.921fb4p+0f;
inline constexpr f32 pio2_lo_f = 0x1.4442d18p-23f;
inline constexpr f32 inv_pio2_f = 0x1.45f306p-1f;

inline constexpr f64 pio2_hi_d = 0x1.921fb54400000p+0;
inline constexpr f64 pio2_mid_d = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_lo_d = 0x1.9880000000000p-77;
inline constexpr f64 inv_pio2_d = 0x1.45f306dc9c883p-1;

using mkbits::cordic_ns::ATAN_F32;
using mkbits::cordic_ns::K_F32;
using mkbits::cordic_ns::N_FP_F32;
using mkbits::cordic_ns::SHIFT_POW_F32;

[[gnu::always_inline]] inline f32
__reduce_lane_f64(f32 xf, int *q) noexcept
{
  const f64 x = f64(xf);
  const f64 fN = mkbits::round_ns::rint<f64>(x * inv_pio2_d);
  const i64 N = i64(fN);
  f64 t = x - fN * pio2_hi_d;
  t = t - fN * pio2_mid_d;
  t = t - fN * pio2_lo_d;
  *q = int(N & 3);
  return f32(t);
}

[[gnu::always_inline]] inline simd::f128
reduce_pio2_f128(simd::f128 x, int32x4_t *q_out) noexcept
{
  int q0, q1, q2, q3;
  const f32 r0 = __reduce_lane_f64(simd::neon::get_lane_f32<0>(x), &q0);
  const f32 r1 = __reduce_lane_f64(simd::neon::get_lane_f32<1>(x), &q1);
  const f32 r2 = __reduce_lane_f64(simd::neon::get_lane_f32<2>(x), &q2);
  const f32 r3 = __reduce_lane_f64(simd::neon::get_lane_f32<3>(x), &q3);
  const f32 r_arr[4] = { r0, r1, r2, r3 };
  const int32_t q_arr[4] = { q0, q1, q2, q3 };
  *q_out = simd::neon::load_i32(q_arr);
  return simd::neon::load_f32(r_arr);
}

[[gnu::flatten]] inline void
sincos_kernel_f128(simd::f128 z, simd::f128 *c_out, simd::f128 *s_out) noexcept
{
  float32x4_t x = simd::neon::splat_f32(K_F32);
  float32x4_t y = simd::neon::splat_f32(0.0f);
  const uint32x4_t sign_bit_mask = simd::neon::splat_u32(0x80000000u);
  for ( int i = 0; i < N_FP_F32; ++i ) {
    const uint32x4_t sm = simd::neon::lt(z, simd::neon::splat_f32(0.0f));      // all-1 where z<0
    const uint32x4_t sb = simd::neon::and_(sm, sign_bit_mask);
    const float32x4_t vshift = simd::neon::splat_f32(SHIFT_POW_F32.v[i]);
    const float32x4_t ys = simd::neon::mul(y, vshift);
    const float32x4_t xs = simd::neon::mul(x, vshift);
    const float32x4_t ai = simd::neon::splat_f32(ATAN_F32.v[i]);
    const float32x4_t ys_s = simd::neon::reinterpret_f32_from_u32(simd::neon::xor_(simd::neon::reinterpret_u32_from_f32(ys), sb));
    const float32x4_t xs_s = simd::neon::reinterpret_f32_from_u32(simd::neon::xor_(simd::neon::reinterpret_u32_from_f32(xs), sb));
    const float32x4_t ai_s = simd::neon::reinterpret_f32_from_u32(simd::neon::xor_(simd::neon::reinterpret_u32_from_f32(ai), sb));
    x = simd::neon::sub(x, ys_s);
    y = simd::neon::add(y, xs_s);
    z = simd::neon::sub(z, ai_s);
  }
  *c_out = x;
  *s_out = y;
}

};      // namespace __cordic_simd_neon

[[gnu::flatten]] inline simd::f128
sin_cordic(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __cordic_simd_neon::reduce_pio2_f128(x, &q);
  float32x4_t c, s;
  __cordic_simd_neon::sincos_kernel_f128(r, &c, &s);
  const uint32x4_t odd = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i32(1)), simd::neon::splat_i32(1));
  const uint32x4_t neg = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i32(2)), simd::neon::splat_i32(2));
  float32x4_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline simd::f128
cos_cordic(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __cordic_simd_neon::reduce_pio2_f128(x, &q);
  float32x4_t c, s;
  __cordic_simd_neon::sincos_kernel_f128(r, &c, &s);
  const int32x4_t qs = simd::neon::add(q, simd::neon::splat_i32(1));
  const uint32x4_t odd = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i32(1)), simd::neon::splat_i32(1));
  const uint32x4_t neg = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i32(2)), simd::neon::splat_i32(2));
  float32x4_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline void
sincos_cordic(simd::f128 x, simd::f128 *sn, simd::f128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::f128
tan_cordic(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __cordic_simd_neon::reduce_pio2_f128(x, &q);
  float32x4_t c, s;
  __cordic_simd_neon::sincos_kernel_f128(r, &c, &s);
#if defined(__micron_arch_arm64)
  const float32x4_t ratio = simd::neon::div(s, c);
  const float32x4_t cot = simd::neon::div(simd::neon::neg(c), s);
#else
  float32x4_t rc = simd::neon::rcp_est(c);
  rc = simd::neon::mul(rc, simd::neon::rcp_step(c, rc));
  rc = simd::neon::mul(rc, simd::neon::rcp_step(c, rc));
  const float32x4_t ratio = simd::neon::mul(s, rc);
  float32x4_t rs = simd::neon::rcp_est(s);
  rs = simd::neon::mul(rs, simd::neon::rcp_step(s, rs));
  rs = simd::neon::mul(rs, simd::neon::rcp_step(s, rs));
  const float32x4_t cot = simd::neon::mul(simd::neon::neg(c), rs);
#endif
  const uint32x4_t odd = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i32(1)), simd::neon::splat_i32(1));
  return simd::neon::select(odd, cot, ratio);
}

#if defined(__micron_arch_arm64)

namespace __cordic_simd_neon_d
{
inline constexpr f64 pio2_hi = 0x1.921fb54400000p+0;
inline constexpr f64 pio2_mid = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_lo = 0x1.9880000000000p-77;
inline constexpr f64 inv_pio2 = 0x1.45f306dc9c883p-1;

using mkbits::cordic_ns::ATAN_F64;
using mkbits::cordic_ns::K_F64;
using mkbits::cordic_ns::N_FP_F64;
using mkbits::cordic_ns::SHIFT_POW_F64;

[[gnu::always_inline]] inline simd::d128
reduce(simd::d128 x, int64x2_t *q_out) noexcept
{
  const float64x2_t fN = simd::neon::rint(simd::neon::mul(x, simd::neon::splat_f64(inv_pio2)));
  float64x2_t t = simd::neon::fms_f64(x, fN, simd::neon::splat_f64(pio2_hi));
  t = simd::neon::fms_f64(t, fN, simd::neon::splat_f64(pio2_mid));
  t = simd::neon::fms_f64(t, fN, simd::neon::splat_f64(pio2_lo));
  *q_out = simd::neon::convert_f64_to_i64(fN);
  return t;
}

[[gnu::flatten]] inline void
sincos_kernel_d128(simd::d128 z, simd::d128 *c_out, simd::d128 *s_out) noexcept
{
  float64x2_t x = simd::neon::splat_f64(K_F64);
  float64x2_t y = simd::neon::splat_f64(0.0);
  const uint64x2_t sign_bit_mask = simd::neon::splat_u64(0x8000000000000000ULL);
  for ( int i = 0; i < N_FP_F64; ++i ) {
    const uint64x2_t sm = simd::neon::lt(z, simd::neon::splat_f64(0.0));
    const uint64x2_t sb = simd::neon::and_(sm, sign_bit_mask);
    const float64x2_t vshift = simd::neon::splat_f64(SHIFT_POW_F64.v[i]);
    const float64x2_t ys = simd::neon::mul(y, vshift);
    const float64x2_t xs = simd::neon::mul(x, vshift);
    const float64x2_t ai = simd::neon::splat_f64(ATAN_F64.v[i]);
    const float64x2_t ys_s = simd::neon::reinterpret_f64_from_u64(simd::neon::xor_(simd::neon::reinterpret_u64_from_f64(ys), sb));
    const float64x2_t xs_s = simd::neon::reinterpret_f64_from_u64(simd::neon::xor_(simd::neon::reinterpret_u64_from_f64(xs), sb));
    const float64x2_t ai_s = simd::neon::reinterpret_f64_from_u64(simd::neon::xor_(simd::neon::reinterpret_u64_from_f64(ai), sb));
    x = simd::neon::sub(x, ys_s);
    y = simd::neon::add(y, xs_s);
    z = simd::neon::sub(z, ai_s);
  }
  *c_out = x;
  *s_out = y;
}

};      // namespace __cordic_simd_neon_d

[[gnu::flatten]] inline simd::d128
sin_cordic(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __cordic_simd_neon_d::reduce(x, &q);
  float64x2_t c, s;
  __cordic_simd_neon_d::sincos_kernel_d128(r, &c, &s);
  const uint64x2_t odd = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i64(1)), simd::neon::splat_i64(1));
  const uint64x2_t neg = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i64(2)), simd::neon::splat_i64(2));
  float64x2_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline simd::d128
cos_cordic(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __cordic_simd_neon_d::reduce(x, &q);
  float64x2_t c, s;
  __cordic_simd_neon_d::sincos_kernel_d128(r, &c, &s);
  const int64x2_t qs = simd::neon::add(q, simd::neon::splat_i64(1));
  const uint64x2_t odd = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i64(1)), simd::neon::splat_i64(1));
  const uint64x2_t neg = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i64(2)), simd::neon::splat_i64(2));
  float64x2_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline void
sincos_cordic(simd::d128 x, simd::d128 *sn, simd::d128 *cs) noexcept
{
  *sn = sin_cordic(x);
  *cs = cos_cordic(x);
}

[[gnu::flatten]] inline simd::d128
tan_cordic(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __cordic_simd_neon_d::reduce(x, &q);
  float64x2_t c, s;
  __cordic_simd_neon_d::sincos_kernel_d128(r, &c, &s);
  const float64x2_t ratio = simd::neon::div(s, c);
  const float64x2_t cot = simd::neon::div(simd::neon::neg(c), s);
  const uint64x2_t odd = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i64(1)), simd::neon::splat_i64(1));
  return simd::neon::select(odd, cot, ratio);
}

#endif      // arm64 d128

#endif      // arm_any && neon

};      // namespace mk
};      // namespace math
};      // namespace micron

#pragma GCC diagnostic pop
