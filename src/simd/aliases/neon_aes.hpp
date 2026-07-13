//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_arm64) && !defined(__micron_arch_arm32)
#error "neon_aes.hpp included on a non-ARM build"
#endif

namespace micron
{
namespace simd
{
namespace neon_crypto
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_nc [[gnu::always_inline, gnu::artificial]] static inline

__inline_nc uint8x16_t
load_u8q(const u8 *p) noexcept
{
  return __bits::vld1q_u8(reinterpret_cast<const unsigned char *>(p));
}

__inline_nc void
store_u8q(u8 *p, uint8x16_t v) noexcept
{
  __bits::vst1q_u8(reinterpret_cast<unsigned char *>(p), v);
}

__inline_nc uint8x16_t
xor_q(uint8x16_t a, uint8x16_t b) noexcept
{
  return __bits::veorq_u8(a, b);
}

__inline_nc uint8x16_t
rev_bytes_q(uint8x16_t v) noexcept
{
  const uint8x16_t r = __bits::vrev64q_u8(v);
  return __bits::vextq_u8(r, r, 8);
}

#if ( defined(__micron_arch_arm64) || defined(__micron_arch_arm32) )                                                                       \
    && (defined(__ARM_FEATURE_PMULL) || defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_pmull) || defined(__micron_arm_crypto))

__inline_nc uint8x16_t
clmul_lo(uint8x16_t a, uint8x16_t b) noexcept
{
  return __bits::vpmull_lo_p64(a, b);
}

__inline_nc uint8x16_t
clmul_hi(uint8x16_t a, uint8x16_t b) noexcept
{
  return __bits::vpmull_hi_p64(a, b);
}

#define __micron_neon_clmul 1

#endif

#if defined(__ARM_FEATURE_AES) || defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_aes)

__inline_nc uint8x16_t
enc_round(uint8x16_t state, uint8x16_t key) noexcept
{
  return __bits::vaeseq_u8(state, key);
}

__inline_nc uint8x16_t
dec_round(uint8x16_t state, uint8x16_t key) noexcept
{
  return __bits::vaesdq_u8(state, key);
}

__inline_nc uint8x16_t
mix_columns(uint8x16_t state) noexcept
{
  return __bits::vaesmcq_u8(state);
}

__inline_nc uint8x16_t
inv_mix_columns(uint8x16_t state) noexcept
{
  return __bits::vaesimcq_u8(state);
}

#define __micron_neon_aes 1

#endif

#undef __inline_nc

#pragma GCC diagnostic pop

};      // namespace neon_crypto
};      // namespace simd
};      // namespace micron
