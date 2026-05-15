//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../simd/aliases.hpp"
#include "../simd/intrin.hpp"
#include "../simd/types.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "bits/sqrt.hpp"

#include "generic.hpp"

#include "__asm/hw.hpp"

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
  return float(mkbits::sqrt_ns::sqrt<f32>(f32(x)));
}

constexpr double
fsqrt(double x) noexcept
{
  return double(mkbits::sqrt_ns::sqrt<f64>(f64(x)));
}

constexpr long double
fsqrt(long double x) noexcept
{
  return static_cast<long double>(mkbits::sqrt_ns::sqrt<f64>(f64(x)));
}

constexpr float
frsqrt(float x) noexcept
{
  return float(mkbits::sqrt_ns::rsqrt<f32>(f32(x)));
}

constexpr double
frsqrt(double x) noexcept
{
  return double(mkbits::sqrt_ns::rsqrt<f64>(f64(x)));
}

constexpr long double
frsqrt(long double x) noexcept
{
  return static_cast<long double>(mkbits::sqrt_ns::rsqrt<f64>(f64(x)));
}

constexpr u32
fsqrt(u32 x) noexcept
{
  return static_cast<u32>(mkbits::sqrt_ns::sqrt<f64>(f64(x)));
}

constexpr u64
fsqrt(u64 x) noexcept
{
  return static_cast<u64>(mkbits::sqrt_ns::sqrt<f64>(f64(x)));
}

constexpr i32
fsqrt(i32 x) noexcept
{
  return x < 0 ? 0 : static_cast<i32>(mkbits::sqrt_ns::sqrt<f64>(f64(x)));
}

constexpr i64
fsqrt(i64 x) noexcept
{
  return x < 0 ? 0 : static_cast<i64>(mkbits::sqrt_ns::sqrt<f64>(f64(x)));
}

template<typename T>
  requires micron::is_arithmetic_v<T>
constexpr T
sqrt(T x) noexcept
{
  return fsqrt(x);
}

inline float
gsqrt(const float x)
{
  return static_cast<float>(hw::sqrt_sd(f64(x)));
};

inline float
fsqrt_simd(const float x)
{
#if defined(__micron_arch_x86_any)
  return simd::sse::extract_low_f32(simd::sse::sqrt_scalar_f32(simd::sse::set_scalar_f32(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return simd::neon::get_lane_f32<0>(simd::neon::sqrt(simd::neon::splat_f32(x)));
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  return float(hw::sqrt_ss(f32(x)));
#else
  return float(hw::sqrt_ss(f32(x)));
#endif
}

inline float
ss_sqrt(const float x)
{
#if defined(__micron_arch_x86_any)
  return simd::sse::extract_low_f32(simd::sse::sqrt_scalar_f32(simd::sse::set_scalar_f32(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return simd::neon::get_lane_f32<0>(simd::neon::sqrt(simd::neon::splat_f32(x)));
#elif defined(__micron_arch_arm32)
  return float(hw::sqrt_ss(f32(x)));
#else
  return float(hw::sqrt_ss(f32(x)));
#endif
}

inline float
ss_rsqrt(const float x)
{
#if defined(__micron_arch_x86_any)
  return 1.0f / simd::sse::extract_low_f32(simd::sse::rsqrt_scalar_f32(simd::sse::set_scalar_f32(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return 1.0f / simd::neon::get_lane_f32<0>(simd::neon::sqrt(simd::neon::splat_f32(x)));
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  return simd::neon::get_lane_f32<0>(simd::neon::rsqrt_est(simd::neon::splat_f32(x)));
#else
  return 1.0f / float(hw::sqrt_ss(f32(x)));
#endif
}

inline double
sd_sqrt(const double x)
{
#if defined(__micron_arch_x86_any)
  return simd::sse::extract_low_f64(simd::sse::sqrt_sd(simd::sse::zero_f64(), simd::sse::set_scalar_f64(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return simd::neon::get_lane_f64<0>(simd::neon::sqrt(simd::neon::splat_f64(x)));
#elif defined(__micron_arch_arm32)
  return double(hw::sqrt_sd(f64(x)));
#else
  return double(hw::sqrt_sd(f64(x)));
#endif
}

inline double
sd_rsqrt(const double x)
{
#if defined(__micron_arch_x86_any)
  return 1.0 / simd::sse::extract_low_f64(simd::sse::sqrt_sd(simd::sse::zero_f64(), simd::sse::set_scalar_f64(x)));
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return 1.0 / simd::neon::get_lane_f64<0>(simd::neon::sqrt(simd::neon::splat_f64(x)));
#elif defined(__micron_arch_arm32)
  return 1.0 / double(hw::sqrt_sd(f64(x)));
#else
  return 1.0 / double(hw::sqrt_sd(f64(x)));
#endif
}

#if defined(__micron_arch_x86_any)
__attribute__((always_inline)) inline simd::f128
vsqrt(simd::f128 v) noexcept
{
  return simd::sse::sqrt_f32(v);
}

__attribute__((always_inline)) inline simd::d128
vsqrt(simd::d128 v) noexcept
{
  return simd::sse::sqrt_f64(v);
}

__attribute__((always_inline)) inline simd::f128
vrsqrt_approx(simd::f128 v) noexcept
{
  return simd::sse::rsqrt_f32(v);
}
#elif defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
__attribute__((always_inline)) inline simd::f128
vsqrt(simd::f128 v) noexcept
{
#if defined(__micron_arch_arm64)
  return simd::neon::sqrt(v);
#else
  float tmp[4];
  simd::neon::store_f32(tmp, v);
  tmp[0] = float(hw::sqrt_ss(f32(tmp[0])));
  tmp[1] = float(hw::sqrt_ss(f32(tmp[1])));
  tmp[2] = float(hw::sqrt_ss(f32(tmp[2])));
  tmp[3] = float(hw::sqrt_ss(f32(tmp[3])));
  return simd::neon::load_f32(tmp);
#endif
}

#if defined(__micron_arch_arm64)
__attribute__((always_inline)) inline simd::d128
vsqrt(simd::d128 v) noexcept
{
  return simd::neon::sqrt(v);
}
#endif

__attribute__((always_inline)) inline simd::f128
vrsqrt_approx(simd::f128 v) noexcept
{
  return simd::neon::rsqrt_est(v);
}
#endif

#if defined(__micron_arch_x86_any)
__attribute__((always_inline)) inline simd::f256
vsqrt(simd::f256 v) noexcept
{
  return simd::avx::sqrt_f32(v);
}

__attribute__((always_inline)) inline simd::d256
vsqrt(simd::d256 v) noexcept
{
  return simd::avx::sqrt_f64(v);
}

__attribute__((always_inline)) inline simd::f256
vrsqrt_approx(simd::f256 v) noexcept
{
  return simd::avx::rsqrt_f32(v);
}
#endif

#if defined(__micron_arch_x86_any) && defined(__micron_x86_avx512f)
__attribute__((always_inline)) inline simd::f512
vsqrt(simd::f512 v) noexcept
{
  return simd::avx512::sqrt_f32(v);
}

__attribute__((always_inline)) inline simd::d512
vsqrt(simd::d512 v) noexcept
{
  return simd::avx512::sqrt_f64(v);
}

__attribute__((always_inline)) inline simd::f512
vrsqrt_approx(simd::f512 v) noexcept
{
  return simd::avx512::rsqrt14_f32(v);
}
#endif

};      // namespace math
};      // namespace micron
