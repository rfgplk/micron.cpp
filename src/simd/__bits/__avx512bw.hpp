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
#error "__avx512bw.hpp included on a non-x86 build"
#endif

// freestanding AVX-512BW [avx512bwintrin.h]

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"
#pragma GCC diagnostic ignored "-Wpedantic"

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("avx512bw")]] static inline

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arith

__inline_g __m512i
_mm512_add_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v64qu)a + (__v64qu)b);
}

__inline_g __m512i
_mm512_add_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v32hu)a + (__v32hu)b);
}

__inline_g __m512i
_mm512_sub_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v64qu)a - (__v64qu)b);
}

__inline_g __m512i
_mm512_sub_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v32hu)a - (__v32hu)b);
}

__inline_g __m512i
_mm512_mullo_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v32hu)a * (__v32hu)b);
}

__inline_g __m512i
_mm512_mulhi_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmulhw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_mulhi_epu16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmulhuw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_madd_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaddwd512_mask((__v32hi)a, (__v32hi)b, (__v16si){ 0 }, (__mmask16)-1);
}

__inline_g __m512i
_mm512_adds_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_paddsb512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_adds_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_paddsw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_adds_epu8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_paddusb512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_adds_epu16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_paddusw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_subs_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_psubsb512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_subs_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_psubsw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_subs_epu8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_psubusb512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_subs_epu16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_psubusw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_avg_epu8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pavgb512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_avg_epu16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pavgw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_min_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminsb512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_max_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxsb512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_min_epu8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminub512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_max_epu8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxub512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_min_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminsw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_max_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxsw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_min_epu16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminuw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_max_epu16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxuw512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_abs_epi8(__m512i a) noexcept
{
  return (__m512i)__builtin_ia32_pabsb512_mask((__v64qi)a, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_abs_epi16(__m512i a) noexcept
{
  return (__m512i)__builtin_ia32_pabsw512_mask((__v32hi)a, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_sad_epu8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_psadbw512((__v64qi)a, (__v64qi)b);
}

__inline_g __m512i
_mm512_packs_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_packsswb512_mask((__v32hi)a, (__v32hi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_packs_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_packssdw512_mask((__v16si)a, (__v16si)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_packus_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_packuswb512_mask((__v32hi)a, (__v32hi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_packus_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_packusdw512_mask((__v16si)a, (__v16si)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_unpackhi_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_punpckhbw512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_unpacklo_epi8(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_punpcklbw512_mask((__v64qi)a, (__v64qi)b, (__v64qi){ 0 }, (__mmask64)-1);
}

__inline_g __m512i
_mm512_unpackhi_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_punpckhwd512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_unpacklo_epi16(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_punpcklwd512_mask((__v32hi)a, (__v32hi)b, (__v32hi){ 0 }, (__mmask32)-1);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shifts  & compares

__inline_g __m512i
_mm512_slli_epi16(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psllwi512_mask((__v32hi)a, n, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_srli_epi16(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psrlwi512_mask((__v32hi)a, n, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __m512i
_mm512_srai_epi16(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psrawi512_mask((__v32hi)a, n, (__v32hi){ 0 }, (__mmask32)-1);
}

__inline_g __mmask64
_mm512_cmpeq_epi8_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask64)__builtin_ia32_cmpb512_mask((__v64qi)a, (__v64qi)b, 0, (__mmask64)-1);
}

__inline_g __mmask64
_mm512_cmplt_epi8_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask64)__builtin_ia32_cmpb512_mask((__v64qi)a, (__v64qi)b, 1, (__mmask64)-1);
}

__inline_g __mmask64
_mm512_cmpgt_epi8_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask64)__builtin_ia32_cmpb512_mask((__v64qi)a, (__v64qi)b, 6, (__mmask64)-1);
}

__inline_g __mmask32
_mm512_cmpeq_epi16_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask32)__builtin_ia32_cmpw512_mask((__v32hi)a, (__v32hi)b, 0, (__mmask32)-1);
}

__inline_g __mmask32
_mm512_cmplt_epi16_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask32)__builtin_ia32_cmpw512_mask((__v32hi)a, (__v32hi)b, 1, (__mmask32)-1);
}

__inline_g __mmask32
_mm512_cmpgt_epi16_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask32)__builtin_ia32_cmpw512_mask((__v32hi)a, (__v32hi)b, 6, (__mmask32)-1);
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron
