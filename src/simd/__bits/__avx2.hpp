//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__avx2.hpp included on a non-x86 build"
#endif

// freestanding AVX2 [avx2intrin.h]

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

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arith

__inline_g __m256i
_mm256_add_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v32qu)a + (__v32qu)b);
}

__inline_g __m256i
_mm256_add_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v16hu)a + (__v16hu)b);
}

__inline_g __m256i
_mm256_add_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v8su)a + (__v8su)b);
}

__inline_g __m256i
_mm256_add_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4du)a + (__v4du)b);
}

__inline_g __m256i
_mm256_sub_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v32qu)a - (__v32qu)b);
}

__inline_g __m256i
_mm256_sub_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v16hu)a - (__v16hu)b);
}

__inline_g __m256i
_mm256_sub_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v8su)a - (__v8su)b);
}

__inline_g __m256i
_mm256_sub_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4du)a - (__v4du)b);
}

__inline_g __m256i
_mm256_adds_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_paddsb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_adds_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_paddsw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_adds_epu8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_paddusb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_adds_epu16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_paddusw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_subs_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psubsb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_subs_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psubsw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_subs_epu8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psubusb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_subs_epu16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psubusw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_mullo_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v16hu)a * (__v16hu)b);
}

__inline_g __m256i
_mm256_mullo_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v8su)a * (__v8su)b);
}

__inline_g __m256i
_mm256_mulhi_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmulhw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_mulhi_epu16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmulhuw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_mul_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmuldq256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_mul_epu32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmuludq256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_madd_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaddwd256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_maddubs_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaddubsw256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_mulhrs_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmulhrsw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_avg_epu8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pavgb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_avg_epu16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pavgw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_min_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminsb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_max_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxsb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_min_epu8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminub256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_max_epu8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxub256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_min_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminsw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_max_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxsw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_min_epu16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminuw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_max_epu16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxuw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_min_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminsd256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_max_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxsd256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_min_epu32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pminud256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_max_epu32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_pmaxud256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_sad_epu8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psadbw256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_abs_epi8(__m256i a) noexcept
{
  return (__m256i)__builtin_ia32_pabsb256((__v32qi)a);
}

__inline_g __m256i
_mm256_abs_epi16(__m256i a) noexcept
{
  return (__m256i)__builtin_ia32_pabsw256((__v16hi)a);
}

__inline_g __m256i
_mm256_abs_epi32(__m256i a) noexcept
{
  return (__m256i)__builtin_ia32_pabsd256((__v8si)a);
}

__inline_g __m256i
_mm256_sign_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psignb256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_sign_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psignw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_sign_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_psignd256((__v8si)a, (__v8si)b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// horizontal arith

__inline_g __m256i
_mm256_hadd_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_phaddw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_hadd_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_phaddd256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_hadds_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_phaddsw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_hsub_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_phsubw256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_hsub_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_phsubd256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_hsubs_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_phsubsw256((__v16hi)a, (__v16hi)b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise

__inline_g __m256i
_mm256_and_si256(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4du)a & (__v4du)b);
}

__inline_g __m256i
_mm256_andnot_si256(__m256i a, __m256i b) noexcept
{
  return (__m256i)(~(__v4du)a & (__v4du)b);
}

__inline_g __m256i
_mm256_or_si256(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4du)a | (__v4du)b);
}

__inline_g __m256i
_mm256_xor_si256(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4du)a ^ (__v4du)b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compares
__inline_g __m256i
_mm256_cmpeq_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v32qi)a == (__v32qi)b);
}

__inline_g __m256i
_mm256_cmpeq_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v16hi)a == (__v16hi)b);
}

__inline_g __m256i
_mm256_cmpeq_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v8si)a == (__v8si)b);
}

__inline_g __m256i
_mm256_cmpeq_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4di)a == (__v4di)b);
}

__inline_g __m256i
_mm256_cmpgt_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v32qi)a > (__v32qi)b);
}

__inline_g __m256i
_mm256_cmpgt_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v16hi)a > (__v16hi)b);
}

__inline_g __m256i
_mm256_cmpgt_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v8si)a > (__v8si)b);
}

