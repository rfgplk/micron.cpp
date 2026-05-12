//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../simd/aliases.hpp"
#include "../../simd/intrin.hpp"
#include "../../types.hpp"
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

#if defined(__micron_x86_avx2)
[[gnu::always_inline]] inline simd::d256
sqrt(simd::d256 x) noexcept
{
  return simd::avx::sqrt_f64(x);
}

[[gnu::always_inline]] inline simd::f256
sqrt(simd::f256 x) noexcept
{
  return simd::avx::sqrt_f32(x);
}

[[gnu::always_inline]] inline simd::d128
sqrt(simd::d128 x) noexcept
{
  return simd::sse::sqrt_f64(x);
}

[[gnu::always_inline]] inline simd::f128
sqrt(simd::f128 x) noexcept
{
  return simd::sse::sqrt_f32(x);
}

[[gnu::always_inline]] inline simd::f256
rsqrt(simd::f256 x) noexcept
{
  simd::f256 r = simd::avx::rsqrt_f32(x);
  // r = r * (1.5 - 0.5 * x * r * r)
  const simd::f256 half = simd::avx::splat_f32(0.5f);
  const simd::f256 three_h = simd::avx::splat_f32(1.5f);
  const simd::f256 hr = simd::avx::mul_f32(simd::avx::mul_f32(half, x), r);
  return simd::avx::mul_f32(r, simd::avx::sub_f32(three_h, simd::avx::mul_f32(hr, r)));
}

[[gnu::always_inline]] inline simd::f128
rsqrt(simd::f128 x) noexcept
{
  simd::f128 r = simd::sse::rsqrt_f32(x);
  const simd::f128 half = simd::sse::splat_f32(0.5f);
  const simd::f128 three_h = simd::sse::splat_f32(1.5f);
  const simd::f128 hr = simd::sse::mul_f32(simd::sse::mul_f32(half, x), r);
  return simd::sse::mul_f32(r, simd::sse::sub_f32(three_h, simd::sse::mul_f32(hr, r)));
}

[[gnu::always_inline]] inline simd::d256
rsqrt(simd::d256 x) noexcept
{
  return simd::avx::div_f64(simd::avx::splat_f64(1.0), simd::avx::sqrt_f64(x));
}

[[gnu::always_inline]] inline simd::d128
rsqrt(simd::d128 x) noexcept
{
  return simd::sse::div_f64(simd::sse::splat_f64(1.0), simd::sse::sqrt_f64(x));
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
  return simd::avx512::sqrt_f64(x);
}

[[gnu::always_inline]] inline simd::f512
sqrt(simd::f512 x) noexcept
{
  return simd::avx512::sqrt_f32(x);
}

[[gnu::always_inline]] inline simd::f512
rsqrt(simd::f512 x) noexcept
{
  simd::f512 r = simd::avx512::rsqrt14_f32(x);
  const simd::f512 half = simd::avx512::splat_f32(0.5f);
  const simd::f512 three_h = simd::avx512::splat_f32(1.5f);
  const simd::f512 hr = simd::avx512::mul_f32(simd::avx512::mul_f32(half, x), r);
  return simd::avx512::mul_f32(r, simd::avx512::sub_f32(three_h, simd::avx512::mul_f32(hr, r)));
}

[[gnu::always_inline]] inline simd::d512
rsqrt(simd::d512 x) noexcept
{
  simd::d512 r = simd::avx512::rsqrt14_f64(x);

  const simd::d512 half = simd::avx512::splat_f64(0.5);
  const simd::d512 three_h = simd::avx512::splat_f64(1.5);
  for ( int i = 0; i < 2; ++i ) {
    const simd::d512 hr = simd::avx512::mul_f64(simd::avx512::mul_f64(half, x), r);
    r = simd::avx512::mul_f64(r, simd::avx512::sub_f64(three_h, simd::avx512::mul_f64(hr, r)));
  }
  return r;
}
#endif

};     // namespace mk
};     // namespace math
};     // namespace micron

#pragma GCC diagnostic pop
