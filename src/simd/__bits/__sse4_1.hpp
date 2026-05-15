//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__sse4_1.hpp included on a non-x86 build"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// freestanding SSE4.1 [smmintrin.h]

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
_mm_min_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminsb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_max_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxsb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_min_epu16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminuw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_max_epu16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxuw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_min_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminsd128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_max_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxsd128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_min_epu32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminud128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_max_epu32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxud128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_mullo_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v4su)a * (__v4su)b);
}

__inline_g __m128i
_mm_mul_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmuldq128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_packus_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_packusdw128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_cmpeq_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v2di)a == (__v2di)b);
}

__inline_g __m128i
_mm_cvtepi8_epi16(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovsxbw128((__v16qi)a);
}

__inline_g __m128i
_mm_cvtepi8_epi32(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovsxbd128((__v16qi)a);
}

__inline_g __m128i
_mm_cvtepi8_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovsxbq128((__v16qi)a);
}

__inline_g __m128i
_mm_cvtepi16_epi32(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovsxwd128((__v8hi)a);
}

__inline_g __m128i
_mm_cvtepi16_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovsxwq128((__v8hi)a);
}

__inline_g __m128i
_mm_cvtepi32_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovsxdq128((__v4si)a);
}

__inline_g __m128i
_mm_cvtepu8_epi16(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovzxbw128((__v16qi)a);
}

__inline_g __m128i
_mm_cvtepu8_epi32(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovzxbd128((__v16qi)a);
}

__inline_g __m128i
_mm_cvtepu8_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovzxbq128((__v16qi)a);
}

__inline_g __m128i
_mm_cvtepu16_epi32(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovzxwd128((__v8hi)a);
}

__inline_g __m128i
_mm_cvtepu16_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovzxwq128((__v8hi)a);
}

__inline_g __m128i
_mm_cvtepu32_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pmovzxdq128((__v4si)a);
}

__inline_g __m128i
_mm_stream_load_si128(const __m128i *p) noexcept
{
  return (__m128i)__builtin_ia32_movntdqa(const_cast<__v2di *>((const __v2di *)p));
}

__inline_g int
_mm_testz_si128(__m128i a, __m128i b) noexcept
{
  return __builtin_ia32_ptestz128((__v2di)a, (__v2di)b);
}

__inline_g int
_mm_testc_si128(__m128i a, __m128i b) noexcept
{
  return __builtin_ia32_ptestc128((__v2di)a, (__v2di)b);
}

__inline_g int
_mm_testnzc_si128(__m128i a, __m128i b) noexcept
{
  return __builtin_ia32_ptestnzc128((__v2di)a, (__v2di)b);
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_mm_min_epi8);
__inject_i(_mm_max_epi8);
__inject_i(_mm_min_epu16);
__inject_i(_mm_max_epu16);
__inject_i(_mm_min_epi32);
__inject_i(_mm_max_epi32);
__inject_i(_mm_min_epu32);
__inject_i(_mm_max_epu32);
__inject_i(_mm_mullo_epi32);
__inject_i(_mm_mul_epi32);
__inject_i(_mm_packus_epi32);
__inject_i(_mm_cmpeq_epi64);
__inject_i(_mm_cvtepi8_epi16);
__inject_i(_mm_cvtepi8_epi32);
__inject_i(_mm_cvtepi8_epi64);
__inject_i(_mm_cvtepi16_epi32);
__inject_i(_mm_cvtepi16_epi64);
__inject_i(_mm_cvtepi32_epi64);
__inject_i(_mm_cvtepu8_epi16);
__inject_i(_mm_cvtepu8_epi32);
__inject_i(_mm_cvtepu8_epi64);
__inject_i(_mm_cvtepu16_epi32);
__inject_i(_mm_cvtepu16_epi64);
__inject_i(_mm_cvtepu32_epi64);
__inject_i(_mm_stream_load_si128);
__inject_i(_mm_testz_si128);
__inject_i(_mm_testc_si128);
__inject_i(_mm_testnzc_si128);
#undef __inject_i
#endif
