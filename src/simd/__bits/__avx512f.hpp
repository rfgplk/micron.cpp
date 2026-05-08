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
#error "__avx512f.hpp included on a non-x86 build"
#endif

// freestanding AVX-512F [avx512fintrin.h]

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("avx512f")]] static inline

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sets

__inline_g __m512
_mm512_setzero_ps() noexcept
{
  return __extension__(__m512){ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
}

__inline_g __m512
_mm512_setzero() noexcept
{
  return _mm512_setzero_ps();
}

__inline_g __m512d
_mm512_setzero_pd() noexcept
{
  return __extension__(__m512d){ 0, 0, 0, 0, 0, 0, 0, 0 };
}

__inline_g __m512i
_mm512_setzero_si512() noexcept
{
  return __extension__(__m512i)(__v8di){ 0, 0, 0, 0, 0, 0, 0, 0 };
}

__inline_g __m512i
_mm512_setzero_epi32() noexcept
{
  return _mm512_setzero_si512();
}

__inline_g __m512
_mm512_set1_ps(float v) noexcept
{
  return __extension__(__m512){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g __m512d
_mm512_set1_pd(double v) noexcept
{
  return __extension__(__m512d){ v, v, v, v, v, v, v, v };
}

__inline_g __m512i
_mm512_set1_epi8(char v) noexcept
{
  return __extension__(__m512i)(__v64qi){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v,
                                          v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g __m512i
_mm512_set1_epi16(short v) noexcept
{
  return __extension__(__m512i)(__v32hi){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g __m512i
_mm512_set1_epi32(int v) noexcept
{
  return __extension__(__m512i)(__v16si){ v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

__inline_g __m512i
_mm512_set1_epi64(long long v) noexcept
{
  return __extension__(__m512i)(__v8di){ v, v, v, v, v, v, v, v };
}

__inline_g __m512
_mm512_set_ps(float v15, float v14, float v13, float v12, float v11, float v10, float v9, float v8, float v7, float v6, float v5, float v4,
              float v3, float v2, float v1, float v0) noexcept
{
  return __extension__(__m512){ v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15 };
}

__inline_g __m512d
_mm512_set_pd(double v7, double v6, double v5, double v4, double v3, double v2, double v1, double v0) noexcept
{
  return __extension__(__m512d){ v0, v1, v2, v3, v4, v5, v6, v7 };
}

__inline_g __m512i
_mm512_set_epi64(long long v7, long long v6, long long v5, long long v4, long long v3, long long v2, long long v1, long long v0) noexcept
{
  return __extension__(__m512i)(__v8di){ v0, v1, v2, v3, v4, v5, v6, v7 };
}

__inline_g __m512i
_mm512_set_epi32(int v15, int v14, int v13, int v12, int v11, int v10, int v9, int v8, int v7, int v6, int v5, int v4, int v3, int v2,
                 int v1, int v0) noexcept
{
  return __extension__(__m512i)(__v16si){ v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15 };
}

__inline_g __m512i
_mm512_set_epi16(short v31, short v30, short v29, short v28, short v27, short v26, short v25, short v24, short v23, short v22, short v21,
                 short v20, short v19, short v18, short v17, short v16, short v15, short v14, short v13, short v12, short v11, short v10,
                 short v9, short v8, short v7, short v6, short v5, short v4, short v3, short v2, short v1, short v0) noexcept
{
  return __extension__(__m512i)(__v32hi){ v0,  v1,  v2,  v3,  v4,  v5,  v6,  v7,  v8,  v9,  v10, v11, v12, v13, v14, v15,
                                          v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31 };
}

__inline_g __m512i
_mm512_set_epi8(char v63, char v62, char v61, char v60, char v59, char v58, char v57, char v56, char v55, char v54, char v53, char v52,
                char v51, char v50, char v49, char v48, char v47, char v46, char v45, char v44, char v43, char v42, char v41, char v40,
                char v39, char v38, char v37, char v36, char v35, char v34, char v33, char v32, char v31, char v30, char v29, char v28,
                char v27, char v26, char v25, char v24, char v23, char v22, char v21, char v20, char v19, char v18, char v17, char v16,
                char v15, char v14, char v13, char v12, char v11, char v10, char v9, char v8, char v7, char v6, char v5, char v4, char v3,
                char v2, char v1, char v0) noexcept
{
  return __extension__(__m512i)(__v64qi){ v0,  v1,  v2,  v3,  v4,  v5,  v6,  v7,  v8,  v9,  v10, v11, v12, v13, v14, v15,
                                          v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31,
                                          v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47,
                                          v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63 };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// loads

__inline_g __m512
_mm512_load_ps(const void *p) noexcept
{
  return *(const __m512 *)p;
}

__inline_g __m512
_mm512_loadu_ps(const void *p) noexcept
{
  return *(const __m512_u *)p;
}

__inline_g __m512d
_mm512_load_pd(const void *p) noexcept
{
  return *(const __m512d *)p;
}

__inline_g __m512d
_mm512_loadu_pd(const void *p) noexcept
{
  return *(const __m512d_u *)p;
}

__inline_g __m512i
_mm512_load_si512(const void *p) noexcept
{
  return *(const __m512i *)p;
}

__inline_g __m512i
_mm512_loadu_si512(const void *p) noexcept
{
  return *(const __m512i_u *)p;
}

__inline_g __m512i
_mm512_load_epi32(const void *p) noexcept
{
  return *(const __m512i *)p;
}

__inline_g __m512i
_mm512_load_epi64(const void *p) noexcept
{
  return *(const __m512i *)p;
}

__inline_g __m512i
_mm512_loadu_epi32(const void *p) noexcept
{
  return *(const __m512i_u *)p;
}

__inline_g __m512i
_mm512_loadu_epi64(const void *p) noexcept
{
  return *(const __m512i_u *)p;
}

__inline_g void
_mm512_store_ps(void *p, __m512 v) noexcept
{
  *(__m512 *)p = v;
}

__inline_g void
_mm512_storeu_ps(void *p, __m512 v) noexcept
{
  *(__m512_u *)p = v;
}

__inline_g void
_mm512_store_pd(void *p, __m512d v) noexcept
{
  *(__m512d *)p = v;
}

__inline_g void
_mm512_storeu_pd(void *p, __m512d v) noexcept
{
  *(__m512d_u *)p = v;
}

__inline_g void
_mm512_store_si512(void *p, __m512i v) noexcept
{
  *(__m512i *)p = v;
}

__inline_g void
_mm512_storeu_si512(void *p, __m512i v) noexcept
{
  *(__m512i_u *)p = v;
}

__inline_g void
_mm512_store_epi32(void *p, __m512i v) noexcept
{
  *(__m512i *)p = v;
}

__inline_g void
_mm512_store_epi64(void *p, __m512i v) noexcept
{
  *(__m512i *)p = v;
}

__inline_g void
_mm512_storeu_epi32(void *p, __m512i v) noexcept
{
  *(__m512i_u *)p = v;
}

__inline_g void
_mm512_storeu_epi64(void *p, __m512i v) noexcept
{
  *(__m512i_u *)p = v;
}

__inline_g void
_mm512_stream_ps(void *p, __m512 v) noexcept
{
  __builtin_ia32_movntps512((float *)p, (__v16sf)v);
}

__inline_g void
_mm512_stream_pd(void *p, __m512d v) noexcept
{
  __builtin_ia32_movntpd512((double *)p, (__v8df)v);
}

__inline_g void
_mm512_stream_si512(void *p, __m512i v) noexcept
{
  __builtin_ia32_movntdq512((__v8di *)p, (__v8di)v);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// casts

__inline_g __m512d
_mm512_castps_pd(__m512 a) noexcept
{
  return (__m512d)a;
}

__inline_g __m512i
_mm512_castps_si512(__m512 a) noexcept
{
  return (__m512i)a;
}

__inline_g __m512
_mm512_castpd_ps(__m512d a) noexcept
{
  return (__m512)a;
}

__inline_g __m512i
_mm512_castpd_si512(__m512d a) noexcept
{
  return (__m512i)a;
}

__inline_g __m512
_mm512_castsi512_ps(__m512i a) noexcept
{
  return (__m512)a;
}

__inline_g __m512d
_mm512_castsi512_pd(__m512i a) noexcept
{
  return (__m512d)a;
}

__inline_g __m512
_mm512_broadcastss_ps(__m128 a) noexcept
{
  return (__m512)__builtin_ia32_broadcastss512((__v4sf)a, (__v16sf){ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, (__mmask16)-1);
}

__inline_g __m512d
_mm512_broadcastsd_pd(__m128d a) noexcept
{
  return (__m512d)__builtin_ia32_broadcastsd512((__v2df)a, (__v8df){ 0, 0, 0, 0, 0, 0, 0, 0 }, (__mmask8)-1);
}

__inline_g __m128
_mm512_castps512_ps128(__m512 a) noexcept
{
  return (__m128)__builtin_shufflevector((__v16sf)a, (__v16sf)a, 0, 1, 2, 3);
}

__inline_g __m256
_mm512_castps512_ps256(__m512 a) noexcept
{
  return (__m256)__builtin_shufflevector((__v16sf)a, (__v16sf)a, 0, 1, 2, 3, 4, 5, 6, 7);
}

__inline_g __m128d
_mm512_castpd512_pd128(__m512d a) noexcept
{
  return (__m128d)__builtin_shufflevector((__v8df)a, (__v8df)a, 0, 1);
}

__inline_g __m256d
_mm512_castpd512_pd256(__m512d a) noexcept
{
  return (__m256d)__builtin_shufflevector((__v8df)a, (__v8df)a, 0, 1, 2, 3);
}

__inline_g __m128i
_mm512_castsi512_si128(__m512i a) noexcept
{
  return (__m128i)__builtin_shufflevector((__v8di)a, (__v8di)a, 0, 1);
}

__inline_g __m256i
_mm512_castsi512_si256(__m512i a) noexcept
{
  return (__m256i)__builtin_shufflevector((__v8di)a, (__v8di)a, 0, 1, 2, 3);
}

__inline_g __m512
_mm512_castps128_ps512(__m128 a) noexcept
{
  return (__m512)__builtin_shufflevector((__v4sf)a, (__v4sf){ 0, 0, 0, 0 }, 0, 1, 2, 3, 4, 5, 6, 7, 4, 5, 6, 7, 4, 5, 6, 7);
}

__inline_g __m512
_mm512_castps256_ps512(__m256 a) noexcept
{
  return (__m512)__builtin_shufflevector((__v8sf)a, (__v8sf){ 0, 0, 0, 0, 0, 0, 0, 0 }, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                                         15);
}

__inline_g __m512d
_mm512_castpd128_pd512(__m128d a) noexcept
{
  return (__m512d)__builtin_shufflevector((__v2df)a, (__v2df){ 0, 0 }, 0, 1, 2, 3, 2, 3, 2, 3);
}

__inline_g __m512d
_mm512_castpd256_pd512(__m256d a) noexcept
{
  return (__m512d)__builtin_shufflevector((__v4df)a, (__v4df){ 0, 0, 0, 0 }, 0, 1, 2, 3, 4, 5, 6, 7);
}

__inline_g __m512i
_mm512_castsi128_si512(__m128i a) noexcept
{
  return (__m512i)__builtin_shufflevector((__v2di)a, (__v2di){ 0, 0 }, 0, 1, 2, 3, 2, 3, 2, 3);
}

__inline_g __m512i
_mm512_castsi256_si512(__m256i a) noexcept
{
  return (__m512i)__builtin_shufflevector((__v4di)a, (__v4di){ 0, 0, 0, 0 }, 0, 1, 2, 3, 4, 5, 6, 7);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arithmetic

__inline_g __m512
_mm512_add_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)((__v16sf)a + (__v16sf)b);
}

__inline_g __m512
_mm512_sub_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)((__v16sf)a - (__v16sf)b);
}

__inline_g __m512
_mm512_mul_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)((__v16sf)a * (__v16sf)b);
}

__inline_g __m512
_mm512_div_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)((__v16sf)a / (__v16sf)b);
}

__inline_g __m512d
_mm512_add_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)((__v8df)a + (__v8df)b);
}

__inline_g __m512d
_mm512_sub_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)((__v8df)a - (__v8df)b);
}

__inline_g __m512d
_mm512_mul_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)((__v8df)a * (__v8df)b);
}

__inline_g __m512d
_mm512_div_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)((__v8df)a / (__v8df)b);
}

__inline_g __m512
_mm512_min_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)__builtin_ia32_minps512_mask((__v16sf)a, (__v16sf)b, (__v16sf)_mm512_setzero_ps(), (__mmask16)-1, 4);
}

__inline_g __m512
_mm512_max_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)__builtin_ia32_maxps512_mask((__v16sf)a, (__v16sf)b, (__v16sf)_mm512_setzero_ps(), (__mmask16)-1, 4);
}

__inline_g __m512d
_mm512_min_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)__builtin_ia32_minpd512_mask((__v8df)a, (__v8df)b, (__v8df)_mm512_setzero_pd(), (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_max_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)__builtin_ia32_maxpd512_mask((__v8df)a, (__v8df)b, (__v8df)_mm512_setzero_pd(), (__mmask8)-1, 4);
}

__inline_g __m512
_mm512_sqrt_ps(__m512 a) noexcept
{
  return (__m512)__builtin_ia32_sqrtps512_mask((__v16sf)a, (__v16sf)_mm512_setzero_ps(), (__mmask16)-1, 4);
}

__inline_g __m512d
_mm512_sqrt_pd(__m512d a) noexcept
{
  return (__m512d)__builtin_ia32_sqrtpd512_mask((__v8df)a, (__v8df)_mm512_setzero_pd(), (__mmask8)-1, 4);
}

__inline_g __m512i
_mm512_add_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v16su)a + (__v16su)b);
}

__inline_g __m512i
_mm512_add_epi64(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v8du)a + (__v8du)b);
}

__inline_g __m512i
_mm512_sub_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v16su)a - (__v16su)b);
}

__inline_g __m512i
_mm512_sub_epi64(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v8du)a - (__v8du)b);
}

__inline_g __m512i
_mm512_mullo_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v16su)a * (__v16su)b);
}

__inline_g __m512i
_mm512_mul_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmuldq512_mask((__v16si)a, (__v16si)b, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_mul_epu32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmuludq512_mask((__v16si)a, (__v16si)b, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_min_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminsd512_mask((__v16si)a, (__v16si)b, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_max_epi32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxsd512_mask((__v16si)a, (__v16si)b, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_min_epu32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminud512_mask((__v16si)a, (__v16si)b, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_max_epu32(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxud512_mask((__v16si)a, (__v16si)b, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_min_epi64(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminsq512_mask((__v8di)a, (__v8di)b, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_max_epi64(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxsq512_mask((__v8di)a, (__v8di)b, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_min_epu64(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pminuq512_mask((__v8di)a, (__v8di)b, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_max_epu64(__m512i a, __m512i b) noexcept
{
  return (__m512i)__builtin_ia32_pmaxuq512_mask((__v8di)a, (__v8di)b, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_abs_epi32(__m512i a) noexcept
{
  return (__m512i)__builtin_ia32_pabsd512_mask((__v16si)a, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_abs_epi64(__m512i a) noexcept
{
  return (__m512i)__builtin_ia32_pabsq512_mask((__v8di)a, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise

__inline_g __m512
_mm512_and_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)((__v16su)(__v16sf)a & (__v16su)(__v16sf)b);
}

__inline_g __m512
_mm512_or_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)((__v16su)(__v16sf)a | (__v16su)(__v16sf)b);
}

__inline_g __m512
_mm512_xor_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)((__v16su)(__v16sf)a ^ (__v16su)(__v16sf)b);
}

__inline_g __m512
_mm512_andnot_ps(__m512 a, __m512 b) noexcept
{
  return (__m512)(~(__v16su)(__v16sf)a & (__v16su)(__v16sf)b);
}

__inline_g __m512d
_mm512_and_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)((__v8du)(__v8df)a & (__v8du)(__v8df)b);
}

__inline_g __m512d
_mm512_or_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)((__v8du)(__v8df)a | (__v8du)(__v8df)b);
}

__inline_g __m512d
_mm512_xor_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)((__v8du)(__v8df)a ^ (__v8du)(__v8df)b);
}

__inline_g __m512d
_mm512_andnot_pd(__m512d a, __m512d b) noexcept
{
  return (__m512d)(~(__v8du)(__v8df)a & (__v8du)(__v8df)b);
}

__inline_g __m512i
_mm512_and_si512(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v8du)a & (__v8du)b);
}

__inline_g __m512i
_mm512_or_si512(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v8du)a | (__v8du)b);
}

__inline_g __m512i
_mm512_xor_si512(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v8du)a ^ (__v8du)b);
}

__inline_g __m512i
_mm512_andnot_si512(__m512i a, __m512i b) noexcept
{
  return (__m512i)(~(__v8du)a & (__v8du)b);
}

__inline_g __m512i
_mm512_and_epi32(__m512i a, __m512i b) noexcept
{
  return _mm512_and_si512(a, b);
}

__inline_g __m512i
_mm512_or_epi32(__m512i a, __m512i b) noexcept
{
  return _mm512_or_si512(a, b);
}

__inline_g __m512i
_mm512_xor_epi32(__m512i a, __m512i b) noexcept
{
  return _mm512_xor_si512(a, b);
}

__inline_g __m512i
_mm512_and_epi64(__m512i a, __m512i b) noexcept
{
  return _mm512_and_si512(a, b);
}

__inline_g __m512i
_mm512_or_epi64(__m512i a, __m512i b) noexcept
{
  return _mm512_or_si512(a, b);
}

__inline_g __m512i
_mm512_xor_epi64(__m512i a, __m512i b) noexcept
{
  return _mm512_xor_si512(a, b);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compares

__inline_g __mmask16
_mm512_cmpeq_epi32_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask16)__builtin_ia32_cmpd512_mask((__v16si)a, (__v16si)b, 0, (__mmask16)-1);
}

__inline_g __mmask16
_mm512_cmplt_epi32_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask16)__builtin_ia32_cmpd512_mask((__v16si)a, (__v16si)b, 1, (__mmask16)-1);
}

__inline_g __mmask16
_mm512_cmple_epi32_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask16)__builtin_ia32_cmpd512_mask((__v16si)a, (__v16si)b, 2, (__mmask16)-1);
}

__inline_g __mmask16
_mm512_cmpneq_epi32_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask16)__builtin_ia32_cmpd512_mask((__v16si)a, (__v16si)b, 4, (__mmask16)-1);
}

__inline_g __mmask16
_mm512_cmpnlt_epi32_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask16)__builtin_ia32_cmpd512_mask((__v16si)a, (__v16si)b, 5, (__mmask16)-1);
}

__inline_g __mmask16
_mm512_cmpgt_epi32_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask16)__builtin_ia32_cmpd512_mask((__v16si)a, (__v16si)b, 6, (__mmask16)-1);
}

__inline_g __mmask16
_mm512_cmpge_epi32_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask16)__builtin_ia32_cmpd512_mask((__v16si)a, (__v16si)b, 5, (__mmask16)-1);
}

__inline_g __mmask8
_mm512_cmpeq_epi64_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask8)__builtin_ia32_cmpq512_mask((__v8di)a, (__v8di)b, 0, (__mmask8)-1);
}

__inline_g __mmask8
_mm512_cmplt_epi64_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask8)__builtin_ia32_cmpq512_mask((__v8di)a, (__v8di)b, 1, (__mmask8)-1);
}

__inline_g __mmask8
_mm512_cmpgt_epi64_mask(__m512i a, __m512i b) noexcept
{
  return (__mmask8)__builtin_ia32_cmpq512_mask((__v8di)a, (__v8di)b, 6, (__mmask8)-1);
}

__inline_g __mmask16
_mm512_cmp_ps_mask(__m512 a, __m512 b, int p) noexcept
{
  return (__mmask16)__builtin_ia32_cmpps512_mask((__v16sf)a, (__v16sf)b, p, (__mmask16)-1, 4);
}

__inline_g __mmask8
_mm512_cmp_pd_mask(__m512d a, __m512d b, int p) noexcept
{
  return (__mmask8)__builtin_ia32_cmppd512_mask((__v8df)a, (__v8df)b, p, (__mmask8)-1, 4);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shifts & converts

__inline_g __m512i
_mm512_slli_epi32(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_pslldi512_mask((__v16si)a, n, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_slli_epi64(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psllqi512_mask((__v8di)a, n, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_srli_epi32(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psrldi512_mask((__v16si)a, n, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_srli_epi64(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psrlqi512_mask((__v8di)a, n, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_srai_epi32(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psradi512_mask((__v16si)a, n, (__v16si)_mm512_setzero_si512(), (__mmask16)-1);
}

__inline_g __m512i
_mm512_srai_epi64(__m512i a, int n) noexcept
{
  return (__m512i)__builtin_ia32_psraqi512_mask((__v8di)a, n, (__v8di)_mm512_setzero_si512(), (__mmask8)-1);
}

__inline_g __m512i
_mm512_cvtps_epi32(__m512 a) noexcept
{
  return (__m512i)__builtin_ia32_cvtps2dq512_mask((__v16sf)a, (__v16si)_mm512_setzero_si512(), (__mmask16)-1, 4);
}

__inline_g __m512i
_mm512_cvttps_epi32(__m512 a) noexcept
{
  return (__m512i)__builtin_ia32_cvttps2dq512_mask((__v16sf)a, (__v16si)_mm512_setzero_si512(), (__mmask16)-1, 4);
}

__inline_g __m512
_mm512_cvtepi32_ps(__m512i a) noexcept
{
  return (__m512)__builtin_ia32_cvtdq2ps512_mask((__v16si)a, (__v16sf)_mm512_setzero_ps(), (__mmask16)-1, 4);
}

__inline_g __m256i
_mm512_cvtpd_epi32(__m512d a) noexcept
{
  return (__m256i)__builtin_ia32_cvtpd2dq512_mask((__v8df)a, (__v8si){ 0, 0, 0, 0, 0, 0, 0, 0 }, (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_cvtepi32_pd(__m256i a) noexcept
{
  return (__m512d)__builtin_ia32_cvtdq2pd512_mask((__v8si)a, (__v8df)_mm512_setzero_pd(), (__mmask8)-1);
}

__inline_g __m256
_mm512_cvtpd_ps(__m512d a) noexcept
{
  return (__m256)__builtin_ia32_cvtpd2ps512_mask((__v8df)a, (__v8sf){ 0, 0, 0, 0, 0, 0, 0, 0 }, (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_cvtps_pd(__m256 a) noexcept
{
  return (__m512d)__builtin_ia32_cvtps2pd512_mask((__v8sf)a, (__v8df)_mm512_setzero_pd(), (__mmask8)-1, 4);
}

// %%%%%%%%%%%%%%%%%%
// masks

__inline_g __mmask16
_mm512_kand(__mmask16 a, __mmask16 b) noexcept
{
  return a & b;
}

__inline_g __mmask16
_mm512_kor(__mmask16 a, __mmask16 b) noexcept
{
  return a | b;
}

__inline_g __mmask16
_mm512_kxor(__mmask16 a, __mmask16 b) noexcept
{
  return a ^ b;
}

__inline_g __mmask16
_mm512_knot(__mmask16 a) noexcept
{
  return ~a;
}

__inline_g __mmask16
_mm512_kandn(__mmask16 a, __mmask16 b) noexcept
{
  return ~a & b;
}

__inline_g __mmask16
_kand_mask16(__mmask16 a, __mmask16 b) noexcept
{
  return a & b;
}

__inline_g __mmask16
_kor_mask16(__mmask16 a, __mmask16 b) noexcept
{
  return a | b;
}

__inline_g __mmask16
_kxor_mask16(__mmask16 a, __mmask16 b) noexcept
{
  return a ^ b;
}

__inline_g __mmask16
_knot_mask16(__mmask16 a) noexcept
{
  return ~a;
}

__inline_g __mmask8
_kand_mask8(__mmask8 a, __mmask8 b) noexcept
{
  return a & b;
}

__inline_g __mmask8
_kor_mask8(__mmask8 a, __mmask8 b) noexcept
{
  return a | b;
}

__inline_g __mmask8
_kxor_mask8(__mmask8 a, __mmask8 b) noexcept
{
  return a ^ b;
}

__inline_g __mmask8
_knot_mask8(__mmask8 a) noexcept
{
  return ~a;
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron
