//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// Cody-Waite π/2 reduction (3-word) followed by Remez polynomial

#include "../../simd/aliases.hpp"
#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/coeff/sin_f32.hpp"
#include "../bits/coeff/sin_f64.hpp"
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

namespace __trig_simd
{
inline constexpr f64 pio2_hi = 0x1.921fb54400000p+0;
inline constexpr f64 pio2_mid = 0x1.0b4611a626000p-34;
inline constexpr f64 pio2_lo = 0x1.9880000000000p-77;
inline constexpr f64 inv_pio2 = 0x1.45f306dc9c883p-1;

[[gnu::always_inline]] inline simd::d256
ksin(simd::d256 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const simd::d256 z = simd::avx::mul_f64(r, r);
  const simd::d256 r3 = simd::avx::mul_f64(z, r);
  simd::d256 p = simd::fma::fma_f64(simd::avx::splat_f64(S6), z, simd::avx::splat_f64(S5));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(S4));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(S3));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(S2));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(S1));
  return simd::fma::fma_f64(r3, p, r);
}

[[gnu::always_inline]] inline simd::d256
kcos(simd::d256 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const simd::d256 z = simd::avx::mul_f64(r, r);
  const simd::d256 hz = simd::avx::mul_f64(simd::avx::splat_f64(0.5), z);
  simd::d256 p = simd::fma::fma_f64(simd::avx::splat_f64(C6), z, simd::avx::splat_f64(C5));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(C4));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(C3));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(C2));
  p = simd::fma::fma_f64(p, z, simd::avx::splat_f64(C1));
  return simd::fma::fma_f64(simd::avx::mul_f64(z, z), p, simd::avx::sub_f64(simd::avx::splat_f64(1.0), hz));
}

[[gnu::always_inline]] inline simd::d256
reduce_pio2(simd::d256 x, simd::i256 *q_out) noexcept
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

};      // namespace __trig_simd

[[gnu::flatten]] inline simd::d256
sin(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __trig_simd::reduce_pio2(x, &q);
  const simd::d256 s = __trig_simd::ksin(r);
  const simd::d256 c = __trig_simd::kcos(r);
  //   q=0: s
  //   q=1: c
  //   q=2: -s
  //   q=3: -c
  const simd::i256 q_and1 = simd::avx2::and_i256(q, simd::avx::splat_i64(1));
  const simd::d256 m_odd = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(q_and1, simd::avx::splat_i64(1)));
  const simd::d256 m_neg
      = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(simd::avx2::and_i256(q, simd::avx::splat_i64(2)), simd::avx::splat_i64(2)));
  simd::d256 result = simd::avx::blendv_f64(s, c, m_odd);
  result = simd::avx::blendv_f64(result, fneg(result), m_neg);
  return result;
}

[[gnu::flatten]] inline simd::d256
cos(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __trig_simd::reduce_pio2(x, &q);
  const simd::d256 s = __trig_simd::ksin(r);
  const simd::d256 c = __trig_simd::kcos(r);
  // q=0: c, q=1: -s, q=2: -c, q=3: s
  // equivalent to sin(x + pi/2)
  const simd::i256 q_shift = simd::avx2::add_i64(q, simd::avx::splat_i64(1));
  const simd::i256 q_and1 = simd::avx2::and_i256(q_shift, simd::avx::splat_i64(1));
  const simd::d256 m_odd = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(q_and1, simd::avx::splat_i64(1)));
  const simd::d256 m_neg
      = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(simd::avx2::and_i256(q_shift, simd::avx::splat_i64(2)), simd::avx::splat_i64(2)));
  simd::d256 result = simd::avx::blendv_f64(s, c, m_odd);
  result = simd::avx::blendv_f64(result, fneg(result), m_neg);
  return result;
}

[[gnu::flatten]] inline void
sincos(simd::d256 x, simd::d256 *sn, simd::d256 *cs) noexcept
{
  *sn = sin(x);
  *cs = cos(x);
}

