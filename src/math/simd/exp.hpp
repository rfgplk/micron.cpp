//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/aliases.hpp"
#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "../bits/coeff/exp_f32.hpp"
#include "../bits/coeff/exp_f64.hpp"
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
exp(simd::d256 x) noexcept
{
  using namespace mkbits::coeff::exp_f64_data;

  const simd::d256 hi = simd::avx::splat_f64(709.78);
  const simd::d256 lo = simd::avx::splat_f64(-745.13);
  x = simd::avx::min_f64(simd::avx::max_f64(x, lo), hi);

  const simd::d256 fN
      = simd::avx::round_f64<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(simd::avx::mul_f64(x, simd::avx::splat_f64(inv_ln2_32)));

  simd::d256 r = simd::fma::fnma_f64(fN, simd::avx::splat_f64(ln2_32_hi), x);
  r = simd::fma::fnma_f64(fN, simd::avx::splat_f64(ln2_32_lo), r);

  simd::d256 p = simd::fma::fma_f64(simd::avx::splat_f64(rem[4]), r, simd::avx::splat_f64(rem[3]));
  p = simd::fma::fma_f64(p, r, simd::avx::splat_f64(rem[2]));
  p = simd::fma::fma_f64(p, r, simd::avx::splat_f64(rem[1]));
  p = simd::fma::fma_f64(p, r, simd::avx::splat_f64(rem[0]));
  const simd::d256 r2 = simd::avx::mul_f64(r, r);
  const simd::d256 exp_r = simd::fma::fma_f64(p, r2, simd::avx::add_f64(r, simd::avx::splat_f64(1.0)));

  const __m128i N32 = simd::avx::convert_f64_to_i32(fN);
  const __m128i j32 = simd::sse::and_i128(N32, simd::sse::splat_i32(31));

  const simd::d256 tw = simd::avx2::gather_f64<8>(reinterpret_cast<const double *>(twoN), j32);

  const __m128i k32 = simd::sse::shr_arith_i32(N32, 5);
  const simd::i256 k64 = simd::avx2::widen_i32_to_i64(k32);
  const simd::i256 biased = simd::avx2::add_i64(k64, simd::avx::splat_i64(1023));
  const simd::i256 expbits = simd::avx2::shl_i64(biased, 52);
  const simd::d256 scale = simd::avx::cast_i256_to_f64(expbits);
  return simd::avx::mul_f64(simd::avx::mul_f64(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::f256
exp(simd::f256 x) noexcept
{
  using namespace mkbits::coeff::exp_f32_data;
  const simd::f256 hi = simd::avx::splat_f32(88.722f);
  const simd::f256 lo = simd::avx::splat_f32(-103.972f);
  x = simd::avx::min_f32(simd::avx::max_f32(x, lo), hi);
  const simd::f256 fN
      = simd::avx::round_f32<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(simd::avx::mul_f32(x, simd::avx::splat_f32(inv_ln2_16)));
  simd::f256 r = simd::fma::fnma_f32(fN, simd::avx::splat_f32(ln2_16_hi), x);
  r = simd::fma::fnma_f32(fN, simd::avx::splat_f32(ln2_16_lo), r);
  simd::f256 p = simd::fma::fma_f32(simd::avx::splat_f32(rem[1]), r, simd::avx::splat_f32(rem[0]));
  const simd::f256 r2 = simd::avx::mul_f32(r, r);
  const simd::f256 exp_r = simd::fma::fma_f32(p, r2, simd::avx::add_f32(r, simd::avx::splat_f32(1.0f)));
  const simd::i256 N = simd::avx::convert_f32_to_i32(fN);
  const simd::i256 j = simd::avx2::and_i256(N, simd::avx::splat_i32(15));
  const simd::f256 tw = simd::avx2::gather_f32<4>(reinterpret_cast<const float *>(twoN), j);
  const simd::i256 k = simd::avx2::shr_arith_i32(N, 4);
  const simd::i256 biased = simd::avx2::add_i32(k, simd::avx::splat_i32(127));
  const simd::i256 expbits = simd::avx2::shl_i32(biased, 23);
  const simd::f256 scale = simd::avx::cast_i256_to_f32(expbits);
  return simd::avx::mul_f32(simd::avx::mul_f32(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::d128
exp(simd::d128 x) noexcept
{

  return simd::avx::cast_f64_to_lo128(exp(simd::avx::cast_lo128_to_f64(x)));
}

[[gnu::flatten]] inline simd::f128
exp(simd::f128 x) noexcept
{
  return simd::avx::cast_f32_to_lo128(exp(simd::avx::cast_lo128_to_f32(x)));
}

[[gnu::flatten]] inline simd::d256
exp2(simd::d256 x) noexcept
{

  constexpr f64 ln2_hi = 0x1.62e42fefa39efp-1;
  constexpr f64 ln2_lo = 0x1.abc9e3b39803fp-56;
  const simd::d256 y = simd::fma::fma_f64(x, simd::avx::splat_f64(ln2_hi), simd::avx::mul_f64(x, simd::avx::splat_f64(ln2_lo)));
  return exp(y);
}

[[gnu::flatten]] inline simd::f256
exp2(simd::f256 x) noexcept
{
  constexpr f32 ln2 = 0x1.62e430p-1f;
  return exp(simd::avx::mul_f32(x, simd::avx::splat_f32(ln2)));
}

[[gnu::flatten]] inline simd::d256
expm1(simd::d256 x) noexcept
{
  return simd::avx::sub_f64(exp(x), simd::avx::splat_f64(1.0));
}

[[gnu::flatten]] inline simd::f256
expm1(simd::f256 x) noexcept
{
  return simd::avx::sub_f32(exp(x), simd::avx::splat_f32(1.0f));
}

#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

[[gnu::flatten]] inline simd::f128
exp(simd::f128 x) noexcept
{
  using namespace mkbits::coeff::exp_f32_data;
  const float32x4_t cap_hi = simd::neon::splat_f32(88.722f);
  const float32x4_t cap_lo = simd::neon::splat_f32(-103.972f);
  x = simd::neon::min(simd::neon::max(x, cap_lo), cap_hi);

#if defined(__micron_arch_arm64) || defined(__micron_arm_directed_rounding)
  const float32x4_t fN = simd::neon::rint(simd::neon::mul(x, simd::neon::splat_f32(inv_ln2_16)));
#else
  const uint32x4_t __sign_mask = simd::neon::splat_u32(0x80000000u);
  const uint32x4_t __xs = simd::neon::and_(simd::neon::reinterpret_u32_from_f32(x), __sign_mask);
  const float32x4_t __half
      = simd::neon::reinterpret_f32_from_u32(simd::neon::or_(__xs, simd::neon::reinterpret_u32_from_f32(simd::neon::splat_f32(0.5f))));
#if defined(__micron_arm_fma)
  const float32x4_t __scaled = simd::neon::fma_f32(__half, x, simd::neon::splat_f32(inv_ln2_16));
#else
  const float32x4_t __scaled = simd::neon::add(simd::neon::mul(x, simd::neon::splat_f32(inv_ln2_16)), __half);
#endif
  const int32x4_t Ni_pre = simd::neon::convert_f32_to_i32(__scaled);
  const float32x4_t fN = simd::neon::convert_i32_to_f32(Ni_pre);
#endif

#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  float32x4_t r = simd::neon::fms_f32(x, fN, simd::neon::splat_f32(ln2_16_hi));
  r = simd::neon::fms_f32(r, fN, simd::neon::splat_f32(ln2_16_lo));

  float32x4_t p = simd::neon::fma_f32(simd::neon::splat_f32(rem[0]), simd::neon::splat_f32(rem[1]), r);
#else
  float32x4_t r = simd::neon::sub(x, simd::neon::mul(fN, simd::neon::splat_f32(ln2_16_hi)));
  r = simd::neon::sub(r, simd::neon::mul(fN, simd::neon::splat_f32(ln2_16_lo)));
  float32x4_t p = simd::neon::add(simd::neon::splat_f32(rem[0]), simd::neon::mul(simd::neon::splat_f32(rem[1]), r));
#endif
  const float32x4_t r2 = simd::neon::mul(r, r);
  const float32x4_t one = simd::neon::splat_f32(1.0f);
#if defined(__micron_arm_fma) || defined(__micron_arch_arm64)
  const float32x4_t exp_r = simd::neon::fma_f32(simd::neon::add(r, one), p, r2);
#else
  const float32x4_t exp_r = simd::neon::add(simd::neon::add(r, one), simd::neon::mul(p, r2));
#endif

  const int32x4_t Ni = simd::neon::convert_f32_to_i32(fN);
  const int32x4_t j = simd::neon::and_(Ni, simd::neon::splat_i32(15));
  const f32 tw_arr[4] = {
    twoN[simd::neon::get_lane_i32<0>(j)],
    twoN[simd::neon::get_lane_i32<1>(j)],
    twoN[simd::neon::get_lane_i32<2>(j)],
    twoN[simd::neon::get_lane_i32<3>(j)],
  };
  const float32x4_t tw = simd::neon::load_f32(tw_arr);

  const int32x4_t k = simd::neon::shr_arith_i32<4>(Ni);
  const int32x4_t biased = simd::neon::add(k, simd::neon::splat_i32(127));
  const int32x4_t expbits = simd::neon::shl_i32<23>(biased);
  const float32x4_t scale = simd::neon::reinterpret_f32_from_s32(expbits);
  return simd::neon::mul(simd::neon::mul(tw, exp_r), scale);
}

#if defined(__micron_arch_arm64)

[[gnu::flatten]] inline simd::d128
exp(simd::d128 x) noexcept
{
  using namespace mkbits::coeff::exp_f64_data;
  const float64x2_t cap_hi = simd::neon::splat_f64(709.78);
  const float64x2_t cap_lo = simd::neon::splat_f64(-745.13);
  x = simd::neon::min(simd::neon::max(x, cap_lo), cap_hi);
  const float64x2_t fN = simd::neon::rint(simd::neon::mul(x, simd::neon::splat_f64(inv_ln2_32)));
  float64x2_t r = simd::neon::fms_f64(x, fN, simd::neon::splat_f64(ln2_32_hi));
  r = simd::neon::fms_f64(r, fN, simd::neon::splat_f64(ln2_32_lo));
  float64x2_t p = simd::neon::fma_f64(simd::neon::splat_f64(rem[3]), simd::neon::splat_f64(rem[4]), r);
  p = simd::neon::fma_f64(simd::neon::splat_f64(rem[2]), p, r);
  p = simd::neon::fma_f64(simd::neon::splat_f64(rem[1]), p, r);
  p = simd::neon::fma_f64(simd::neon::splat_f64(rem[0]), p, r);
  const float64x2_t r2 = simd::neon::mul(r, r);
  const float64x2_t one = simd::neon::splat_f64(1.0);
  const float64x2_t exp_r = simd::neon::fma_f64(simd::neon::add(r, one), p, r2);
  const int64x2_t Ni = simd::neon::convert_f64_to_i64(fN);
  const int64x2_t j = simd::neon::and_(Ni, simd::neon::splat_i64(31));
  const f64 tw_arr[2] = {
    twoN[simd::neon::get_lane_i64<0>(j)],
    twoN[simd::neon::get_lane_i64<1>(j)],
  };
  const float64x2_t tw = simd::neon::load_f64(tw_arr);
  const int64x2_t k = simd::neon::shr_arith_i64<5>(Ni);
  const int64x2_t biased = simd::neon::add(k, simd::neon::splat_i64(1023));
  const int64x2_t expbits = simd::neon::shl_i64<52>(biased);
  const float64x2_t scale = simd::neon::reinterpret_f64_from_s64(expbits);
  return simd::neon::mul(simd::neon::mul(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::d128
exp2(simd::d128 x) noexcept
{
  constexpr f64 ln2_hi = 0x1.62e42fefa39efp-1;
  constexpr f64 ln2_lo = 0x1.abc9e3b39803fp-56;
  const float64x2_t y = simd::neon::fma_f64(simd::neon::mul(x, simd::neon::splat_f64(ln2_lo)), x, simd::neon::splat_f64(ln2_hi));
  return exp(y);
}

[[gnu::flatten]] inline simd::d128
expm1(simd::d128 x) noexcept
{
  return simd::neon::sub(exp(x), simd::neon::splat_f64(1.0));
}
#endif

[[gnu::flatten]] inline simd::f128
exp2(simd::f128 x) noexcept
{
  constexpr f32 ln2 = 0x1.62e430p-1f;
  return exp(simd::neon::mul(x, simd::neon::splat_f32(ln2)));
}

[[gnu::flatten]] inline simd::f128
expm1(simd::f128 x) noexcept
{
  return simd::neon::sub(exp(x), simd::neon::splat_f32(1.0f));
}

#endif

#if defined(__micron_x86_avx512f)

[[gnu::flatten]] inline simd::d512
exp(simd::d512 x) noexcept
{
  using namespace mkbits::coeff::exp_f64_data;
  const simd::d512 hi = simd::avx512::splat_f64(709.78);
  const simd::d512 lo = simd::avx512::splat_f64(-745.13);
  x = simd::avx512::min_f64(simd::avx512::max_f64(x, lo), hi);
  const simd::d512 fN = simd::avx512::roundscale_f64<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(
      simd::avx512::mul_f64(x, simd::avx512::splat_f64(inv_ln2_32)));
  simd::d512 r = simd::fma::fnma_f64_v512(fN, simd::avx512::splat_f64(ln2_32_hi), x);
  r = simd::fma::fnma_f64_v512(fN, simd::avx512::splat_f64(ln2_32_lo), r);
  simd::d512 p = simd::fma::fma_f64_v512(simd::avx512::splat_f64(rem[4]), r, simd::avx512::splat_f64(rem[3]));
  p = simd::fma::fma_f64_v512(p, r, simd::avx512::splat_f64(rem[2]));
  p = simd::fma::fma_f64_v512(p, r, simd::avx512::splat_f64(rem[1]));
  p = simd::fma::fma_f64_v512(p, r, simd::avx512::splat_f64(rem[0]));
  const simd::d512 r2 = simd::avx512::mul_f64(r, r);
  const simd::d512 exp_r = simd::fma::fma_f64_v512(p, r2, simd::avx512::add_f64(r, simd::avx512::splat_f64(1.0)));
  const simd::i512 N = simd::avx512::convert_f64_to_i64(fN);
  const simd::i512 j = simd::avx512::and_i512(N, simd::avx512::splat_i64(31));
  const simd::d512 tw = simd::avx512::gather_f64<8>(j, reinterpret_cast<const double *>(twoN));
  const simd::i512 k = simd::avx512::shr_arith_i64(N, 5);
  const simd::i512 biased = simd::avx512::add_i64(k, simd::avx512::splat_i64(1023));
  const simd::i512 expbits = simd::avx512::shl_i64(biased, 52);
  const simd::d512 scale = simd::avx512::cast_i512_to_f64(expbits);
  return simd::avx512::mul_f64(simd::avx512::mul_f64(tw, exp_r), scale);
}

[[gnu::flatten]] inline simd::f512
exp(simd::f512 x) noexcept
{
  using namespace mkbits::coeff::exp_f32_data;
  const simd::f512 hi = simd::avx512::splat_f32(88.722f);
  const simd::f512 lo = simd::avx512::splat_f32(-103.972f);
  x = simd::avx512::min_f32(simd::avx512::max_f32(x, lo), hi);
  const simd::f512 fN = simd::avx512::roundscale_f32<_MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC>(
      simd::avx512::mul_f32(x, simd::avx512::splat_f32(inv_ln2_16)));
  simd::f512 r = simd::fma::fnma_f32_v512(fN, simd::avx512::splat_f32(ln2_16_hi), x);
  r = simd::fma::fnma_f32_v512(fN, simd::avx512::splat_f32(ln2_16_lo), r);
  simd::f512 p = simd::fma::fma_f32_v512(simd::avx512::splat_f32(rem[1]), r, simd::avx512::splat_f32(rem[0]));
  const simd::f512 r2 = simd::avx512::mul_f32(r, r);
  const simd::f512 exp_r = simd::fma::fma_f32_v512(p, r2, simd::avx512::add_f32(r, simd::avx512::splat_f32(1.0f)));
  const simd::i512 N = simd::avx512::convert_f32_to_i32(fN);
  const simd::i512 j = simd::avx512::and_i512(N, simd::avx512::splat_i32(15));
  const simd::f512 tw = simd::avx512::gather_f32<4>(j, reinterpret_cast<const float *>(twoN));
  const simd::i512 k = simd::avx512::shr_arith_i32(N, 4);
  const simd::i512 biased = simd::avx512::add_i32(k, simd::avx512::splat_i32(127));
  const simd::i512 expbits = simd::avx512::shl_i32(biased, 23);
  const simd::f512 scale = simd::avx512::cast_i512_to_f32(expbits);
  return simd::avx512::mul_f32(simd::avx512::mul_f32(tw, exp_r), scale);
}
#endif

};      // namespace mk
};      // namespace math
};      // namespace micron

#pragma GCC diagnostic pop
