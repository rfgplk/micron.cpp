//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__imm_shuffles.hpp included on a non-x86 build"
#endif

// NOTE: these must be macros, gcc (and likely clang) will reject with "arg must be a constant integer"

#undef _mm_shuffle_ps
#undef _mm_shuffle_pd
#undef _mm_slli_si128
#undef _mm_srli_si128
#undef _mm_bslli_si128
#undef _mm_bsrli_si128
#undef _mm_alignr_epi8
#undef _mm_extract_epi8
#undef _mm_extract_epi16
#undef _mm_extract_epi32
#undef _mm_extract_epi64
#undef _mm_insert_epi8
#undef _mm_insert_epi16
#undef _mm_insert_epi32
#undef _mm_insert_epi64
#undef _mm_extract_ps
#undef _mm_insert_ps
#undef _mm_blend_ps
#undef _mm_blend_pd
#undef _mm_blend_epi16
#undef _mm_blend_epi32
#undef _mm_blendv_ps
#undef _mm_blendv_pd
#undef _mm_blendv_epi8
#undef _mm_round_ps
#undef _mm_round_pd
#undef _mm_round_ss
#undef _mm_round_sd
#undef _mm_ceil_ps
#undef _mm_ceil_pd
#undef _mm_ceil_ss
#undef _mm_ceil_sd
#undef _mm_floor_ps
#undef _mm_floor_pd
#undef _mm_floor_ss
#undef _mm_floor_sd
#undef _mm_dp_ps
#undef _mm_dp_pd
#undef _mm_mpsadbw_epu8
#undef _mm_cmp_ps
#undef _mm_cmp_pd
#undef _mm_cmp_ss
#undef _mm_cmp_sd
#undef _mm_shuffle_epi32
#undef _mm_shufflehi_epi16
#undef _mm_shufflelo_epi16

#undef _mm256_shuffle_ps
#undef _mm256_shuffle_pd
#undef _mm256_blend_ps
#undef _mm256_blend_pd
#undef _mm256_blend_epi16
#undef _mm256_blend_epi32
#undef _mm256_blendv_ps
#undef _mm256_blendv_pd
#undef _mm256_blendv_epi8
#undef _mm256_round_ps
#undef _mm256_round_pd
#undef _mm256_ceil_ps
#undef _mm256_ceil_pd
#undef _mm256_floor_ps
#undef _mm256_floor_pd
#undef _mm256_dp_ps
#undef _mm256_mpsadbw_epu8
#undef _mm256_cmp_ps
#undef _mm256_cmp_pd
#undef _mm256_extractf128_ps
#undef _mm256_extractf128_pd
#undef _mm256_extractf128_si256
#undef _mm256_extracti128_si256
#undef _mm256_insertf128_ps
#undef _mm256_insertf128_pd
#undef _mm256_insertf128_si256
#undef _mm256_inserti128_si256
#undef _mm256_extract_epi8
#undef _mm256_extract_epi16
#undef _mm256_extract_epi32
#undef _mm256_extract_epi64
#undef _mm256_insert_epi8
#undef _mm256_insert_epi16
#undef _mm256_insert_epi32
#undef _mm256_insert_epi64
#undef _mm256_permute_ps
#undef _mm256_permute_pd
#undef _mm256_permute2f128_ps
#undef _mm256_permute2f128_pd
#undef _mm256_permute2f128_si256
#undef _mm256_permute2x128_si256
#undef _mm256_permute4x64_pd
#undef _mm256_permute4x64_epi64
#undef _mm256_shuffle_epi32
#undef _mm256_shufflehi_epi16
#undef _mm256_shufflelo_epi16
#undef _mm_permute_ps
#undef _mm_permute_pd

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SSEs

#define _mm_shuffle_ps(A, B, MASK)                                                                                                         \
  ((::__m128)__builtin_ia32_shufps((::micron::simd::__bits::__v4sf)(::__m128)(A), (::micron::simd::__bits::__v4sf)(::__m128)(B),           \
                                   (int)(MASK)))
#define _mm_shuffle_pd(A, B, MASK)                                                                                                         \
  ((::__m128d)__builtin_ia32_shufpd((::micron::simd::__bits::__v2df)(::__m128d)(A), (::micron::simd::__bits::__v2df)(::__m128d)(B),        \
                                    (int)(MASK)))

#define _mm_slli_si128(A, IMM) ((::__m128i)__builtin_ia32_pslldqi128((::__m128i)(A), (int)(IMM) * 8))
#define _mm_srli_si128(A, IMM) ((::__m128i)__builtin_ia32_psrldqi128((::__m128i)(A), (int)(IMM) * 8))
#define _mm_bslli_si128(A, IMM) _mm_slli_si128((A), (IMM))
#define _mm_bsrli_si128(A, IMM) _mm_srli_si128((A), (IMM))

#define _mm_alignr_epi8(A, B, N)                                                                                                           \
  ((::__m128i)__builtin_ia32_palignr128((::micron::simd::__bits::__v2di)(::__m128i)(A), (::micron::simd::__bits::__v2di)(::__m128i)(B),    \
                                        (int)(N) * 8))

#define _mm_shuffle_epi32(A, IMM) ((::__m128i)__builtin_ia32_pshufd((::micron::simd::__bits::__v4si)(::__m128i)(A), (int)(IMM)))
#define _mm_shufflehi_epi16(A, IMM) ((::__m128i)__builtin_ia32_pshufhw((::micron::simd::__bits::__v8hi)(::__m128i)(A), (int)(IMM)))
#define _mm_shufflelo_epi16(A, IMM) ((::__m128i)__builtin_ia32_pshuflw((::micron::simd::__bits::__v8hi)(::__m128i)(A), (int)(IMM)))

#define _mm_extract_epi8(A, N) ((int)(unsigned char)__builtin_ia32_vec_ext_v16qi((::micron::simd::__bits::__v16qi)(::__m128i)(A), (int)(N)))
#define _mm_extract_epi16(A, N) ((int)(unsigned short)__builtin_ia32_vec_ext_v8hi((::micron::simd::__bits::__v8hi)(::__m128i)(A), (int)(N)))
#define _mm_extract_epi32(A, N) ((int)__builtin_ia32_vec_ext_v4si((::micron::simd::__bits::__v4si)(::__m128i)(A), (int)(N)))
#define _mm_extract_epi64(A, N) ((long long)__builtin_ia32_vec_ext_v2di((::micron::simd::__bits::__v2di)(::__m128i)(A), (int)(N)))
#define _mm_extract_ps(A, N) __builtin_ia32_vec_ext_v4sf((::micron::simd::__bits::__v4sf)(::__m128)(A), (int)(N))

#define _mm_insert_epi8(A, B, N)                                                                                                           \
  ((::__m128i)__builtin_ia32_vec_set_v16qi((::micron::simd::__bits::__v16qi)(::__m128i)(A), (int)(B), (int)(N)))
#define _mm_insert_epi16(A, B, N)                                                                                                          \
  ((::__m128i)__builtin_ia32_vec_set_v8hi((::micron::simd::__bits::__v8hi)(::__m128i)(A), (int)(B), (int)(N)))
#define _mm_insert_epi32(A, B, N)                                                                                                          \
  ((::__m128i)__builtin_ia32_vec_set_v4si((::micron::simd::__bits::__v4si)(::__m128i)(A), (int)(B), (int)(N)))
#define _mm_insert_epi64(A, B, N)                                                                                                          \
  ((::__m128i)__builtin_ia32_vec_set_v2di((::micron::simd::__bits::__v2di)(::__m128i)(A), (long long)(B), (int)(N)))

#define _mm_insert_ps(D, S, N)                                                                                                             \
  ((::__m128)__builtin_ia32_insertps128((::micron::simd::__bits::__v4sf)(::__m128)(D), (::micron::simd::__bits::__v4sf)(::__m128)(S),      \
                                        (int)(N)))

#define _mm_blend_epi16(X, Y, M)                                                                                                           \
  ((::__m128i)__builtin_ia32_pblendw128((::micron::simd::__bits::__v8hi)(::__m128i)(X), (::micron::simd::__bits::__v8hi)(::__m128i)(Y),    \
                                        (int)(M)))
#define _mm_blend_epi32(X, Y, M)                                                                                                           \
  ((::__m128i)__builtin_ia32_pblendd128((::micron::simd::__bits::__v4si)(::__m128i)(X), (::micron::simd::__bits::__v4si)(::__m128i)(Y),    \
                                        (int)(M)))
#define _mm_blend_ps(X, Y, M)                                                                                                              \
  ((::__m128)__builtin_ia32_blendps((::micron::simd::__bits::__v4sf)(::__m128)(X), (::micron::simd::__bits::__v4sf)(::__m128)(Y), (int)(M)))
#define _mm_blend_pd(X, Y, M)                                                                                                              \
  ((::__m128d)__builtin_ia32_blendpd((::micron::simd::__bits::__v2df)(::__m128d)(X), (::micron::simd::__bits::__v2df)(::__m128d)(Y),       \
                                     (int)(M)))

#define _mm_blendv_ps(X, Y, M)                                                                                                             \
  ((::__m128)__builtin_ia32_blendvps((::micron::simd::__bits::__v4sf)(::__m128)(X), (::micron::simd::__bits::__v4sf)(::__m128)(Y),         \
                                     (::micron::simd::__bits::__v4sf)(::__m128)(M)))
#define _mm_blendv_pd(X, Y, M)                                                                                                             \
  ((::__m128d)__builtin_ia32_blendvpd((::micron::simd::__bits::__v2df)(::__m128d)(X), (::micron::simd::__bits::__v2df)(::__m128d)(Y),      \
                                      (::micron::simd::__bits::__v2df)(::__m128d)(M)))
#define _mm_blendv_epi8(X, Y, M)                                                                                                           \
  ((::__m128i)__builtin_ia32_pblendvb128((::micron::simd::__bits::__v16qi)(::__m128i)(X), (::micron::simd::__bits::__v16qi)(::__m128i)(Y), \
                                         (::micron::simd::__bits::__v16qi)(::__m128i)(M)))

#define _mm_round_ps(V, M) ((::__m128)__builtin_ia32_roundps((::micron::simd::__bits::__v4sf)(::__m128)(V), (int)(M)))
#define _mm_round_pd(V, M) ((::__m128d)__builtin_ia32_roundpd((::micron::simd::__bits::__v2df)(::__m128d)(V), (int)(M)))
#define _mm_round_ss(D, V, M)                                                                                                              \
  ((::__m128)__builtin_ia32_roundss((::micron::simd::__bits::__v4sf)(::__m128)(D), (::micron::simd::__bits::__v4sf)(::__m128)(V), (int)(M)))
#define _mm_round_sd(D, V, M)                                                                                                              \
  ((::__m128d)__builtin_ia32_roundsd((::micron::simd::__bits::__v2df)(::__m128d)(D), (::micron::simd::__bits::__v2df)(::__m128d)(V),       \
                                     (int)(M)))

#define _mm_ceil_ps(V) _mm_round_ps((V), (::micron::simd::__bits::_MM_FROUND_CEIL))
#define _mm_ceil_pd(V) _mm_round_pd((V), (::micron::simd::__bits::_MM_FROUND_CEIL))
#define _mm_floor_ps(V) _mm_round_ps((V), (::micron::simd::__bits::_MM_FROUND_FLOOR))
#define _mm_floor_pd(V) _mm_round_pd((V), (::micron::simd::__bits::_MM_FROUND_FLOOR))
#define _mm_ceil_ss(D, V) _mm_round_ss((D), (V), (::micron::simd::__bits::_MM_FROUND_CEIL))
#define _mm_ceil_sd(D, V) _mm_round_sd((D), (V), (::micron::simd::__bits::_MM_FROUND_CEIL))
#define _mm_floor_ss(D, V) _mm_round_ss((D), (V), (::micron::simd::__bits::_MM_FROUND_FLOOR))
#define _mm_floor_sd(D, V) _mm_round_sd((D), (V), (::micron::simd::__bits::_MM_FROUND_FLOOR))

#define _mm_dp_ps(X, Y, M)                                                                                                                 \
  ((::__m128)__builtin_ia32_dpps((::micron::simd::__bits::__v4sf)(::__m128)(X), (::micron::simd::__bits::__v4sf)(::__m128)(Y), (int)(M)))
#define _mm_dp_pd(X, Y, M)                                                                                                                 \
  ((::__m128d)__builtin_ia32_dppd((::micron::simd::__bits::__v2df)(::__m128d)(X), (::micron::simd::__bits::__v2df)(::__m128d)(Y), (int)(M)))

#define _mm_mpsadbw_epu8(X, Y, M)                                                                                                          \
  ((::__m128i)__builtin_ia32_mpsadbw128((::micron::simd::__bits::__v16qi)(::__m128i)(X), (::micron::simd::__bits::__v16qi)(::__m128i)(Y),  \
                                        (int)(M)))

#define _mm_cmp_ps(X, Y, P)                                                                                                                \
  ((::__m128)__builtin_ia32_cmpps((::micron::simd::__bits::__v4sf)(::__m128)(X), (::micron::simd::__bits::__v4sf)(::__m128)(Y), (int)(P)))
#define _mm_cmp_pd(X, Y, P)                                                                                                                \
  ((::__m128d)__builtin_ia32_cmppd((::micron::simd::__bits::__v2df)(::__m128d)(X), (::micron::simd::__bits::__v2df)(::__m128d)(Y),         \
                                   (int)(P)))
#define _mm_cmp_ss(X, Y, P)                                                                                                                \
  ((::__m128)__builtin_ia32_cmpss((::micron::simd::__bits::__v4sf)(::__m128)(X), (::micron::simd::__bits::__v4sf)(::__m128)(Y), (int)(P)))
#define _mm_cmp_sd(X, Y, P)                                                                                                                \
  ((::__m128d)__builtin_ia32_cmpsd((::micron::simd::__bits::__v2df)(::__m128d)(X), (::micron::simd::__bits::__v2df)(::__m128d)(Y),         \
                                   (int)(P)))

#define _mm_permute_ps(A, MASK) ((::__m128)__builtin_ia32_vpermilps((::micron::simd::__bits::__v4sf)(::__m128)(A), (int)(MASK)))
#define _mm_permute_pd(A, MASK) ((::__m128d)__builtin_ia32_vpermilpd((::micron::simd::__bits::__v2df)(::__m128d)(A), (int)(MASK)))

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// AVXs

#define _mm256_shuffle_ps(A, B, MASK)                                                                                                      \
  ((::__m256)__builtin_ia32_shufps256((::micron::simd::__bits::__v8sf)(::__m256)(A), (::micron::simd::__bits::__v8sf)(::__m256)(B),        \
                                      (int)(MASK)))
#define _mm256_shuffle_pd(A, B, MASK)                                                                                                      \
  ((::__m256d)__builtin_ia32_shufpd256((::micron::simd::__bits::__v4df)(::__m256d)(A), (::micron::simd::__bits::__v4df)(::__m256d)(B),     \
                                       (int)(MASK)))

#define _mm256_blend_ps(X, Y, M)                                                                                                           \
  ((::__m256)__builtin_ia32_blendps256((::micron::simd::__bits::__v8sf)(::__m256)(X), (::micron::simd::__bits::__v8sf)(::__m256)(Y),       \
                                       (int)(M)))
#define _mm256_blend_pd(X, Y, M)                                                                                                           \
  ((::__m256d)__builtin_ia32_blendpd256((::micron::simd::__bits::__v4df)(::__m256d)(X), (::micron::simd::__bits::__v4df)(::__m256d)(Y),    \
                                        (int)(M)))
#define _mm256_blend_epi16(X, Y, M)                                                                                                        \
  ((::__m256i)__builtin_ia32_pblendw256((::micron::simd::__bits::__v16hi)(::__m256i)(X), (::micron::simd::__bits::__v16hi)(::__m256i)(Y),  \
                                        (int)(M)))
#define _mm256_blend_epi32(X, Y, M)                                                                                                        \
  ((::__m256i)__builtin_ia32_pblendd256((::micron::simd::__bits::__v8si)(::__m256i)(X), (::micron::simd::__bits::__v8si)(::__m256i)(Y),    \
                                        (int)(M)))

#define _mm256_blendv_ps(X, Y, M)                                                                                                          \
  ((::__m256)__builtin_ia32_blendvps256((::micron::simd::__bits::__v8sf)(::__m256)(X), (::micron::simd::__bits::__v8sf)(::__m256)(Y),      \
                                        (::micron::simd::__bits::__v8sf)(::__m256)(M)))
#define _mm256_blendv_pd(X, Y, M)                                                                                                          \
  ((::__m256d)__builtin_ia32_blendvpd256((::micron::simd::__bits::__v4df)(::__m256d)(X), (::micron::simd::__bits::__v4df)(::__m256d)(Y),   \
                                         (::micron::simd::__bits::__v4df)(::__m256d)(M)))
#define _mm256_blendv_epi8(X, Y, M)                                                                                                        \
  ((::__m256i)__builtin_ia32_pblendvb256((::micron::simd::__bits::__v32qi)(::__m256i)(X), (::micron::simd::__bits::__v32qi)(::__m256i)(Y), \
                                         (::micron::simd::__bits::__v32qi)(::__m256i)(M)))

#define _mm256_round_ps(V, M) ((::__m256)__builtin_ia32_roundps256((::micron::simd::__bits::__v8sf)(::__m256)(V), (int)(M)))
#define _mm256_round_pd(V, M) ((::__m256d)__builtin_ia32_roundpd256((::micron::simd::__bits::__v4df)(::__m256d)(V), (int)(M)))
#define _mm256_ceil_ps(V) _mm256_round_ps((V), (::micron::simd::__bits::_MM_FROUND_CEIL))
#define _mm256_ceil_pd(V) _mm256_round_pd((V), (::micron::simd::__bits::_MM_FROUND_CEIL))
#define _mm256_floor_ps(V) _mm256_round_ps((V), (::micron::simd::__bits::_MM_FROUND_FLOOR))
#define _mm256_floor_pd(V) _mm256_round_pd((V), (::micron::simd::__bits::_MM_FROUND_FLOOR))

#define _mm256_dp_ps(X, Y, M)                                                                                                              \
  ((::__m256)__builtin_ia32_dpps256((::micron::simd::__bits::__v8sf)(::__m256)(X), (::micron::simd::__bits::__v8sf)(::__m256)(Y), (int)(M)))
#define _mm256_mpsadbw_epu8(X, Y, M)                                                                                                       \
  ((::__m256i)__builtin_ia32_mpsadbw256((::micron::simd::__bits::__v32qi)(::__m256i)(X), (::micron::simd::__bits::__v32qi)(::__m256i)(Y),  \
                                        (int)(M)))

#define _mm256_cmp_ps(X, Y, P)                                                                                                             \
  ((::__m256)__builtin_ia32_cmpps256((::micron::simd::__bits::__v8sf)(::__m256)(X), (::micron::simd::__bits::__v8sf)(::__m256)(Y),         \
                                     (int)(P)))
#define _mm256_cmp_pd(X, Y, P)                                                                                                             \
  ((::__m256d)__builtin_ia32_cmppd256((::micron::simd::__bits::__v4df)(::__m256d)(X), (::micron::simd::__bits::__v4df)(::__m256d)(Y),      \
                                      (int)(P)))

#define _mm256_extractf128_ps(X, N) ((::__m128)__builtin_ia32_vextractf128_ps256((::micron::simd::__bits::__v8sf)(::__m256)(X), (int)(N)))
#define _mm256_extractf128_pd(X, N) ((::__m128d)__builtin_ia32_vextractf128_pd256((::micron::simd::__bits::__v4df)(::__m256d)(X), (int)(N)))
#define _mm256_extractf128_si256(X, N)                                                                                                     \
  ((::__m128i)__builtin_ia32_vextractf128_si256((::micron::simd::__bits::__v8si)(::__m256i)(X), (int)(N)))
#define _mm256_extracti128_si256(X, N) _mm256_extractf128_si256((X), (N))

#define _mm256_insertf128_ps(X, Y, N)                                                                                                      \
  ((::__m256)__builtin_ia32_vinsertf128_ps256((::micron::simd::__bits::__v8sf)(::__m256)(X),                                               \
                                              (::micron::simd::__bits::__v4sf)(::__m128)(Y), (int)(N)))
#define _mm256_insertf128_pd(X, Y, N)                                                                                                      \
  ((::__m256d)__builtin_ia32_vinsertf128_pd256((::micron::simd::__bits::__v4df)(::__m256d)(X),                                             \
                                               (::micron::simd::__bits::__v2df)(::__m128d)(Y), (int)(N)))
#define _mm256_insertf128_si256(X, Y, N)                                                                                                   \
  ((::__m256i)__builtin_ia32_vinsertf128_si256((::micron::simd::__bits::__v8si)(::__m256i)(X),                                             \
                                               (::micron::simd::__bits::__v4si)(::__m128i)(Y), (int)(N)))
#define _mm256_inserti128_si256(X, Y, N) _mm256_insertf128_si256((X), (Y), (N))

#define _mm256_permute2f128_ps(X, Y, M)                                                                                                    \
  ((::__m256)__builtin_ia32_vperm2f128_ps256((::micron::simd::__bits::__v8sf)(::__m256)(X), (::micron::simd::__bits::__v8sf)(::__m256)(Y), \
                                             (int)(M)))
#define _mm256_permute2f128_pd(X, Y, M)                                                                                                    \
  ((::__m256d)__builtin_ia32_vperm2f128_pd256((::micron::simd::__bits::__v4df)(::__m256d)(X),                                              \
                                              (::micron::simd::__bits::__v4df)(::__m256d)(Y), (int)(M)))