[[gnu::flatten]] inline simd::d256
tan(simd::d256 x) noexcept
{
  simd::i256 q;
  const simd::d256 r = __trig_simd::reduce_pio2(x, &q);
  const simd::d256 s = __trig_simd::ksin(r);
  const simd::d256 c = __trig_simd::kcos(r);
  const simd::d256 ratio = simd::avx::div_f64(s, c);
  const simd::d256 cot = simd::avx::div_f64(fneg(c), s);
  const simd::i256 q_and1 = simd::avx2::and_i256(q, simd::avx::splat_i64(1));
  const simd::d256 m_odd = simd::avx::cast_i256_to_f64(simd::avx2::eq_i64(q_and1, simd::avx::splat_i64(1)));
  return simd::avx::blendv_f64(ratio, cot, m_odd);
}

[[gnu::flatten]] inline simd::f256
sin(simd::f256 x) noexcept
{
  const simd::d256 lo_d = simd::avx::convert_f32_to_f64(simd::avx::cast_f32_to_lo128(x));
  const simd::d256 hi_d = simd::avx::convert_f32_to_f64(simd::avx::extract_f128_f32<1>(x));
  const simd::f128 lo = simd::avx::convert_f64_to_f32(sin(lo_d));
  const simd::f128 hi = simd::avx::convert_f64_to_f32(sin(hi_d));
  return simd::avx::insert_f128_f32<1>(simd::avx::cast_lo128_to_f32(lo), hi);
}

[[gnu::flatten]] inline simd::f256
cos(simd::f256 x) noexcept
{
  const simd::d256 lo_d = simd::avx::convert_f32_to_f64(simd::avx::cast_f32_to_lo128(x));
  const simd::d256 hi_d = simd::avx::convert_f32_to_f64(simd::avx::extract_f128_f32<1>(x));
  const simd::f128 lo = simd::avx::convert_f64_to_f32(cos(lo_d));
  const simd::f128 hi = simd::avx::convert_f64_to_f32(cos(hi_d));
  return simd::avx::insert_f128_f32<1>(simd::avx::cast_lo128_to_f32(lo), hi);
}

