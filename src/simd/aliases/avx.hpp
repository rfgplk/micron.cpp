//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_x86_any)
#error "avx.hpp included on a non-x86 build"
#endif

namespace micron
{
namespace simd
{
namespace avx
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_avx [[gnu::always_inline, gnu::artificial]] static inline

__inline_avx __m256
zero_f32() noexcept
{
  return _mm256_setzero_ps();
}

__inline_avx __m256d
zero_f64() noexcept
{
  return _mm256_setzero_pd();
}

__inline_avx __m256i
zero_i256() noexcept
{
  return _mm256_setzero_si256();
}

__inline_avx __m256
splat_f32(float v) noexcept
{
  return _mm256_set1_ps(v);
}

__inline_avx __m256d
splat_f64(double v) noexcept
{
  return _mm256_set1_pd(v);
}

__inline_avx __m256i
splat_i8(char v) noexcept
{
  return _mm256_set1_epi8(v);
}

__inline_avx __m256i
splat_i16(short v) noexcept
{
  return _mm256_set1_epi16(v);
}

__inline_avx __m256i
splat_i32(int v) noexcept
{
  return _mm256_set1_epi32(v);
}

__inline_avx __m256i
splat_i64(long long v) noexcept
{
  return _mm256_set1_epi64x(v);
}

__inline_avx __m256
set_f32(float a, float b, float c, float d, float e, float f, float g, float h) noexcept
{
  return _mm256_set_ps(a, b, c, d, e, f, g, h);
}

__inline_avx __m256
setr_f32(float a, float b, float c, float d, float e, float f, float g, float h) noexcept
{
  return _mm256_setr_ps(a, b, c, d, e, f, g, h);
}

__inline_avx __m256d
set_f64(double a, double b, double c, double d) noexcept
{
  return _mm256_set_pd(a, b, c, d);
}

__inline_avx __m256d
setr_f64(double a, double b, double c, double d) noexcept
{
  return _mm256_setr_pd(a, b, c, d);
}

__inline_avx __m256
set_2x_f32(__m128 hi, __m128 lo) noexcept
{
  return _mm256_set_m128(hi, lo);
}

__inline_avx __m256i
set_2x_i128(__m128i hi, __m128i lo) noexcept
{
  return _mm256_set_m128i(hi, lo);
}

__inline_avx __m256d
set_2x_f64(__m128d hi, __m128d lo) noexcept
{
  return _mm256_set_m128d(hi, lo);
}

__inline_avx __m256
load_f32(const float *p) noexcept
{
  return _mm256_load_ps(p);
}

__inline_avx __m256
loadu_f32(const float *p) noexcept
{
  return _mm256_loadu_ps(p);
}

__inline_avx __m256d
load_f64(const double *p) noexcept
{
  return _mm256_load_pd(p);
}

__inline_avx __m256d
loadu_f64(const double *p) noexcept
{
  return _mm256_loadu_pd(p);
}

__inline_avx __m256i
load_i256(const __m256i *p) noexcept
{
  return _mm256_load_si256(p);
}

__inline_avx __m256i
loadu_i256(const __m256i_u *p) noexcept
{
  return _mm256_loadu_si256(p);
}

__inline_avx void
store_f32(float *p, __m256 v) noexcept
{
  _mm256_store_ps(p, v);
}

__inline_avx void
storeu_f32(float *p, __m256 v) noexcept
{
  _mm256_storeu_ps(p, v);
}

__inline_avx void
store_f64(double *p, __m256d v) noexcept
{
  _mm256_store_pd(p, v);
}

__inline_avx void
storeu_f64(double *p, __m256d v) noexcept
{
  _mm256_storeu_pd(p, v);
}

__inline_avx void
store_i256(__m256i *p, __m256i v) noexcept
{
  _mm256_store_si256(p, v);
}

__inline_avx void
storeu_i256(__m256i_u *p, __m256i v) noexcept
{
  _mm256_storeu_si256(p, v);
}

__inline_avx void
store_nt_f32(float *p, __m256 v) noexcept
{
  _mm256_stream_ps(p, v);
}

__inline_avx void
store_nt_f64(double *p, __m256d v) noexcept
{
  _mm256_stream_pd(p, v);
}

__inline_avx void
store_nt_i256(__m256i *p, __m256i v) noexcept
{
  _mm256_stream_si256(p, v);
}

__inline_avx __m256d
cast_f32_to_f64(__m256 a) noexcept
{
  return _mm256_castps_pd(a);
}

__inline_avx __m256i
cast_f32_to_i256(__m256 a) noexcept
{
  return _mm256_castps_si256(a);
}

__inline_avx __m256
cast_f64_to_f32(__m256d a) noexcept
{
  return _mm256_castpd_ps(a);
}

__inline_avx __m256i
cast_f64_to_i256(__m256d a) noexcept
{
  return _mm256_castpd_si256(a);
}

__inline_avx __m256
cast_i256_to_f32(__m256i a) noexcept
{
  return _mm256_castsi256_ps(a);
}

__inline_avx __m256d
cast_i256_to_f64(__m256i a) noexcept
{
  return _mm256_castsi256_pd(a);
}

__inline_avx __m128
cast_f32_to_lo128(__m256 a) noexcept
{
  return _mm256_castps256_ps128(a);
}

__inline_avx __m128d
cast_f64_to_lo128(__m256d a) noexcept
{
  return _mm256_castpd256_pd128(a);
}

__inline_avx __m128i
cast_i256_to_lo128(__m256i a) noexcept
{
  return _mm256_castsi256_si128(a);
}

__inline_avx __m256
cast_lo128_to_f32(__m128 a) noexcept
{
  return _mm256_castps128_ps256(a);
}

__inline_avx __m256d
cast_lo128_to_f64(__m128d a) noexcept
{
  return _mm256_castpd128_pd256(a);
}

__inline_avx __m256i
cast_lo128_to_i256(__m128i a) noexcept
{
  return _mm256_castsi128_si256(a);
}

__inline_avx __m256
zext_f32_from_lo128(__m128 a) noexcept
{
  return _mm256_zextps128_ps256(a);
}

__inline_avx __m256d
zext_f64_from_lo128(__m128d a) noexcept
{
  return _mm256_zextpd128_pd256(a);
}

__inline_avx __m256i
zext_i256_from_lo128(__m128i a) noexcept
{
  return _mm256_zextsi128_si256(a);
}

__inline_avx __m256
add_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_add_ps(a, b);
}

__inline_avx __m256
sub_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_sub_ps(a, b);
}

