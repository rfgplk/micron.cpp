//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

#if !defined(__micron_arch_arm64) && !defined(__micron_arch_arm32)
#error "__neon_pmull.hpp included on a non-ARM build"
#endif

#if ( defined(__micron_arch_arm64) || defined(__micron_arch_arm32) )                                                                       \
    && (defined(__ARM_FEATURE_PMULL) || defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_pmull) || defined(__micron_arm_crypto))

#if defined(__micron_arch_arm64)
#include "__vector_types_arm64.hpp"
#else
#include "__neon_arm32.hpp"
#include "__vector_types_arm32.hpp"
#endif

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_pm [[gnu::always_inline, gnu::artificial]] static inline

#if defined(__micron_arch_arm64)

__inline_pm uint8x16_t
vpmull_lo_p64(uint8x16_t a, uint8x16_t b) noexcept
{
  uint8x16_t r;
  __asm__("pmull %0.1q, %1.1d, %2.1d" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

__inline_pm uint8x16_t
vpmull_hi_p64(uint8x16_t a, uint8x16_t b) noexcept
{
  uint8x16_t r;
  __asm__("pmull2 %0.1q, %1.2d, %2.2d" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

#else

__inline_pm uint8x16_t
vpmull_lo_p64(uint8x16_t a, uint8x16_t b) noexcept
{
  return vmull_p64((poly64x1_t)vget_low_u8(a), (poly64x1_t)vget_low_u8(b));
}

__inline_pm uint8x16_t
vpmull_hi_p64(uint8x16_t a, uint8x16_t b) noexcept
{
  return vmull_p64((poly64x1_t)vget_high_u8(a), (poly64x1_t)vget_high_u8(b));
}

#endif

#undef __inline_pm

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#endif
