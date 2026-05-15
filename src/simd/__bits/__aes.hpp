//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__aes.hpp included on a non-x86 build"
#endif

// freestanding AES-NI + PCLMUL [wmmintrin.h]

#undef _mm_aesenc_si128
#undef _mm_aesenclast_si128
#undef _mm_aesdec_si128
#undef _mm_aesdeclast_si128
#undef _mm_aeskeygenassist_si128
#undef _mm_clmulepi64_si128

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("aes")]] static inline

__inline_g __m128i
_mm_aesimc_si128(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_aesimc128((__v2di)a);
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#define _mm_aesenc_si128(X, Y)                                                                                                             \
  ((::__m128i)__builtin_ia32_aesenc128((::micron::simd::__bits::__v2di)(::__m128i)(X), (::micron::simd::__bits::__v2di)(::__m128i)(Y)))
#define _mm_aesenclast_si128(X, Y)                                                                                                         \
  ((::__m128i)__builtin_ia32_aesenclast128((::micron::simd::__bits::__v2di)(::__m128i)(X), (::micron::simd::__bits::__v2di)(::__m128i)(Y)))
#define _mm_aesdec_si128(X, Y)                                                                                                             \
  ((::__m128i)__builtin_ia32_aesdec128((::micron::simd::__bits::__v2di)(::__m128i)(X), (::micron::simd::__bits::__v2di)(::__m128i)(Y)))
#define _mm_aesdeclast_si128(X, Y)                                                                                                         \
  ((::__m128i)__builtin_ia32_aesdeclast128((::micron::simd::__bits::__v2di)(::__m128i)(X), (::micron::simd::__bits::__v2di)(::__m128i)(Y)))
#define _mm_aeskeygenassist_si128(X, C)                                                                                                    \
  ((::__m128i)__builtin_ia32_aeskeygenassist128((::micron::simd::__bits::__v2di)(::__m128i)(X), (int)(C)))

#define _mm_clmulepi64_si128(X, Y, I)                                                                                                      \
  ((::__m128i)__builtin_ia32_pclmulqdq128((::micron::simd::__bits::__v2di)(::__m128i)(X), (::micron::simd::__bits::__v2di)(::__m128i)(Y),  \
                                          (int)(I)))

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_mm_aesimc_si128);
#undef __inject_i
#endif
