//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__ssse3.hpp included on a non-x86 build"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// freestanding replacement for SSSE3 [tmmintrin.h]

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

__inline_g __m128i
_mm_abs_epi8(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pabsb128((__v16qi)a);
}

__inline_g __m128i
_mm_abs_epi16(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pabsw128((__v8hi)a);
}

__inline_g __m128i
_mm_abs_epi32(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pabsd128((__v4si)a);
}

__inline_g __m128i
_mm_hadd_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_phaddw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_hadd_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_phaddd128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_hadds_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_phaddsw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_hsub_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_phsubw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_hsub_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_phsubd128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_hsubs_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_phsubsw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_maddubs_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaddubsw128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_mulhrs_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmulhrsw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_shuffle_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pshufb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_sign_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psignb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_sign_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psignw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_sign_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psignd128((__v4si)a, (__v4si)b);
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_mm_abs_epi8);
__inject_i(_mm_abs_epi16);
__inject_i(_mm_abs_epi32);
__inject_i(_mm_hadd_epi16);
__inject_i(_mm_hadd_epi32);
__inject_i(_mm_hadds_epi16);
__inject_i(_mm_hsub_epi16);
__inject_i(_mm_hsub_epi32);
__inject_i(_mm_hsubs_epi16);
__inject_i(_mm_maddubs_epi16);
__inject_i(_mm_mulhrs_epi16);
__inject_i(_mm_shuffle_epi8);
__inject_i(_mm_sign_epi8);
__inject_i(_mm_sign_epi16);
__inject_i(_mm_sign_epi32);
#undef __inject_i
#endif
