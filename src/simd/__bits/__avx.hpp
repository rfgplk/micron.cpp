//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__vector_types_amd64.hpp"

#if !defined(__micron_arch_x86_any)
#error "__avx.hpp included on a non-x86 build"
#endif

// freestanding AVX [avxintrin.h]

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

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("avx")]] static inline

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sets
__inline_g __m256
_mm256_setzero_ps() noexcept
{
  return __extension__(__m256){ 0, 0, 0, 0, 0, 0, 0, 0 };
}

__inline_g __m256
_mm256_set1_ps(float v) noexcept
{
  return __extension__(__m256){ v, v, v, v, v, v, v, v };
}

__inline_g __m256
_mm256_set_ps(float v7, float v6, float v5, float v4, float v3, float v2, float v1, float v0) noexcept
{
  return __extension__(__m256){ v0, v1, v2, v3, v4, v5, v6, v7 };
}

__inline_g __m256
_mm256_setr_ps(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7) noexcept
{
  return __extension__(__m256){ v0, v1, v2, v3, v4, v5, v6, v7 };
}

__inline_g __m256d
_mm256_setzero_pd() noexcept
{
  return __extension__(__m256d){ 0, 0, 0, 0 };
}

__inline_g __m256d
_mm256_set1_pd(double v) noexcept
{
  return __extension__(__m256d){ v, v, v, v };
}

__inline_g __m256d
_mm256_set_pd(double v3, double v2, double v1, double v0) noexcept
{
  return __extension__(__m256d){ v0, v1, v2, v3 };
}

__inline_g __m256d
_mm256_setr_pd(double v0, double v1, double v2, double v3) noexcept
{
  return __extension__(__m256d){ v0, v1, v2, v3 };
}

__inline_g __m256i
_mm256_setzero_si256() noexcept
{
  return __extension__(__m256i)(__v4di){ 0, 0, 0, 0 };
}

__inline_g __m256i
_mm256_set1_epi8(char v) noexcept
{
  return __extension__(__m256i)(__v32qi){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g __m256i
_mm256_set1_epi16(short v) noexcept
{
  return __extension__(__m256i)(__v16hi){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g __m256i
_mm256_set1_epi32(int v) noexcept
{
  return __extension__(__m256i)(__v8si){ v, v, v, v, v, v, v, v };
}

__inline_g __m256i
_mm256_set1_epi64x(long long v) noexcept
{
  return __extension__(__m256i)(__v4di){ v, v, v, v };
}

__inline_g __m256i
_mm256_set_epi8(char b31, char b30, char b29, char b28, char b27, char b26, char b25, char b24, char b23, char b22, char b21, char b20,
                char b19, char b18, char b17, char b16, char b15, char b14, char b13, char b12, char b11, char b10, char b9, char b8,
                char b7, char b6, char b5, char b4, char b3, char b2, char b1, char b0) noexcept
{
  return __extension__(__m256i)(__v32qi){ b0,  b1,  b2,  b3,  b4,  b5,  b6,  b7,  b8,  b9,  b10, b11, b12, b13, b14, b15,
                                          b16, b17, b18, b19, b20, b21, b22, b23, b24, b25, b26, b27, b28, b29, b30, b31 };
}

__inline_g __m256i
_mm256_set_epi16(short w15, short w14, short w13, short w12, short w11, short w10, short w9, short w8, short w7, short w6, short w5,
                 short w4, short w3, short w2, short w1, short w0) noexcept
{
  return __extension__(__m256i)(__v16hi){ w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15 };
}

__inline_g __m256i
_mm256_set_epi32(int e7, int e6, int e5, int e4, int e3, int e2, int e1, int e0) noexcept
{
  return __extension__(__m256i)(__v8si){ e0, e1, e2, e3, e4, e5, e6, e7 };
}

__inline_g __m256i
_mm256_set_epi64x(long long e3, long long e2, long long e1, long long e0) noexcept
{
  return __extension__(__m256i)(__v4di){ e0, e1, e2, e3 };
}

__inline_g __m256i
_mm256_setr_epi8(char b0, char b1, char b2, char b3, char b4, char b5, char b6, char b7, char b8, char b9, char b10, char b11, char b12,
                 char b13, char b14, char b15, char b16, char b17, char b18, char b19, char b20, char b21, char b22, char b23, char b24,
                 char b25, char b26, char b27, char b28, char b29, char b30, char b31) noexcept
{
  return __extension__(__m256i)(__v32qi){ b0,  b1,  b2,  b3,  b4,  b5,  b6,  b7,  b8,  b9,  b10, b11, b12, b13, b14, b15,
                                          b16, b17, b18, b19, b20, b21, b22, b23, b24, b25, b26, b27, b28, b29, b30, b31 };
}

__inline_g __m256i
_mm256_setr_epi16(short w0, short w1, short w2, short w3, short w4, short w5, short w6, short w7, short w8, short w9, short w10, short w11,
                  short w12, short w13, short w14, short w15) noexcept
{
  return __extension__(__m256i)(__v16hi){ w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15 };
}

__inline_g __m256i
_mm256_setr_epi32(int e0, int e1, int e2, int e3, int e4, int e5, int e6, int e7) noexcept
{
  return __extension__(__m256i)(__v8si){ e0, e1, e2, e3, e4, e5, e6, e7 };
}

__inline_g __m256i
_mm256_setr_epi64x(long long e0, long long e1, long long e2, long long e3) noexcept
{
  return __extension__(__m256i)(__v4di){ e0, e1, e2, e3 };
}

__inline_g __m256
_mm256_set_m128(__m128 hi, __m128 lo) noexcept
{
  return (__m256)__builtin_ia32_vinsertf128_ps256((__v8sf)__builtin_ia32_ps256_ps((__v4sf)lo), (__v4sf)hi, 1);
}

__inline_g __m256i
_mm256_set_m128i(__m128i hi, __m128i lo) noexcept
{
  return (__m256i)__builtin_ia32_vinsertf128_si256((__v8si)__builtin_ia32_si256_si((__v4si)lo), (__v4si)hi, 1);
}

__inline_g __m256d
_mm256_set_m128d(__m128d hi, __m128d lo) noexcept
{
  return (__m256d)__builtin_ia32_vinsertf128_pd256((__v4df)__builtin_ia32_pd256_pd((__v2df)lo), (__v2df)hi, 1);
}

__inline_g __m256
_mm256_setr_m128(__m128 lo, __m128 hi) noexcept
{
  return _mm256_set_m128(hi, lo);
}

__inline_g __m256i
_mm256_setr_m128i(__m128i lo, __m128i hi) noexcept
{
  return _mm256_set_m128i(hi, lo);
}

__inline_g __m256d
_mm256_setr_m128d(__m128d lo, __m128d hi) noexcept
{
  return _mm256_set_m128d(hi, lo);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// loads stores

__inline_g __m256
_mm256_load_ps(const float *p) noexcept
{
  return *(const __m256 *)p;
}

__inline_g __m256
_mm256_loadu_ps(const float *p) noexcept
{
  return *(const __m256_u *)p;
}

__inline_g __m256d
_mm256_load_pd(const double *p) noexcept
{
  return *(const __m256d *)p;
}

__inline_g __m256d
_mm256_loadu_pd(const double *p) noexcept
{
  return *(const __m256d_u *)p;
}

__inline_g __m256i
_mm256_load_si256(const __m256i *p) noexcept
{
  return *p;
}

__inline_g __m256i
_mm256_loadu_si256(const __m256i_u *p) noexcept
{
  return *p;
}

__inline_g void
_mm256_store_ps(float *p, __m256 v) noexcept
{
  *(__m256 *)p = v;
}

__inline_g void
_mm256_storeu_ps(float *p, __m256 v) noexcept
{
  *(__m256_u *)p = v;
}

__inline_g void
_mm256_store_pd(double *p, __m256d v) noexcept
{
  *(__m256d *)p = v;
}

__inline_g void
_mm256_storeu_pd(double *p, __m256d v) noexcept
{
  *(__m256d_u *)p = v;
}

__inline_g void
_mm256_store_si256(__m256i *p, __m256i v) noexcept
{
  *p = v;
}

__inline_g void
_mm256_storeu_si256(__m256i_u *p, __m256i v) noexcept
{
  *p = v;
}

__inline_g void
_mm256_stream_si256(__m256i *p, __m256i v) noexcept
{
  __builtin_ia32_movntdq256((__v4di *)p, (__v4di)v);
}

__inline_g void
_mm256_stream_pd(double *p, __m256d v) noexcept
{
  __builtin_ia32_movntpd256(p, (__v4df)v);
}

__inline_g void
_mm256_stream_ps(float *p, __m256 v) noexcept
{
  __builtin_ia32_movntps256(p, (__v8sf)v);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// casts

__inline_g __m256d
_mm256_castps_pd(__m256 a) noexcept
{
  return (__m256d)a;
}

__inline_g __m256i
_mm256_castps_si256(__m256 a) noexcept
{
  return (__m256i)a;
}

__inline_g __m256
_mm256_castpd_ps(__m256d a) noexcept
{
  return (__m256)a;
}

__inline_g __m256i
_mm256_castpd_si256(__m256d a) noexcept
{
  return (__m256i)a;
}

__inline_g __m256
_mm256_castsi256_ps(__m256i a) noexcept
{
  return (__m256)a;
}

__inline_g __m256d
_mm256_castsi256_pd(__m256i a) noexcept
{
  return (__m256d)a;
}

__inline_g __m128
_mm256_castps256_ps128(__m256 a) noexcept
{
  return (__m128)__builtin_ia32_ps_ps256((__v8sf)a);
}

__inline_g __m128d
_mm256_castpd256_pd128(__m256d a) noexcept
{
  return (__m128d)__builtin_ia32_pd_pd256((__v4df)a);
}

__inline_g __m128i
_mm256_castsi256_si128(__m256i a) noexcept
{
  return (__m128i)__builtin_ia32_si_si256((__v8si)a);
}

__inline_g __m256
_mm256_castps128_ps256(__m128 a) noexcept
{
  return (__m256)__builtin_ia32_ps256_ps((__v4sf)a);
}

__inline_g __m256d
_mm256_castpd128_pd256(__m128d a) noexcept
{
  return (__m256d)__builtin_ia32_pd256_pd((__v2df)a);
}

__inline_g __m256i
_mm256_castsi128_si256(__m128i a) noexcept
{
  return (__m256i)__builtin_ia32_si256_si((__v4si)a);
}

__inline_g __m256
_mm256_zextps128_ps256(__m128 a) noexcept
{
  return (__m256)__builtin_shufflevector((__v4sf)a, (__v4sf){ 0, 0, 0, 0 }, 0, 1, 2, 3, 4, 5, 6, 7);
}

__inline_g __m256d
_mm256_zextpd128_pd256(__m128d a) noexcept
{
  return (__m256d)__builtin_shufflevector((__v2df)a, (__v2df){ 0, 0 }, 0, 1, 2, 3);
}

__inline_g __m256i
_mm256_zextsi128_si256(__m128i a) noexcept
{
  return (__m256i)__builtin_shufflevector((__v2di)a, (__v2di){ 0, 0 }, 0, 1, 2, 3);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arith
__inline_g __m256
_mm256_add_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)((__v8sf)a + (__v8sf)b);
}

__inline_g __m256
_mm256_sub_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)((__v8sf)a - (__v8sf)b);
}

__inline_g __m256
_mm256_mul_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)((__v8sf)a * (__v8sf)b);
}

__inline_g __m256
_mm256_div_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)((__v8sf)a / (__v8sf)b);
}

