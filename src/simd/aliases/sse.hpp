//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_x86_any)
#error "sse.hpp included on a non-x86 build"
#endif

namespace micron
{
namespace simd
{
namespace sse
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpedantic"

#define __inline_sse [[gnu::always_inline, gnu::artificial]] static inline

__inline_sse __m128
zero_f32() noexcept
{
  return _mm_setzero_ps();
}

__inline_sse __m128d
zero_f64() noexcept
{
  return _mm_setzero_pd();
}

__inline_sse __m128i
zero_i128() noexcept
{
  return _mm_setzero_si128();
}

__inline_sse __m128
splat_f32(float v) noexcept
{
  return _mm_set1_ps(v);
}

__inline_sse __m128d
splat_f64(double v) noexcept
{
  return _mm_set1_pd(v);
}

__inline_sse __m128i
splat_i8(char v) noexcept
{
  return _mm_set1_epi8(v);
}

__inline_sse __m128i
splat_i16(short v) noexcept
{
  return _mm_set1_epi16(v);
}

__inline_sse __m128i
splat_i32(int v) noexcept
{
  return _mm_set1_epi32(v);
}

__inline_sse __m128i
splat_i64(long long v) noexcept
{
  return _mm_set1_epi64x(v);
}

__inline_sse __m128
set_f32(float w, float z, float y, float x) noexcept
{
  return _mm_set_ps(w, z, y, x);
}

__inline_sse __m128
setr_f32(float x, float y, float z, float w) noexcept
{
  return _mm_setr_ps(x, y, z, w);
}

__inline_sse __m128d
set_f64(double y, double x) noexcept
{
  return _mm_set_pd(y, x);
}

__inline_sse __m128d
setr_f64(double x, double y) noexcept
{
  return _mm_setr_pd(x, y);
}

__inline_sse __m128i
set_i32(int w, int z, int y, int x) noexcept
{
  return _mm_set_epi32(w, z, y, x);
}

__inline_sse __m128i
setr_i32(int x, int y, int z, int w) noexcept
{
  return _mm_setr_epi32(x, y, z, w);
}

__inline_sse __m128i
set_i64(long long hi, long long lo) noexcept
{
  return _mm_set_epi64x(hi, lo);
}

__inline_sse __m128i
setr_i64(long long lo, long long hi) noexcept
{
  return _mm_set_epi64x(hi, lo);
}

__inline_sse __m128i
set_i8(char b15, char b14, char b13, char b12, char b11, char b10, char b9, char b8, char b7, char b6, char b5, char b4, char b3, char b2,
       char b1, char b0) noexcept
{
  return _mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0);
}

__inline_sse __m128i
setr_i8(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12, char b13,
        char b14, char b15) noexcept
{
  return _mm_setr_epi8(b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15);
}

__inline_sse __m128i
set_i16(short s7, short s6, short s5, short s4, short s3, short s2, short s1, short s0) noexcept
{
  return _mm_set_epi16(s7, s6, s5, s4, s3, s2, s1, s0);
}

__inline_sse __m128i
setr_i16(short s0, short s1, short s2, short s3, short s4, short s5, short s6, short s7) noexcept
{
  return _mm_setr_epi16(s0, s1, s2, s3, s4, s5, s6, s7);
}

__inline_sse __m128
set_scalar_f32(float v) noexcept
{
  return _mm_set_ss(v);
}

__inline_sse __m128d
set_scalar_f64(double v) noexcept
{
  return _mm_set_sd(v);
}

__inline_sse __m128
load_f32(const float *p) noexcept
{
  return _mm_load_ps(p);
}

__inline_sse __m128
loadu_f32(const float *p) noexcept
{
  return _mm_loadu_ps(p);
}

__inline_sse __m128d
load_f64(const double *p) noexcept
{
  return _mm_load_pd(p);
}

__inline_sse __m128d
loadu_f64(const double *p) noexcept
{
  return _mm_loadu_pd(p);
}

__inline_sse __m128i
load_i128(const __m128i *p) noexcept
{
  return _mm_load_si128(p);
}

__inline_sse __m128i
loadu_i128(const __m128i_u *p) noexcept
{
  return _mm_loadu_si128(p);
}

__inline_sse __m128i
load_lo_i64(const __m128i_u *p) noexcept
{
  return _mm_loadl_epi64(p);
}

__inline_sse __m128
load_lo_f32(__m128 a, const __m64 *p) noexcept
{
  return _mm_loadl_pi(a, p);
}

__inline_sse __m128
load_hi_f32(__m128 a, const __m64 *p) noexcept
{
  return _mm_loadh_pi(a, p);
}

__inline_sse __m128d
load_lo_f64(__m128d a, const double *p) noexcept
{
  return _mm_loadl_pd(a, p);
}

__inline_sse __m128d
load_hi_f64(__m128d a, const double *p) noexcept
{
  return _mm_loadh_pd(a, p);
}

__inline_sse __m128
load_scalar_f32(const float *p) noexcept
{
  return _mm_load_ss(p);
}

__inline_sse __m128d
load_scalar_f64(const double *p) noexcept
{
  return _mm_load_sd(p);
}

__inline_sse __m128i
load_dqu_i128(const __m128i *p) noexcept
{
  return _mm_lddqu_si128(p);
}

__inline_sse __m128i
load_stream_i128(const __m128i *p) noexcept
{
  return _mm_stream_load_si128(p);
}

__inline_sse void
store_f32(float *p, __m128 v) noexcept
{
  _mm_store_ps(p, v);
}

__inline_sse void
storeu_f32(float *p, __m128 v) noexcept
{
  _mm_storeu_ps(p, v);
}

__inline_sse void
store_f64(double *p, __m128d v) noexcept
{
  _mm_store_pd(p, v);
}

__inline_sse void
storeu_f64(double *p, __m128d v) noexcept
{
  _mm_storeu_pd(p, v);
}

__inline_sse void
store_i128(__m128i *p, __m128i v) noexcept
{
  _mm_store_si128(p, v);
}

__inline_sse void
storeu_i128(__m128i_u *p, __m128i v) noexcept
{
  _mm_storeu_si128(p, v);
}

__inline_sse void
store_lo_i64(__m128i_u *p, __m128i v) noexcept
{
  _mm_storel_epi64(p, v);
}

__inline_sse void
store_lo_f32(__m64 *p, __m128 v) noexcept
{
  _mm_storel_pi(p, v);
}

__inline_sse void
store_hi_f32(__m64 *p, __m128 v) noexcept
{
  _mm_storeh_pi(p, v);
}

__inline_sse void
store_lo_f64(double *p, __m128d v) noexcept
{
  _mm_storel_pd(p, v);
}

__inline_sse void
store_hi_f64(double *p, __m128d v) noexcept
{
  _mm_storeh_pd(p, v);
}

__inline_sse void
store_scalar_f32(float *p, __m128 v) noexcept
{
  _mm_store_ss(p, v);
}

__inline_sse void
store_scalar_f64(double *p, __m128d v) noexcept
{
  _mm_store_sd(p, v);
}

__inline_sse void
store_nt_f32(float *p, __m128 v) noexcept
{
  _mm_stream_ps(p, v);
}

__inline_sse void
store_nt_f64(double *p, __m128d v) noexcept
{
  _mm_stream_pd(p, v);
}

__inline_sse void
store_nt_i128(__m128i *p, __m128i v) noexcept
{
  _mm_stream_si128(p, v);
}

__inline_sse __m128
cast_f64_to_f32(__m128d a) noexcept
{
  return _mm_castpd_ps(a);
}

__inline_sse __m128i
cast_f64_to_i128(__m128d a) noexcept
{
  return _mm_castpd_si128(a);
}

__inline_sse __m128d
cast_f32_to_f64(__m128 a) noexcept
{
  return _mm_castps_pd(a);
}

__inline_sse __m128i
cast_f32_to_i128(__m128 a) noexcept
{
  return _mm_castps_si128(a);
}

__inline_sse __m128
cast_i128_to_f32(__m128i a) noexcept
{
  return _mm_castsi128_ps(a);
}

__inline_sse __m128d
cast_i128_to_f64(__m128i a) noexcept
{
  return _mm_castsi128_pd(a);
}

__inline_sse __m128
add_f32(__m128 a, __m128 b) noexcept
{
  return _mm_add_ps(a, b);
}

__inline_sse __m128
sub_f32(__m128 a, __m128 b) noexcept
{
  return _mm_sub_ps(a, b);
}

__inline_sse __m128
mul_f32(__m128 a, __m128 b) noexcept
{
  return _mm_mul_ps(a, b);
}

__inline_sse __m128
div_f32(__m128 a, __m128 b) noexcept
{
  return _mm_div_ps(a, b);
}

__inline_sse __m128
min_f32(__m128 a, __m128 b) noexcept
{
  return _mm_min_ps(a, b);
}

__inline_sse __m128
max_f32(__m128 a, __m128 b) noexcept
{
  return _mm_max_ps(a, b);
}

__inline_sse __m128
sqrt_f32(__m128 a) noexcept
{
  return _mm_sqrt_ps(a);
}

__inline_sse __m128
rcp_f32(__m128 a) noexcept
{
  return _mm_rcp_ps(a);
}

__inline_sse __m128
rsqrt_f32(__m128 a) noexcept
{
  return _mm_rsqrt_ps(a);
}

__inline_sse __m128
add_scalar_f32(__m128 a, __m128 b) noexcept
{
  return _mm_add_ss(a, b);
}

__inline_sse __m128
sub_scalar_f32(__m128 a, __m128 b) noexcept
{
  return _mm_sub_ss(a, b);
}

__inline_sse __m128
mul_scalar_f32(__m128 a, __m128 b) noexcept
{
  return _mm_mul_ss(a, b);
}

__inline_sse __m128
div_scalar_f32(__m128 a, __m128 b) noexcept
{
  return _mm_div_ss(a, b);
}

__inline_sse __m128
sqrt_scalar_f32(__m128 a) noexcept
{
  return _mm_sqrt_ss(a);
}

__inline_sse __m128
rcp_scalar_f32(__m128 a) noexcept
{
  return _mm_rcp_ss(a);
}

__inline_sse __m128
rsqrt_scalar_f32(__m128 a) noexcept
{
  return _mm_rsqrt_ss(a);
}

__inline_sse __m128d
add_f64(__m128d a, __m128d b) noexcept
{
  return _mm_add_pd(a, b);
}

__inline_sse __m128d
sub_f64(__m128d a, __m128d b) noexcept
{
  return _mm_sub_pd(a, b);
}

__inline_sse __m128d
mul_f64(__m128d a, __m128d b) noexcept
{
  return _mm_mul_pd(a, b);
}

__inline_sse __m128d
div_f64(__m128d a, __m128d b) noexcept
{
  return _mm_div_pd(a, b);
}

__inline_sse __m128d
min_f64(__m128d a, __m128d b) noexcept
{
  return _mm_min_pd(a, b);
}

__inline_sse __m128d
max_f64(__m128d a, __m128d b) noexcept
{
  return _mm_max_pd(a, b);
}

__inline_sse __m128d
sqrt_f64(__m128d a) noexcept
{
  return _mm_sqrt_pd(a);
}

__inline_sse __m128
hadd_f32(__m128 a, __m128 b) noexcept
{
  return _mm_hadd_ps(a, b);
}

__inline_sse __m128
hsub_f32(__m128 a, __m128 b) noexcept
{
  return _mm_hsub_ps(a, b);
}

__inline_sse __m128
addsub_f32(__m128 a, __m128 b) noexcept
{
  return _mm_addsub_ps(a, b);
}

__inline_sse __m128d
hadd_f64(__m128d a, __m128d b) noexcept
{
  return _mm_hadd_pd(a, b);
}

__inline_sse __m128d
hsub_f64(__m128d a, __m128d b) noexcept
{
  return _mm_hsub_pd(a, b);
}

__inline_sse __m128d
addsub_f64(__m128d a, __m128d b) noexcept
{
  return _mm_addsub_pd(a, b);
}

__inline_sse __m128i
add_i8(__m128i a, __m128i b) noexcept
{
  return _mm_add_epi8(a, b);
}

__inline_sse __m128i
add_i16(__m128i a, __m128i b) noexcept
{
  return _mm_add_epi16(a, b);
}

__inline_sse __m128i
add_i32(__m128i a, __m128i b) noexcept
{
  return _mm_add_epi32(a, b);
}

__inline_sse __m128i
add_i64(__m128i a, __m128i b) noexcept
{
  return _mm_add_epi64(a, b);
}

__inline_sse __m128i
sub_i8(__m128i a, __m128i b) noexcept
{
  return _mm_sub_epi8(a, b);
}

__inline_sse __m128i
sub_i16(__m128i a, __m128i b) noexcept
{
  return _mm_sub_epi16(a, b);
}

__inline_sse __m128i
sub_i32(__m128i a, __m128i b) noexcept
{
  return _mm_sub_epi32(a, b);
}

__inline_sse __m128i
sub_i64(__m128i a, __m128i b) noexcept
{
  return _mm_sub_epi64(a, b);
}

__inline_sse __m128i
add_sat_i8(__m128i a, __m128i b) noexcept
{
  return _mm_adds_epi8(a, b);
}

__inline_sse __m128i
add_sat_i16(__m128i a, __m128i b) noexcept
{
  return _mm_adds_epi16(a, b);
}

__inline_sse __m128i
add_sat_u8(__m128i a, __m128i b) noexcept
{
  return _mm_adds_epu8(a, b);
}

__inline_sse __m128i
add_sat_u16(__m128i a, __m128i b) noexcept
{
  return _mm_adds_epu16(a, b);
}

__inline_sse __m128i
sub_sat_i8(__m128i a, __m128i b) noexcept
{
  return _mm_subs_epi8(a, b);
}

__inline_sse __m128i
sub_sat_i16(__m128i a, __m128i b) noexcept
{
  return _mm_subs_epi16(a, b);
}

__inline_sse __m128i
sub_sat_u8(__m128i a, __m128i b) noexcept
{
  return _mm_subs_epu8(a, b);
}

__inline_sse __m128i
sub_sat_u16(__m128i a, __m128i b) noexcept
{
  return _mm_subs_epu16(a, b);
}

__inline_sse __m128i
mul_lo_i16(__m128i a, __m128i b) noexcept
{
  return _mm_mullo_epi16(a, b);
}

__inline_sse __m128i
mul_hi_i16(__m128i a, __m128i b) noexcept
{
  return _mm_mulhi_epi16(a, b);
}

__inline_sse __m128i
mul_hi_u16(__m128i a, __m128i b) noexcept
{
  return _mm_mulhi_epu16(a, b);
}

__inline_sse __m128i
mul_lo_i32(__m128i a, __m128i b) noexcept
{
  return _mm_mullo_epi32(a, b);
}

__inline_sse __m128i
mul_2x32_to_2x64_s(__m128i a, __m128i b) noexcept
{
  return _mm_mul_epi32(a, b);
}

__inline_sse __m128i
mul_2x32_to_2x64_u(__m128i a, __m128i b) noexcept
{
  return _mm_mul_epu32(a, b);
}

__inline_sse __m128i
madd_i16(__m128i a, __m128i b) noexcept
{
  return _mm_madd_epi16(a, b);
}

__inline_sse __m128i
madd_u8s8(__m128i a, __m128i b) noexcept
{
  return _mm_maddubs_epi16(a, b);
}

__inline_sse __m128i
mul_hi_round_i16(__m128i a, __m128i b) noexcept
{
  return _mm_mulhrs_epi16(a, b);
}

__inline_sse __m128i
avg_u8(__m128i a, __m128i b) noexcept
{
  return _mm_avg_epu8(a, b);
}

__inline_sse __m128i
avg_u16(__m128i a, __m128i b) noexcept
{
  return _mm_avg_epu16(a, b);
}

__inline_sse __m128i
sad_u8(__m128i a, __m128i b) noexcept
{
  return _mm_sad_epu8(a, b);
}

__inline_sse __m128i
abs_i8(__m128i a) noexcept
{
  return _mm_abs_epi8(a);
}

__inline_sse __m128i
abs_i16(__m128i a) noexcept
{
  return _mm_abs_epi16(a);
}

__inline_sse __m128i
abs_i32(__m128i a) noexcept
{
  return _mm_abs_epi32(a);
}

__inline_sse __m128i
sign_i8(__m128i a, __m128i b) noexcept
{
  return _mm_sign_epi8(a, b);
}

__inline_sse __m128i
sign_i16(__m128i a, __m128i b) noexcept
{
  return _mm_sign_epi16(a, b);
}

__inline_sse __m128i
sign_i32(__m128i a, __m128i b) noexcept
{
  return _mm_sign_epi32(a, b);
}

__inline_sse __m128i
hadd_i16(__m128i a, __m128i b) noexcept
{
  return _mm_hadd_epi16(a, b);
}

__inline_sse __m128i
hadd_i32(__m128i a, __m128i b) noexcept
{
  return _mm_hadd_epi32(a, b);
}

__inline_sse __m128i
hadd_sat_i16(__m128i a, __m128i b) noexcept
{
  return _mm_hadds_epi16(a, b);
}

__inline_sse __m128i
hsub_i16(__m128i a, __m128i b) noexcept
{
  return _mm_hsub_epi16(a, b);
}

__inline_sse __m128i
hsub_i32(__m128i a, __m128i b) noexcept
{
  return _mm_hsub_epi32(a, b);
}

__inline_sse __m128i
hsub_sat_i16(__m128i a, __m128i b) noexcept
{
  return _mm_hsubs_epi16(a, b);
}

__inline_sse __m128i
min_u8(__m128i a, __m128i b) noexcept
{
  return _mm_min_epu8(a, b);
}

__inline_sse __m128i
max_u8(__m128i a, __m128i b) noexcept
{
  return _mm_max_epu8(a, b);
}

__inline_sse __m128i
min_i16(__m128i a, __m128i b) noexcept
{
  return _mm_min_epi16(a, b);
}

__inline_sse __m128i
max_i16(__m128i a, __m128i b) noexcept
{
  return _mm_max_epi16(a, b);
}

__inline_sse __m128i
min_i8(__m128i a, __m128i b) noexcept
{
  return _mm_min_epi8(a, b);
}

__inline_sse __m128i
max_i8(__m128i a, __m128i b) noexcept
{
  return _mm_max_epi8(a, b);
}

__inline_sse __m128i
min_u16(__m128i a, __m128i b) noexcept
{
  return _mm_min_epu16(a, b);
}

__inline_sse __m128i
max_u16(__m128i a, __m128i b) noexcept
{
  return _mm_max_epu16(a, b);
}

__inline_sse __m128i
min_i32(__m128i a, __m128i b) noexcept
{
  return _mm_min_epi32(a, b);
}

__inline_sse __m128i
max_i32(__m128i a, __m128i b) noexcept
{
  return _mm_max_epi32(a, b);
}

__inline_sse __m128i
min_u32(__m128i a, __m128i b) noexcept
{
  return _mm_min_epu32(a, b);
}

__inline_sse __m128i
max_u32(__m128i a, __m128i b) noexcept
{
  return _mm_max_epu32(a, b);
}

__inline_sse __m128
and_f32(__m128 a, __m128 b) noexcept
{
  return _mm_and_ps(a, b);
}

__inline_sse __m128
or_f32(__m128 a, __m128 b) noexcept
{
  return _mm_or_ps(a, b);
}

__inline_sse __m128
xor_f32(__m128 a, __m128 b) noexcept
{
  return _mm_xor_ps(a, b);
}

__inline_sse __m128
andnot_f32(__m128 a, __m128 b) noexcept
{
  return _mm_andnot_ps(a, b);
}

__inline_sse __m128d
and_f64(__m128d a, __m128d b) noexcept
{
  return _mm_and_pd(a, b);
}

__inline_sse __m128d
or_f64(__m128d a, __m128d b) noexcept
{
  return _mm_or_pd(a, b);
}

__inline_sse __m128d
xor_f64(__m128d a, __m128d b) noexcept
{
  return _mm_xor_pd(a, b);
}

__inline_sse __m128d
andnot_f64(__m128d a, __m128d b) noexcept
{
  return _mm_andnot_pd(a, b);
}

__inline_sse __m128i
and_i128(__m128i a, __m128i b) noexcept
{
  return _mm_and_si128(a, b);
}

__inline_sse __m128i
or_i128(__m128i a, __m128i b) noexcept
{
  return _mm_or_si128(a, b);
}

__inline_sse __m128i
xor_i128(__m128i a, __m128i b) noexcept
{
  return _mm_xor_si128(a, b);
}

__inline_sse __m128i
andnot_i128(__m128i a, __m128i b) noexcept
{
  return _mm_andnot_si128(a, b);
}

__inline_sse __m128i
eq_i8(__m128i a, __m128i b) noexcept
{
  return _mm_cmpeq_epi8(a, b);
}

__inline_sse __m128i
eq_i16(__m128i a, __m128i b) noexcept
{
  return _mm_cmpeq_epi16(a, b);
}

__inline_sse __m128i
eq_i32(__m128i a, __m128i b) noexcept
{
  return _mm_cmpeq_epi32(a, b);
}

__inline_sse __m128i
eq_i64(__m128i a, __m128i b) noexcept
{
  return _mm_cmpeq_epi64(a, b);
}

__inline_sse __m128i
gt_i8(__m128i a, __m128i b) noexcept
{
  return _mm_cmpgt_epi8(a, b);
}

__inline_sse __m128i
gt_i16(__m128i a, __m128i b) noexcept
{
  return _mm_cmpgt_epi16(a, b);
}

__inline_sse __m128i
gt_i32(__m128i a, __m128i b) noexcept
{
  return _mm_cmpgt_epi32(a, b);
}

__inline_sse __m128i
gt_i64(__m128i a, __m128i b) noexcept
{
  return _mm_cmpgt_epi64(a, b);
}

__inline_sse __m128i
lt_i8(__m128i a, __m128i b) noexcept
{
  return _mm_cmplt_epi8(a, b);
}

__inline_sse __m128i
lt_i16(__m128i a, __m128i b) noexcept
{
  return _mm_cmplt_epi16(a, b);
}

__inline_sse __m128i
lt_i32(__m128i a, __m128i b) noexcept
{
  return _mm_cmplt_epi32(a, b);
}

__inline_sse __m128
eq_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmpeq_ps(a, b);
}

