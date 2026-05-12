//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_x86_any)
#error "avx512.hpp included on a non-x86 build"
#endif

namespace micron
{
namespace simd
{
namespace avx512
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpedantic"

#pragma GCC push_options
#pragma GCC target("avx512f,avx512bw,avx512dq,avx512vl")

#define __inline_avx512 [[gnu::always_inline, gnu::artificial]] static inline

__inline_avx512 __m512
zero_f32() noexcept
{
  return _mm512_setzero_ps();
}

__inline_avx512 __m512d
zero_f64() noexcept
{
  return _mm512_setzero_pd();
}

__inline_avx512 __m512i
zero_i512() noexcept
{
  return _mm512_setzero_si512();
}

__inline_avx512 __m512
splat_f32(float v) noexcept
{
  return _mm512_set1_ps(v);
}

__inline_avx512 __m512d
splat_f64(double v) noexcept
{
  return _mm512_set1_pd(v);
}

__inline_avx512 __m512i
splat_i8(char v) noexcept
{
  return _mm512_set1_epi8(v);
}

__inline_avx512 __m512i
splat_i16(short v) noexcept
{
  return _mm512_set1_epi16(v);
}

__inline_avx512 __m512i
splat_i32(int v) noexcept
{
  return _mm512_set1_epi32(v);
}

__inline_avx512 __m512i
splat_i64(long long v) noexcept
{
  return _mm512_set1_epi64(v);
}

__inline_avx512 __m512
load_f32(const void *p) noexcept
{
  return _mm512_load_ps(p);
}

__inline_avx512 __m512
loadu_f32(const void *p) noexcept
{
  return _mm512_loadu_ps(p);
}

__inline_avx512 __m512d
load_f64(const void *p) noexcept
{
  return _mm512_load_pd(p);
}

__inline_avx512 __m512d
loadu_f64(const void *p) noexcept
{
  return _mm512_loadu_pd(p);
}

__inline_avx512 __m512i
load_i512(const void *p) noexcept
{
  return _mm512_load_si512(p);
}

__inline_avx512 __m512i
loadu_i512(const void *p) noexcept
{
  return _mm512_loadu_si512(p);
}

__inline_avx512 void
store_f32(void *p, __m512 v) noexcept
{
  _mm512_store_ps(p, v);
}

__inline_avx512 void
storeu_f32(void *p, __m512 v) noexcept
{
  _mm512_storeu_ps(p, v);
}

__inline_avx512 void
store_f64(void *p, __m512d v) noexcept
{
  _mm512_store_pd(p, v);
}

__inline_avx512 void
storeu_f64(void *p, __m512d v) noexcept
{
  _mm512_storeu_pd(p, v);
}

__inline_avx512 void
store_i512(void *p, __m512i v) noexcept
{
  _mm512_store_si512(p, v);
}

__inline_avx512 void
storeu_i512(void *p, __m512i v) noexcept
{
  _mm512_storeu_si512(p, v);
}

__inline_avx512 void
store_nt_f32(void *p, __m512 v) noexcept
{
  _mm512_stream_ps(p, v);
}

__inline_avx512 void
store_nt_f64(void *p, __m512d v) noexcept
{
  _mm512_stream_pd(p, v);
}

__inline_avx512 void
store_nt_i512(void *p, __m512i v) noexcept
{
  _mm512_stream_si512(p, v);
}

__inline_avx512 __m512d
cast_f32_to_f64(__m512 a) noexcept
{
  return _mm512_castps_pd(a);
}

__inline_avx512 __m512i
cast_f32_to_i512(__m512 a) noexcept
{
  return _mm512_castps_si512(a);
}

__inline_avx512 __m512
cast_f64_to_f32(__m512d a) noexcept
{
  return _mm512_castpd_ps(a);
}

__inline_avx512 __m512i
cast_f64_to_i512(__m512d a) noexcept
{
  return _mm512_castpd_si512(a);
}

__inline_avx512 __m512
cast_i512_to_f32(__m512i a) noexcept
{
  return _mm512_castsi512_ps(a);
}

__inline_avx512 __m512d
cast_i512_to_f64(__m512i a) noexcept
{
  return _mm512_castsi512_pd(a);
}

__inline_avx512 __m128
cast_f32_to_lo128(__m512 a) noexcept
{
  return _mm512_castps512_ps128(a);
}

__inline_avx512 __m256
cast_f32_to_lo256(__m512 a) noexcept
{
  return _mm512_castps512_ps256(a);
}

__inline_avx512 __m128d
cast_f64_to_lo128(__m512d a) noexcept
{
  return _mm512_castpd512_pd128(a);
}

__inline_avx512 __m256d
cast_f64_to_lo256(__m512d a) noexcept
{
  return _mm512_castpd512_pd256(a);
}

__inline_avx512 __m128i
cast_i512_to_lo128(__m512i a) noexcept
{
  return _mm512_castsi512_si128(a);
}

__inline_avx512 __m256i
cast_i512_to_lo256(__m512i a) noexcept
{
  return _mm512_castsi512_si256(a);
}

__inline_avx512 __m512
add_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_add_ps(a, b);
}

__inline_avx512 __m512
sub_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_sub_ps(a, b);
}

__inline_avx512 __m512
mul_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_mul_ps(a, b);
}

__inline_avx512 __m512
div_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_div_ps(a, b);
}

__inline_avx512 __m512
min_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_min_ps(a, b);
}

__inline_avx512 __m512
max_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_max_ps(a, b);
}

__inline_avx512 __m512
sqrt_f32(__m512 a) noexcept
{
  return _mm512_sqrt_ps(a);
}

__inline_avx512 __m512d
add_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_add_pd(a, b);
}

__inline_avx512 __m512d
sub_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_sub_pd(a, b);
}

__inline_avx512 __m512d
mul_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_mul_pd(a, b);
}

__inline_avx512 __m512d
div_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_div_pd(a, b);
}

__inline_avx512 __m512d
min_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_min_pd(a, b);
}

__inline_avx512 __m512d
max_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_max_pd(a, b);
}

__inline_avx512 __m512d
sqrt_f64(__m512d a) noexcept
{
  return _mm512_sqrt_pd(a);
}

__inline_avx512 __m512i
add_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_add_epi32(a, b);
}

__inline_avx512 __m512i
add_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_add_epi64(a, b);
}

__inline_avx512 __m512i
sub_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_sub_epi32(a, b);
}

__inline_avx512 __m512i
sub_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_sub_epi64(a, b);
}

__inline_avx512 __m512i
mul_lo_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_mullo_epi32(a, b);
}

__inline_avx512 __m512i
mul_lo_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_mullo_epi64(a, b);
}

__inline_avx512 __m512i
mul_2x32_to_2x64_s(__m512i a, __m512i b) noexcept
{
  return _mm512_mul_epi32(a, b);
}

__inline_avx512 __m512i
mul_2x32_to_2x64_u(__m512i a, __m512i b) noexcept
{
  return _mm512_mul_epu32(a, b);
}

__inline_avx512 __m512i
min_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epi32(a, b);
}

__inline_avx512 __m512i
max_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epi32(a, b);
}

__inline_avx512 __m512i
min_u32(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epu32(a, b);
}

__inline_avx512 __m512i
max_u32(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epu32(a, b);
}

__inline_avx512 __m512i
min_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epi64(a, b);
}

__inline_avx512 __m512i
max_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epi64(a, b);
}

__inline_avx512 __m512i
min_u64(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epu64(a, b);
}

__inline_avx512 __m512i
max_u64(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epu64(a, b);
}

__inline_avx512 __m512i
abs_i32(__m512i a) noexcept
{
  return _mm512_abs_epi32(a);
}