__inline_g __m256
_mm256_min_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_minps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_max_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_maxps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_sqrt_ps(__m256 a) noexcept
{
  return (__m256)__builtin_ia32_sqrtps256((__v8sf)a);
}

__inline_g __m256
_mm256_rcp_ps(__m256 a) noexcept
{
  return (__m256)__builtin_ia32_rcpps256((__v8sf)a);
}

__inline_g __m256
_mm256_rsqrt_ps(__m256 a) noexcept
{
  return (__m256)__builtin_ia32_rsqrtps256((__v8sf)a);
}

__inline_g __m256d
_mm256_add_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)((__v4df)a + (__v4df)b);
}

__inline_g __m256d
_mm256_sub_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)((__v4df)a - (__v4df)b);
}

__inline_g __m256d
_mm256_mul_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)((__v4df)a * (__v4df)b);
}

__inline_g __m256d
_mm256_div_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)((__v4df)a / (__v4df)b);
}

__inline_g __m256d
_mm256_min_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_minpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_max_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_maxpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_sqrt_pd(__m256d a) noexcept
{
  return (__m256d)__builtin_ia32_sqrtpd256((__v4df)a);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwises
__inline_g __m256
_mm256_and_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_andps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_andnot_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_andnps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_or_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_orps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_xor_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_xorps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256d
_mm256_and_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_andpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_andnot_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_andnpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_or_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_orpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_xor_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_xorpd256((__v4df)a, (__v4df)b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// horizontals

__inline_g __m256
_mm256_hadd_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_haddps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_hsub_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_hsubps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256
_mm256_addsub_ps(__m256 a, __m256 b) noexcept
{
  return (__m256)__builtin_ia32_addsubps256((__v8sf)a, (__v8sf)b);
}

__inline_g __m256d
_mm256_hadd_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_haddpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_hsub_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_hsubpd256((__v4df)a, (__v4df)b);
}

__inline_g __m256d
_mm256_addsub_pd(__m256d a, __m256d b) noexcept
{
  return (__m256d)__builtin_ia32_addsubpd256((__v4df)a, (__v4df)b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// broadcasts

__inline_g __m256
_mm256_broadcast_ss(const float *p) noexcept
{
  return (__m256)__builtin_ia32_vbroadcastss256(p);
}

__inline_g __m256d
_mm256_broadcast_sd(const double *p) noexcept
{
  return (__m256d)__builtin_ia32_vbroadcastsd256(p);
}

__inline_g __m128
_mm_broadcast_ss(const float *p) noexcept
{
  return (__m128)__builtin_ia32_vbroadcastss(p);
}

__inline_g __m256
_mm256_broadcast_ps(const __m128 *p) noexcept
{
  return (__m256)__builtin_ia32_vbroadcastf128_ps256((const __v4sf *)p);
}

__inline_g __m256d
_mm256_broadcast_pd(const __m128d *p) noexcept
{
  return (__m256d)__builtin_ia32_vbroadcastf128_pd256((const __v2df *)p);
}

__inline_g __m256
_mm256_movehdup_ps(__m256 a) noexcept
{
  return (__m256)__builtin_ia32_movshdup256((__v8sf)a);
}

__inline_g __m256
_mm256_moveldup_ps(__m256 a) noexcept
{
  return (__m256)__builtin_ia32_movsldup256((__v8sf)a);
}

__inline_g __m256d
_mm256_movedup_pd(__m256d a) noexcept
{
  return (__m256d)__builtin_ia32_movddup256((__v4df)a);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// movemasks and converts

__inline_g int
_mm256_movemask_ps(__m256 a) noexcept
{
  return __builtin_ia32_movmskps256((__v8sf)a);
}

__inline_g int
_mm256_movemask_pd(__m256d a) noexcept
{
  return __builtin_ia32_movmskpd256((__v4df)a);
}

__inline_g __m256i
_mm256_cvtps_epi32(__m256 a) noexcept
{
  return (__m256i)__builtin_ia32_cvtps2dq256((__v8sf)a);
}

__inline_g __m256i
_mm256_cvttps_epi32(__m256 a) noexcept
{
  return (__m256i)__builtin_ia32_cvttps2dq256((__v8sf)a);
}

__inline_g __m256
_mm256_cvtepi32_ps(__m256i a) noexcept
{
  return (__m256)__builtin_ia32_cvtdq2ps256((__v8si)a);
}

__inline_g __m128i
_mm256_cvtpd_epi32(__m256d a) noexcept
{
  return (__m128i)__builtin_ia32_cvtpd2dq256((__v4df)a);
}

__inline_g __m128i
_mm256_cvttpd_epi32(__m256d a) noexcept
{
  return (__m128i)__builtin_ia32_cvttpd2dq256((__v4df)a);
}

__inline_g __m256d
_mm256_cvtepi32_pd(__m128i a) noexcept
{
  return (__m256d)__builtin_ia32_cvtdq2pd256((__v4si)a);
}

__inline_g __m128
_mm256_cvtpd_ps(__m256d a) noexcept
{
  return (__m128)__builtin_ia32_cvtpd2ps256((__v4df)a);
}

__inline_g __m256d
_mm256_cvtps_pd(__m128 a) noexcept
{
  return (__m256d)__builtin_ia32_cvtps2pd256((__v4sf)a);
}

__inline_g float
_mm256_cvtss_f32(__m256 a) noexcept
{
  return ((__v8sf)a)[0];
}

__inline_g double
_mm256_cvtsd_f64(__m256d a) noexcept
{
  return ((__v4df)a)[0];
}

__inline_g int
_mm256_cvtsi256_si32(__m256i a) noexcept
{
  return ((__v8si)a)[0];
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tests

__inline_g int
_mm256_testz_ps(__m256 a, __m256 b) noexcept
{
  return __builtin_ia32_vtestzps256((__v8sf)a, (__v8sf)b);
}

__inline_g int
_mm256_testc_ps(__m256 a, __m256 b) noexcept
{
  return __builtin_ia32_vtestcps256((__v8sf)a, (__v8sf)b);
}

__inline_g int
_mm256_testnzc_ps(__m256 a, __m256 b) noexcept
{
  return __builtin_ia32_vtestnzcps256((__v8sf)a, (__v8sf)b);
}

__inline_g int
_mm256_testz_pd(__m256d a, __m256d b) noexcept
{
  return __builtin_ia32_vtestzpd256((__v4df)a, (__v4df)b);
}

__inline_g int
_mm256_testc_pd(__m256d a, __m256d b) noexcept
{
  return __builtin_ia32_vtestcpd256((__v4df)a, (__v4df)b);
}

__inline_g int
_mm256_testnzc_pd(__m256d a, __m256d b) noexcept
{
  return __builtin_ia32_vtestnzcpd256((__v4df)a, (__v4df)b);
}

__inline_g int
_mm256_testz_si256(__m256i a, __m256i b) noexcept
{
  return __builtin_ia32_ptestz256((__v4di)a, (__v4di)b);
}

__inline_g int
_mm256_testc_si256(__m256i a, __m256i b) noexcept
{
  return __builtin_ia32_ptestc256((__v4di)a, (__v4di)b);
}

__inline_g int
_mm256_testnzc_si256(__m256i a, __m256i b) noexcept
{
  return __builtin_ia32_ptestnzc256((__v4di)a, (__v4di)b);
}

__inline_g int
_mm_testz_ps(__m128 a, __m128 b) noexcept
{
  return __builtin_ia32_vtestzps((__v4sf)a, (__v4sf)b);
}

__inline_g int
_mm_testc_ps(__m128 a, __m128 b) noexcept
{
  return __builtin_ia32_vtestcps((__v4sf)a, (__v4sf)b);
}

__inline_g int
_mm_testnzc_ps(__m128 a, __m128 b) noexcept
{
  return __builtin_ia32_vtestnzcps((__v4sf)a, (__v4sf)b);
}

__inline_g int
_mm_testz_pd(__m128d a, __m128d b) noexcept
{
  return __builtin_ia32_vtestzpd((__v2df)a, (__v2df)b);
}

__inline_g int
_mm_testc_pd(__m128d a, __m128d b) noexcept
{
  return __builtin_ia32_vtestcpd((__v2df)a, (__v2df)b);
}

__inline_g int
_mm_testnzc_pd(__m128d a, __m128d b) noexcept
{
  return __builtin_ia32_vtestnzcpd((__v2df)a, (__v2df)b);
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name

__inject_i(_mm256_setzero_ps);
__inject_i(_mm256_set1_ps);
__inject_i(_mm256_set_ps);
__inject_i(_mm256_setr_ps);
__inject_i(_mm256_setzero_pd);
__inject_i(_mm256_set1_pd);
__inject_i(_mm256_set_pd);
__inject_i(_mm256_setr_pd);
__inject_i(_mm256_setzero_si256);
__inject_i(_mm256_set1_epi8);
__inject_i(_mm256_set1_epi16);
__inject_i(_mm256_set1_epi32);
__inject_i(_mm256_set1_epi64x);
__inject_i(_mm256_set_epi8);
__inject_i(_mm256_set_epi16);
__inject_i(_mm256_set_epi32);
__inject_i(_mm256_set_epi64x);

__inject_i(_mm256_load_ps);
__inject_i(_mm256_loadu_ps);
__inject_i(_mm256_load_pd);
__inject_i(_mm256_loadu_pd);
__inject_i(_mm256_load_si256);
__inject_i(_mm256_loadu_si256);
__inject_i(_mm256_store_ps);
__inject_i(_mm256_storeu_ps);
__inject_i(_mm256_store_pd);
__inject_i(_mm256_storeu_pd);
__inject_i(_mm256_store_si256);
__inject_i(_mm256_storeu_si256);
__inject_i(_mm256_stream_si256);
__inject_i(_mm256_stream_pd);
__inject_i(_mm256_stream_ps);

__inject_i(_mm256_castps_pd);
__inject_i(_mm256_castps_si256);
__inject_i(_mm256_castpd_ps);
__inject_i(_mm256_castpd_si256);
__inject_i(_mm256_castsi256_ps);
__inject_i(_mm256_castsi256_pd);
__inject_i(_mm256_castps256_ps128);
__inject_i(_mm256_castpd256_pd128);
__inject_i(_mm256_castsi256_si128);
__inject_i(_mm256_castps128_ps256);
__inject_i(_mm256_castpd128_pd256);
__inject_i(_mm256_castsi128_si256);
__inject_i(_mm256_zextps128_ps256);
__inject_i(_mm256_zextpd128_pd256);
__inject_i(_mm256_zextsi128_si256);

__inject_i(_mm256_add_ps);
__inject_i(_mm256_sub_ps);
__inject_i(_mm256_mul_ps);
__inject_i(_mm256_div_ps);
__inject_i(_mm256_min_ps);
__inject_i(_mm256_max_ps);
__inject_i(_mm256_sqrt_ps);
__inject_i(_mm256_rcp_ps);
__inject_i(_mm256_rsqrt_ps);
__inject_i(_mm256_add_pd);
__inject_i(_mm256_sub_pd);
__inject_i(_mm256_mul_pd);
__inject_i(_mm256_div_pd);
__inject_i(_mm256_min_pd);
__inject_i(_mm256_max_pd);
__inject_i(_mm256_sqrt_pd);

__inject_i(_mm256_and_ps);
__inject_i(_mm256_andnot_ps);
__inject_i(_mm256_or_ps);
__inject_i(_mm256_xor_ps);
__inject_i(_mm256_and_pd);
__inject_i(_mm256_andnot_pd);
__inject_i(_mm256_or_pd);
__inject_i(_mm256_xor_pd);

__inject_i(_mm256_hadd_ps);
__inject_i(_mm256_hsub_ps);
__inject_i(_mm256_addsub_ps);
__inject_i(_mm256_hadd_pd);
__inject_i(_mm256_hsub_pd);
__inject_i(_mm256_addsub_pd);

__inject_i(_mm256_broadcast_ss);
__inject_i(_mm256_broadcast_sd);
__inject_i(_mm_broadcast_ss);
__inject_i(_mm256_broadcast_ps);
__inject_i(_mm256_broadcast_pd);
__inject_i(_mm256_movehdup_ps);
__inject_i(_mm256_moveldup_ps);
__inject_i(_mm256_movedup_pd);

__inject_i(_mm256_movemask_ps);
__inject_i(_mm256_movemask_pd);

__inject_i(_mm256_cvtps_epi32);
__inject_i(_mm256_cvttps_epi32);
__inject_i(_mm256_cvtepi32_ps);
__inject_i(_mm256_cvtpd_epi32);
__inject_i(_mm256_cvttpd_epi32);
__inject_i(_mm256_cvtepi32_pd);
__inject_i(_mm256_cvtpd_ps);
__inject_i(_mm256_cvtps_pd);
__inject_i(_mm256_cvtss_f32);
__inject_i(_mm256_cvtsd_f64);
__inject_i(_mm256_cvtsi256_si32);

__inject_i(_mm256_testz_ps);
__inject_i(_mm256_testc_ps);
__inject_i(_mm256_testnzc_ps);
__inject_i(_mm256_testz_pd);
__inject_i(_mm256_testc_pd);
__inject_i(_mm256_testnzc_pd);
__inject_i(_mm256_testz_si256);
__inject_i(_mm256_testc_si256);
__inject_i(_mm256_testnzc_si256);
__inject_i(_mm_testz_ps);
__inject_i(_mm_testc_ps);
__inject_i(_mm_testnzc_ps);
__inject_i(_mm_testz_pd);
__inject_i(_mm_testc_pd);
__inject_i(_mm_testnzc_pd);

#undef __inject_i
#endif