__inline_sse __m128
ne_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmpneq_ps(a, b);
}

__inline_sse __m128
lt_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmplt_ps(a, b);
}

__inline_sse __m128
le_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmple_ps(a, b);
}

__inline_sse __m128
gt_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmpgt_ps(a, b);
}

__inline_sse __m128
ge_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmpge_ps(a, b);
}

__inline_sse __m128
ord_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmpord_ps(a, b);
}

__inline_sse __m128
unord_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmpunord_ps(a, b);
}

__inline_sse __m128d
eq_f64(__m128d a, __m128d b) noexcept
{
  return _mm_cmpeq_pd(a, b);
}

__inline_sse __m128d
ne_f64(__m128d a, __m128d b) noexcept
{
  return _mm_cmpneq_pd(a, b);
}

__inline_sse __m128d
lt_f64(__m128d a, __m128d b) noexcept
{
  return _mm_cmplt_pd(a, b);
}

__inline_sse __m128d
le_f64(__m128d a, __m128d b) noexcept
{
  return _mm_cmple_pd(a, b);
}

__inline_sse __m128d
gt_f64(__m128d a, __m128d b) noexcept
{
  return _mm_cmpgt_pd(a, b);
}

__inline_sse __m128d
ge_f64(__m128d a, __m128d b) noexcept
{
  return _mm_cmpge_pd(a, b);
}

__inline_sse __m128i
shl_i16(__m128i a, int n) noexcept
{
  return _mm_slli_epi16(a, n);
}

__inline_sse __m128i
shl_i32(__m128i a, int n) noexcept
{
  return _mm_slli_epi32(a, n);
}

__inline_sse __m128i
shl_i64(__m128i a, int n) noexcept
{
  return _mm_slli_epi64(a, n);
}

__inline_sse __m128i
shr_i16(__m128i a, int n) noexcept
{
  return _mm_srli_epi16(a, n);
}

__inline_sse __m128i
shr_i32(__m128i a, int n) noexcept
{
  return _mm_srli_epi32(a, n);
}

__inline_sse __m128i
shr_i64(__m128i a, int n) noexcept
{
  return _mm_srli_epi64(a, n);
}

template<int IMM>
__inline_sse __m128i
bsrl_i128(__m128i a) noexcept
{
  return _mm_bsrli_si128(a, IMM);
}

template<int IMM>
__inline_sse __m128i
bsll_i128(__m128i a) noexcept
{
  return _mm_bslli_si128(a, IMM);
}

__inline_sse __m128i
shr_arith_i16(__m128i a, int n) noexcept
{
  return _mm_srai_epi16(a, n);
}

__inline_sse __m128i
shr_arith_i32(__m128i a, int n) noexcept
{
  return _mm_srai_epi32(a, n);
}