__inline_avx512 __m512i
abs_i64(__m512i a) noexcept
{
  return _mm512_abs_epi64(a);
}

__inline_avx512 __m512i
add_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_add_epi8(a, b);
}

__inline_avx512 __m512i
add_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_add_epi16(a, b);
}

__inline_avx512 __m512i
sub_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_sub_epi8(a, b);
}

__inline_avx512 __m512i
sub_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_sub_epi16(a, b);
}

__inline_avx512 __m512i
mul_lo_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_mullo_epi16(a, b);
}

__inline_avx512 __m512i
mul_hi_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_mulhi_epi16(a, b);
}

__inline_avx512 __m512i
mul_hi_u16(__m512i a, __m512i b) noexcept
{
  return _mm512_mulhi_epu16(a, b);
}

__inline_avx512 __m512i
madd_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_madd_epi16(a, b);
}

__inline_avx512 __m512i
add_sat_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_adds_epi8(a, b);
}

__inline_avx512 __m512i
add_sat_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_adds_epi16(a, b);
}

__inline_avx512 __m512i
add_sat_u8(__m512i a, __m512i b) noexcept
{
  return _mm512_adds_epu8(a, b);
}

__inline_avx512 __m512i
add_sat_u16(__m512i a, __m512i b) noexcept
{
  return _mm512_adds_epu16(a, b);
}

__inline_avx512 __m512i
sub_sat_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_subs_epi8(a, b);
}

__inline_avx512 __m512i
sub_sat_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_subs_epi16(a, b);
}

__inline_avx512 __m512i
sub_sat_u8(__m512i a, __m512i b) noexcept
{
  return _mm512_subs_epu8(a, b);
}

__inline_avx512 __m512i
sub_sat_u16(__m512i a, __m512i b) noexcept
{
  return _mm512_subs_epu16(a, b);
}

__inline_avx512 __m512i
avg_u8(__m512i a, __m512i b) noexcept
{
  return _mm512_avg_epu8(a, b);
}

__inline_avx512 __m512i
avg_u16(__m512i a, __m512i b) noexcept
{
  return _mm512_avg_epu16(a, b);
}

__inline_avx512 __m512i
sad_u8(__m512i a, __m512i b) noexcept
{
  return _mm512_sad_epu8(a, b);
}

__inline_avx512 __m512i
abs_i8(__m512i a) noexcept
{
  return _mm512_abs_epi8(a);
}

__inline_avx512 __m512i
abs_i16(__m512i a) noexcept
{
  return _mm512_abs_epi16(a);
}

__inline_avx512 __m512i
min_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epi8(a, b);
}

__inline_avx512 __m512i
max_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epi8(a, b);
}

__inline_avx512 __m512i
min_u8(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epu8(a, b);
}

__inline_avx512 __m512i
max_u8(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epu8(a, b);
}

__inline_avx512 __m512i
min_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epi16(a, b);
}

__inline_avx512 __m512i
max_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epi16(a, b);
}

__inline_avx512 __m512i
min_u16(__m512i a, __m512i b) noexcept
{
  return _mm512_min_epu16(a, b);
}

__inline_avx512 __m512i
max_u16(__m512i a, __m512i b) noexcept
{
  return _mm512_max_epu16(a, b);
}

__inline_avx512 __m512i
pack_sat_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_packs_epi16(a, b);
}

__inline_avx512 __m512i
pack_sat_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_packs_epi32(a, b);
}

__inline_avx512 __m512i
pack_satu_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_packus_epi16(a, b);
}

__inline_avx512 __m512i
pack_satu_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_packus_epi32(a, b);
}

__inline_avx512 __m512i
unpack_lo_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_unpacklo_epi8(a, b);
}

__inline_avx512 __m512i
unpack_lo_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_unpacklo_epi16(a, b);
}

__inline_avx512 __m512i
unpack_hi_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_unpackhi_epi8(a, b);
}

__inline_avx512 __m512i
unpack_hi_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_unpackhi_epi16(a, b);
}

__inline_avx512 __m512
and_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_and_ps(a, b);
}

__inline_avx512 __m512
or_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_or_ps(a, b);
}

__inline_avx512 __m512
xor_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_xor_ps(a, b);
}

__inline_avx512 __m512
andnot_f32(__m512 a, __m512 b) noexcept
{
  return _mm512_andnot_ps(a, b);
}

__inline_avx512 __m512d
and_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_and_pd(a, b);
}

__inline_avx512 __m512d
or_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_or_pd(a, b);
}

__inline_avx512 __m512d
xor_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_xor_pd(a, b);
}

__inline_avx512 __m512d
andnot_f64(__m512d a, __m512d b) noexcept
{
  return _mm512_andnot_pd(a, b);
}

__inline_avx512 __m512i
and_i512(__m512i a, __m512i b) noexcept
{
  return _mm512_and_si512(a, b);
}

__inline_avx512 __m512i
or_i512(__m512i a, __m512i b) noexcept
{
  return _mm512_or_si512(a, b);
}

__inline_avx512 __m512i
xor_i512(__m512i a, __m512i b) noexcept
{
  return _mm512_xor_si512(a, b);
}

__inline_avx512 __m512i
andnot_i512(__m512i a, __m512i b) noexcept
{
  return _mm512_andnot_si512(a, b);
}

__inline_avx512 ::__mmask16
eq_mask_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpeq_epi32_mask(a, b);
}

__inline_avx512 ::__mmask16
lt_mask_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_cmplt_epi32_mask(a, b);
}

__inline_avx512 ::__mmask16
gt_mask_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpgt_epi32_mask(a, b);
}

__inline_avx512 ::__mmask16
ne_mask_i32(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpneq_epi32_mask(a, b);
}

__inline_avx512 ::__mmask8
eq_mask_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpeq_epi64_mask(a, b);
}

__inline_avx512 ::__mmask8
lt_mask_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_cmplt_epi64_mask(a, b);
}

__inline_avx512 ::__mmask8
gt_mask_i64(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpgt_epi64_mask(a, b);
}

__inline_avx512 ::__mmask64
eq_mask_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpeq_epi8_mask(a, b);
}

__inline_avx512 ::__mmask64
lt_mask_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_cmplt_epi8_mask(a, b);
}

__inline_avx512 ::__mmask64
gt_mask_i8(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpgt_epi8_mask(a, b);
}

__inline_avx512 ::__mmask32
eq_mask_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpeq_epi16_mask(a, b);
}

__inline_avx512 ::__mmask32
lt_mask_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_cmplt_epi16_mask(a, b);
}

__inline_avx512 ::__mmask32
gt_mask_i16(__m512i a, __m512i b) noexcept
{
  return _mm512_cmpgt_epi16_mask(a, b);
}

__inline_avx512 ::__mmask16
cmp_mask_f32(__m512 a, __m512 b, int p) noexcept
{
  return _mm512_cmp_ps_mask(a, b, p);
}

__inline_avx512 ::__mmask8
cmp_mask_f64(__m512d a, __m512d b, int p) noexcept
{
  return _mm512_cmp_pd_mask(a, b, p);
}

__inline_avx512 __m512i
shl_i32(__m512i a, int n) noexcept
{
  return _mm512_slli_epi32(a, n);
}

__inline_avx512 __m512i
shl_i64(__m512i a, int n) noexcept
{
  return _mm512_slli_epi64(a, n);
}

__inline_avx512 __m512i
shr_i32(__m512i a, int n) noexcept
{
  return _mm512_srli_epi32(a, n);
}

__inline_avx512 __m512i
shr_i64(__m512i a, int n) noexcept
{
  return _mm512_srli_epi64(a, n);
}

