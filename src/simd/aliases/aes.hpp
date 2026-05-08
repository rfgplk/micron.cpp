//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_x86_any)
#error "aes.hpp included on a non-x86 build"
#endif

namespace micron
{
namespace simd
{
namespace aes
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_aes [[gnu::always_inline, gnu::artificial, gnu::target("aes")]] static inline

__inline_aes __m128i
enc_round(__m128i state, __m128i key) noexcept
{
  return _mm_aesenc_si128(state, key);
}

__inline_aes __m128i
enc_round_last(__m128i state, __m128i key) noexcept
{
  return _mm_aesenclast_si128(state, key);
}

__inline_aes __m128i
dec_round(__m128i state, __m128i key) noexcept
{
  return _mm_aesdec_si128(state, key);
}

__inline_aes __m128i
dec_round_last(__m128i state, __m128i key) noexcept
{
  return _mm_aesdeclast_si128(state, key);
}

__inline_aes __m128i
inv_mix_columns(__m128i a) noexcept
{
  return _mm_aesimc_si128(a);
}

template <int RCON>
__inline_aes __m128i
keygen_assist(__m128i a) noexcept
{
  return _mm_aeskeygenassist_si128(a, RCON);
}

#undef __inline_aes

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_pcl [[gnu::always_inline, gnu::artificial, gnu::target("pclmul")]] static inline

template <int IMM>
__inline_pcl __m128i
clmul_64(__m128i a, __m128i b) noexcept
{
  return _mm_clmulepi64_si128(a, b, IMM);
}

#undef __inline_pcl

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

};     // namespace aes
};     // namespace simd
};     // namespace micron
