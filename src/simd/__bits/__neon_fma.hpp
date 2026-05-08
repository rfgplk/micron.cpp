//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

#if defined(__micron_arch_arm64)
#include "__vector_types_arm64.hpp"
#elif defined(__micron_arch_arm32)
#include "__vector_types_arm32.hpp"
#else
#error "__neon_fma.hpp included on a non-ARM build"
#endif

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

#if defined(__micron_compiler_gcc)

__inline_g float32x4_t
vfmaq_f32(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return __builtin_aarch64_fmav4sf(b, c, a);
}

__inline_g float32x2_t
vfma_f32(float32x2_t a, float32x2_t b, float32x2_t c) noexcept
{
  return __builtin_aarch64_fmav2sf(b, c, a);
}

__inline_g float32x4_t
vfmsq_f32(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return __builtin_aarch64_fmav4sf(-b, c, a);
}

__inline_g float32x2_t
vfms_f32(float32x2_t a, float32x2_t b, float32x2_t c) noexcept
{
  return __builtin_aarch64_fmav2sf(-b, c, a);
}

#if defined(__micron_arch_arm64)
__inline_g float64x2_t
vfmaq_f64(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return __builtin_aarch64_fmav2df(b, c, a);
}

__inline_g float64x1_t
vfma_f64(float64x1_t a, float64x1_t b, float64x1_t c) noexcept
{
  return __builtin_aarch64_fmadf(b[0], c[0], a[0]);
}

__inline_g float64x2_t
vfmsq_f64(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return __builtin_aarch64_fmav2df(-b, c, a);
}

__inline_g float64x1_t
vfms_f64(float64x1_t a, float64x1_t b, float64x1_t c) noexcept
{
  return __builtin_aarch64_fmadf(-b[0], c[0], a[0]);
}
#endif

#elif defined(__micron_compiler_clang)

__inline_g float32x4_t
vfmaq_f32(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return __builtin_neon_vfmaq_v(a, b, c, 41);
}

__inline_g float32x2_t
vfma_f32(float32x2_t a, float32x2_t b, float32x2_t c) noexcept
{
  return __builtin_neon_vfma_v(a, b, c, 9);
}

__inline_g float32x4_t
vfmsq_f32(float32x4_t a, float32x4_t b, float32x4_t c) noexcept
{
  return __builtin_neon_vfmaq_v(a, -b, c, 41);
}

__inline_g float32x2_t
vfms_f32(float32x2_t a, float32x2_t b, float32x2_t c) noexcept
{
  return __builtin_neon_vfma_v(a, -b, c, 9);
}

#if defined(__micron_arch_arm64)
__inline_g float64x2_t
vfmaq_f64(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return __builtin_neon_vfmaq_v(a, b, c, 42);
}

__inline_g float64x1_t
vfma_f64(float64x1_t a, float64x1_t b, float64x1_t c) noexcept
{
  return __builtin_neon_vfma_v(a, b, c, 10);
}

__inline_g float64x2_t
vfmsq_f64(float64x2_t a, float64x2_t b, float64x2_t c) noexcept
{
  return __builtin_neon_vfmaq_v(a, -b, c, 42);
}

__inline_g float64x1_t
vfms_f64(float64x1_t a, float64x1_t b, float64x1_t c) noexcept
{
  return __builtin_neon_vfma_v(a, -b, c, 10);
}
#endif

#endif

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron
