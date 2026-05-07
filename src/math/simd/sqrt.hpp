//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/intrin.hpp"
#include "../../types.hpp"
#include "_dispatch.hpp"

namespace micron
{
namespace math
{
namespace mk
{

#if defined(__micron_x86_avx2)
[[gnu::always_inline]] inline simd::d256
sqrt(simd::d256 x) noexcept
{
  return _mm256_sqrt_pd(x);
}

[[gnu::always_inline]] inline simd::f256
sqrt(simd::f256 x) noexcept
{
  return _mm256_sqrt_ps(x);
}

[[gnu::always_inline]] inline simd::d128
sqrt(simd::d128 x) noexcept
{
  return _mm_sqrt_pd(x);
}

[[gnu::always_inline]] inline simd::f128
sqrt(simd::f128 x) noexcept
{
  return _mm_sqrt_ps(x);
}

[[gnu::always_inline]] inline simd::f256
rsqrt(simd::f256 x) noexcept
{
  simd::f256 r = _mm256_rsqrt_ps(x);
  // r = r * (1.5 - 0.5 * x * r * r)
  const simd::f256 half = _mm256_set1_ps(0.5f);
  const simd::f256 three_h = _mm256_set1_ps(1.5f);
  const simd::f256 hr = _mm256_mul_ps(_mm256_mul_ps(half, x), r);
  return _mm256_mul_ps(r, _mm256_sub_ps(three_h, _mm256_mul_ps(hr, r)));
}

[[gnu::always_inline]] inline simd::f128
rsqrt(simd::f128 x) noexcept
{
  simd::f128 r = _mm_rsqrt_ps(x);
  const simd::f128 half = _mm_set1_ps(0.5f);
  const simd::f128 three_h = _mm_set1_ps(1.5f);
  const simd::f128 hr = _mm_mul_ps(_mm_mul_ps(half, x), r);
  return _mm_mul_ps(r, _mm_sub_ps(three_h, _mm_mul_ps(hr, r)));
}

[[gnu::always_inline]] inline simd::d256
rsqrt(simd::d256 x) noexcept
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(x));
}

[[gnu::always_inline]] inline simd::d128
rsqrt(simd::d128 x) noexcept
{
  return _mm_div_pd(_mm_set1_pd(1.0), _mm_sqrt_pd(x));
}
#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)

#if defined(__micron_arch_arm64) || (defined(__ARM_ARCH) && __ARM_ARCH >= 8)
[[gnu::always_inline]] inline simd::f128
sqrt(simd::f128 x) noexcept
{
  return vsqrtq_f32(x);
}
#else

[[gnu::always_inline]] inline simd::f128
sqrt(simd::f128 x) noexcept
{
  const float32x4_t e = vrsqrteq_f32(x);
  const float32x4_t e1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, e), e), e);
  const float32x4_t e2 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, e1), e1), e1);
  return vmulq_f32(x, e2);
}
#endif

#if defined(__micron_arch_arm64)
[[gnu::always_inline]] inline simd::d128
sqrt(simd::d128 x) noexcept
{
  return vsqrtq_f64(x);
}
#endif

[[gnu::always_inline]] inline simd::f128
rsqrt(simd::f128 x) noexcept
{
  const float32x4_t r = vrsqrteq_f32(x);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x, r), r), r);
}

#if defined(__micron_arch_arm64)
[[gnu::always_inline]] inline simd::d128
rsqrt(simd::d128 x) noexcept
{
  const float64x2_t r = vrsqrteq_f64(x);

  const float64x2_t r1 = vmulq_f64(vrsqrtsq_f64(vmulq_f64(x, r), r), r);
  return vmulq_f64(vrsqrtsq_f64(vmulq_f64(x, r1), r1), r1);
}
#endif

#endif

#if defined(__micron_x86_avx512f)
[[gnu::always_inline]] inline simd::d512
sqrt(simd::d512 x) noexcept
{
  return _mm512_sqrt_pd(x);
}

[[gnu::always_inline]] inline simd::f512
sqrt(simd::f512 x) noexcept
{
  return _mm512_sqrt_ps(x);
}

[[gnu::always_inline]] inline simd::f512
rsqrt(simd::f512 x) noexcept
{
  simd::f512 r = _mm512_rsqrt14_ps(x);
  const simd::f512 half = _mm512_set1_ps(0.5f);
  const simd::f512 three_h = _mm512_set1_ps(1.5f);
  const simd::f512 hr = _mm512_mul_ps(_mm512_mul_ps(half, x), r);
  return _mm512_mul_ps(r, _mm512_sub_ps(three_h, _mm512_mul_ps(hr, r)));
}

[[gnu::always_inline]] inline simd::d512
rsqrt(simd::d512 x) noexcept
{
  simd::d512 r = _mm512_rsqrt14_pd(x);

  const simd::d512 half = _mm512_set1_pd(0.5);
  const simd::d512 three_h = _mm512_set1_pd(1.5);
  for ( int i = 0; i < 2; ++i ) {
    const simd::d512 hr = _mm512_mul_pd(_mm512_mul_pd(half, x), r);
    r = _mm512_mul_pd(r, _mm512_sub_pd(three_h, _mm512_mul_pd(hr, r)));
  }
  return r;
}
#endif

};     // namespace mk
};     // namespace math
};     // namespace micron
