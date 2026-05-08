//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__mask_types.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__avx512vl.hpp included on a non-x86 build"
#endif

// freestanding AVX-512VL [avx512vlintrin.h]
// NOTE: many missing as of now

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("avx512vl,avx512f")]] static inline

__inline_g __m128i
_mm_min_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminsq128_mask((__v2di)a, (__v2di)b, (__v2di){ 0, 0 }, (__mmask8)-1);
}

__inline_g __m128i
_mm_max_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxsq128_mask((__v2di)a, (__v2di)b, (__v2di){ 0, 0 }, (__mmask8)-1);
}

__inline_g __m128i
_mm_min_epu64(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminuq128_mask((__v2di)a, (__v2di)b, (__v2di){ 0, 0 }, (__mmask8)-1);
}

__inline_g __m128i
_mm_max_epu64(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxuq128_mask((__v2di)a, (__v2di)b, (__v2di){ 0, 0 }, (__mmask8)-1);
}

__inline_g __m256i
_mm256_min_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminsq256_mask((__v4di)a, (__v4di)b, (__v4di){ 0, 0, 0, 0 }, (__mmask8)-1);
}

__inline_g __m256i
_mm256_max_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxsq256_mask((__v4di)a, (__v4di)b, (__v4di){ 0, 0, 0, 0 }, (__mmask8)-1);
}

__inline_g __m256i
_mm256_min_epu64(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminuq256_mask((__v4di)a, (__v4di)b, (__v4di){ 0, 0, 0, 0 }, (__mmask8)-1);
}

__inline_g __m256i
_mm256_max_epu64(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxuq256_mask((__v4di)a, (__v4di)b, (__v4di){ 0, 0, 0, 0 }, (__mmask8)-1);
}

__inline_g __m128i
_mm_abs_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pabsq128_mask((__v2di)a, (__v2di){ 0, 0 }, (__mmask8)-1);
}

__inline_g __m256i
_mm256_abs_epi64(__m256i a) noexcept
{
  return (__m256i)__builtin_ia32_pabsq256_mask((__v4di)a, (__v4di){ 0, 0, 0, 0 }, (__mmask8)-1);
}

__inline_g __m128i
_mm_mullo_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v2du)a * (__v2du)b);
}

__inline_g __m256i
_mm256_mullo_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4du)a * (__v4du)b);
}

__inline_g __mmask8
_mm_cmpeq_epi32_mask(__m128i a, __m128i b) noexcept
{
  return (__mmask8)__builtin_ia32_cmpd128_mask((__v4si)a, (__v4si)b, 0, (__mmask8)-1);
}

__inline_g __mmask8
_mm_cmpgt_epi32_mask(__m128i a, __m128i b) noexcept
{
  return (__mmask8)__builtin_ia32_cmpd128_mask((__v4si)a, (__v4si)b, 6, (__mmask8)-1);
}

__inline_g __mmask8
_mm256_cmpeq_epi32_mask(__m256i a, __m256i b) noexcept
{
  return (__mmask8)__builtin_ia32_cmpd256_mask((__v8si)a, (__v8si)b, 0, (__mmask8)-1);
}

__inline_g __mmask8
_mm256_cmpgt_epi32_mask(__m256i a, __m256i b) noexcept
{
  return (__mmask8)__builtin_ia32_cmpd256_mask((__v8si)a, (__v8si)b, 6, (__mmask8)-1);
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron
