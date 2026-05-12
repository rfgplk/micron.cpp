//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_x86_any)
#error "avx2.hpp included on a non-x86 build"
#endif

namespace micron
{
namespace simd
{
namespace avx2
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpedantic"

#define __inline_avx [[gnu::always_inline, gnu::artificial]] static inline

__inline_avx __m256i
add_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_add_epi8(a, b);
}

__inline_avx __m256i
add_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_add_epi16(a, b);
}

__inline_avx __m256i
add_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_add_epi32(a, b);
}

__inline_avx __m256i
add_i64(__m256i a, __m256i b) noexcept
{
  return _mm256_add_epi64(a, b);
}

__inline_avx __m256i
sub_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_sub_epi8(a, b);
}

__inline_avx __m256i
sub_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_sub_epi16(a, b);
}

__inline_avx __m256i
sub_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_sub_epi32(a, b);
}

__inline_avx __m256i
sub_i64(__m256i a, __m256i b) noexcept
{
  return _mm256_sub_epi64(a, b);
}

__inline_avx __m256i
add_sat_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_adds_epi8(a, b);
}

__inline_avx __m256i
add_sat_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_adds_epi16(a, b);
}

__inline_avx __m256i
add_sat_u8(__m256i a, __m256i b) noexcept
{
  return _mm256_adds_epu8(a, b);
}

__inline_avx __m256i
add_sat_u16(__m256i a, __m256i b) noexcept
{
  return _mm256_adds_epu16(a, b);
}

__inline_avx __m256i
sub_sat_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_subs_epi8(a, b);
}

__inline_avx __m256i
sub_sat_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_subs_epi16(a, b);
}

__inline_avx __m256i
sub_sat_u8(__m256i a, __m256i b) noexcept
{
  return _mm256_subs_epu8(a, b);
}

__inline_avx __m256i
sub_sat_u16(__m256i a, __m256i b) noexcept
{
  return _mm256_subs_epu16(a, b);
}

__inline_avx __m256i
mul_lo_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_mullo_epi16(a, b);
}

__inline_avx __m256i
mul_lo_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_mullo_epi32(a, b);
}

__inline_avx __m256i
mul_hi_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_mulhi_epi16(a, b);
}

__inline_avx __m256i
mul_hi_u16(__m256i a, __m256i b) noexcept
{
  return _mm256_mulhi_epu16(a, b);
}

__inline_avx __m256i
mul_hi_round_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_mulhrs_epi16(a, b);
}

__inline_avx __m256i
mul_2x32_to_2x64_s(__m256i a, __m256i b) noexcept
{
  return _mm256_mul_epi32(a, b);
}

__inline_avx __m256i
mul_2x32_to_2x64_u(__m256i a, __m256i b) noexcept
{
  return _mm256_mul_epu32(a, b);
}

__inline_avx __m256i
madd_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_madd_epi16(a, b);
}

__inline_avx __m256i
madd_u8s8(__m256i a, __m256i b) noexcept
{
  return _mm256_maddubs_epi16(a, b);
}

__inline_avx __m256i
avg_u8(__m256i a, __m256i b) noexcept
{
  return _mm256_avg_epu8(a, b);
}

__inline_avx __m256i
avg_u16(__m256i a, __m256i b) noexcept
{
  return _mm256_avg_epu16(a, b);
}

__inline_avx __m256i
sad_u8(__m256i a, __m256i b) noexcept
{
  return _mm256_sad_epu8(a, b);
}

__inline_avx __m256i
abs_i8(__m256i a) noexcept
{
  return _mm256_abs_epi8(a);
}

__inline_avx __m256i
abs_i16(__m256i a) noexcept
{
  return _mm256_abs_epi16(a);
}

__inline_avx __m256i
abs_i32(__m256i a) noexcept
{
  return _mm256_abs_epi32(a);
}

__inline_avx __m256i
sign_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_sign_epi8(a, b);
}

__inline_avx __m256i
sign_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_sign_epi16(a, b);
}

__inline_avx __m256i
sign_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_sign_epi32(a, b);
}

__inline_avx __m256i
hadd_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_hadd_epi16(a, b);
}

__inline_avx __m256i
hadd_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_hadd_epi32(a, b);
}

__inline_avx __m256i
hadd_sat_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_hadds_epi16(a, b);
}

__inline_avx __m256i
hsub_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_hsub_epi16(a, b);
}