[[gnu::flatten]] inline simd::d128
sin(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(sin(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::f128
sin(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(sin(simd::avx::cast_lo128_to_f32(x)));
}

[[gnu::flatten]] inline simd::d128
cos(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(cos(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::f128
cos(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(cos(simd::avx::cast_lo128_to_f32(x)));
}

#endif      // AVX2 + FMA

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

namespace __trig_simd_neon
{
inline constexpr f32 pio2_hi_f = 0x1.921fb4p+0f;
inline constexpr f32 pio2_lo_f = 0x1.4442d18p-23f;      // residual to f32 precision
inline constexpr f32 inv_pio2_f = 0x1.45f306p-1f;

[[gnu::always_inline]] inline simd::f128
ksin_f128(simd::f128 r) noexcept
{
  using namespace mkbits::coeff::sin_f32_data;
  const float32x4_t z = simd::neon::mul(r, r);
  const float32x4_t r3 = simd::neon::mul(z, r);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t p = simd::neon::fma_f32(simd::neon::splat_f32(S3), simd::neon::splat_f32(S4), z);
  p = simd::neon::fma_f32(simd::neon::splat_f32(S2), p, z);
  p = simd::neon::fma_f32(simd::neon::splat_f32(S1), p, z);
  return simd::neon::fma_f32(r, r3, p);
#else
  float32x4_t p = simd::neon::add(simd::neon::splat_f32(S3), simd::neon::mul(simd::neon::splat_f32(S4), z));
  p = simd::neon::add(simd::neon::splat_f32(S2), simd::neon::mul(p, z));
  p = simd::neon::add(simd::neon::splat_f32(S1), simd::neon::mul(p, z));
  return simd::neon::add(r, simd::neon::mul(r3, p));
#endif
}

[[gnu::always_inline]] inline simd::f128
kcos_f128(simd::f128 r) noexcept
{
  using namespace mkbits::coeff::sin_f32_data;
  const float32x4_t z = simd::neon::mul(r, r);
  const float32x4_t hz = simd::neon::mul(simd::neon::splat_f32(0.5f), z);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t p = simd::neon::fma_f32(simd::neon::splat_f32(C3), simd::neon::splat_f32(C4), z);
  p = simd::neon::fma_f32(simd::neon::splat_f32(C2), p, z);
  p = simd::neon::fma_f32(simd::neon::splat_f32(C1), p, z);
  return simd::neon::fma_f32(simd::neon::sub(simd::neon::splat_f32(1.0f), hz), simd::neon::mul(z, z), p);
#else
  float32x4_t p = simd::neon::add(simd::neon::splat_f32(C3), simd::neon::mul(simd::neon::splat_f32(C4), z));
  p = simd::neon::add(simd::neon::splat_f32(C2), simd::neon::mul(p, z));
  p = simd::neon::add(simd::neon::splat_f32(C1), simd::neon::mul(p, z));
  return simd::neon::add(simd::neon::sub(simd::neon::splat_f32(1.0f), hz), simd::neon::mul(simd::neon::mul(z, z), p));
#endif
}

[[gnu::always_inline]] inline simd::f128
reduce_pio2_f128(simd::f128 x, int32x4_t *q_out) noexcept
{
#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
  const float32x4_t fN = simd::neon::rint(simd::neon::mul(x, simd::neon::splat_f32(inv_pio2_f)));
#else
  const float32x4_t scaled = simd::neon::mul(x, simd::neon::splat_f32(inv_pio2_f));
  const int32x4_t Ni0 = simd::neon::convert_f32_to_i32(scaled);
  const float32x4_t fN = simd::neon::convert_i32_to_f32(Ni0);
#endif
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t t = simd::neon::fms_f32(x, fN, simd::neon::splat_f32(pio2_hi_f));
  t = simd::neon::fms_f32(t, fN, simd::neon::splat_f32(pio2_lo_f));
#else
  float32x4_t t = simd::neon::sub(x, simd::neon::mul(fN, simd::neon::splat_f32(pio2_hi_f)));
  t = simd::neon::sub(t, simd::neon::mul(fN, simd::neon::splat_f32(pio2_lo_f)));
#endif
  *q_out = simd::neon::convert_f32_to_i32(fN);
  return t;
}

};      // namespace __trig_simd_neon

[[gnu::flatten]] inline simd::f128
sin(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __trig_simd_neon::reduce_pio2_f128(x, &q);
  const float32x4_t s = __trig_simd_neon::ksin_f128(r);
  const float32x4_t c = __trig_simd_neon::kcos_f128(r);
  const uint32x4_t odd = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i32(1)), simd::neon::splat_i32(1));
  const uint32x4_t neg = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i32(2)), simd::neon::splat_i32(2));
  float32x4_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline simd::f128
cos(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __trig_simd_neon::reduce_pio2_f128(x, &q);
  const float32x4_t s = __trig_simd_neon::ksin_f128(r);
  const float32x4_t c = __trig_simd_neon::kcos_f128(r);
  // cos = sin(x + π/2)
  const int32x4_t qs = simd::neon::add(q, simd::neon::splat_i32(1));
  const uint32x4_t odd = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i32(1)), simd::neon::splat_i32(1));
  const uint32x4_t neg = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i32(2)), simd::neon::splat_i32(2));
  float32x4_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline void
sincos(simd::f128 x, simd::f128 *sn, simd::f128 *cs) noexcept
{
  *sn = sin(x);
  *cs = cos(x);
}