#define _mm256_permute2f128_si256(X, Y, M)                                                                                                 \
  ((::__m256i)__builtin_ia32_vperm2f128_si256((::micron::simd::__bits::__v8si)(::__m256i)(X),                                              \
                                              (::micron::simd::__bits::__v8si)(::__m256i)(Y), (int)(M)))
#define _mm256_permute2x128_si256(X, Y, M)                                                                                                 \
  ((::__m256i)__builtin_ia32_permti256((::micron::simd::__bits::__v4di)(::__m256i)(X), (::micron::simd::__bits::__v4di)(::__m256i)(Y),     \
                                       (int)(M)))
#define _mm256_permute4x64_pd(X, M) ((::__m256d)__builtin_ia32_permdf256((::micron::simd::__bits::__v4df)(::__m256d)(X), (int)(M)))
#define _mm256_permute4x64_epi64(X, M) ((::__m256i)__builtin_ia32_permdi256((::micron::simd::__bits::__v4di)(::__m256i)(X), (int)(M)))

#define _mm256_permute_ps(A, MASK) ((::__m256)__builtin_ia32_vpermilps256((::micron::simd::__bits::__v8sf)(::__m256)(A), (int)(MASK)))
#define _mm256_permute_pd(A, MASK) ((::__m256d)__builtin_ia32_vpermilpd256((::micron::simd::__bits::__v4df)(::__m256d)(A), (int)(MASK)))

#define _mm256_extract_epi8(A, N)                                                                                                          \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 4);                                                                       \
    _mm_extract_epi8(__mb_y, (int)(N) & 15);                                                                                               \
  }))
#define _mm256_extract_epi16(A, N)                                                                                                         \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 3);                                                                       \
    _mm_extract_epi16(__mb_y, (int)(N) & 7);                                                                                               \
  }))
#define _mm256_extract_epi32(A, N)                                                                                                         \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 2);                                                                       \
    _mm_extract_epi32(__mb_y, (int)(N) & 3);                                                                                               \
  }))
#define _mm256_extract_epi64(A, N)                                                                                                         \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 1);                                                                       \
    _mm_extract_epi64(__mb_y, (int)(N) & 1);                                                                                               \
  }))

#define _mm256_insert_epi8(A, V, N)                                                                                                        \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 4);                                                                       \
    __mb_y = _mm_insert_epi8(__mb_y, (int)(V), (int)(N) & 15);                                                                             \
    _mm256_insertf128_si256((A), __mb_y, (int)(N) >> 4);                                                                                   \
  }))
#define _mm256_insert_epi16(A, V, N)                                                                                                       \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 3);                                                                       \
    __mb_y = _mm_insert_epi16(__mb_y, (int)(V), (int)(N) & 7);                                                                             \
    _mm256_insertf128_si256((A), __mb_y, (int)(N) >> 3);                                                                                   \
  }))
#define _mm256_insert_epi32(A, V, N)                                                                                                       \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 2);                                                                       \
    __mb_y = _mm_insert_epi32(__mb_y, (int)(V), (int)(N) & 3);                                                                             \
    _mm256_insertf128_si256((A), __mb_y, (int)(N) >> 2);                                                                                   \
  }))
#define _mm256_insert_epi64(A, V, N)                                                                                                       \
  (__extension__({                                                                                                                         \
    ::__m128i __mb_y = _mm256_extractf128_si256((A), (int)(N) >> 1);                                                                       \
    __mb_y = _mm_insert_epi64(__mb_y, (long long)(V), (int)(N) & 1);                                                                       \
    _mm256_insertf128_si256((A), __mb_y, (int)(N) >> 1);                                                                                   \
  }))

#define _mm256_shuffle_epi32(A, N) ((::__m256i)__builtin_ia32_pshufd256((::micron::simd::__bits::__v8si)(::__m256i)(A), (int)(N)))
#define _mm256_shufflehi_epi16(A, N) ((::__m256i)__builtin_ia32_pshufhw256((::micron::simd::__bits::__v16hi)(::__m256i)(A), (int)(N)))
#define _mm256_shufflelo_epi16(A, N) ((::__m256i)__builtin_ia32_pshuflw256((::micron::simd::__bits::__v16hi)(::__m256i)(A), (int)(N)))

#define _mm256_alignr_epi8(A, B, N)                                                                                                        \
  ((::__m256i)__builtin_ia32_palignr256((::micron::simd::__bits::__v4di)(::__m256i)(A), (::micron::simd::__bits::__v4di)(::__m256i)(B),    \
                                        (int)(N) * 8))
