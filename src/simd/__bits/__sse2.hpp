//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__sse2.hpp included on a non-x86 build"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// freestanding replacement for SSE / SSE2 [xmmintrin.h, emmintrin.h]
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
#define __inline_g_T(...) [[gnu::always_inline, gnu::artificial, gnu::target(__VA_ARGS__)]] static inline

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SSEs

__inline_g __m128
_mm_setzero_ps() noexcept
{
  return __extension__(__m128){ 0.0f, 0.0f, 0.0f, 0.0f };
}

__inline_g __m128
_mm_set1_ps(float v) noexcept
{
  return __extension__(__m128){ v, v, v, v };
}

__inline_g __m128
_mm_set_ps(float w, float z, float y, float x) noexcept
{
  return __extension__(__m128){ x, y, z, w };
}

__inline_g __m128
_mm_set_ps1(float v) noexcept
{
  return _mm_set1_ps(v);
}

__inline_g __m128
_mm_setr_ps(float x, float y, float z, float w) noexcept
{
  return __extension__(__m128){ x, y, z, w };
}

__inline_g __m128
_mm_set_ss(float v) noexcept
{
  return __extension__(__m128){ v, 0.0f, 0.0f, 0.0f };
}

__inline_g __m128
_mm_load_ps(const float *p) noexcept
{
  return *(const __m128 *)p;
}

__inline_g __m128
_mm_loadu_ps(const float *p) noexcept
{
  return *(const __m128_u *)p;
}

__inline_g __m128
_mm_load_ps1(const float *p) noexcept
{
  return _mm_set1_ps(*p);
}

__inline_g __m128
_mm_load1_ps(const float *p) noexcept
{
  return _mm_set1_ps(*p);
}

__inline_g __m128
_mm_load_ss(const float *p) noexcept
{
  return __extension__(__m128){ *p, 0.0f, 0.0f, 0.0f };
}

__inline_g __m128
_mm_loadl_pi(__m128 a, const __m64 *p) noexcept
{
  return (__m128)__builtin_ia32_loadlps((__v4sf)a, (const __v2sf *)p);
}

__inline_g __m128
_mm_loadh_pi(__m128 a, const __m64 *p) noexcept
{
  return (__m128)__builtin_ia32_loadhps((__v4sf)a, (const __v2sf *)p);
}

__inline_g void
_mm_storel_pi(__m64 *p, __m128 a) noexcept
{
  __builtin_ia32_storelps((__v2sf *)p, (__v4sf)a);
}

__inline_g void
_mm_storeh_pi(__m64 *p, __m128 a) noexcept
{
  __builtin_ia32_storehps((__v2sf *)p, (__v4sf)a);
}

__inline_g void
_mm_store_ps(float *p, __m128 v) noexcept
{
  *(__m128 *)p = v;
}

__inline_g void
_mm_storeu_ps(float *p, __m128 v) noexcept
{
  *(__m128_u *)p = v;
}

__inline_g void
_mm_store_ps1(float *p, __m128 v) noexcept
{
  *p = ((__v4sf)v)[0];
  *(p + 1) = ((__v4sf)v)[0];
  *(p + 2) = ((__v4sf)v)[0];
  *(p + 3) = ((__v4sf)v)[0];
}

__inline_g void
_mm_store1_ps(float *p, __m128 v) noexcept
{
  _mm_store_ps1(p, v);
}

__inline_g void
_mm_store_ss(float *p, __m128 v) noexcept
{
  *p = ((__v4sf)v)[0];
}

__inline_g float
_mm_cvtss_f32(__m128 v) noexcept
{
  return ((__v4sf)v)[0];
}

__inline_g __m128
_mm_add_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)((__v4sf)a + (__v4sf)b);
}

__inline_g __m128
_mm_sub_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)((__v4sf)a - (__v4sf)b);
}

__inline_g __m128
_mm_mul_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)((__v4sf)a * (__v4sf)b);
}

__inline_g __m128
_mm_div_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)((__v4sf)a / (__v4sf)b);
}

__inline_g __m128
_mm_min_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_minps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_max_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_maxps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_sqrt_ps(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_sqrtps((__v4sf)a);
}

__inline_g __m128
_mm_rcp_ps(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_rcpps((__v4sf)a);
}

__inline_g __m128
_mm_rsqrt_ps(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_rsqrtps((__v4sf)a);
}

__inline_g __m128
_mm_add_ss(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_addss((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_sub_ss(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_subss((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_mul_ss(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_mulss((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_div_ss(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_divss((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_min_ss(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_minss((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_max_ss(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_maxss((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_sqrt_ss(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_sqrtss((__v4sf)a);
}

__inline_g __m128
_mm_rcp_ss(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_rcpss((__v4sf)a);
}

__inline_g __m128
_mm_rsqrt_ss(__m128 a) noexcept
{
  return (__m128)__builtin_ia32_rsqrtss((__v4sf)a);
}

__inline_g __m128
_mm_and_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)((__v4su)a & (__v4su)b);
}

__inline_g __m128
_mm_andnot_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)(~(__v4su)a & (__v4su)b);
}

__inline_g __m128
_mm_or_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)((__v4su)a | (__v4su)b);
}

__inline_g __m128
_mm_xor_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)((__v4su)a ^ (__v4su)b);
}

__inline_g __m128
_mm_cmpeq_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpeqps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_cmplt_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpltps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_cmple_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpleps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_cmpgt_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpltps((__v4sf)b, (__v4sf)a);
}

__inline_g __m128
_mm_cmpge_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpleps((__v4sf)b, (__v4sf)a);
}

__inline_g __m128
_mm_cmpneq_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpneqps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_cmpnlt_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpnltps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_cmpnle_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpnleps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_cmpngt_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpnltps((__v4sf)b, (__v4sf)a);
}

__inline_g __m128
_mm_cmpnge_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpnleps((__v4sf)b, (__v4sf)a);
}

__inline_g __m128
_mm_cmpord_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpordps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_cmpunord_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_cmpunordps((__v4sf)a, (__v4sf)b);
}

__inline_g int
_mm_movemask_ps(__m128 a) noexcept
{
  return __builtin_ia32_movmskps((__v4sf)a);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SSE2s

__inline_g __m128d
_mm_setzero_pd() noexcept
{
  return __extension__(__m128d){ 0.0, 0.0 };
}

__inline_g __m128d
_mm_set1_pd(double v) noexcept
{
  return __extension__(__m128d){ v, v };
}

__inline_g __m128d
_mm_set_pd(double y, double x) noexcept
{
  return __extension__(__m128d){ x, y };
}

__inline_g __m128d
_mm_set_pd1(double v) noexcept
{
  return _mm_set1_pd(v);
}

__inline_g __m128d
_mm_setr_pd(double x, double y) noexcept
{
  return __extension__(__m128d){ x, y };
}

__inline_g __m128d
_mm_set_sd(double x) noexcept
{
  return __extension__(__m128d){ x, 0.0 };
}

__inline_g __m128d
_mm_load_pd(const double *p) noexcept
{
  return *(const __m128d *)p;
}

__inline_g __m128d
_mm_loadu_pd(const double *p) noexcept
{
  return *(const __m128d_u *)p;
}

__inline_g __m128d
_mm_load1_pd(const double *p) noexcept
{
  return _mm_set1_pd(*p);
}

__inline_g __m128d
_mm_load_pd1(const double *p) noexcept
{
  return _mm_set1_pd(*p);
}

__inline_g __m128d
_mm_load_sd(const double *p) noexcept
{
  return __extension__(__m128d){ *p, 0.0 };
}

__inline_g __m128d
_mm_loadr_pd(const double *p) noexcept
{
  return __extension__(__m128d){ p[1], p[0] };
}

__inline_g __m128d
_mm_loadl_pd(__m128d a, const double *p) noexcept
{
  return __extension__(__m128d){ *(const __x86_double_u *)p, ((__v2df)a)[1] };
}

__inline_g __m128d
_mm_loadh_pd(__m128d a, const double *p) noexcept
{
  return __extension__(__m128d){ ((__v2df)a)[0], *(const __x86_double_u *)p };
}

__inline_g void
_mm_storel_pd(double *p, __m128d a) noexcept
{
  *(__x86_double_u *)p = ((__v2df)a)[0];
}

__inline_g void
_mm_storeh_pd(double *p, __m128d a) noexcept
{
  *(__x86_double_u *)p = ((__v2df)a)[1];
}

__inline_g void
_mm_store_pd(double *p, __m128d v) noexcept
{
  *(__m128d *)p = v;
}

__inline_g void
_mm_storeu_pd(double *p, __m128d v) noexcept
{
  *(__m128d_u *)p = v;
}

__inline_g void
_mm_store1_pd(double *p, __m128d v) noexcept
{
  p[0] = ((__v2df)v)[0];
  p[1] = ((__v2df)v)[0];
}

__inline_g void
_mm_store_pd1(double *p, __m128d v) noexcept
{
  _mm_store1_pd(p, v);
}

__inline_g void
_mm_store_sd(double *p, __m128d v) noexcept
{
  *p = ((__v2df)v)[0];
}

__inline_g double
_mm_cvtsd_f64(__m128d v) noexcept
{
  return ((__v2df)v)[0];
}

__inline_g __m128d
_mm_add_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)((__v2df)a + (__v2df)b);
}

__inline_g __m128d
_mm_sub_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)((__v2df)a - (__v2df)b);
}

__inline_g __m128d
_mm_mul_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)((__v2df)a * (__v2df)b);
}

__inline_g __m128d
_mm_div_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)((__v2df)a / (__v2df)b);
}

__inline_g __m128d
_mm_min_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_minpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_max_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_maxpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_sqrt_pd(__m128d a) noexcept
{
  return (__m128d)__builtin_ia32_sqrtpd((__v2df)a);
}

__inline_g __m128d
_mm_add_sd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_addsd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_sub_sd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_subsd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_mul_sd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_mulsd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_div_sd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_divsd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_min_sd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_minsd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_max_sd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_maxsd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_sqrt_sd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_sqrtsd((__v2df)b);
}

__inline_g __m128d
_mm_and_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)((__v2du)a & (__v2du)b);
}

__inline_g __m128d
_mm_andnot_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)(~(__v2du)a & (__v2du)b);
}

__inline_g __m128d
_mm_or_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)((__v2du)a | (__v2du)b);
}

__inline_g __m128d
_mm_xor_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)((__v2du)a ^ (__v2du)b);
}

__inline_g __m128d
_mm_cmpeq_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpeqpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_cmplt_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpltpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_cmple_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmplepd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_cmpgt_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpltpd((__v2df)b, (__v2df)a);
}

__inline_g __m128d
_mm_cmpge_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmplepd((__v2df)b, (__v2df)a);
}

__inline_g __m128d
_mm_cmpneq_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpneqpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_cmpnlt_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpnltpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_cmpnle_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpnlepd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_cmpngt_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpnltpd((__v2df)b, (__v2df)a);
}

__inline_g __m128d
_mm_cmpnge_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpnlepd((__v2df)b, (__v2df)a);
}

__inline_g __m128d
_mm_cmpord_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpordpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_cmpunord_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_cmpunordpd((__v2df)a, (__v2df)b);
}

__inline_g int
_mm_movemask_pd(__m128d a) noexcept
{
  return __builtin_ia32_movmskpd((__v2df)a);
}

__inline_g __m128i
_mm_setzero_si128() noexcept
{
  return __extension__(__m128i)(__v4si){ 0, 0, 0, 0 };
}

__inline_g __m128i
_mm_set1_epi8(char v) noexcept
{
  return __extension__(__m128i)(__v16qi){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g __m128i
_mm_set1_epi16(short v) noexcept
{
  return __extension__(__m128i)(__v8hi){ v, v, v, v, v, v, v, v };
}

__inline_g __m128i
_mm_set1_epi32(int v) noexcept
{
  return __extension__(__m128i)(__v4si){ v, v, v, v };
}

__inline_g __m128i
_mm_set1_epi64x(long long v) noexcept
{
  return __extension__(__m128i)(__v2di){ v, v };
}

__inline_g __m128i
_mm_set1_epi64(__m64 v) noexcept
{
  long long x = ((long long *)&v)[0];
  return __extension__(__m128i)(__v2di){ x, x };
}

__inline_g __m128i
_mm_set_epi8(char b15, char b14, char b13, char b12, char b11, char b10, char b9, char b8, char b7, char b6, char b5, char b4, char b3,
             char b2, char b1, char b0) noexcept
{
  return __extension__(__m128i)(__v16qi){ b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 };
}

__inline_g __m128i
_mm_set_epi16(short w7, short w6, short w5, short w4, short w3, short w2, short w1, short w0) noexcept
{
  return __extension__(__m128i)(__v8hi){ w0, w1, w2, w3, w4, w5, w6, w7 };
}

__inline_g __m128i
_mm_set_epi32(int e3, int e2, int e1, int e0) noexcept
{
  return __extension__(__m128i)(__v4si){ e0, e1, e2, e3 };
}

__inline_g __m128i
_mm_set_epi64x(long long e1, long long e0) noexcept
{
  return __extension__(__m128i)(__v2di){ e0, e1 };
}

__inline_g __m128i
_mm_setr_epi8(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12,
              char b13, char b14, char b15) noexcept
{
  return __extension__(__m128i)(__v16qi){ b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15 };
}

__inline_g __m128i
_mm_setr_epi16(short w0, short w1, short w2, short w3, short w4, short w5, short w6, short w7) noexcept
{
  return __extension__(__m128i)(__v8hi){ w0, w1, w2, w3, w4, w5, w6, w7 };
}

__inline_g __m128i
_mm_setr_epi32(int e0, int e1, int e2, int e3) noexcept
{
  return __extension__(__m128i)(__v4si){ e0, e1, e2, e3 };
}

__inline_g __m128i
_mm_load_si128(const __m128i *p) noexcept
{
  return *p;
}

__inline_g __m128i
_mm_loadu_si128(const __m128i_u *p) noexcept
{
  return *p;
}

__inline_g __m128i
_mm_loadl_epi64(const __m128i_u *p) noexcept
{
  __m128i r = _mm_setzero_si128();
  long long x;
  __builtin_memcpy(&x, p, sizeof(x));
  ((__v2di &)r)[0] = x;
  return r;
}

__inline_g void
_mm_store_si128(__m128i *p, __m128i v) noexcept
{
  *p = v;
}

__inline_g void
_mm_storeu_si128(__m128i_u *p, __m128i v) noexcept
{
  *p = v;
}

__inline_g void
_mm_storel_epi64(__m128i_u *p, __m128i v) noexcept
{
  long long x = ((__v2di)v)[0];
  __builtin_memcpy(p, &x, sizeof(x));
}

__inline_g void
_mm_stream_si128(__m128i *p, __m128i v) noexcept
{
  __builtin_ia32_movntdq((__v2di *)p, (__v2di)v);
}

__inline_g void
_mm_stream_pd(double *p, __m128d v) noexcept
{
  __builtin_ia32_movntpd(p, (__v2df)v);
}

__inline_g void
_mm_stream_ps(float *p, __m128 v) noexcept
{
  __builtin_ia32_movntps(p, (__v4sf)v);
}

__inline_g void
_mm_stream_si32(int *p, int v) noexcept
{
  __builtin_ia32_movnti(p, v);
}

__inline_g void
_mm_stream_si64(long long *p, long long v) noexcept
{
  __builtin_ia32_movnti64(p, v);
}

__inline_g __m128
_mm_castpd_ps(__m128d a) noexcept
{
  return (__m128)a;
}

__inline_g __m128i
_mm_castpd_si128(__m128d a) noexcept
{
  return (__m128i)a;
}

__inline_g __m128d
_mm_castps_pd(__m128 a) noexcept
{
  return (__m128d)a;
}

__inline_g __m128i
_mm_castps_si128(__m128 a) noexcept
{
  return (__m128i)a;
}

__inline_g __m128
_mm_castsi128_ps(__m128i a) noexcept
{
  return (__m128)a;
}

__inline_g __m128d
_mm_castsi128_pd(__m128i a) noexcept
{
  return (__m128d)a;
}

__inline_g __m128i
_mm_add_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v16qu)a + (__v16qu)b);
}

__inline_g __m128i
_mm_add_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v8hu)a + (__v8hu)b);
}

__inline_g __m128i
_mm_add_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v4su)a + (__v4su)b);
}

__inline_g __m128i
_mm_add_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v2du)a + (__v2du)b);
}

__inline_g __m128i
_mm_sub_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v16qu)a - (__v16qu)b);
}

__inline_g __m128i
_mm_sub_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v8hu)a - (__v8hu)b);
}

__inline_g __m128i
_mm_sub_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v4su)a - (__v4su)b);
}

__inline_g __m128i
_mm_sub_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v2du)a - (__v2du)b);
}

__inline_g __m128i
_mm_adds_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_paddsb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_adds_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_paddsw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_adds_epu8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_paddusb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_adds_epu16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_paddusw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_subs_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psubsb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_subs_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psubsw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_subs_epu8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psubusb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_subs_epu16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psubusw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_mullo_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v8hu)a * (__v8hu)b);
}

__inline_g __m128i
_mm_mulhi_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmulhw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_mulhi_epu16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmulhuw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_mul_epu32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmuludq128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_madd_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaddwd128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_avg_epu8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pavgb128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_avg_epu16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pavgw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_min_epu8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminub128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_max_epu8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxub128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_min_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pminsw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_max_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_pmaxsw128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_sad_epu8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_psadbw128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_and_si128(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v2du)a & (__v2du)b);
}

__inline_g __m128i
_mm_andnot_si128(__m128i a, __m128i b) noexcept
{
  return (__m128i)(~(__v2du)a & (__v2du)b);
}

__inline_g __m128i
_mm_or_si128(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v2du)a | (__v2du)b);
}

__inline_g __m128i
_mm_xor_si128(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v2du)a ^ (__v2du)b);
}

__inline_g __m128i
_mm_cmpeq_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v16qi)a == (__v16qi)b);
}

__inline_g __m128i
_mm_cmpeq_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v8hi)a == (__v8hi)b);
}

__inline_g __m128i
_mm_cmpeq_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v4si)a == (__v4si)b);
}

__inline_g __m128i
_mm_cmpgt_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v16qi)a > (__v16qi)b);
}

__inline_g __m128i
_mm_cmpgt_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v8hi)a > (__v8hi)b);
}

__inline_g __m128i
_mm_cmpgt_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v4si)a > (__v4si)b);
}

__inline_g __m128i
_mm_cmplt_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v16qi)b > (__v16qi)a);
}