__inline_avx __m256
mul_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_mul_ps(a, b);
}

__inline_avx __m256
div_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_div_ps(a, b);
}

__inline_avx __m256
min_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_min_ps(a, b);
}

__inline_avx __m256
max_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_max_ps(a, b);
}

__inline_avx __m256
sqrt_f32(__m256 a) noexcept
{
  return _mm256_sqrt_ps(a);
}

__inline_avx __m256
rcp_f32(__m256 a) noexcept
{
  return _mm256_rcp_ps(a);
}

__inline_avx __m256
rsqrt_f32(__m256 a) noexcept
{
  return _mm256_rsqrt_ps(a);
}

__inline_avx __m256d
add_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_add_pd(a, b);
}

__inline_avx __m256d
sub_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_sub_pd(a, b);
}

__inline_avx __m256d
mul_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_mul_pd(a, b);
}

__inline_avx __m256d
div_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_div_pd(a, b);
}

__inline_avx __m256d
min_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_min_pd(a, b);
}

__inline_avx __m256d
max_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_max_pd(a, b);
}

__inline_avx __m256d
sqrt_f64(__m256d a) noexcept
{
  return _mm256_sqrt_pd(a);
}

__inline_avx __m256
hadd_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_hadd_ps(a, b);
}

__inline_avx __m256
hsub_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_hsub_ps(a, b);
}

__inline_avx __m256
addsub_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_addsub_ps(a, b);
}

__inline_avx __m256d
hadd_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_hadd_pd(a, b);
}

__inline_avx __m256d
hsub_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_hsub_pd(a, b);
}

__inline_avx __m256d
addsub_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_addsub_pd(a, b);
}

__inline_avx __m256
and_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_and_ps(a, b);
}