__inline_sse __m128i
shl_v_i16(__m128i a, __m128i n) noexcept
{
  return _mm_sll_epi16(a, n);
}

__inline_sse __m128i
shl_v_i32(__m128i a, __m128i n) noexcept
{
  return _mm_sll_epi32(a, n);
}

__inline_sse __m128i
shl_v_i64(__m128i a, __m128i n) noexcept
{
  return _mm_sll_epi64(a, n);
}

__inline_sse __m128i
shr_v_i16(__m128i a, __m128i n) noexcept
{
  return _mm_srl_epi16(a, n);
}

__inline_sse __m128i
shr_v_i32(__m128i a, __m128i n) noexcept
{
  return _mm_srl_epi32(a, n);
}

__inline_sse __m128i
shr_v_i64(__m128i a, __m128i n) noexcept
{
  return _mm_srl_epi64(a, n);
}

__inline_sse __m128i
shr_arith_v_i16(__m128i a, __m128i n) noexcept
{
  return _mm_sra_epi16(a, n);
}

__inline_sse __m128i
shr_arith_v_i32(__m128i a, __m128i n) noexcept
{
  return _mm_sra_epi32(a, n);
}

__inline_sse __m128i
pack_sat_i16(__m128i a, __m128i b) noexcept
{
  return _mm_packs_epi16(a, b);
}

__inline_sse __m128i
pack_sat_i32(__m128i a, __m128i b) noexcept
{
  return _mm_packs_epi32(a, b);
}

__inline_sse __m128i
pack_satu_i16(__m128i a, __m128i b) noexcept
{
  return _mm_packus_epi16(a, b);
}

__inline_sse __m128i
pack_satu_i32(__m128i a, __m128i b) noexcept
{
  return _mm_packus_epi32(a, b);
}

__inline_sse __m128i
unpack_lo_i8(__m128i a, __m128i b) noexcept
{
  return _mm_unpacklo_epi8(a, b);
}

__inline_sse __m128i
unpack_lo_i16(__m128i a, __m128i b) noexcept
{
  return _mm_unpacklo_epi16(a, b);
}

__inline_sse __m128i
unpack_lo_i32(__m128i a, __m128i b) noexcept
{
  return _mm_unpacklo_epi32(a, b);
}

__inline_sse __m128i
unpack_lo_i64(__m128i a, __m128i b) noexcept
{
  return _mm_unpacklo_epi64(a, b);
}

__inline_sse __m128i
unpack_hi_i8(__m128i a, __m128i b) noexcept
{
  return _mm_unpackhi_epi8(a, b);
}

__inline_sse __m128i
unpack_hi_i16(__m128i a, __m128i b) noexcept
{
  return _mm_unpackhi_epi16(a, b);
}

__inline_sse __m128i
unpack_hi_i32(__m128i a, __m128i b) noexcept
{
  return _mm_unpackhi_epi32(a, b);
}

__inline_sse __m128i
unpack_hi_i64(__m128i a, __m128i b) noexcept
{
  return _mm_unpackhi_epi64(a, b);
}

__inline_sse __m128
unpack_lo_f32(__m128 a, __m128 b) noexcept
{
  return _mm_unpacklo_ps(a, b);
}

__inline_sse __m128
unpack_hi_f32(__m128 a, __m128 b) noexcept
{
  return _mm_unpackhi_ps(a, b);
}

__inline_sse __m128d
unpack_lo_f64(__m128d a, __m128d b) noexcept
{
  return _mm_unpacklo_pd(a, b);
}

__inline_sse __m128d
unpack_hi_f64(__m128d a, __m128d b) noexcept
{
  return _mm_unpackhi_pd(a, b);
}

__inline_sse __m128i
shuffle_v_i8(__m128i a, __m128i mask) noexcept
{
  return _mm_shuffle_epi8(a, mask);
}

__inline_sse int
movemask_i8(__m128i a) noexcept
{
  return _mm_movemask_epi8(a);
}

__inline_sse int
movemask_f32(__m128 a) noexcept
{
  return _mm_movemask_ps(a);
}