__inline_g __m128i
_mm_cmplt_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v8hi)b > (__v8hi)a);
}

__inline_g __m128i
_mm_cmplt_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)((__v4si)b > (__v4si)a);
}

__inline_g __m128i
_mm_slli_epi16(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_psllwi128((__v8hi)a, c);
}

__inline_g __m128i
_mm_slli_epi32(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_pslldi128((__v4si)a, c);
}

__inline_g __m128i
_mm_slli_epi64(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_psllqi128((__v2di)a, c);
}

__inline_g __m128i
_mm_srli_epi16(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_psrlwi128((__v8hi)a, c);
}

__inline_g __m128i
_mm_srli_epi32(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_psrldi128((__v4si)a, c);
}

__inline_g __m128i
_mm_srli_epi64(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_psrlqi128((__v2di)a, c);
}

__inline_g __m128i
_mm_srai_epi16(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_psrawi128((__v8hi)a, c);
}

__inline_g __m128i
_mm_srai_epi32(__m128i a, int c) noexcept
{
  return (__m128i)__builtin_ia32_psradi128((__v4si)a, c);
}

__inline_g __m128i
_mm_sll_epi16(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psllw128((__v8hi)a, (__v8hi)c);
}

__inline_g __m128i
_mm_sll_epi32(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_pslld128((__v4si)a, (__v4si)c);
}

__inline_g __m128i
_mm_sll_epi64(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psllq128((__v2di)a, (__v2di)c);
}

__inline_g __m128i
_mm_srl_epi16(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psrlw128((__v8hi)a, (__v8hi)c);
}

__inline_g __m128i
_mm_srl_epi32(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psrld128((__v4si)a, (__v4si)c);
}

__inline_g __m128i
_mm_srl_epi64(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psrlq128((__v2di)a, (__v2di)c);
}

__inline_g __m128i
_mm_sra_epi16(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psraw128((__v8hi)a, (__v8hi)c);
}

__inline_g __m128i
_mm_sra_epi32(__m128i a, __m128i c) noexcept
{
  return (__m128i)__builtin_ia32_psrad128((__v4si)a, (__v4si)c);
}

__inline_g __m128i
_mm_packs_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_packsswb128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_packs_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_packssdw128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_packus_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_packuswb128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_unpackhi_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpckhbw128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_unpackhi_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpckhwd128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_unpackhi_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpckhdq128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_unpackhi_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpckhqdq128((__v2di)a, (__v2di)b);
}

__inline_g __m128i
_mm_unpacklo_epi8(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpcklbw128((__v16qi)a, (__v16qi)b);
}

__inline_g __m128i
_mm_unpacklo_epi16(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpcklwd128((__v8hi)a, (__v8hi)b);
}

__inline_g __m128i
_mm_unpacklo_epi32(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpckldq128((__v4si)a, (__v4si)b);
}

__inline_g __m128i
_mm_unpacklo_epi64(__m128i a, __m128i b) noexcept
{
  return (__m128i)__builtin_ia32_punpcklqdq128((__v2di)a, (__v2di)b);
}

__inline_g __m128
_mm_unpackhi_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_unpckhps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_unpacklo_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_unpcklps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_movehl_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_movhlps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128
_mm_movelh_ps(__m128 a, __m128 b) noexcept
{
  return (__m128)__builtin_ia32_movlhps((__v4sf)a, (__v4sf)b);
}

__inline_g __m128d
_mm_unpackhi_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_unpckhpd((__v2df)a, (__v2df)b);
}

__inline_g __m128d
_mm_unpacklo_pd(__m128d a, __m128d b) noexcept
{
  return (__m128d)__builtin_ia32_unpcklpd((__v2df)a, (__v2df)b);
}

__inline_g int
_mm_movemask_epi8(__m128i a) noexcept
{
  return __builtin_ia32_pmovmskb128((__v16qi)a);
}

__inline_g __m128i
_mm_cvtps_epi32(__m128 a) noexcept
{
  return (__m128i)__builtin_ia32_cvtps2dq((__v4sf)a);
}

__inline_g __m128i
_mm_cvttps_epi32(__m128 a) noexcept
{
  return (__m128i)__builtin_ia32_cvttps2dq((__v4sf)a);
}

__inline_g __m128
_mm_cvtepi32_ps(__m128i a) noexcept
{
  return (__m128)__builtin_ia32_cvtdq2ps((__v4si)a);
}

__inline_g __m128i
_mm_cvtpd_epi32(__m128d a) noexcept
{
  return (__m128i)__builtin_ia32_cvtpd2dq((__v2df)a);
}

__inline_g __m128i
_mm_cvttpd_epi32(__m128d a) noexcept
{
  return (__m128i)__builtin_ia32_cvttpd2dq((__v2df)a);
}