__inline_avx512 __m512i
shr_arith_i32(__m512i a, int n) noexcept
{
  return _mm512_srai_epi32(a, n);
}

__inline_avx512 __m512i
shr_arith_i64(__m512i a, int n) noexcept
{
  return _mm512_srai_epi64(a, n);
}

__inline_avx512 __m512i
shl_i16(__m512i a, int n) noexcept
{
  return _mm512_slli_epi16(a, n);
}

__inline_avx512 __m512i
shr_i16(__m512i a, int n) noexcept
{
  return _mm512_srli_epi16(a, n);
}

__inline_avx512 __m512i
shr_arith_i16(__m512i a, int n) noexcept
{
  return _mm512_srai_epi16(a, n);
}

__inline_avx512 __m512i
convert_f32_to_i32(__m512 a) noexcept
{
  return _mm512_cvtps_epi32(a);
}

__inline_avx512 __m512i
convert_trunc_f32_to_i32(__m512 a) noexcept
{
  return _mm512_cvttps_epi32(a);
}

__inline_avx512 __m512
convert_i32_to_f32(__m512i a) noexcept
{
  return _mm512_cvtepi32_ps(a);
}

__inline_avx512 __m256i
convert_f64_to_i32(__m512d a) noexcept
{
  return _mm512_cvtpd_epi32(a);
}

__inline_avx512 __m512d
convert_i32_to_f64(__m256i a) noexcept
{
  return _mm512_cvtepi32_pd(a);
}

__inline_avx512 __m256
convert_f64_to_f32(__m512d a) noexcept
{
  return _mm512_cvtpd_ps(a);
}

__inline_avx512 __m512d
convert_f32_to_f64(__m256 a) noexcept
{
  return _mm512_cvtps_pd(a);
}

__inline_avx512 __m512
broadcast_f32_to_f512(__m128 a) noexcept
{
  return _mm512_broadcastss_ps(a);
}

__inline_avx512 __m512d
broadcast_f64_to_f512(__m128d a) noexcept
{
  return _mm512_broadcastsd_pd(a);
}

__inline_avx512 ::__mmask16
kand_16(::__mmask16 a, ::__mmask16 b) noexcept
{
  return _kand_mask16(a, b);
}

__inline_avx512 ::__mmask16
kor_16(::__mmask16 a, ::__mmask16 b) noexcept
{
  return _kor_mask16(a, b);
}

__inline_avx512 ::__mmask16
kxor_16(::__mmask16 a, ::__mmask16 b) noexcept
{
  return _kxor_mask16(a, b);
}

__inline_avx512 ::__mmask16
knot_16(::__mmask16 a) noexcept
{
  return _knot_mask16(a);
}

__inline_avx512 ::__mmask8
kand_8(::__mmask8 a, ::__mmask8 b) noexcept
{
  return _kand_mask8(a, b);
}

__inline_avx512 ::__mmask8
kor_8(::__mmask8 a, ::__mmask8 b) noexcept
{
  return _kor_mask8(a, b);
}

__inline_avx512 ::__mmask8
kxor_8(::__mmask8 a, ::__mmask8 b) noexcept
{
  return _kxor_mask8(a, b);
}

__inline_avx512 ::__mmask8
knot_8(::__mmask8 a) noexcept
{
  return _knot_mask8(a);
}

__inline_avx512 __m128i
min_i64_v128(__m128i a, __m128i b) noexcept
{
  return _mm_min_epi64(a, b);
}

__inline_avx512 __m128i
max_i64_v128(__m128i a, __m128i b) noexcept
{
  return _mm_max_epi64(a, b);
}

__inline_avx512 __m128i
min_u64_v128(__m128i a, __m128i b) noexcept
{
  return _mm_min_epu64(a, b);
}

__inline_avx512 __m128i
max_u64_v128(__m128i a, __m128i b) noexcept
{
  return _mm_max_epu64(a, b);
}

__inline_avx512 __m128i
abs_i64_v128(__m128i a) noexcept
{
  return _mm_abs_epi64(a);
}

__inline_avx512 __m128i
mul_lo_i64_v128(__m128i a, __m128i b) noexcept
{
  return _mm_mullo_epi64(a, b);
}

__inline_avx512 __m256i
min_i64_v256(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epi64(a, b);
}

__inline_avx512 __m256i
max_i64_v256(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epi64(a, b);
}

__inline_avx512 __m256i
min_u64_v256(__m256i a, __m256i b) noexcept
{
  return _mm256_min_epu64(a, b);
}

__inline_avx512 __m256i
max_u64_v256(__m256i a, __m256i b) noexcept
{
  return _mm256_max_epu64(a, b);
}

__inline_avx512 __m256i
abs_i64_v256(__m256i a) noexcept
{
  return _mm256_abs_epi64(a);
}

__inline_avx512 __m256i
mul_lo_i64_v256(__m256i a, __m256i b) noexcept
{
  return _mm256_mullo_epi64(a, b);
}

__inline_avx512 __m512
abs_f32(__m512 a) noexcept
{
  return _mm512_abs_ps(a);
}

__inline_avx512 __m512d
abs_f64(__m512d a) noexcept
{
  return _mm512_abs_pd(a);
}

__inline_avx512 __m512
floor_f32(__m512 a) noexcept
{
  return _mm512_floor_ps(a);
}

__inline_avx512 __m512d
floor_f64(__m512d a) noexcept
{
  return _mm512_floor_pd(a);
}

__inline_avx512 __m512
ceil_f32(__m512 a) noexcept
{
  return _mm512_ceil_ps(a);
}

__inline_avx512 __m512d
ceil_f64(__m512d a) noexcept
{
  return _mm512_ceil_pd(a);
}

template <int IMM>
__inline_avx512 __m512
roundscale_f32(__m512 a) noexcept
{
  return _mm512_roundscale_ps(a, IMM);
}

template <int IMM>
__inline_avx512 __m512d
roundscale_f64(__m512d a) noexcept
{
  return _mm512_roundscale_pd(a, IMM);
}

__inline_avx512 __m512
rsqrt14_f32(__m512 a) noexcept
{
  return _mm512_rsqrt14_ps(a);
}

__inline_avx512 __m512d
rsqrt14_f64(__m512d a) noexcept
{
  return _mm512_rsqrt14_pd(a);
}

__inline_avx512 __m512
rcp14_f32(__m512 a) noexcept
{
  return _mm512_rcp14_ps(a);
}

__inline_avx512 __m512d
rcp14_f64(__m512d a) noexcept
{
  return _mm512_rcp14_pd(a);
}

__inline_avx512 __m512i
convert_f64_to_i64(__m512d a) noexcept
{
  return _mm512_cvtpd_epi64(a);
}

__inline_avx512 __m512d
convert_i64_to_f64(__m512i a) noexcept
{
  return _mm512_cvtepi64_pd(a);
}

template <int SCALE>
__inline_avx512 __m512
gather_f32(__m512i idx, const float *base) noexcept
{
  return _mm512_i32gather_ps(idx, base, SCALE);
}

template <int SCALE>
__inline_avx512 __m512d
gather_f64(__m512i idx, const double *base) noexcept
{
  return _mm512_i64gather_pd(idx, base, SCALE);
}

template <int SCALE>
__inline_avx512 __m512i
gather_i32(__m512i idx, const int *base) noexcept
{
  return _mm512_i32gather_epi32(idx, base, SCALE);
}

template <int SCALE>
__inline_avx512 __m512i
gather_i64(__m512i idx, const long long *base) noexcept
{
  return _mm512_i64gather_epi64(idx, base, SCALE);
}

#undef __inline_avx512

#pragma GCC pop_options

#pragma GCC diagnostic pop

};     // namespace avx512
};     // namespace simd
};     // namespace micron