__inline_sse int
movemask_f64(__m128d a) noexcept
{
  return _mm_movemask_pd(a);
}

__inline_sse __m128i
convert_f32_to_i32(__m128 a) noexcept
{
  return _mm_cvtps_epi32(a);
}

__inline_sse __m128i
convert_trunc_f32_to_i32(__m128 a) noexcept
{
  return _mm_cvttps_epi32(a);
}

__inline_sse __m128
convert_i32_to_f32(__m128i a) noexcept
{
  return _mm_cvtepi32_ps(a);
}

__inline_sse __m128i
convert_f64_to_i32(__m128d a) noexcept
{
  return _mm_cvtpd_epi32(a);
}

__inline_sse __m128i
convert_trunc_f64_to_i32(__m128d a) noexcept
{
  return _mm_cvttpd_epi32(a);
}

__inline_sse __m128d
convert_i32_to_f64(__m128i a) noexcept
{
  return _mm_cvtepi32_pd(a);
}

__inline_sse __m128
convert_f64_to_f32(__m128d a) noexcept
{
  return _mm_cvtpd_ps(a);
}

__inline_sse __m128d
convert_f32_to_f64(__m128 a) noexcept
{
  return _mm_cvtps_pd(a);
}

__inline_sse int
extract_low_i32(__m128i a) noexcept
{
  return _mm_cvtsi128_si32(a);
}

__inline_sse long long
extract_low_i64(__m128i a) noexcept
{
  return _mm_cvtsi128_si64(a);
}

__inline_sse __m128i
broadcast_i32_to_i128(int v) noexcept
{
  return _mm_cvtsi32_si128(v);
}

__inline_sse __m128i
broadcast_i64_to_i128(long long v) noexcept
{
  return _mm_cvtsi64_si128(v);
}

__inline_sse float
extract_low_f32(__m128 a) noexcept
{
  return _mm_cvtss_f32(a);
}

__inline_sse double
extract_low_f64(__m128d a) noexcept
{
  return _mm_cvtsd_f64(a);
}

__inline_sse __m128i
widen_i8_to_i16(__m128i a) noexcept
{
  return _mm_cvtepi8_epi16(a);
}

__inline_sse __m128i
widen_i8_to_i32(__m128i a) noexcept
{
  return _mm_cvtepi8_epi32(a);
}

__inline_sse __m128i
widen_i8_to_i64(__m128i a) noexcept
{
  return _mm_cvtepi8_epi64(a);
}

__inline_sse __m128i
widen_i16_to_i32(__m128i a) noexcept
{
  return _mm_cvtepi16_epi32(a);
}

__inline_sse __m128i
widen_i16_to_i64(__m128i a) noexcept
{
  return _mm_cvtepi16_epi64(a);
}

__inline_sse __m128i
widen_i32_to_i64(__m128i a) noexcept
{
  return _mm_cvtepi32_epi64(a);
}

__inline_sse __m128i
widen_u8_to_i16(__m128i a) noexcept
{
  return _mm_cvtepu8_epi16(a);
}

__inline_sse __m128i
widen_u8_to_i32(__m128i a) noexcept
{
  return _mm_cvtepu8_epi32(a);
}

__inline_sse __m128i
widen_u8_to_i64(__m128i a) noexcept
{
  return _mm_cvtepu8_epi64(a);
}

__inline_sse __m128i
widen_u16_to_i32(__m128i a) noexcept
{
  return _mm_cvtepu16_epi32(a);
}

__inline_sse __m128i
widen_u16_to_i64(__m128i a) noexcept
{
  return _mm_cvtepu16_epi64(a);
}

__inline_sse __m128i
widen_u32_to_i64(__m128i a) noexcept
{
  return _mm_cvtepu32_epi64(a);
}

__inline_sse int
testz_i128(__m128i a, __m128i b) noexcept
{
  return _mm_testz_si128(a, b);
}

__inline_sse int
testc_i128(__m128i a, __m128i b) noexcept
{
  return _mm_testc_si128(a, b);
}

__inline_sse int
testnzc_i128(__m128i a, __m128i b) noexcept
{
  return _mm_testnzc_si128(a, b);
}

template<int IMM>
__inline_sse __m128
round_f32(__m128 a) noexcept
{
  return _mm_round_ps(a, IMM);
}

template<int IMM>
__inline_sse __m128d
round_f64(__m128d a) noexcept
{
  return _mm_round_pd(a, IMM);
}

template<int IMM>
__inline_sse __m128
shuffle_f32(__m128 a, __m128 b) noexcept
{
  return _mm_shuffle_ps(a, b, IMM);
}

template<int IMM>
__inline_sse __m128d
shuffle_f64(__m128d a, __m128d b) noexcept
{
  return _mm_shuffle_pd(a, b, IMM);
}

template<int IMM>
__inline_sse __m128i
shuffle_i32(__m128i a) noexcept
{
  return _mm_shuffle_epi32(a, IMM);
}