__inline_avx __m256i
hsub_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_hsub_epi32(a, b);
}

__inline_avx __m256i
hsub_sat_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_hsubs_epi16(a, b);
}

__inline_avx __m256i
min_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epi8(a, b);
}

__inline_avx __m256i
max_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epi8(a, b);
}

__inline_avx __m256i
min_u8(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epu8(a, b);
}

__inline_avx __m256i
max_u8(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epu8(a, b);
}

__inline_avx __m256i
min_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epi16(a, b);
}

__inline_avx __m256i
max_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epi16(a, b);
}

__inline_avx __m256i
min_u16(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epu16(a, b);
}

__inline_avx __m256i
max_u16(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epu16(a, b);
}

__inline_avx __m256i
min_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epi32(a, b);
}

__inline_avx __m256i
max_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epi32(a, b);
}

__inline_avx __m256i
min_u32(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epu32(a, b);
}

__inline_avx __m256i
max_u32(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epu32(a, b);
}

__inline_avx __m256i
and_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_and_si256(a, b);
}

__inline_avx __m256i
or_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_or_si256(a, b);
}

__inline_avx __m256i
xor_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_xor_si256(a, b);
}

__inline_avx __m256i
andnot_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_andnot_si256(a, b);
}

__inline_avx __m256i
eq_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpeq_epi8(a, b);
}

__inline_avx __m256i
eq_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpeq_epi16(a, b);
}

__inline_avx __m256i
eq_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpeq_epi32(a, b);
}

__inline_avx __m256i
eq_i64(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpeq_epi64(a, b);
}

__inline_avx __m256i
gt_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpgt_epi8(a, b);
}

__inline_avx __m256i
gt_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpgt_epi16(a, b);
}

__inline_avx __m256i
gt_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpgt_epi32(a, b);
}

__inline_avx __m256i
gt_i64(__m256i a, __m256i b) noexcept
{
  return _mm256_cmpgt_epi64(a, b);
}

__inline_avx __m256i
shl_i16(__m256i a, int n) noexcept
{
  return _mm256_slli_epi16(a, n);
}

__inline_avx __m256i
shl_i32(__m256i a, int n) noexcept
{
  return _mm256_slli_epi32(a, n);
}

__inline_avx __m256i
shl_i64(__m256i a, int n) noexcept
{
  return _mm256_slli_epi64(a, n);
}

__inline_avx __m256i
shr_i16(__m256i a, int n) noexcept
{
  return _mm256_srli_epi16(a, n);
}

__inline_avx __m256i
shr_i32(__m256i a, int n) noexcept
{
  return _mm256_srli_epi32(a, n);
}

__inline_avx __m256i
shr_i64(__m256i a, int n) noexcept
{
  return _mm256_srli_epi64(a, n);
}

__inline_avx __m256i
shr_arith_i16(__m256i a, int n) noexcept
{
  return _mm256_srai_epi16(a, n);
}

__inline_avx __m256i
shr_arith_i32(__m256i a, int n) noexcept
{
  return _mm256_srai_epi32(a, n);
}

__inline_avx __m256i
shl_v_i16(__m256i a, __m128i n) noexcept
{
  return _mm256_sll_epi16(a, n);
}

__inline_avx __m256i
shl_v_i32(__m256i a, __m128i n) noexcept
{
  return _mm256_sll_epi32(a, n);
}

__inline_avx __m256i
shl_v_i64(__m256i a, __m128i n) noexcept
{
  return _mm256_sll_epi64(a, n);
}

__inline_avx __m256i
shr_v_i16(__m256i a, __m128i n) noexcept
{
  return _mm256_srl_epi16(a, n);
}

__inline_avx __m256i
shr_v_i32(__m256i a, __m128i n) noexcept
{
  return _mm256_srl_epi32(a, n);
}

__inline_avx __m256i
shr_v_i64(__m256i a, __m128i n) noexcept
{
  return _mm256_srl_epi64(a, n);
}

__inline_avx __m256i
shr_arith_v_i16(__m256i a, __m128i n) noexcept
{
  return _mm256_sra_epi16(a, n);
}

__inline_avx __m256i
shr_arith_v_i32(__m256i a, __m128i n) noexcept
{
  return _mm256_sra_epi32(a, n);
}

__inline_avx __m128i
shl_per_i32(__m128i a, __m128i n) noexcept
{
  return _mm_sllv_epi32(a, n);
}

__inline_avx __m128i
shl_per_i64(__m128i a, __m128i n) noexcept
{
  return _mm_sllv_epi64(a, n);
}

__inline_avx __m128i
shr_per_i32(__m128i a, __m128i n) noexcept
{
  return _mm_srlv_epi32(a, n);
}

__inline_avx __m128i
shr_per_i64(__m128i a, __m128i n) noexcept
{
  return _mm_srlv_epi64(a, n);
}

__inline_avx __m128i
shr_arith_per_i32(__m128i a, __m128i n) noexcept
{
  return _mm_srav_epi32(a, n);
}

__inline_avx __m256i
shl_per_i32(__m256i a, __m256i n) noexcept
{
  return _mm256_sllv_epi32(a, n);
}

__inline_avx __m256i
shl_per_i64(__m256i a, __m256i n) noexcept
{
  return _mm256_sllv_epi64(a, n);
}

__inline_avx __m256i
shr_per_i32(__m256i a, __m256i n) noexcept
{
  return _mm256_srlv_epi32(a, n);
}

__inline_avx __m256i
shr_per_i64(__m256i a, __m256i n) noexcept
{
  return _mm256_srlv_epi64(a, n);
}

__inline_avx __m256i
shr_arith_per_i32(__m256i a, __m256i n) noexcept
{
  return _mm256_srav_epi32(a, n);
}

__inline_avx __m256i
pack_sat_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_packs_epi16(a, b);
}

__inline_avx __m256i
pack_sat_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_packs_epi32(a, b);
}

__inline_avx __m256i
pack_satu_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_packus_epi16(a, b);
}

__inline_avx __m256i
pack_satu_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_packus_epi32(a, b);
}

__inline_avx __m256i
unpack_lo_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_unpacklo_epi8(a, b);
}

__inline_avx __m256i
unpack_lo_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_unpacklo_epi16(a, b);
}

__inline_avx __m256i
unpack_lo_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_unpacklo_epi32(a, b);
}

__inline_avx __m256i
unpack_lo_i64(__m256i a, __m256i b) noexcept
{
  return _mm256_unpacklo_epi64(a, b);
}

__inline_avx __m256i
unpack_hi_i8(__m256i a, __m256i b) noexcept
{
  return _mm256_unpackhi_epi8(a, b);
}

__inline_avx __m256i
unpack_hi_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_unpackhi_epi16(a, b);
}

__inline_avx __m256i
unpack_hi_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_unpackhi_epi32(a, b);
}

__inline_avx __m256i
unpack_hi_i64(__m256i a, __m256i b) noexcept
{
  return _mm256_unpackhi_epi64(a, b);
}

__inline_avx int
movemask_i8(__m256i a) noexcept
{
  return _mm256_movemask_epi8(a);
}

__inline_avx __m256i
broadcast_i128_to_i256(__m128i a) noexcept
{
  return _mm256_broadcastsi128_si256(a);
}

__inline_avx __m256i
broadcast_i8_to_i256(__m128i a) noexcept
{
  return _mm256_broadcastb_epi8(a);
}

__inline_avx __m256i
broadcast_i16_to_i256(__m128i a) noexcept
{
  return _mm256_broadcastw_epi16(a);
}

__inline_avx __m256i
broadcast_i32_to_i256(__m128i a) noexcept
{
  return _mm256_broadcastd_epi32(a);
}

__inline_avx __m256i
broadcast_i64_to_i256(__m128i a) noexcept
{
  return _mm256_broadcastq_epi64(a);
}

__inline_avx __m256
broadcast_f32_to_f256(__m128 a) noexcept
{
  return _mm256_broadcastss_ps(a);
}

__inline_avx __m256d
broadcast_f64_to_f256(__m128d a) noexcept
{
  return _mm256_broadcastsd_pd(a);
}

__inline_avx __m128i
broadcast_i8_to_i128(__m128i a) noexcept
{
  return _mm_broadcastb_epi8(a);
}

__inline_avx __m128i
broadcast_i16_to_i128(__m128i a) noexcept
{
  return _mm_broadcastw_epi16(a);
}

__inline_avx __m128i
broadcast_i32_to_i128(__m128i a) noexcept
{
  return _mm_broadcastd_epi32(a);
}

__inline_avx __m128i
broadcast_i64_to_i128(__m128i a) noexcept
{
  return _mm_broadcastq_epi64(a);
}

__inline_avx __m128
broadcast_f32_to_f128(__m128 a) noexcept
{
  return _mm_broadcastss_ps(a);
}

__inline_avx __m256i
load_stream_i256(const __m256i *p) noexcept
{
  return _mm256_stream_load_si256(p);
}

__inline_avx __m256i
widen_i8_to_i16(__m128i a) noexcept
{
  return _mm256_cvtepi8_epi16(a);
}

__inline_avx __m256i
widen_i8_to_i32(__m128i a) noexcept
{
  return _mm256_cvtepi8_epi32(a);
}

__inline_avx __m256i
widen_i8_to_i64(__m128i a) noexcept
{
  return _mm256_cvtepi8_epi64(a);
}

__inline_avx __m256i
widen_i16_to_i32(__m128i a) noexcept
{
  return _mm256_cvtepi16_epi32(a);
}

__inline_avx __m256i
widen_i16_to_i64(__m128i a) noexcept
{
  return _mm256_cvtepi16_epi64(a);
}

__inline_avx __m256i
widen_i32_to_i64(__m128i a) noexcept
{
  return _mm256_cvtepi32_epi64(a);
}

__inline_avx __m256i
widen_u8_to_i16(__m128i a) noexcept
{
  return _mm256_cvtepu8_epi16(a);
}

__inline_avx __m256i
widen_u8_to_i32(__m128i a) noexcept
{
  return _mm256_cvtepu8_epi32(a);
}

__inline_avx __m256i
widen_u8_to_i64(__m128i a) noexcept
{
  return _mm256_cvtepu8_epi64(a);
}

__inline_avx __m256i
widen_u16_to_i32(__m128i a) noexcept
{
  return _mm256_cvtepu16_epi32(a);
}

__inline_avx __m256i
widen_u16_to_i64(__m128i a) noexcept
{
  return _mm256_cvtepu16_epi64(a);
}

__inline_avx __m256i
widen_u32_to_i64(__m128i a) noexcept
{
  return _mm256_cvtepu32_epi64(a);
}

template <int IMM>
__inline_avx __m256d
permute4x64_f64(__m256d a) noexcept
{
  return _mm256_permute4x64_pd(a, IMM);
}

template <int IMM>
__inline_avx __m256i
permute4x64_i64(__m256i a) noexcept
{
  return _mm256_permute4x64_epi64(a, IMM);
}

template <int IMM>
__inline_avx __m256i
permute2x128_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_permute2x128_si256(a, b, IMM);
}

template <int IMM>
__inline_avx __m256i
blend_i16(__m256i a, __m256i b) noexcept
{
  return _mm256_blend_epi16(a, b, IMM);
}

template <int IMM>
__inline_avx __m256i
blend_i32(__m256i a, __m256i b) noexcept
{
  return _mm256_blend_epi32(a, b, IMM);
}

__inline_avx __m256i
blendv_i8(__m256i a, __m256i b, __m256i mask) noexcept
{
  return _mm256_blendv_epi8(a, b, mask);
}

template <int IMM>
__inline_avx __m256i
shuffle_i32(__m256i a) noexcept
{
  return _mm256_shuffle_epi32(a, IMM);
}

template <int SCALE>
__inline_avx __m256
gather_f32(const float *base, __m256i idx) noexcept
{
  return _mm256_i32gather_ps(base, idx, SCALE);
}

template <int SCALE>
__inline_avx __m256d
gather_f64(const double *base, __m128i idx) noexcept
{
  return _mm256_i32gather_pd(base, idx, SCALE);
}

template <int SCALE>
__inline_avx __m256i
gather_i32(const int *base, __m256i idx) noexcept
{
  return _mm256_i32gather_epi32(base, idx, SCALE);
}

template <int SCALE>
__inline_avx __m256i
gather_i64(const long long *base, __m128i idx) noexcept
{
  return _mm256_i32gather_epi64(base, idx, SCALE);
}

template <int IMM>
__inline_avx long long
extract_i64(__m256i a) noexcept
{
  return _mm256_extract_epi64(a, IMM);
}

template <int IMM>
__inline_avx int
extract_i32(__m256i a) noexcept
{
  return _mm256_extract_epi32(a, IMM);
}

#undef __inline_avx

#pragma GCC diagnostic pop

};     // namespace avx2
};     // namespace simd
};     // namespace micron