[[gnu::flatten]] inline simd::f128
tan(simd::f128 x) noexcept
{
  int32x4_t q;
  const float32x4_t r = __trig_simd_neon::reduce_pio2_f128(x, &q);
  const float32x4_t s = __trig_simd_neon::ksin_f128(r);
  const float32x4_t c = __trig_simd_neon::kcos_f128(r);
#if defined(__micron_arch_arm64)
  const float32x4_t ratio = simd::neon::div(s, c);
  const float32x4_t cot = simd::neon::div(simd::neon::neg(c), s);
#else
  // arm32 lacks hw vdivq_f32; refine vrecpeq via two Newton-Raphson steps.
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
namespace __trig_simd_neon_d
{
[[gnu::always_inline]] inline simd::d128
ksin(simd::d128 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const float64x2_t z = simd::neon::mul(r, r);
  const float64x2_t r3 = simd::neon::mul(z, r);
  float64x2_t p = simd::neon::fma_f64(simd::neon::splat_f64(S5), simd::neon::splat_f64(S6), z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(S4), p, z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(S3), p, z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(S2), p, z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(S1), p, z);
  return simd::neon::fma_f64(r, r3, p);
}

[[gnu::always_inline]] inline simd::d128
kcos(simd::d128 r) noexcept
{
  using namespace mkbits::coeff::sin_f64_data;
  const float64x2_t z = simd::neon::mul(r, r);
  const float64x2_t hz = simd::neon::mul(simd::neon::splat_f64(0.5), z);
  float64x2_t p = simd::neon::fma_f64(simd::neon::splat_f64(C5), simd::neon::splat_f64(C6), z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(C4), p, z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(C3), p, z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(C2), p, z);
  p = simd::neon::fma_f64(simd::neon::splat_f64(C1), p, z);
  return simd::neon::fma_f64(simd::neon::sub(simd::neon::splat_f64(1.0), hz), simd::neon::mul(z, z), p);
}

[[gnu::always_inline]] inline simd::d128
reduce(simd::d128 x, int64x2_t *q_out) noexcept
{
  using namespace __trig_simd;
  const float64x2_t fN = simd::neon::rint(simd::neon::mul(x, simd::neon::splat_f64(inv_pio2)));
  float64x2_t t = simd::neon::fms_f64(x, fN, simd::neon::splat_f64(pio2_hi));
  t = simd::neon::fms_f64(t, fN, simd::neon::splat_f64(pio2_mid));
  t = simd::neon::fms_f64(t, fN, simd::neon::splat_f64(pio2_lo));
  *q_out = simd::neon::convert_f64_to_i64(fN);
  return t;
}
};      // namespace __trig_simd_neon_d

[[gnu::flatten]] inline simd::d128
sin(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __trig_simd_neon_d::reduce(x, &q);
  const float64x2_t s = __trig_simd_neon_d::ksin(r);
  const float64x2_t c = __trig_simd_neon_d::kcos(r);
  const uint64x2_t odd = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i64(1)), simd::neon::splat_i64(1));
  const uint64x2_t neg = simd::neon::eq(simd::neon::and_(q, simd::neon::splat_i64(2)), simd::neon::splat_i64(2));
  float64x2_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline simd::d128
cos(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __trig_simd_neon_d::reduce(x, &q);
  const float64x2_t s = __trig_simd_neon_d::ksin(r);
  const float64x2_t c = __trig_simd_neon_d::kcos(r);
  const int64x2_t qs = simd::neon::add(q, simd::neon::splat_i64(1));
  const uint64x2_t odd = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i64(1)), simd::neon::splat_i64(1));
  const uint64x2_t neg = simd::neon::eq(simd::neon::and_(qs, simd::neon::splat_i64(2)), simd::neon::splat_i64(2));
  float64x2_t result = simd::neon::select(odd, c, s);
  result = simd::neon::select(neg, simd::neon::neg(result), result);
  return result;
}

[[gnu::flatten]] inline void
sincos(simd::d128 x, simd::d128 *sn, simd::d128 *cs) noexcept
{
  *sn = sin(x);
  *cs = cos(x);
}

[[gnu::flatten]] inline simd::d128
tan(simd::d128 x) noexcept
{
  int64x2_t q;
  const float64x2_t r = __trig_simd_neon_d::reduce(x, &q);
  const float64x2_t s = __trig_simd_neon_d::ksin(r);
  const float64x2_t c = __trig_simd_neon_d::kcos(r);
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