template<int IMM>
__inline_sse __m128
blend_f32(__m128 a, __m128 b) noexcept
{
  return _mm_blend_ps(a, b, IMM);
}

template<int IMM>
__inline_sse __m128d
blend_f64(__m128d a, __m128d b) noexcept
{
  return _mm_blend_pd(a, b, IMM);
}

template<int IMM>
__inline_sse __m128i
blend_i16(__m128i a, __m128i b) noexcept
{
  return _mm_blend_epi16(a, b, IMM);
}

__inline_sse __m128
blendv_f32(__m128 a, __m128 b, __m128 mask) noexcept
{
  return _mm_blendv_ps(a, b, mask);
}

__inline_sse __m128d
blendv_f64(__m128d a, __m128d b, __m128d mask) noexcept
{
  return _mm_blendv_pd(a, b, mask);
}

__inline_sse __m128i
blendv_i8(__m128i a, __m128i b, __m128i mask) noexcept
{
  return _mm_blendv_epi8(a, b, mask);
}

template<int IMM>
__inline_sse __m128
cmp_f32(__m128 a, __m128 b) noexcept
{
  return _mm_cmp_ps(a, b, IMM);
}

template<int IMM>
__inline_sse __m128d
cmp_f64(__m128d a, __m128d b) noexcept
{
  return _mm_cmp_pd(a, b, IMM);
}

template<int IMM>
__inline_sse __m128
dp_f32(__m128 a, __m128 b) noexcept
{
  return _mm_dp_ps(a, b, IMM);
}

template<int IMM>
__inline_sse __m128d
dp_f64(__m128d a, __m128d b) noexcept
{
  return _mm_dp_pd(a, b, IMM);
}

__inline_sse __m128
move_hl_f32(__m128 a, __m128 b) noexcept
{
  return _mm_movehl_ps(a, b);
}

__inline_sse __m128
move_lh_f32(__m128 a, __m128 b) noexcept
{
  return _mm_movelh_ps(a, b);
}

template<int IMM>
__inline_sse int
extract_f32(__m128 a) noexcept
{
  // _mm_extract_ps yields a float lane; return its raw bits (not an arithmetic truncation)
  return _mm_extract_epi32(_mm_castps_si128(a), IMM);
}

template<int IMM>
__inline_sse int
extract_i8(__m128i a) noexcept
{
  return _mm_extract_epi8(a, IMM);
}

template<int IMM>
__inline_sse int
extract_i16(__m128i a) noexcept
{
  return _mm_extract_epi16(a, IMM);
}

template<int IMM>
__inline_sse int
extract_i32_imm(__m128i a) noexcept
{
  return _mm_extract_epi32(a, IMM);
}

template<int IMM>
__inline_sse long long
extract_i64_imm(__m128i a) noexcept
{
  return _mm_extract_epi64(a, IMM);
}

template<int IMM>
__inline_sse __m128
insert_f32(__m128 a, __m128 b) noexcept
{
  return _mm_insert_ps(a, b, IMM);
}

template<int IMM>
__inline_sse __m128i
insert_i8(__m128i a, int v) noexcept
{
  return _mm_insert_epi8(a, v, IMM);
}

template<int IMM>
__inline_sse __m128i
insert_i16(__m128i a, int v) noexcept
{
  return _mm_insert_epi16(a, v, IMM);
}

template<int IMM>
__inline_sse __m128i
insert_i32(__m128i a, int v) noexcept
{
  return _mm_insert_epi32(a, v, IMM);
}

template<int IMM>
__inline_sse __m128i
insert_i64(__m128i a, long long v) noexcept
{
  return _mm_insert_epi64(a, v, IMM);
}

template<int SCALE>
__inline_sse __m128
gather_f32(const float *base, __m128i idx) noexcept
{
  return _mm_i32gather_ps(base, idx, SCALE);
}

template<int SCALE>
__inline_sse __m128d
gather_f64(const double *base, __m128i idx) noexcept
{
  return _mm_i32gather_pd(base, idx, SCALE);
}

__inline_sse __m128d
sqrt_sd(__m128d a, __m128d b) noexcept
{
  return _mm_sqrt_sd(a, b);
}

__inline_sse __m128d
add_scalar_f64(__m128d a, __m128d b) noexcept
{
  return _mm_add_sd(a, b);
}

__inline_sse __m128d
sub_scalar_f64(__m128d a, __m128d b) noexcept
{
  return _mm_sub_sd(a, b);
}

__inline_sse __m128d
mul_scalar_f64(__m128d a, __m128d b) noexcept
{
  return _mm_mul_sd(a, b);
}

__inline_sse __m128d
div_scalar_f64(__m128d a, __m128d b) noexcept
{
  return _mm_div_sd(a, b);
}

#undef __inline_sse

#pragma GCC diagnostic pop

};      // namespace sse
};      // namespace simd
};      // namespace micron