__inline_g __m128d
_mm_cvtepi32_pd(__m128i a) noexcept
{
  return (__m128d)__builtin_ia32_cvtdq2pd((__v4si)a);
}

__inline_g __m128
_mm_cvtpd_ps(__m128d a) noexcept
{
  return (__m128)__builtin_ia32_cvtpd2ps((__v2df)a);
}

__inline_g __m128d
_mm_cvtps_pd(__m128 a) noexcept
{
  return (__m128d)__builtin_ia32_cvtps2pd((__v4sf)a);
}

__inline_g int
_mm_cvtsi128_si32(__m128i a) noexcept
{
  return __builtin_ia32_vec_ext_v4si((__v4si)a, 0);
}

__inline_g long long
_mm_cvtsi128_si64(__m128i a) noexcept
{
  return ((__v2di)a)[0];
}

__inline_g long long
_mm_cvtsi128_si64x(__m128i a) noexcept
{
  return ((__v2di)a)[0];
}

__inline_g __m128i
_mm_cvtsi32_si128(int v) noexcept
{
  return __extension__(__m128i)(__v4si){ v, 0, 0, 0 };
}

__inline_g __m128i
_mm_cvtsi64_si128(long long v) noexcept
{
  return __extension__(__m128i)(__v2di){ v, 0 };
}

__inline_g __m128i
_mm_cvtsi64x_si128(long long v) noexcept
{
  return __extension__(__m128i)(__v2di){ v, 0 };
}

__inline_g int
_mm_cvtss_si32(__m128 a) noexcept
{
  return __builtin_ia32_cvtss2si((__v4sf)a);
}

__inline_g long long
_mm_cvtss_si64(__m128 a) noexcept
{
  return __builtin_ia32_cvtss2si64((__v4sf)a);
}

__inline_g __m128
_mm_cvtsi32_ss(__m128 a, int v) noexcept
{
  return (__m128)__builtin_ia32_cvtsi2ss((__v4sf)a, v);
}

__inline_g __m128
_mm_cvtsi64_ss(__m128 a, long long v) noexcept
{
  return (__m128)__builtin_ia32_cvtsi642ss((__v4sf)a, v);
}

__inline_g int
_mm_cvtsd_si32(__m128d a) noexcept
{
  return __builtin_ia32_cvtsd2si((__v2df)a);
}

__inline_g long long
_mm_cvtsd_si64(__m128d a) noexcept
{
  return __builtin_ia32_cvtsd2si64((__v2df)a);
}

__inline_g __m128d
_mm_cvtsi32_sd(__m128d a, int v) noexcept
{
  return (__m128d)__builtin_ia32_cvtsi2sd((__v2df)a, v);
}

__inline_g __m128d
_mm_cvtsi64_sd(__m128d a, long long v) noexcept
{
  return (__m128d)__builtin_ia32_cvtsi642sd((__v2df)a, v);
}

__inline_g __m128d
_mm_cvtss_sd(__m128d a, __m128 b) noexcept
{
  return (__m128d)__builtin_ia32_cvtss2sd((__v2df)a, (__v4sf)b);
}

__inline_g __m128
_mm_cvtsd_ss(__m128 a, __m128d b) noexcept
{
  return (__m128)__builtin_ia32_cvtsd2ss((__v4sf)a, (__v2df)b);
}

#undef __inline_g
#undef __inline_g_T

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)

#define __inject_i(name) using ::micron::simd::__bits::name

// SSE
__inject_i(_mm_setzero_ps);
__inject_i(_mm_set1_ps);
__inject_i(_mm_set_ps);
__inject_i(_mm_set_ps1);
__inject_i(_mm_setr_ps);
__inject_i(_mm_load_ps);
__inject_i(_mm_loadu_ps);
__inject_i(_mm_load_ps1);
__inject_i(_mm_load1_ps);
__inject_i(_mm_load_ss);
__inject_i(_mm_loadl_pi);
__inject_i(_mm_loadh_pi);
__inject_i(_mm_storel_pi);
__inject_i(_mm_storeh_pi);
__inject_i(_mm_store_ps);
__inject_i(_mm_storeu_ps);
__inject_i(_mm_store_ps1);
__inject_i(_mm_store1_ps);
__inject_i(_mm_store_ss);
__inject_i(_mm_cvtss_f32);
__inject_i(_mm_add_ps);
__inject_i(_mm_sub_ps);
__inject_i(_mm_mul_ps);
__inject_i(_mm_div_ps);
__inject_i(_mm_min_ps);
__inject_i(_mm_max_ps);
__inject_i(_mm_sqrt_ps);
__inject_i(_mm_rcp_ps);
__inject_i(_mm_rsqrt_ps);
__inject_i(_mm_add_ss);
__inject_i(_mm_sub_ss);
__inject_i(_mm_mul_ss);
__inject_i(_mm_div_ss);
__inject_i(_mm_min_ss);
__inject_i(_mm_max_ss);
__inject_i(_mm_sqrt_ss);
__inject_i(_mm_and_ps);
__inject_i(_mm_andnot_ps);
__inject_i(_mm_or_ps);
__inject_i(_mm_xor_ps);
__inject_i(_mm_cmpeq_ps);
__inject_i(_mm_cmplt_ps);
__inject_i(_mm_cmple_ps);
__inject_i(_mm_cmpgt_ps);
__inject_i(_mm_cmpge_ps);
__inject_i(_mm_cmpneq_ps);
__inject_i(_mm_cmpnlt_ps);
__inject_i(_mm_cmpnle_ps);
__inject_i(_mm_cmpngt_ps);
__inject_i(_mm_cmpnge_ps);
__inject_i(_mm_cmpord_ps);
__inject_i(_mm_cmpunord_ps);
__inject_i(_mm_movemask_ps);

// SSE2
__inject_i(_mm_setzero_pd);
__inject_i(_mm_set1_pd);
__inject_i(_mm_set_pd);
__inject_i(_mm_set_pd1);
__inject_i(_mm_setr_pd);
__inject_i(_mm_set_sd);
__inject_i(_mm_load_pd);
__inject_i(_mm_loadu_pd);
__inject_i(_mm_load1_pd);
__inject_i(_mm_load_pd1);
__inject_i(_mm_load_sd);
__inject_i(_mm_loadr_pd);
__inject_i(_mm_loadl_pd);
__inject_i(_mm_loadh_pd);
__inject_i(_mm_storel_pd);
__inject_i(_mm_storeh_pd);
__inject_i(_mm_store_pd);
__inject_i(_mm_storeu_pd);
__inject_i(_mm_store1_pd);
__inject_i(_mm_store_pd1);
__inject_i(_mm_store_sd);
__inject_i(_mm_cvtsd_f64);
__inject_i(_mm_add_pd);
__inject_i(_mm_sub_pd);
__inject_i(_mm_mul_pd);
__inject_i(_mm_div_pd);
__inject_i(_mm_min_pd);
__inject_i(_mm_max_pd);
__inject_i(_mm_sqrt_pd);
__inject_i(_mm_add_sd);
__inject_i(_mm_sub_sd);
__inject_i(_mm_mul_sd);
__inject_i(_mm_div_sd);
__inject_i(_mm_min_sd);
__inject_i(_mm_max_sd);
__inject_i(_mm_sqrt_sd);
__inject_i(_mm_and_pd);
__inject_i(_mm_andnot_pd);
__inject_i(_mm_or_pd);
__inject_i(_mm_xor_pd);
__inject_i(_mm_cmpeq_pd);
__inject_i(_mm_cmplt_pd);
__inject_i(_mm_cmple_pd);
__inject_i(_mm_cmpgt_pd);
__inject_i(_mm_cmpge_pd);
__inject_i(_mm_cmpneq_pd);
__inject_i(_mm_cmpnlt_pd);
__inject_i(_mm_cmpnle_pd);
__inject_i(_mm_cmpngt_pd);
__inject_i(_mm_cmpnge_pd);
__inject_i(_mm_cmpord_pd);
__inject_i(_mm_cmpunord_pd);
__inject_i(_mm_movemask_pd);

