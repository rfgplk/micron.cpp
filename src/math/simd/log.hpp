//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/aliases.hpp"
#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/coeff/log_f32.hpp"
#include "../bits/coeff/log_f64.hpp"
#include "_dispatch.hpp"

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

[[gnu::flatten]] inline simd::d256
log(simd::d256 x) noexcept
{
  using namespace mkbits::coeff::log_f64_data;

  const simd::i256 xi = simd::avx::cast_f64_to_i256(x);

  const simd::i256 exp_field = simd::avx2::shr_i64(xi, 52);
  simd::i256 k = simd::avx2::sub_i64(simd::avx2::and_i256(exp_field, simd::avx::splat_i64(0x7FF)), simd::avx::splat_i64(1023));
  const simd::i256 mant_mask = simd::avx::splat_i64(0x000FFFFFFFFFFFFFLL);
  const simd::i256 bias_field = simd::avx::splat_i64(0x3FF0000000000000LL);
  simd::d256 m = simd::avx::cast_i256_to_f64(simd::avx2::or_i256(simd::avx2::and_i256(xi, mant_mask), bias_field));

  const simd::d256 sqrt_half = simd::avx::splat_f64(0x1.6a09e667f3bcdp-1);
  const simd::d256 mask = simd::avx::cmp_f64<_CMP_LT_OQ>(m, sqrt_half);
  m = simd::avx::blendv_f64(m, simd::avx::add_f64(m, m), mask);
  k = simd::avx2::sub_i64(k, simd::avx2::and_i256(simd::avx::cast_f64_to_i256(mask), simd::avx::splat_i64(1)));

  const simd::d256 f = simd::avx::sub_f64(m, simd::avx::splat_f64(1.0));
  const simd::d256 s = simd::avx::div_f64(f, simd::avx::add_f64(simd::avx::splat_f64(2.0), f));
  const simd::d256 z = simd::avx::mul_f64(s, s);
  const simd::d256 w = simd::avx::mul_f64(z, z);

  simd::d256 t1 = simd::fma::fma_f64(simd::avx::splat_f64(Lg6), w, simd::avx::splat_f64(Lg4));
  t1 = simd::fma::fma_f64(t1, w, simd::avx::splat_f64(Lg2));
  t1 = simd::avx::mul_f64(t1, w);

  simd::d256 t2 = simd::fma::fma_f64(simd::avx::splat_f64(Lg7), w, simd::avx::splat_f64(Lg5));
  t2 = simd::fma::fma_f64(t2, w, simd::avx::splat_f64(Lg3));
  t2 = simd::fma::fma_f64(t2, w, simd::avx::splat_f64(Lg1));
  t2 = simd::avx::mul_f64(t2, z);
  const simd::d256 R = simd::avx::add_f64(t1, t2);
  const simd::d256 hfsq = simd::avx::mul_f64(simd::avx::splat_f64(0.5), simd::avx::mul_f64(f, f));
  const simd::d256 log_m = simd::avx::add_f64(simd::avx::mul_f64(s, simd::avx::add_f64(hfsq, R)), simd::avx::sub_f64(f, hfsq));

  const simd::d256 fk = simd::avx::setr_f64(f64(simd::avx2::extract_i64<0>(k)), f64(simd::avx2::extract_i64<1>(k)),
                                            f64(simd::avx2::extract_i64<2>(k)), f64(simd::avx2::extract_i64<3>(k)));
  return simd::fma::fma_f64(fk, simd::avx::splat_f64(ln2_hi), simd::fma::fma_f64(fk, simd::avx::splat_f64(ln2_lo), log_m));
}

[[gnu::flatten]] inline simd::f256
log(simd::f256 x) noexcept
{
  using namespace mkbits::coeff::log_f32_data;
  const simd::i256 xi = simd::avx::cast_f32_to_i256(x);
  simd::i256 k
      = simd::avx2::sub_i32(simd::avx2::and_i256(simd::avx2::shr_i32(xi, 23), simd::avx::splat_i32(0xFF)), simd::avx::splat_i32(127));
  simd::f256 m = simd::avx::cast_i256_to_f32(
      simd::avx2::or_i256(simd::avx2::and_i256(xi, simd::avx::splat_i32(0x007FFFFF)), simd::avx::splat_i32(0x3F800000)));
  const simd::f256 sqrt_half = simd::avx::splat_f32(0x1.6a09e6p-1f);
  const simd::f256 mask = simd::avx::cmp_f32<_CMP_LT_OQ>(m, sqrt_half);
  m = simd::avx::blendv_f32(m, simd::avx::add_f32(m, m), mask);
  k = simd::avx2::sub_i32(k, simd::avx2::and_i256(simd::avx::cast_f32_to_i256(mask), simd::avx::splat_i32(1)));
  const simd::f256 f = simd::avx::sub_f32(m, simd::avx::splat_f32(1.0f));
  const simd::f256 s = simd::avx::div_f32(f, simd::avx::add_f32(simd::avx::splat_f32(2.0f), f));
  const simd::f256 z = simd::avx::mul_f32(s, s);
  const simd::f256 w = simd::avx::mul_f32(z, z);
  simd::f256 t1 = simd::fma::fma_f32(simd::avx::splat_f32(Lg4), w, simd::avx::splat_f32(Lg2));
  t1 = simd::avx::mul_f32(t1, w);
  simd::f256 t2 = simd::fma::fma_f32(simd::avx::splat_f32(Lg3), w, simd::avx::splat_f32(Lg1));
  t2 = simd::avx::mul_f32(t2, z);
  const simd::f256 R = simd::avx::add_f32(t1, t2);
  const simd::f256 hfsq = simd::avx::mul_f32(simd::avx::splat_f32(0.5f), simd::avx::mul_f32(f, f));
  const simd::f256 log_m = simd::avx::add_f32(simd::avx::mul_f32(s, simd::avx::add_f32(hfsq, R)), simd::avx::sub_f32(f, hfsq));
  const simd::f256 fk = simd::avx::convert_i32_to_f32(k);
  return simd::fma::fma_f32(fk, simd::avx::splat_f32(ln2_hi), simd::fma::fma_f32(fk, simd::avx::splat_f32(ln2_lo), log_m));
}

[[gnu::flatten]] inline simd::d256
log2(simd::d256 x) noexcept
{
  return simd::avx::mul_f64(log(x), simd::avx::splat_f64(0x1.71547652b82fep+0));
}

[[gnu::flatten]] inline simd::f256
log2(simd::f256 x) noexcept
{
  return simd::avx::mul_f32(log(x), simd::avx::splat_f32(0x1.715476p+0f));
}

[[gnu::flatten]] inline simd::d256
log10(simd::d256 x) noexcept
{
  return simd::avx::mul_f64(log(x), simd::avx::splat_f64(0x1.bcb7b1526e50ep-2));
}

[[gnu::flatten]] inline simd::f256
log10(simd::f256 x) noexcept
{
  return simd::avx::mul_f32(log(x), simd::avx::splat_f32(0x1.bcb7b2p-2f));
}

[[gnu::flatten]] inline simd::d128
log(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(log(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::f128
log(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(log(simd::avx::cast_lo128_to_f32(x)));
}

[[gnu::flatten]] inline simd::d128
log2(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(log2(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::f128
log2(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(log2(simd::avx::cast_lo128_to_f32(x)));
}

[[gnu::flatten]] inline simd::d128
log10(simd::d128 x) noexcept
{
  return simd::avx::cast_f64_to_lo128(log10(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::f128
log10(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(log10(simd::avx::cast_lo128_to_f32(x)));
}

#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::flatten]] inline simd::f128
log(simd::f128 x) noexcept
{
  using namespace mkbits::coeff::log_f32_data;
  const int32x4_t xi = simd::neon::reinterpret_s32_from_f32(x);

  int32x4_t k
      = simd::neon::sub(simd::neon::and_(simd::neon::shr_arith_i32<23>(xi), simd::neon::splat_i32(0xFF)), simd::neon::splat_i32(127));

  const int32x4_t mant_mask = simd::neon::splat_i32(0x007FFFFF);
  const int32x4_t bias = simd::neon::splat_i32(0x3F800000);
  float32x4_t m = simd::neon::reinterpret_f32_from_s32(simd::neon::or_(simd::neon::and_(xi, mant_mask), bias));

  const float32x4_t sqrt_half = simd::neon::splat_f32(0x1.6a09e6p-1f);
  const uint32x4_t lt = simd::neon::lt(m, sqrt_half);
  m = simd::neon::select(lt, simd::neon::add(m, m), m);
  k = simd::neon::sub(k, simd::neon::and_(simd::neon::reinterpret_s32_from_u32(lt), simd::neon::splat_i32(1)));
  const float32x4_t f = simd::neon::sub(m, simd::neon::splat_f32(1.0f));
#if defined(__micron_arch_arm64)
  const float32x4_t s = simd::neon::div(f, simd::neon::add(simd::neon::splat_f32(2.0f), f));
#else

  const float32x4_t denom = simd::neon::add(simd::neon::splat_f32(2.0f), f);
  float32x4_t recip = simd::neon::rcp_est(denom);
  recip = simd::neon::mul(recip, simd::neon::rcp_step(denom, recip));
  recip = simd::neon::mul(recip, simd::neon::rcp_step(denom, recip));
  const float32x4_t s = simd::neon::mul(f, recip);
#endif
  const float32x4_t z = simd::neon::mul(s, s);
  const float32x4_t w = simd::neon::mul(z, z);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t t1 = simd::neon::fma_f32(simd::neon::splat_f32(Lg2), simd::neon::splat_f32(Lg4), w);
  t1 = simd::neon::mul(t1, w);
  float32x4_t t2 = simd::neon::fma_f32(simd::neon::splat_f32(Lg1), simd::neon::splat_f32(Lg3), w);
  t2 = simd::neon::mul(t2, z);
#else
  float32x4_t t1 = simd::neon::mul(simd::neon::add(simd::neon::splat_f32(Lg2), simd::neon::mul(simd::neon::splat_f32(Lg4), w)), w);
  float32x4_t t2 = simd::neon::mul(simd::neon::add(simd::neon::splat_f32(Lg1), simd::neon::mul(simd::neon::splat_f32(Lg3), w)), z);
#endif
  const float32x4_t R = simd::neon::add(t1, t2);
  const float32x4_t hfsq = simd::neon::mul(simd::neon::splat_f32(0.5f), simd::neon::mul(f, f));
  const float32x4_t log_m = simd::neon::add(simd::neon::mul(s, simd::neon::add(hfsq, R)), simd::neon::sub(f, hfsq));
  const float32x4_t fk = simd::neon::convert_i32_to_f32(k);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  return simd::neon::fma_f32(simd::neon::fma_f32(log_m, fk, simd::neon::splat_f32(ln2_lo)), fk, simd::neon::splat_f32(ln2_hi));
#else
  return simd::neon::add(
      simd::neon::add(simd::neon::mul(fk, simd::neon::splat_f32(ln2_hi)), simd::neon::mul(fk, simd::neon::splat_f32(ln2_lo))), log_m);
#endif
}

[[gnu::flatten]] inline simd::f128
log2(simd::f128 x) noexcept
{
  return simd::neon::mul(log(x), simd::neon::splat_f32(0x1.715476p+0f));
}

[[gnu::flatten]] inline simd::f128
log10(simd::f128 x) noexcept
{
  return simd::neon::mul(log(x), simd::neon::splat_f32(0x1.bcb7b2p-2f));
}

#if defined(__micron_arch_arm64)

[[gnu::flatten]] inline simd::d128
log(simd::d128 x) noexcept
{
  using namespace mkbits::coeff::log_f64_data;
  const int64x2_t xi = simd::neon::reinterpret_s64_from_f64(x);
  int64x2_t k
      = simd::neon::sub(simd::neon::and_(simd::neon::shr_arith_i64<52>(xi), simd::neon::splat_i64(0x7FF)), simd::neon::splat_i64(1023));
  const int64x2_t mant_mask = simd::neon::splat_i64(0x000FFFFFFFFFFFFFLL);
  const int64x2_t bias = simd::neon::splat_i64(0x3FF0000000000000LL);
  float64x2_t m = simd::neon::reinterpret_f64_from_s64(simd::neon::or_(simd::neon::and_(xi, mant_mask), bias));
  const float64x2_t sqrt_half = simd::neon::splat_f64(0x1.6a09e667f3bcdp-1);
  const uint64x2_t lt = simd::neon::lt(m, sqrt_half);
  m = simd::neon::select(lt, simd::neon::add(m, m), m);
  k = simd::neon::sub(k, simd::neon::and_(simd::neon::reinterpret_s64_from_u64(lt), simd::neon::splat_i64(1)));
  const float64x2_t f = simd::neon::sub(m, simd::neon::splat_f64(1.0));
  const float64x2_t s = simd::neon::div(f, simd::neon::add(simd::neon::splat_f64(2.0), f));
  const float64x2_t z = simd::neon::mul(s, s);
  const float64x2_t w = simd::neon::mul(z, z);
  float64x2_t t1 = simd::neon::fma_f64(simd::neon::splat_f64(Lg4), simd::neon::splat_f64(Lg6), w);
  t1 = simd::neon::fma_f64(simd::neon::splat_f64(Lg2), t1, w);
  t1 = simd::neon::mul(t1, w);
  float64x2_t t2 = simd::neon::fma_f64(simd::neon::splat_f64(Lg5), simd::neon::splat_f64(Lg7), w);
  t2 = simd::neon::fma_f64(simd::neon::splat_f64(Lg3), t2, w);
  t2 = simd::neon::fma_f64(simd::neon::splat_f64(Lg1), t2, w);
  t2 = simd::neon::mul(t2, z);
  const float64x2_t R = simd::neon::add(t1, t2);
  const float64x2_t hfsq = simd::neon::mul(simd::neon::splat_f64(0.5), simd::neon::mul(f, f));
  const float64x2_t log_m = simd::neon::add(simd::neon::mul(s, simd::neon::add(hfsq, R)), simd::neon::sub(f, hfsq));

  const float64x2_t fk = simd::neon::convert_i64_to_f64(k);
  return simd::neon::fma_f64(simd::neon::fma_f64(log_m, fk, simd::neon::splat_f64(ln2_lo)), fk, simd::neon::splat_f64(ln2_hi));
}

[[gnu::flatten]] inline simd::d128
log2(simd::d128 x) noexcept
{
  return simd::neon::mul(log(x), simd::neon::splat_f64(0x1.71547652b82fep+0));
}

[[gnu::flatten]] inline simd::d128
log10(simd::d128 x) noexcept
{
  return simd::neon::mul(log(x), simd::neon::splat_f64(0x1.bcb7b1526e50ep-2));
}
#endif

#endif

};      // namespace mk
};      // namespace math
};      // namespace micron

#pragma GCC diagnostic pop