__inline_avx __m256
or_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_or_ps(a, b);
}

__inline_avx __m256
xor_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_xor_ps(a, b);
}

__inline_avx __m256
andnot_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_andnot_ps(a, b);
}

__inline_avx __m256d
and_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_and_pd(a, b);
}

__inline_avx __m256d
or_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_or_pd(a, b);
}

__inline_avx __m256d
xor_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_xor_pd(a, b);
}

__inline_avx __m256d
andnot_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_andnot_pd(a, b);
}

__inline_avx __m256
broadcast_f32_from_mem(const float *p) noexcept
{
  return _mm256_broadcast_ss(p);
}

__inline_avx __m256d
broadcast_f64_from_mem(const double *p) noexcept
{
  return _mm256_broadcast_sd(p);
}

__inline_avx __m256
broadcast_f32_from_128(const __m128 *p) noexcept
{
  return _mm256_broadcast_ps(p);
}

__inline_avx __m256d
broadcast_f64_from_128(const __m128d *p) noexcept
{
  return _mm256_broadcast_pd(p);
}

__inline_avx __m256
movehdup_f32(__m256 a) noexcept
{
  return _mm256_movehdup_ps(a);
}

__inline_avx __m256
moveldup_f32(__m256 a) noexcept
{
  return _mm256_moveldup_ps(a);
}

__inline_avx __m256d
movedup_f64(__m256d a) noexcept
{
  return _mm256_movedup_pd(a);
}

__inline_avx int
movemask_f32(__m256 a) noexcept
{
  return _mm256_movemask_ps(a);
}

__inline_avx int
movemask_f64(__m256d a) noexcept
{
  return _mm256_movemask_pd(a);
}

__inline_avx __m256i
convert_f32_to_i32(__m256 a) noexcept
{
  return _mm256_cvtps_epi32(a);
}

__inline_avx __m256i
convert_trunc_f32_to_i32(__m256 a) noexcept
{
  return _mm256_cvttps_epi32(a);
}

__inline_avx __m256
convert_i32_to_f32(__m256i a) noexcept
{
  return _mm256_cvtepi32_ps(a);
}

__inline_avx __m128i
convert_f64_to_i32(__m256d a) noexcept
{
  return _mm256_cvtpd_epi32(a);
}

__inline_avx __m128i
convert_trunc_f64_to_i32(__m256d a) noexcept
{
  return _mm256_cvttpd_epi32(a);
}

__inline_avx __m256d
convert_i32_to_f64(__m128i a) noexcept
{
  return _mm256_cvtepi32_pd(a);
}

__inline_avx __m128
convert_f64_to_f32(__m256d a) noexcept
{
  return _mm256_cvtpd_ps(a);
}

__inline_avx __m256d
convert_f32_to_f64(__m128 a) noexcept
{
  return _mm256_cvtps_pd(a);
}

__inline_avx float
extract_low_f32(__m256 a) noexcept
{
  return _mm256_cvtss_f32(a);
}

__inline_avx double
extract_low_f64(__m256d a) noexcept
{
  return _mm256_cvtsd_f64(a);
}

__inline_avx int
extract_low_i32(__m256i a) noexcept
{
  return _mm256_cvtsi256_si32(a);
}

__inline_avx int
testz_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_testz_ps(a, b);
}

__inline_avx int
testc_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_testc_ps(a, b);
}

__inline_avx int
testnzc_f32(__m256 a, __m256 b) noexcept
{
  return _mm256_testnzc_ps(a, b);
}

__inline_avx int
testz_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_testz_pd(a, b);
}

__inline_avx int
testc_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_testc_pd(a, b);
}

__inline_avx int
testnzc_f64(__m256d a, __m256d b) noexcept
{
  return _mm256_testnzc_pd(a, b);
}

__inline_avx int
testz_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_testz_si256(a, b);
}

__inline_avx int
testc_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_testc_si256(a, b);
}

__inline_avx int
testnzc_i256(__m256i a, __m256i b) noexcept
{
  return _mm256_testnzc_si256(a, b);
}

#undef __inline_avx

#pragma GCC diagnostic pop

};     // namespace avx
};     // namespace simd
};     // namespace micron