__inline_g __m256i
_mm256_cmpgt_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)((__v4di)a > (__v4di)b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shifts

__inline_g __m256i
_mm256_slli_epi16(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_psllwi256((__v16hi)a, c);
}

__inline_g __m256i
_mm256_slli_epi32(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_pslldi256((__v8si)a, c);
}

__inline_g __m256i
_mm256_slli_epi64(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_psllqi256((__v4di)a, c);
}

__inline_g __m256i
_mm256_srli_epi16(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_psrlwi256((__v16hi)a, c);
}

__inline_g __m256i
_mm256_srli_epi32(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_psrldi256((__v8si)a, c);
}

__inline_g __m256i
_mm256_srli_epi64(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_psrlqi256((__v4di)a, c);
}

__inline_g __m256i
_mm256_srai_epi16(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_psrawi256((__v16hi)a, c);
}

__inline_g __m256i
_mm256_srai_epi32(__m256i a, int c) noexcept
{
  return (__m256i)__builtin_ia32_psradi256((__v8si)a, c);
}

__inline_g __m256i
_mm256_sll_epi16(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_psllw256((__v16hi)a, (__v8hi)c);
}

__inline_g __m256i
_mm256_sll_epi32(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_pslld256((__v8si)a, (__v4si)c);
}

__inline_g __m256i
_mm256_sll_epi64(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_psllq256((__v4di)a, (__v2di)c);
}

__inline_g __m256i
_mm256_srl_epi16(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_psrlw256((__v16hi)a, (__v8hi)c);
}

__inline_g __m256i
_mm256_srl_epi32(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_psrld256((__v8si)a, (__v4si)c);
}

__inline_g __m256i
_mm256_srl_epi64(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_psrlq256((__v4di)a, (__v2di)c);
}

__inline_g __m256i
_mm256_sra_epi16(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_psraw256((__v16hi)a, (__v8hi)c);
}

__inline_g __m256i
_mm256_sra_epi32(__m256i a, __m128i c) noexcept
{
  return (__m256i)__builtin_ia32_psrad256((__v8si)a, (__v4si)c);
}

__inline_g __m128i
_mm_sllv_epi32(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psllv4si((__v4si)a, (__v4si)c);
}

__inline_g __m128i
_mm_sllv_epi64(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psllv2di((__v2di)a, (__v2di)c);
}

__inline_g __m128i
_mm_srlv_epi32(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psrlv4si((__v4si)a, (__v4si)c);
}

__inline_g __m128i
_mm_srlv_epi64(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psrlv2di((__v2di)a, (__v2di)c);
}

__inline_g __m128i
_mm_srav_epi32(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psrav4si((__v4si)a, (__v4si)c);
}

__inline_g __m256i
_mm256_sllv_epi32(__m256i a, __m256i c) noexcept
{
  return (__m256i)__builtin_ia32_psllv8si((__v8si)a, (__v8si)c);
}

__inline_g __m256i
_mm256_sllv_epi64(__m256i a, __m256i c) noexcept
{
  return (__m256i)__builtin_ia32_psllv4di((__v4di)a, (__v4di)c);
}

__inline_g __m256i
_mm256_srlv_epi32(__m256i a, __m256i c) noexcept
{
  return (__m256i)__builtin_ia32_psrlv8si((__v8si)a, (__v8si)c);
}

__inline_g __m256i
_mm256_srlv_epi64(__m256i a, __m256i c) noexcept
{
  return (__m256i)__builtin_ia32_psrlv4di((__v4di)a, (__v4di)c);
}

__inline_g __m256i
_mm256_srav_epi32(__m256i a, __m256i c) noexcept
{
  return (__m256i)__builtin_ia32_psrav8si((__v8si)a, (__v8si)c);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// packs & unpacks

__inline_g __m256i
_mm256_packs_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_packsswb256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_packs_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_packssdw256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_packus_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_packuswb256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_packus_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_packusdw256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_unpackhi_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpckhbw256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_unpackhi_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpckhwd256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_unpackhi_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpckhdq256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_unpackhi_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpckhqdq256((__v4di)a, (__v4di)b);
}

__inline_g __m256i
_mm256_unpacklo_epi8(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpcklbw256((__v32qi)a, (__v32qi)b);
}

__inline_g __m256i
_mm256_unpacklo_epi16(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpcklwd256((__v16hi)a, (__v16hi)b);
}

__inline_g __m256i
_mm256_unpacklo_epi32(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpckldq256((__v8si)a, (__v8si)b);
}

__inline_g __m256i
_mm256_unpacklo_epi64(__m256i a, __m256i b) noexcept
{
  return (__m256i)__builtin_ia32_punpcklqdq256((__v4di)a, (__v4di)b);
}

__inline_g __m256
_mm256_unpackhi_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_unpckhps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_unpacklo_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_unpcklps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256d
_mm256_unpackhi_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_unpckhpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_unpacklo_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_unpcklpd256((__v4df)a, (__v4df)b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// movemasks & bits

__inline_g int
_mm256_movemask_epi8(__m256i a) noexcept
{
  return __builtin_ia32_pmovmskb256((__v32qi)a);
}

__inline_g __m256i
_mm256_cvtepi8_epi16(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovsxbw256((__v16qi)a);
}

__inline_g __m256i
_mm256_cvtepi8_epi32(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovsxbd256((__v16qi)a);
}

__inline_g __m256i
_mm256_cvtepi8_epi64(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovsxbq256((__v16qi)a);
}

__inline_g __m256i
_mm256_cvtepi16_epi32(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovsxwd256((__v8hi)a);
}

__inline_g __m256i
_mm256_cvtepi16_epi64(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovsxwq256((__v8hi)a);
}

__inline_g __m256i
_mm256_cvtepi32_epi64(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovsxdq256((__v4si)a);
}

__inline_g __m256i
_mm256_cvtepu8_epi16(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovzxbw256((__v16qi)a);
}

__inline_g __m256i
_mm256_cvtepu8_epi32(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovzxbd256((__v16qi)a);
}

__inline_g __m256i
_mm256_cvtepu8_epi64(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovzxbq256((__v16qi)a);
}

__inline_g __m256i
_mm256_cvtepu16_epi32(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovzxwd256((__v8hi)a);
}

__inline_g __m256i
_mm256_cvtepu16_epi64(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovzxwq256((__v8hi)a);
}

__inline_g __m256i
_mm256_cvtepu32_epi64(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pmovzxdq256((__v4si)a);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// broadcasts

__inline_g __m256i
_mm256_broadcastsi128_si256(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_vbroadcastsi256((__v2di)a);
}

__inline_g __m256i
_mm256_broadcastb_epi8(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pbroadcastb256((__v16qi)a);
}

__inline_g __m256i
_mm256_broadcastw_epi16(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pbroadcastw256((__v8hi)a);
}

__inline_g __m256i
_mm256_broadcastd_epi32(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pbroadcastd256((__v4si)a);
}

__inline_g __m256i
_mm256_broadcastq_epi64(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_pbroadcastq256((__v2di)a);
}

__inline_g __m256
_mm256_broadcastss_ps(__m128 a) noexcept
{
  return (__m256)__builtin_ia32_vbroadcastss_ps256((__v4sf)a);
}

__inline_g __m256d
_mm256_broadcastsd_pd(__m128d a) noexcept
{
  return (__m256d)__builtin_ia32_vbroadcastsd_pd256((__v2df)a);
}

__inline_g __m128i
_mm_broadcastb_epi8(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pbroadcastb128((__v16qi)a);
}

__inline_g __m128i
_mm_broadcastw_epi16(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pbroadcastw128((__v8hi)a);
}

__inline_g __m128i
_mm_broadcastd_epi32(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pbroadcastd128((__v4si)a);
}

__inline_g __m128i
_mm_broadcastq_epi64(__m128i a) noexcept
{
  return (__m128i)__builtin_ia32_pbroadcastq128((__v2di)a);
}

__inline_g __m128
_mm_broadcastss_ps(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_vbroadcastss_ps((__v4sf)a);
}

__inline_g __m256i
_mm256_stream_load_si256(const __m256i *p) noexcept
{
  return (__m256i)__builtin_ia32_movntdqa256(const_cast<__v4di *>((const __v4di *)p));
}

#undef __inline_g

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#define _mm_i32gather_ps(BASE, INDEX, SCALE)                                                                                               \
  ((::__m128)__builtin_ia32_gathersiv4sf(                                                                                                  \
      (::micron::simd::__bits::__v4sf){ 0, 0, 0, 0 }, (float const *)(BASE), (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),           \
      (::micron::simd::__bits::__v4sf)(::micron::simd::__bits::__v4si){ -1, -1, -1, -1 }, (int)(SCALE)))
#define _mm256_i32gather_ps(BASE, INDEX, SCALE)                                                                                            \
  ((::__m256)__builtin_ia32_gathersiv8sf(                                                                                                  \
      (::micron::simd::__bits::__v8sf){ 0, 0, 0, 0, 0, 0, 0, 0 }, (float const *)(BASE),                                                   \
      (::micron::simd::__bits::__v8si)(::__m256i)(INDEX),                                                                                  \
      (::micron::simd::__bits::__v8sf)(::micron::simd::__bits::__v8si){ -1, -1, -1, -1, -1, -1, -1, -1 }, (int)(SCALE)))
#define _mm_i32gather_pd(BASE, INDEX, SCALE)                                                                                               \
  ((::__m128d)__builtin_ia32_gathersiv2df((::micron::simd::__bits::__v2df){ 0, 0 }, (double const *)(BASE),                                \
                                          (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                                              \
                                          (::micron::simd::__bits::__v2df)(::micron::simd::__bits::__v2di){ -1, -1 }, (int)(SCALE)))
#define _mm256_i32gather_pd(BASE, INDEX, SCALE)                                                                                            \
  ((::__m256d)__builtin_ia32_gathersiv4df(                                                                                                 \
      (::micron::simd::__bits::__v4df){ 0, 0, 0, 0 }, (double const *)(BASE), (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),          \
      (::micron::simd::__bits::__v4df)(::micron::simd::__bits::__v4di){ -1, -1, -1, -1 }, (int)(SCALE)))
#define _mm_i32gather_epi32(BASE, INDEX, SCALE)                                                                                            \
  ((::__m128i)__builtin_ia32_gathersiv4si((::micron::simd::__bits::__v4si)::micron::simd::__bits::_mm_setzero_si128(),                     \
                                          (int const *)(BASE), (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                         \
                                          (::micron::simd::__bits::__v4si)::micron::simd::__bits::_mm_set1_epi32(-1), (int)(SCALE)))
#define _mm256_i32gather_epi32(BASE, INDEX, SCALE)                                                                                         \
  ((::__m256i)__builtin_ia32_gathersiv8si((::micron::simd::__bits::__v8si)::micron::simd::__bits::_mm256_setzero_si256(),                  \
                                          (int const *)(BASE), (::micron::simd::__bits::__v8si)(::__m256i)(INDEX),                         \
                                          (::micron::simd::__bits::__v8si)::micron::simd::__bits::_mm256_set1_epi32(-1), (int)(SCALE)))
#define _mm_i32gather_epi64(BASE, INDEX, SCALE)                                                                                            \
  ((::__m128i)__builtin_ia32_gathersiv2di((::micron::simd::__bits::__v2di)::micron::simd::__bits::_mm_setzero_si128(),                     \
                                          (long long const *)(BASE), (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                   \
                                          (::micron::simd::__bits::__v2di)::micron::simd::__bits::_mm_set1_epi64x(-1), (int)(SCALE)))
#define _mm256_i32gather_epi64(BASE, INDEX, SCALE)                                                                                         \
  ((::__m256i)__builtin_ia32_gathersiv4di((::micron::simd::__bits::__v4di)::micron::simd::__bits::_mm256_setzero_si256(),                  \
                                          (long long const *)(BASE), (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                   \
                                          (::micron::simd::__bits::__v4di)::micron::simd::__bits::_mm256_set1_epi64x(-1), (int)(SCALE)))

#define _mm_mask_i32gather_ps(SRC, BASE, INDEX, MASK, SCALE)                                                                               \
  ((::__m128)__builtin_ia32_gathersiv4sf((::micron::simd::__bits::__v4sf)(::__m128)(SRC), (float const *)(BASE),                           \
                                         (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                                               \
                                         (::micron::simd::__bits::__v4sf)(::__m128)(MASK), (int)(SCALE)))
#define _mm256_mask_i32gather_ps(SRC, BASE, INDEX, MASK, SCALE)                                                                            \
  ((::__m256)__builtin_ia32_gathersiv8sf((::micron::simd::__bits::__v8sf)(::__m256)(SRC), (float const *)(BASE),                           \
                                         (::micron::simd::__bits::__v8si)(::__m256i)(INDEX),                                               \
                                         (::micron::simd::__bits::__v8sf)(::__m256)(MASK), (int)(SCALE)))
#define _mm_mask_i32gather_pd(SRC, BASE, INDEX, MASK, SCALE)                                                                               \
  ((::__m128d)__builtin_ia32_gathersiv2df((::micron::simd::__bits::__v2df)(::__m128d)(SRC), (double const *)(BASE),                        \
                                          (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                                              \
                                          (::micron::simd::__bits::__v2df)(::__m128d)(MASK), (int)(SCALE)))
#define _mm256_mask_i32gather_pd(SRC, BASE, INDEX, MASK, SCALE)                                                                            \
  ((::__m256d)__builtin_ia32_gathersiv4df((::micron::simd::__bits::__v4df)(::__m256d)(SRC), (double const *)(BASE),                        \
                                          (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                                              \
                                          (::micron::simd::__bits::__v4df)(::__m256d)(MASK), (int)(SCALE)))
#define _mm_mask_i32gather_epi32(SRC, BASE, INDEX, MASK, SCALE)                                                                            \
  ((::__m128i)__builtin_ia32_gathersiv4si((::micron::simd::__bits::__v4si)(::__m128i)(SRC), (int const *)(BASE),                           \
                                          (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                                              \
                                          (::micron::simd::__bits::__v4si)(::__m128i)(MASK), (int)(SCALE)))
#define _mm256_mask_i32gather_epi32(SRC, BASE, INDEX, MASK, SCALE)                                                                         \
  ((::__m256i)__builtin_ia32_gathersiv8si((::micron::simd::__bits::__v8si)(::__m256i)(SRC), (int const *)(BASE),                           \
                                          (::micron::simd::__bits::__v8si)(::__m256i)(INDEX),                                              \
                                          (::micron::simd::__bits::__v8si)(::__m256i)(MASK), (int)(SCALE)))
#define _mm_mask_i32gather_epi64(SRC, BASE, INDEX, MASK, SCALE)                                                                            \
  ((::__m128i)__builtin_ia32_gathersiv2di((::micron::simd::__bits::__v2di)(::__m128i)(SRC), (long long const *)(BASE),                     \
                                          (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                                              \
                                          (::micron::simd::__bits::__v2di)(::__m128i)(MASK), (int)(SCALE)))
#define _mm256_mask_i32gather_epi64(SRC, BASE, INDEX, MASK, SCALE)                                                                         \
  ((::__m256i)__builtin_ia32_gathersiv4di((::micron::simd::__bits::__v4di)(::__m256i)(SRC), (long long const *)(BASE),                     \
                                          (::micron::simd::__bits::__v4si)(::__m128i)(INDEX),                                              \
                                          (::micron::simd::__bits::__v4di)(::__m256i)(MASK), (int)(SCALE)))

#pragma GCC diagnostic pop

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name

__inject_i(_mm256_add_epi8);
__inject_i(_mm256_add_epi16);
__inject_i(_mm256_add_epi32);
__inject_i(_mm256_add_epi64);
__inject_i(_mm256_sub_epi8);
__inject_i(_mm256_sub_epi16);
__inject_i(_mm256_sub_epi32);
__inject_i(_mm256_sub_epi64);
__inject_i(_mm256_adds_epi8);
__inject_i(_mm256_adds_epi16);
__inject_i(_mm256_adds_epu8);
__inject_i(_mm256_adds_epu16);
__inject_i(_mm256_subs_epi8);
__inject_i(_mm256_subs_epi16);
__inject_i(_mm256_subs_epu8);
__inject_i(_mm256_subs_epu16);
__inject_i(_mm256_mullo_epi16);
__inject_i(_mm256_mullo_epi32);
__inject_i(_mm256_mulhi_epi16);
__inject_i(_mm256_mulhi_epu16);
__inject_i(_mm256_mul_epi32);
__inject_i(_mm256_mul_epu32);
__inject_i(_mm256_madd_epi16);
__inject_i(_mm256_maddubs_epi16);
__inject_i(_mm256_mulhrs_epi16);
__inject_i(_mm256_avg_epu8);
__inject_i(_mm256_avg_epu16);
__inject_i(_mm256_min_epi8);
__inject_i(_mm256_max_epi8);
__inject_i(_mm256_min_epu8);
__inject_i(_mm256_max_epu8);
__inject_i(_mm256_min_epi16);
__inject_i(_mm256_max_epi16);
__inject_i(_mm256_min_epu16);
__inject_i(_mm256_max_epu16);
__inject_i(_mm256_min_epi32);
__inject_i(_mm256_max_epi32);
__inject_i(_mm256_min_epu32);
__inject_i(_mm256_max_epu32);
__inject_i(_mm256_sad_epu8);
__inject_i(_mm256_abs_epi8);
__inject_i(_mm256_abs_epi16);
__inject_i(_mm256_abs_epi32);
__inject_i(_mm256_sign_epi8);
__inject_i(_mm256_sign_epi16);
__inject_i(_mm256_sign_epi32);
__inject_i(_mm256_hadd_epi16);
__inject_i(_mm256_hadd_epi32);
__inject_i(_mm256_hadds_epi16);
__inject_i(_mm256_hsub_epi16);
__inject_i(_mm256_hsub_epi32);
__inject_i(_mm256_hsubs_epi16);
__inject_i(_mm256_and_si256);
__inject_i(_mm256_andnot_si256);
__inject_i(_mm256_or_si256);
__inject_i(_mm256_xor_si256);
__inject_i(_mm256_cmpeq_epi8);
__inject_i(_mm256_cmpeq_epi16);
__inject_i(_mm256_cmpeq_epi32);
__inject_i(_mm256_cmpeq_epi64);
__inject_i(_mm256_cmpgt_epi8);
__inject_i(_mm256_cmpgt_epi16);
__inject_i(_mm256_cmpgt_epi32);
__inject_i(_mm256_cmpgt_epi64);
__inject_i(_mm256_slli_epi16);
__inject_i(_mm256_slli_epi32);
__inject_i(_mm256_slli_epi64);
__inject_i(_mm256_srli_epi16);
__inject_i(_mm256_srli_epi32);
__inject_i(_mm256_srli_epi64);
__inject_i(_mm256_srai_epi16);
__inject_i(_mm256_srai_epi32);
__inject_i(_mm256_sll_epi16);
__inject_i(_mm256_sll_epi32);
__inject_i(_mm256_sll_epi64);
__inject_i(_mm256_srl_epi16);
__inject_i(_mm256_srl_epi32);
__inject_i(_mm256_srl_epi64);
__inject_i(_mm256_sra_epi16);
__inject_i(_mm256_sra_epi32);
__inject_i(_mm_sllv_epi32);
__inject_i(_mm_sllv_epi64);
__inject_i(_mm_srlv_epi32);
__inject_i(_mm_srlv_epi64);
__inject_i(_mm_srav_epi32);
__inject_i(_mm256_sllv_epi32);
__inject_i(_mm256_sllv_epi64);
__inject_i(_mm256_srlv_epi32);
__inject_i(_mm256_srlv_epi64);
__inject_i(_mm256_srav_epi32);
__inject_i(_mm256_packs_epi16);
__inject_i(_mm256_packs_epi32);
__inject_i(_mm256_packus_epi16);
__inject_i(_mm256_packus_epi32);
__inject_i(_mm256_unpackhi_epi8);
__inject_i(_mm256_unpackhi_epi16);
__inject_i(_mm256_unpackhi_epi32);
__inject_i(_mm256_unpackhi_epi64);
__inject_i(_mm256_unpacklo_epi8);
__inject_i(_mm256_unpacklo_epi16);
__inject_i(_mm256_unpacklo_epi32);
__inject_i(_mm256_unpacklo_epi64);
__inject_i(_mm256_unpackhi_ps);
__inject_i(_mm256_unpacklo_ps);
__inject_i(_mm256_unpackhi_pd);
__inject_i(_mm256_unpacklo_pd);
__inject_i(_mm256_movemask_epi8);
__inject_i(_mm256_cvtepi8_epi16);
__inject_i(_mm256_cvtepi8_epi32);
__inject_i(_mm256_cvtepi8_epi64);
__inject_i(_mm256_cvtepi16_epi32);
__inject_i(_mm256_cvtepi16_epi64);
__inject_i(_mm256_cvtepi32_epi64);
__inject_i(_mm256_cvtepu8_epi16);
__inject_i(_mm256_cvtepu8_epi32);
__inject_i(_mm256_cvtepu8_epi64);
__inject_i(_mm256_cvtepu16_epi32);
__inject_i(_mm256_cvtepu16_epi64);
__inject_i(_mm256_cvtepu32_epi64);
__inject_i(_mm256_broadcastsi128_si256);
__inject_i(_mm256_broadcastb_epi8);
__inject_i(_mm256_broadcastw_epi16);
__inject_i(_mm256_broadcastd_epi32);
__inject_i(_mm256_broadcastq_epi64);
__inject_i(_mm256_broadcastss_ps);
__inject_i(_mm256_broadcastsd_pd);
__inject_i(_mm_broadcastb_epi8);
__inject_i(_mm_broadcastw_epi16);
__inject_i(_mm_broadcastd_epi32);
__inject_i(_mm_broadcastq_epi64);
__inject_i(_mm_broadcastss_ps);
__inject_i(_mm256_stream_load_si256);

#undef __inject_i
#endif
