//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../simd/intrin.hpp"
#include "../simd/types.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "generic.hpp"

namespace micron
{
namespace math
{

inline f32
cbrt(const f32 x)
{
  return math::powerf32(x, 1 / 3.f);
};

inline f64
cbrtd(const f64 x)
{
  return math::powerf(x, 1 / 3.f);
};

inline flong
cbrtdl(const flong x)
{
  return math::powerflong(x, 1 / 3.f);
};

constexpr float
fsqrt(float x) noexcept
{
  return __builtin_sqrtf(x);
}

constexpr double
fsqrt(double x) noexcept
{
  return __builtin_sqrt(x);
}

constexpr long double
fsqrt(long double x) noexcept
{
  return __builtin_sqrtl(x);
}

constexpr float
frsqrt(float x) noexcept
{
  return 1.f / __builtin_sqrtf(x);
}

constexpr double
frsqrt(double x) noexcept
{
  return 1.0 / __builtin_sqrt(x);
}

constexpr long double
frsqrt(long double x) noexcept
{
  return 1.0L / __builtin_sqrtl(x);
}

constexpr u32
fsqrt(u32 x) noexcept
{
  return static_cast<u32>(__builtin_sqrt(static_cast<double>(x)));
}

constexpr u64
fsqrt(u64 x) noexcept
{
  return static_cast<u64>(__builtin_sqrtl(static_cast<long double>(x)));
}

constexpr i32
fsqrt(i32 x) noexcept
{
  return x < 0 ? 0 : static_cast<i32>(__builtin_sqrt(static_cast<double>(x)));
}

constexpr i64
fsqrt(i64 x) noexcept
{
  return x < 0 ? 0 : static_cast<i64>(__builtin_sqrtl(static_cast<long double>(x)));
}

template <typename T>
  requires micron::is_arithmetic_v<T>
constexpr T
sqrt(T x) noexcept
{
  return fsqrt(x);
}

inline float
gsqrt(const float x)
{
  return static_cast<float>(__builtin_sqrt(x));
};

inline float
fsqrt_simd(const float x)
{
#if defined(__micron_arch_x86_any)
  return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return vgetq_lane_f32(vsqrtq_f32(vdupq_n_f32(x)), 0);
#else
  return __builtin_sqrtf(x);
#endif
}

inline float
ss_sqrt(const float x)
{
#if defined(__micron_arch_x86_any)
  return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return vgetq_lane_f32(vsqrtq_f32(vdupq_n_f32(x)), 0);
#else
  return __builtin_sqrtf(x);
#endif
}

inline float
ss_rsqrt(const float x)
{
#if defined(__micron_arch_x86_any)
  return 1.0f / _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return 1.0f / vgetq_lane_f32(vsqrtq_f32(vdupq_n_f32(x)), 0);
#else
  return 1.0f / __builtin_sqrtf(x);
#endif
}

inline double
sd_sqrt(const double x)
{
#if defined(__micron_arch_x86_any)
  return _mm_cvtsd_f64(_mm_sqrt_sd(_mm_setzero_pd(), _mm_set_sd(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return vgetq_lane_f64(vsqrtq_f64(vdupq_n_f64(x)), 0);
#else
  return __builtin_sqrt(x);
#endif
}

inline double
sd_rsqrt(const double x)
{
#if defined(__micron_arch_x86_any)
  return 1.0 / _mm_cvtsd_f64(_mm_sqrt_sd(_mm_setzero_pd(), _mm_set_sd(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return 1.0 / vgetq_lane_f64(vsqrtq_f64(vdupq_n_f64(x)), 0);
#else
  return 1.0 / __builtin_sqrt(x);
#endif
}

#if defined(__micron_arch_x86_any)
__attribute__((always_inline)) inline simd::f128
vsqrt(simd::f128 v) noexcept
{
  return _mm_sqrt_ps(v);
}

__attribute__((always_inline)) inline simd::d128
vsqrt(simd::d128 v) noexcept
{
  return _mm_sqrt_pd(v);
}

__attribute__((always_inline)) inline simd::f128
vrsqrt_approx(simd::f128 v) noexcept
{
  return _mm_rsqrt_ps(v);
}
#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
__attribute__((always_inline)) inline simd::f128
vsqrt(simd::f128 v) noexcept
{
#if defined(__micron_arch_arm64)
  return vsqrtq_f32(v);
#else
  float tmp[4];
  vst1q_f32(tmp, v);
  tmp[0] = __builtin_sqrtf(tmp[0]);
  tmp[1] = __builtin_sqrtf(tmp[1]);
  tmp[2] = __builtin_sqrtf(tmp[2]);
  tmp[3] = __builtin_sqrtf(tmp[3]);
  return vld1q_f32(tmp);
#endif
}

#if defined(__micron_arch_arm64)
__attribute__((always_inline)) inline simd::d128
vsqrt(simd::d128 v) noexcept
{
  return vsqrtq_f64(v);
}
#endif

__attribute__((always_inline)) inline simd::f128
vrsqrt_approx(simd::f128 v) noexcept
{
  return vrsqrteq_f32(v);
}
#endif

#if defined(__micron_arch_x86_any)
__attribute__((always_inline)) inline simd::f256
vsqrt(simd::f256 v) noexcept
{
  return _mm256_sqrt_ps(v);
}

__attribute__((always_inline)) inline simd::d256
vsqrt(simd::d256 v) noexcept
{
  return _mm256_sqrt_pd(v);
}

__attribute__((always_inline)) inline simd::f256
vrsqrt_approx(simd::f256 v) noexcept
{
  return _mm256_rsqrt_ps(v);
}
#endif

#if defined(__micron_arch_x86_any) && defined(__micron_x86_avx512f)
__attribute__((always_inline)) inline simd::f512
vsqrt(simd::f512 v) noexcept
{
  return _mm512_sqrt_ps(v);
}

__attribute__((always_inline)) inline simd::d512
vsqrt(simd::d512 v) noexcept
{
  return _mm512_sqrt_pd(v);
}

__attribute__((always_inline)) inline simd::f512
vrsqrt_approx(simd::f512 v) noexcept
{
  return _mm512_rsqrt14_ps(v);
}
#endif

};     // namespace math
};     // namespace micron