// SSE2
__inject_i(_mm_setzero_si128);
__inject_i(_mm_set1_epi8);
__inject_i(_mm_set1_epi16);
__inject_i(_mm_set1_epi32);
__inject_i(_mm_set1_epi64);
__inject_i(_mm_set1_epi64x);
__inject_i(_mm_set_epi8);
__inject_i(_mm_set_epi16);
__inject_i(_mm_set_epi32);
__inject_i(_mm_set_epi64x);
__inject_i(_mm_setr_epi8);
__inject_i(_mm_setr_epi16);
__inject_i(_mm_setr_epi32);
__inject_i(_mm_load_si128);
__inject_i(_mm_loadu_si128);
__inject_i(_mm_loadl_epi64);
__inject_i(_mm_store_si128);
__inject_i(_mm_storeu_si128);
__inject_i(_mm_storel_epi64);
__inject_i(_mm_stream_si128);
__inject_i(_mm_stream_pd);
__inject_i(_mm_stream_ps);
__inject_i(_mm_stream_si32);
__inject_i(_mm_stream_si64);
__inject_i(_mm_castpd_ps);
__inject_i(_mm_castpd_si128);
__inject_i(_mm_castps_pd);
__inject_i(_mm_castps_si128);
__inject_i(_mm_castsi128_ps);
__inject_i(_mm_castsi128_pd);
__inject_i(_mm_add_epi8);
__inject_i(_mm_add_epi16);
__inject_i(_mm_add_epi32);
__inject_i(_mm_add_epi64);
__inject_i(_mm_sub_epi8);
__inject_i(_mm_sub_epi16);
__inject_i(_mm_sub_epi32);
__inject_i(_mm_sub_epi64);
__inject_i(_mm_adds_epi8);
__inject_i(_mm_adds_epi16);
__inject_i(_mm_adds_epu8);
__inject_i(_mm_adds_epu16);
__inject_i(_mm_subs_epi8);
__inject_i(_mm_subs_epi16);
__inject_i(_mm_subs_epu8);
__inject_i(_mm_subs_epu16);
__inject_i(_mm_mullo_epi16);
__inject_i(_mm_mulhi_epi16);
__inject_i(_mm_mulhi_epu16);
__inject_i(_mm_mul_epu32);
__inject_i(_mm_madd_epi16);
__inject_i(_mm_avg_epu8);
__inject_i(_mm_avg_epu16);
__inject_i(_mm_min_epu8);
__inject_i(_mm_max_epu8);
__inject_i(_mm_min_epi16);
__inject_i(_mm_max_epi16);
__inject_i(_mm_sad_epu8);
__inject_i(_mm_and_si128);
__inject_i(_mm_andnot_si128);
__inject_i(_mm_or_si128);
__inject_i(_mm_xor_si128);
__inject_i(_mm_cmpeq_epi8);
__inject_i(_mm_cmpeq_epi16);
__inject_i(_mm_cmpeq_epi32);
__inject_i(_mm_cmpgt_epi8);
__inject_i(_mm_cmpgt_epi16);
__inject_i(_mm_cmpgt_epi32);
__inject_i(_mm_cmplt_epi8);
__inject_i(_mm_cmplt_epi16);
__inject_i(_mm_cmplt_epi32);
__inject_i(_mm_slli_epi16);
__inject_i(_mm_slli_epi32);
__inject_i(_mm_slli_epi64);
__inject_i(_mm_srli_epi16);
__inject_i(_mm_srli_epi32);
__inject_i(_mm_srli_epi64);
__inject_i(_mm_srai_epi16);
__inject_i(_mm_srai_epi32);
__inject_i(_mm_sll_epi16);
__inject_i(_mm_sll_epi32);
__inject_i(_mm_sll_epi64);
__inject_i(_mm_srl_epi16);
__inject_i(_mm_srl_epi32);
__inject_i(_mm_srl_epi64);
__inject_i(_mm_sra_epi16);
__inject_i(_mm_sra_epi32);
__inject_i(_mm_packs_epi16);
__inject_i(_mm_packs_epi32);
__inject_i(_mm_packus_epi16);
__inject_i(_mm_unpackhi_epi8);
__inject_i(_mm_unpackhi_epi16);
__inject_i(_mm_unpackhi_epi32);
__inject_i(_mm_unpackhi_epi64);
__inject_i(_mm_unpacklo_epi8);
__inject_i(_mm_unpacklo_epi16);
__inject_i(_mm_unpacklo_epi32);
__inject_i(_mm_unpacklo_epi64);
__inject_i(_mm_unpackhi_ps);
__inject_i(_mm_unpacklo_ps);
__inject_i(_mm_unpackhi_pd);
__inject_i(_mm_unpacklo_pd);
__inject_i(_mm_movehl_ps);
__inject_i(_mm_movelh_ps);
__inject_i(_mm_movemask_epi8);
__inject_i(_mm_cvtps_epi32);
__inject_i(_mm_cvttps_epi32);
__inject_i(_mm_cvtepi32_ps);
__inject_i(_mm_cvtpd_epi32);
__inject_i(_mm_cvttpd_epi32);
__inject_i(_mm_cvtepi32_pd);
__inject_i(_mm_cvtpd_ps);
__inject_i(_mm_cvtps_pd);
__inject_i(_mm_cvtsi128_si32);
__inject_i(_mm_cvtsi128_si64);
__inject_i(_mm_cvtsi128_si64x);
__inject_i(_mm_cvtsi32_si128);
__inject_i(_mm_cvtsi64_si128);
__inject_i(_mm_cvtsi64x_si128);
__inject_i(_mm_cvtss_si32);
__inject_i(_mm_cvtss_si64);
__inject_i(_mm_cvtsi32_ss);
__inject_i(_mm_cvtsi64_ss);
__inject_i(_mm_cvtsd_si32);
__inject_i(_mm_cvtsd_si64);
__inject_i(_mm_cvtsi32_sd);
__inject_i(_mm_cvtsi64_sd);
__inject_i(_mm_cvtss_sd);
__inject_i(_mm_cvtsd_ss);

#undef __inject_i

#endif
