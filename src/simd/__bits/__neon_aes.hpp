//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

#if !defined(__micron_arch_arm64) && !defined(__micron_arch_arm32)
#error "__neon_aes.hpp included on a non-ARM build"
#endif

#if !defined(__ARM_FEATURE_AES) && !defined(__ARM_FEATURE_CRYPTO) && !defined(__micron_arm_aes)
#else

#include "__vector_types_arm64.hpp"

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

#if defined(__micron_compiler_gcc)
__inline_g uint8x16_t
vaeseq_u8(uint8x16_t d, uint8x16_t k) noexcept
{
  return (uint8x16_t)__builtin_aarch64_crypto_aesev16qi_uuu(d, k);
}

__inline_g uint8x16_t
vaesdq_u8(uint8x16_t d, uint8x16_t k) noexcept
{
  return (uint8x16_t)__builtin_aarch64_crypto_aesdv16qi_uuu(d, k);
}

__inline_g uint8x16_t
vaesmcq_u8(uint8x16_t d) noexcept
{
  return (uint8x16_t)__builtin_aarch64_crypto_aesmcv16qi_uu(d);
}

__inline_g uint8x16_t
vaesimcq_u8(uint8x16_t d) noexcept
{
  return (uint8x16_t)__builtin_aarch64_crypto_aesimcv16qi_uu(d);
}
#elif defined(__micron_compiler_clang)
__inline_g uint8x16_t
vaeseq_u8(uint8x16_t d, uint8x16_t k) noexcept
{
  return __builtin_neon_vaeseq_v(d, k, 48);
}

__inline_g uint8x16_t
vaesdq_u8(uint8x16_t d, uint8x16_t k) noexcept
{
  return __builtin_neon_vaesdq_v(d, k, 48);
}

__inline_g uint8x16_t
vaesmcq_u8(uint8x16_t d) noexcept
{
  return __builtin_neon_vaesmcq_v(d, 48);
}

__inline_g uint8x16_t
vaesimcq_u8(uint8x16_t d) noexcept
{
  return __builtin_neon_vaesimcq_v(d, 48);
}
#endif

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron

#endif     // __ARM_FEATURE_AES
