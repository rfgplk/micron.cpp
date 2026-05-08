//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__sse3.hpp included on a non-x86 build"
#endif

// freestanding replacement for SSE3 [pmmintrin.h]

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

__inline_g __m128
_mm_addsub_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_addsubps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128d
_mm_addsub_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_addsubpd((__v2df)a, (__v2df)b);
}

__inline_g __m128
_mm_hadd_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_haddps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_hsub_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_hsubps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128d
_mm_hadd_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_haddpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_hsub_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_hsubpd((__v2df)a, (__v2df)b);
}

__inline_g __m128
_mm_movehdup_ps(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_movshdup((__v4sf)a);
}

__inline_g __m128
_mm_moveldup_ps(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_movsldup((__v4sf)a);
}

__inline_g __m128d
_mm_movedup_pd(__m128d a) noexcept
{
  return __extension__(__m128d) __builtin_shufflevector((__v2df)a, (__v2df)a, 0, 0);
}

__inline_g __m128i
_mm_lddqu_si128(const __m128i *p) noexcept
{
  return (__m128i)__builtin_ia32_lddqu((const char *)p);
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_mm_addsub_ps);
__inject_i(_mm_addsub_pd);
__inject_i(_mm_hadd_ps);
__inject_i(_mm_hsub_ps);
__inject_i(_mm_hadd_pd);
__inject_i(_mm_hsub_pd);
__inject_i(_mm_movehdup_ps);
__inject_i(_mm_moveldup_ps);
__inject_i(_mm_movedup_pd);
__inject_i(_mm_lddqu_si128);
#undef __inject_i
#endif
