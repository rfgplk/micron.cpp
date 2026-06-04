//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../simd.hpp"

namespace micron
{
namespace simd
{

#if !defined(__micron_simd_svml)
// #define __micron_simd_svml 1   // requires an SVML backend; OFF by default
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// abs

template<is_simd_class T>
inline T
abs_8(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_abs_epi8(o);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_abs_epi8(o);
  } else
    static_assert(!sizeof(T), "abs_8<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
abs_16(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_abs_epi16(o);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_abs_epi16(o);
  } else
    static_assert(!sizeof(T), "abs_16<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
abs_32(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_abs_epi32(o);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_abs_epi32(o);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_abs_epi32(o);      // AVX-512F
#endif
  } else
    static_assert(!sizeof(T), "abs_32<T>: unsupported simd class width (need AVX-512F for the 512-bit path)");
}

#if defined(__micron_x86_avx512f)
[[gnu::target("avx512f")]] inline i512
abs_64(i512 &o)
{
  return _mm512_abs_epi64(o);
}      // AVX-512F
#endif

template<is_simd_class T>
inline T
abs(T &o)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return abs_8(o);
  } else if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return abs_16(o);
  } else if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return abs_32(o);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return abs_64(o);
#endif
  } else
    static_assert(!sizeof(T), "abs<T>: unsupported lane width (64-bit abs needs AVX-512F)");
}

// AVX-512F masked abs (_mm512_mask*/_maskz*)
#if defined(__micron_simd_svml)
template<typename M>
inline i512
mask_abs_32(i512 src, M k, i512 &o)
{
  return _mm512_mask_abs_epi32(src, k, o);
}

template<typename M>
inline i512
maskz_abs_32(M k, i512 &o)
{
  return _mm512_maskz_abs_epi32(k, o);
}

template<typename M>
inline i512
mask_abs_64(i512 src, M k, i512 &o)
{
  return _mm512_mask_abs_epi64(src, k, o);
}      // AVX-512F

template<typename M>
inline i512
maskz_abs_64(M k, i512 &o)
{
  return _mm512_maskz_abs_epi64(k, o);
}      // AVX-512F
#endif      // __micron_simd_svml

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sqrt

template<is_simd_type B>
inline B
sqrt(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_sqrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_sqrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_sqrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_sqrt_pd(o);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_sqrt_ps(o);      // AVX-512F
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_sqrt_pd(o);      // AVX-512F
#endif
#if defined(__micron_simd_svml)
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_sqrt_ph(o);      // AVX-512FP16
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_sqrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_sqrt_ph(o);
#endif
  } else
    static_assert(!sizeof(B), "sqrt<B>: unsupported simd type (512-bit needs AVX-512F; _Float16 paths need an SVML/FP16 backend)");
}

inline f128
sqrt_ss(f128 a)
{
  return _mm_sqrt_ss(a);
}

inline d128
sqrt_sd(d128 a, d128 b)
{
  return _mm_sqrt_sd(a, b);
}

// All masked / embedded-rounding / FP16 sqrt variants below are AVX-512(FP16)
#if defined(__micron_simd_svml)
template<is_simd_type B>
inline B
sqrt_round(B &o, int rounding)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_sqrt_round_ps(o, rounding);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_sqrt_round_pd(o, rounding);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_sqrt_round_ph(o, rounding);
  } else
    static_assert(!sizeof(B), "sqrt_round<B>: only 512-bit ps/pd/ph are supported");
}

inline f128
sqrt_round_ss(f128 a, f128 b, int r)
{
  return _mm_sqrt_round_ss(a, b, r);
}

inline d128
sqrt_round_sd(d128 a, d128 b, int r)
{
  return _mm_sqrt_round_sd(a, b, r);
}

template<is_simd_type B, typename M>
inline B
mask_sqrt(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_mask_sqrt_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_mask_sqrt_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_sqrt_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_mask_sqrt_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_mask_sqrt_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_sqrt_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_mask_sqrt_ph(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_mask_sqrt_ph(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_sqrt_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_sqrt<B>: unsupported simd type");
}

template<is_simd_type B, typename M>
inline B
maskz_sqrt(M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_maskz_sqrt_ps(k, o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_maskz_sqrt_ps(k, o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_sqrt_ps(k, o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_maskz_sqrt_pd(k, o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_maskz_sqrt_pd(k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_sqrt_pd(k, o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_maskz_sqrt_ph(k, o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_maskz_sqrt_ph(k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_sqrt_ph(k, o);
  } else
    static_assert(!sizeof(B), "maskz_sqrt<B>: unsupported simd type");
}

template<is_simd_type B, typename M>
inline B
mask_sqrt_round(B src, M k, B &o, int rounding)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_sqrt_round_ps(src, k, o, rounding);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_sqrt_round_pd(src, k, o, rounding);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_sqrt_round_ph(src, k, o, rounding);
  } else
    static_assert(!sizeof(B), "mask_sqrt_round<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_sqrt_round(M k, B &o, int rounding)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_sqrt_round_ps(k, o, rounding);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_sqrt_round_pd(k, o, rounding);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_sqrt_round_ph(k, o, rounding);
  } else
    static_assert(!sizeof(B), "maskz_sqrt_round<B>: only 512-bit ps/pd/ph are supported");
}

template<typename M>
inline f128
mask_sqrt_ss(f128 src, M k, f128 a, f128 b)
{
  return _mm_mask_sqrt_ss(src, k, a, b);
}

template<typename M>
inline f128
maskz_sqrt_ss(M k, f128 a, f128 b)
{
  return _mm_maskz_sqrt_ss(k, a, b);
}

template<typename M>
inline d128
mask_sqrt_sd(d128 src, M k, d128 a, d128 b)
{
  return _mm_mask_sqrt_sd(src, k, a, b);
}

template<typename M>
inline d128
maskz_sqrt_sd(M k, d128 a, d128 b)
{
  return _mm_maskz_sqrt_sd(k, a, b);
}

template<typename M>
inline f128
mask_sqrt_round_ss(f128 src, M k, f128 a, f128 b, int r)
{
  return _mm_mask_sqrt_round_ss(src, k, a, b, r);
}

template<typename M>
inline f128
maskz_sqrt_round_ss(M k, f128 a, f128 b, int r)
{
  return _mm_maskz_sqrt_round_ss(k, a, b, r);
}

template<typename M>
inline d128
mask_sqrt_round_sd(d128 src, M k, d128 a, d128 b, int r)
{
  return _mm_mask_sqrt_round_sd(src, k, a, b, r);
}

template<typename M>
inline d128
maskz_sqrt_round_sd(M k, d128 a, d128 b, int r)
{
  return _mm_maskz_sqrt_round_sd(k, a, b, r);
}
#endif      // __micron_simd_svml

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reciptrocal sqrt

inline f128
rsqrt_ps(f128 &o)
{
  return _mm_rsqrt_ps(o);
}

inline f256
rsqrt_ps(f256 &o)
{
  return _mm256_rsqrt_ps(o);
}

inline f128
rsqrt_ss(f128 a)
{
  return _mm_rsqrt_ss(a);
}

// _Float16 reciprocal-sqrt (AVX-512FP16 _ph/_sh)
#if defined(__micron_simd_svml)
template<is_simd_type B>
inline B
rsqrt_ph(B &o)
{
  if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_rsqrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_rsqrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_rsqrt_ph(o);
  } else
    static_assert(!sizeof(B), "rsqrt_ph<B>: only _Float16 simd types are supported");
}

template<is_simd_type B, typename M>
inline B
mask_rsqrt_ph(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_mask_rsqrt_ph(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_mask_rsqrt_ph(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_rsqrt_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_rsqrt_ph<B>: only _Float16 simd types are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_rsqrt_ph(M k, B &o)
{
  if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_maskz_rsqrt_ph(k, o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_maskz_rsqrt_ph(k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_rsqrt_ph(k, o);
  } else
    static_assert(!sizeof(B), "maskz_rsqrt_ph<B>: only _Float16 simd types are supported");
}

inline h128
rsqrt_sh(h128 a, h128 b)
{
  return _mm_rsqrt_sh(a, b);
}

template<typename M>
inline h128
mask_rsqrt_sh(h128 src, M k, h128 a, h128 b)
{
  return _mm_mask_rsqrt_sh(src, k, a, b);
}

template<typename M>
inline h128
maskz_rsqrt_sh(M k, h128 a, h128 b)
{
  return _mm_maskz_rsqrt_sh(k, a, b);
}
#endif      // __micron_simd_svml

#if defined(__micron_x86_avx512f)
template<is_simd_type B>
[[gnu::target("avx512f")]] inline B
rsqrt14(B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_rsqrt14_ps(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_rsqrt14_pd(o);
  } else
    static_assert(!sizeof(B), "rsqrt14<B>: only 512-bit ps/pd are supported");
}
#endif      // __micron_x86_avx512f

// AVX-512 masked rsqrt14 (_mm512_mask*/_maskz*) and the scalar _ss/_sd (+masked)
// variants are not declared in micron
#if defined(__micron_simd_svml)
template<is_simd_type B, typename M>
inline B
mask_rsqrt14(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_rsqrt14_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_rsqrt14_pd(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_rsqrt14<B>: only 512-bit ps/pd are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_rsqrt14(M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_rsqrt14_ps(k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_rsqrt14_pd(k, o);
  } else
    static_assert(!sizeof(B), "maskz_rsqrt14<B>: only 512-bit ps/pd are supported");
}

inline f128
rsqrt14_ss(f128 a, f128 b)
{
  return _mm_rsqrt14_ss(a, b);
}

inline d128
rsqrt14_sd(d128 a, d128 b)
{
  return _mm_rsqrt14_sd(a, b);
}

template<typename M>
inline f128
mask_rsqrt14_ss(f128 src, M k, f128 a, f128 b)
{
  return _mm_mask_rsqrt14_ss(src, k, a, b);
}

template<typename M>
inline f128
maskz_rsqrt14_ss(M k, f128 a, f128 b)
{
  return _mm_maskz_rsqrt14_ss(k, a, b);
}

template<typename M>
inline d128
mask_rsqrt14_sd(d128 src, M k, d128 a, d128 b)
{
  return _mm_mask_rsqrt14_sd(src, k, a, b);
}

template<typename M>
inline d128
maskz_rsqrt14_sd(M k, d128 a, d128 b)
{
  return _mm_maskz_rsqrt14_sd(k, a, b);
}
#endif      // __micron_simd_svml

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reciprocals

inline f128
rcp_ps(f128 &o)
{
  return _mm_rcp_ps(o);
}

inline f256
rcp_ps(f256 &o)
{
  return _mm256_rcp_ps(o);
}

inline f128
rcp_ss(f128 a)
{
  return _mm_rcp_ss(a);
}

// _Float16 reciprocal (AVX-512FP16 _ph/_sh)
#if defined(__micron_simd_svml)
template<is_simd_type B>
inline B
rcp_ph(B &o)
{
  if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_rcp_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_rcp_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_rcp_ph(o);
  } else
    static_assert(!sizeof(B), "rcp_ph<B>: only _Float16 simd types are supported");
}

template<is_simd_type B, typename M>
inline B
mask_rcp_ph(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_mask_rcp_ph(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_mask_rcp_ph(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_rcp_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_rcp_ph<B>: only _Float16 simd types are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_rcp_ph(M k, B &o)
{
  if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_maskz_rcp_ph(k, o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_maskz_rcp_ph(k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_rcp_ph(k, o);
  } else
    static_assert(!sizeof(B), "maskz_rcp_ph<B>: only _Float16 simd types are supported");
}

inline h128
rcp_sh(h128 a, h128 b)
{
  return _mm_rcp_sh(a, b);
}

template<typename M>
inline h128
mask_rcp_sh(h128 src, M k, h128 a, h128 b)
{
  return _mm_mask_rcp_sh(src, k, a, b);
}

template<typename M>
inline h128
maskz_rcp_sh(M k, h128 a, h128 b)
{
  return _mm_maskz_rcp_sh(k, a, b);
}
#endif      // __micron_simd_svml

#if defined(__micron_x86_avx512f)
template<is_simd_type B>
[[gnu::target("avx512f")]] inline B
rcp14(B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_rcp14_ps(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_rcp14_pd(o);
  } else
    static_assert(!sizeof(B), "rcp14<B>: only 512-bit ps/pd are supported");
}
#endif      // __micron_x86_avx512f

// AVX-512 masked rcp14 (_mm512_mask*/_maskz*), scalar _ss/_sd (+masked), and the
// _mm512_recip_* (SVML) family are not declared in micron
#if defined(__micron_simd_svml)
template<is_simd_type B, typename M>
inline B
mask_rcp14(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_rcp14_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_rcp14_pd(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_rcp14<B>: only 512-bit ps/pd are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_rcp14(M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_rcp14_ps(k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_rcp14_pd(k, o);
  } else
    static_assert(!sizeof(B), "maskz_rcp14<B>: only 512-bit ps/pd are supported");
}

inline f128
rcp14_ss(f128 a, f128 b)
{
  return _mm_rcp14_ss(a, b);
}

inline d128
rcp14_sd(d128 a, d128 b)
{
  return _mm_rcp14_sd(a, b);
}

template<typename M>
inline f128
mask_rcp14_ss(f128 src, M k, f128 a, f128 b)
{
  return _mm_mask_rcp14_ss(src, k, a, b);
}

template<typename M>
inline f128
maskz_rcp14_ss(M k, f128 a, f128 b)
{
  return _mm_maskz_rcp14_ss(k, a, b);
}

template<typename M>
inline d128
mask_rcp14_sd(d128 src, M k, d128 a, d128 b)
{
  return _mm_mask_rcp14_sd(src, k, a, b);
}

template<typename M>
inline d128
maskz_rcp14_sd(M k, d128 a, d128 b)
{
  return _mm_maskz_rcp14_sd(k, a, b);
}

template<is_simd_type B>
inline B
recip(B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_recip_ps(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_recip_pd(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_recip_ph(o);
  } else
    static_assert(!sizeof(B), "recip<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B, typename M>
inline B
mask_recip(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_recip_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_recip_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_recip_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_recip<B>: only 512-bit ps/pd/ph are supported");
}
#endif      // __micron_simd_svml

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// min/maxes

template<is_simd_type B>
inline B
min(B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_min_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_min_pd(o, b);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_min_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_min_pd(o, b);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_min_ps(o, b);      // AVX-512F
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_min_pd(o, b);      // AVX-512F
#endif
#if defined(__micron_simd_svml)
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_min_ph(o, b);      // AVX-512FP16
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_min_ph(o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_min_ph(o, b);
#endif
  } else
    static_assert(!sizeof(B), "min<B>: unsupported simd type (512-bit needs AVX-512F; _Float16 paths need an SVML/FP16 backend)");
}

template<is_simd_type B>
inline B
max(B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_max_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_max_pd(o, b);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_max_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_max_pd(o, b);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_max_ps(o, b);      // AVX-512F
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_max_pd(o, b);      // AVX-512F
#endif
#if defined(__micron_simd_svml)
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_max_ph(o, b);      // AVX-512FP16
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_max_ph(o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_max_ph(o, b);
#endif
  } else
    static_assert(!sizeof(B), "max<B>: unsupported simd type (512-bit needs AVX-512F; _Float16 paths need an SVML/FP16 backend)");
}

inline f128
min_ss(f128 a, f128 b)
{
  return _mm_min_ss(a, b);
}

inline d128
min_sd(d128 a, d128 b)
{
  return _mm_min_sd(a, b);
}

inline f128
max_ss(f128 a, f128 b)
{
  return _mm_max_ss(a, b);
}

inline d128
max_sd(d128 a, d128 b)
{
  return _mm_max_sd(a, b);
}

// _Float16 scalar min/max (AVX-512FP16 _sh)
#if defined(__micron_simd_svml)
inline h128
min_sh(h128 a, h128 b)
{
  return _mm_min_sh(a, b);
}      // AVX-512FP16

inline h128
max_sh(h128 a, h128 b)
{
  return _mm_max_sh(a, b);
}      // AVX-512FP16
#endif      // __micron_simd_svml

#if defined(__micron_simd_svml)
template<typename M>
inline f128
mask_min_ss(f128 src, M k, f128 a, f128 b)
{
  return _mm_mask_min_ss(src, k, a, b);
}

template<typename M>
inline f128
maskz_min_ss(M k, f128 a, f128 b)
{
  return _mm_maskz_min_ss(k, a, b);
}

template<typename M>
inline d128
mask_min_sd(d128 src, M k, d128 a, d128 b)
{
  return _mm_mask_min_sd(src, k, a, b);
}

template<typename M>
inline d128
maskz_min_sd(M k, d128 a, d128 b)
{
  return _mm_maskz_min_sd(k, a, b);
}

template<typename M>
inline h128
mask_min_sh(h128 src, M k, h128 a, h128 b)
{
  return _mm_mask_min_sh(src, k, a, b);
}

template<typename M>
inline h128
maskz_min_sh(M k, h128 a, h128 b)
{
  return _mm_maskz_min_sh(k, a, b);
}

template<typename M>
inline f128
mask_max_ss(f128 src, M k, f128 a, f128 b)
{
  return _mm_mask_max_ss(src, k, a, b);
}

template<typename M>
inline f128
maskz_max_ss(M k, f128 a, f128 b)
{
  return _mm_maskz_max_ss(k, a, b);
}

template<typename M>
inline d128
mask_max_sd(d128 src, M k, d128 a, d128 b)
{
  return _mm_mask_max_sd(src, k, a, b);
}

template<typename M>
inline d128
maskz_max_sd(M k, d128 a, d128 b)
{
  return _mm_maskz_max_sd(k, a, b);
}

template<typename M>
inline h128
mask_max_sh(h128 src, M k, h128 a, h128 b)
{
  return _mm_mask_max_sh(src, k, a, b);
}

template<typename M>
inline h128
maskz_max_sh(M k, h128 a, h128 b)
{
  return _mm_maskz_max_sh(k, a, b);
}

template<is_simd_type B>
inline B
min_round(B &o, B &b, int sae)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_min_round_ps(o, b, sae);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_min_round_pd(o, b, sae);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_min_round_ph(o, b, sae);
  } else
    static_assert(!sizeof(B), "min_round<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
max_round(B &o, B &b, int sae)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_max_round_ps(o, b, sae);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_max_round_pd(o, b, sae);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_max_round_ph(o, b, sae);
  } else
    static_assert(!sizeof(B), "max_round<B>: only 512-bit ps/pd/ph are supported");
}

inline f128
min_round_ss(f128 a, f128 b, int sae)
{
  return _mm_min_round_ss(a, b, sae);
}

inline d128
min_round_sd(d128 a, d128 b, int sae)
{
  return _mm_min_round_sd(a, b, sae);
}

inline h128
min_round_sh(h128 a, h128 b, int sae)
{
  return _mm_min_round_sh(a, b, sae);
}

inline f128
max_round_ss(f128 a, f128 b, int sae)
{
  return _mm_max_round_ss(a, b, sae);
}

inline d128
max_round_sd(d128 a, d128 b, int sae)
{
  return _mm_max_round_sd(a, b, sae);
}

inline h128
max_round_sh(h128 a, h128 b, int sae)
{
  return _mm_max_round_sh(a, b, sae);
}

template<is_simd_type B, typename M>
inline B
mask_min(B src, M k, B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_min_ps(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_min_pd(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_mask_min_ph(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_mask_min_ph(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_min_ph(src, k, o, b);
  } else
    static_assert(!sizeof(B), "mask_min<B>: only 512-bit ps/pd and FP16 ph are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_min(M k, B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_min_ps(k, o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_min_pd(k, o, b);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_maskz_min_ph(k, o, b);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_maskz_min_ph(k, o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_min_ph(k, o, b);
  } else
    static_assert(!sizeof(B), "maskz_min<B>: only 512-bit ps/pd and FP16 ph are supported");
}

template<is_simd_type B, typename M>
inline B
mask_max(B src, M k, B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_max_ps(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_max_pd(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_mask_max_ph(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_mask_max_ph(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_max_ph(src, k, o, b);
  } else
    static_assert(!sizeof(B), "mask_max<B>: only 512-bit ps/pd and FP16 ph are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_max(M k, B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_max_ps(k, o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_max_pd(k, o, b);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_maskz_max_ph(k, o, b);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_maskz_max_ph(k, o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_max_ph(k, o, b);
  } else
    static_assert(!sizeof(B), "maskz_max<B>: only 512-bit ps/pd and FP16 ph are supported");
}

template<is_simd_type B, typename M>
inline B
mask_min_round(B src, M k, B &o, B &b, int sae)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_min_round_ps(src, k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_min_round_pd(src, k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_min_round_ph(src, k, o, b, sae);
  } else
    static_assert(!sizeof(B), "mask_min_round<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_min_round(M k, B &o, B &b, int sae)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_min_round_ps(k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_min_round_pd(k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_min_round_ph(k, o, b, sae);
  } else
    static_assert(!sizeof(B), "maskz_min_round<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B, typename M>
inline B
mask_max_round(B src, M k, B &o, B &b, int sae)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_max_round_ps(src, k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_max_round_pd(src, k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_max_round_ph(src, k, o, b, sae);
  } else
    static_assert(!sizeof(B), "mask_max_round<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B, typename M>
inline B
maskz_max_round(M k, B &o, B &b, int sae)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_maskz_max_round_ps(k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_maskz_max_round_pd(k, o, b, sae);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_maskz_max_round_ph(k, o, b, sae);
  } else
    static_assert(!sizeof(B), "maskz_max_round<B>: only 512-bit ps/pd/ph are supported");
}

template<typename M>
inline f128
mask_min_round_ss(f128 src, M k, f128 a, f128 b, int sae)
{
  return _mm_mask_min_round_ss(src, k, a, b, sae);
}

template<typename M>
inline f128
maskz_min_round_ss(M k, f128 a, f128 b, int sae)
{
  return _mm_maskz_min_round_ss(k, a, b, sae);
}

template<typename M>
inline d128
mask_min_round_sd(d128 src, M k, d128 a, d128 b, int sae)
{
  return _mm_mask_min_round_sd(src, k, a, b, sae);
}

template<typename M>
inline d128
maskz_min_round_sd(M k, d128 a, d128 b, int sae)
{
  return _mm_maskz_min_round_sd(k, a, b, sae);
}

template<typename M>
inline h128
mask_min_round_sh(h128 src, M k, h128 a, h128 b, int sae)
{
  return _mm_mask_min_round_sh(src, k, a, b, sae);
}

template<typename M>
inline h128
maskz_min_round_sh(M k, h128 a, h128 b, int sae)
{
  return _mm_maskz_min_round_sh(k, a, b, sae);
}

template<typename M>
inline f128
mask_max_round_ss(f128 src, M k, f128 a, f128 b, int sae)
{
  return _mm_mask_max_round_ss(src, k, a, b, sae);
}

template<typename M>
inline f128
maskz_max_round_ss(M k, f128 a, f128 b, int sae)
{
  return _mm_maskz_max_round_ss(k, a, b, sae);
}

template<typename M>
inline d128
mask_max_round_sd(d128 src, M k, d128 a, d128 b, int sae)
{
  return _mm_mask_max_round_sd(src, k, a, b, sae);
}

template<typename M>
inline d128
maskz_max_round_sd(M k, d128 a, d128 b, int sae)
{
  return _mm_maskz_max_round_sd(k, a, b, sae);
}

template<typename M>
inline h128
mask_max_round_sh(h128 src, M k, h128 a, h128 b, int sae)
{
  return _mm_mask_max_round_sh(src, k, a, b, sae);
}

template<typename M>
inline h128
maskz_max_round_sh(M k, h128 a, h128 b, int sae)
{
  return _mm_maskz_max_round_sh(k, a, b, sae);
}
#endif      // __micron_simd_svml

template<is_simd_class T>
inline T
min_i8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epi8(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epi8(o, b);
  } else
    static_assert(!sizeof(T), "min_i8<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
min_i16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epi16(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epi16(o, b);
  } else
    static_assert(!sizeof(T), "min_i16<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
min_i32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epi32(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epi32(o, b);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_min_epi32(o, b);      // AVX-512F
#endif
  } else
    static_assert(!sizeof(T), "min_i32<T>: unsupported simd class width (need AVX-512F for the 512-bit path)");
}

#if defined(__micron_x86_avx512f)
[[gnu::target("avx512f")]] inline i512
min_i64(i512 &o, i512 &b)
{
  return _mm512_min_epi64(o, b);
}      // AVX-512F only
#endif      // __micron_x86_avx512f

// AVX-512 masked integer min (_mm512_mask*/_maskz*)
#if defined(__micron_simd_svml)
template<typename M>
inline i512
mask_min_i32(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_min_epi32(src, k, o, b);
}

template<typename M>
inline i512
maskz_min_i32(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_min_epi32(k, o, b);
}

template<typename M>
inline i512
mask_min_i64(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_min_epi64(src, k, o, b);
}

template<typename M>
inline i512
maskz_min_i64(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_min_epi64(k, o, b);
}
#endif      // __micron_simd_svml

template<is_simd_class T>
inline T
max_i8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi8(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi8(o, b);
  } else
    static_assert(!sizeof(T), "max_i8<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
max_i16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi16(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi16(o, b);
  } else
    static_assert(!sizeof(T), "max_i16<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
max_i32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi32(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi32(o, b);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_max_epi32(o, b);      // AVX-512F
#endif
  } else
    static_assert(!sizeof(T), "max_i32<T>: unsupported simd class width (need AVX-512F for the 512-bit path)");
}

#if defined(__micron_x86_avx512f)
[[gnu::target("avx512f")]] inline i512
max_i64(i512 &o, i512 &b)
{
  return _mm512_max_epi64(o, b);
}      // AVX-512F only
#endif      // __micron_x86_avx512f

// AVX-512 masked integer max (_mm512_mask*/_maskz*)
#if defined(__micron_simd_svml)
template<typename M>
inline i512
mask_max_i32(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_max_epi32(src, k, o, b);
}

template<typename M>
inline i512
maskz_max_i32(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_max_epi32(k, o, b);
}

template<typename M>
inline i512
mask_max_i64(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_max_epi64(src, k, o, b);
}

template<typename M>
inline i512
maskz_max_i64(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_max_epi64(k, o, b);
}
#endif      // __micron_simd_svml

template<is_simd_class T>
inline T
min_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epu8(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epu8(o, b);
  } else
    static_assert(!sizeof(T), "min_u8<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
min_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epu16(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epu16(o, b);
  } else
    static_assert(!sizeof(T), "min_u16<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
min_u32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epu32(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epu32(o, b);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_min_epu32(o, b);      // AVX-512F
#endif
  } else
    static_assert(!sizeof(T), "min_u32<T>: unsupported simd class width (need AVX-512F for the 512-bit path)");
}

#if defined(__micron_x86_avx512f)
[[gnu::target("avx512f")]] inline i512
min_u64(i512 &o, i512 &b)
{
  return _mm512_min_epu64(o, b);
}      // AVX-512F only
#endif      // __micron_x86_avx512f

// AVX-512 masked unsigned-integer min (_mm512_mask*/_maskz*)
#if defined(__micron_simd_svml)
template<typename M>
inline i512
mask_min_u32(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_min_epu32(src, k, o, b);
}

template<typename M>
inline i512
maskz_min_u32(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_min_epu32(k, o, b);
}

template<typename M>
inline i512
mask_min_u64(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_min_epu64(src, k, o, b);
}

template<typename M>
inline i512
maskz_min_u64(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_min_epu64(k, o, b);
}
#endif      // __micron_simd_svml

template<is_simd_class T>
inline T
max_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epu8(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epu8(o, b);
  } else
    static_assert(!sizeof(T), "max_u8<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
max_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epu16(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epu16(o, b);
  } else
    static_assert(!sizeof(T), "max_u16<T>: only 128-bit/256-bit integer simd classes are supported");
}

template<is_simd_class T>
inline T
max_u32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epu32(o, b);
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epu32(o, b);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_max_epu32(o, b);      // AVX-512F
#endif
  } else
    static_assert(!sizeof(T), "max_u32<T>: unsupported simd class width (need AVX-512F for the 512-bit path)");
}

#if defined(__micron_x86_avx512f)
[[gnu::target("avx512f")]] inline i512
max_u64(i512 &o, i512 &b)
{
  return _mm512_max_epu64(o, b);
}      // AVX-512F only
#endif      // __micron_x86_avx512f

// AVX-512 masked unsigned-integer max (_mm512_mask*/_maskz*)
#if defined(__micron_simd_svml)
template<typename M>
inline i512
mask_max_u32(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_max_epu32(src, k, o, b);
}

template<typename M>
inline i512
maskz_max_u32(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_max_epu32(k, o, b);
}

template<typename M>
inline i512
mask_max_u64(i512 src, M k, i512 &o, i512 &b)
{
  return _mm512_mask_max_epu64(src, k, o, b);
}

template<typename M>
inline i512
maskz_max_u64(M k, i512 &o, i512 &b)
{
  return _mm512_maskz_max_epu64(k, o, b);
}
#endif      // __micron_simd_svml

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// (horizontal) reduces
#if defined(__micron_simd_svml)
inline float
reduce_max_ps(f512 &o)
{
  return _mm512_reduce_max_ps(o);
}

inline double
reduce_max_pd(d512 &o)
{
  return _mm512_reduce_max_pd(o);
}

inline int
reduce_max_i32(i512 &o)
{
  return _mm512_reduce_max_epi32(o);
}

inline long long
reduce_max_i64(i512 &o)
{
  return _mm512_reduce_max_epi64(o);
}

inline unsigned int
reduce_max_u32(i512 &o)
{
  return _mm512_reduce_max_epu32(o);
}

inline unsigned long long
reduce_max_u64(i512 &o)
{
  return _mm512_reduce_max_epu64(o);
}

inline short
reduce_max_i16(i128 &o)
{
  return _mm_reduce_max_epi16(o);
}

inline short
reduce_max_i16(i256 &o)
{
  return _mm256_reduce_max_epi16(o);
}

inline char
reduce_max_i8(i128 &o)
{
  return _mm_reduce_max_epi8(o);
}

inline char
reduce_max_i8(i256 &o)
{
  return _mm256_reduce_max_epi8(o);
}

inline unsigned short
reduce_max_u16(i128 &o)
{
  return _mm_reduce_max_epu16(o);
}

inline unsigned short
reduce_max_u16(i256 &o)
{
  return _mm256_reduce_max_epu16(o);
}

inline unsigned char
reduce_max_u8(i128 &o)
{
  return _mm_reduce_max_epu8(o);
}

inline unsigned char
reduce_max_u8(i256 &o)
{
  return _mm256_reduce_max_epu8(o);
}

template<typename M>
inline float
mask_reduce_max_ps(M k, f512 &o)
{
  return _mm512_mask_reduce_max_ps(k, o);
}

template<typename M>
inline double
mask_reduce_max_pd(M k, d512 &o)
{
  return _mm512_mask_reduce_max_pd(k, o);
}

template<typename M>
inline int
mask_reduce_max_i32(M k, i512 &o)
{
  return _mm512_mask_reduce_max_epi32(k, o);
}

template<typename M>
inline long long
mask_reduce_max_i64(M k, i512 &o)
{
  return _mm512_mask_reduce_max_epi64(k, o);
}

template<typename M>
inline unsigned int
mask_reduce_max_u32(M k, i512 &o)
{
  return _mm512_mask_reduce_max_epu32(k, o);
}

template<typename M>
inline unsigned long long
mask_reduce_max_u64(M k, i512 &o)
{
  return _mm512_mask_reduce_max_epu64(k, o);
}

template<typename M>
inline short
mask_reduce_max_i16(M k, i128 &o)
{
  return _mm_mask_reduce_max_epi16(k, o);
}

template<typename M>
inline short
mask_reduce_max_i16(M k, i256 &o)
{
  return _mm256_mask_reduce_max_epi16(k, o);
}

template<typename M>
inline char
mask_reduce_max_i8(M k, i128 &o)
{
  return _mm_mask_reduce_max_epi8(k, o);
}

template<typename M>
inline char
mask_reduce_max_i8(M k, i256 &o)
{
  return _mm256_mask_reduce_max_epi8(k, o);
}

template<typename M>
inline unsigned short
mask_reduce_max_u16(M k, i128 &o)
{
  return _mm_mask_reduce_max_epu16(k, o);
}

template<typename M>
inline unsigned short
mask_reduce_max_u16(M k, i256 &o)
{
  return _mm256_mask_reduce_max_epu16(k, o);
}

template<typename M>
inline unsigned char
mask_reduce_max_u8(M k, i128 &o)
{
  return _mm_mask_reduce_max_epu8(k, o);
}

template<typename M>
inline unsigned char
mask_reduce_max_u8(M k, i256 &o)
{
  return _mm256_mask_reduce_max_epu8(k, o);
}

inline float
reduce_min_ps(f512 &o)
{
  return _mm512_reduce_min_ps(o);
}

inline double
reduce_min_pd(d512 &o)
{
  return _mm512_reduce_min_pd(o);
}

inline int
reduce_min_i32(i512 &o)
{
  return _mm512_reduce_min_epi32(o);
}

inline long long
reduce_min_i64(i512 &o)
{
  return _mm512_reduce_min_epi64(o);
}

inline unsigned int
reduce_min_u32(i512 &o)
{
  return _mm512_reduce_min_epu32(o);
}

inline unsigned long long
reduce_min_u64(i512 &o)
{
  return _mm512_reduce_min_epu64(o);
}

inline short
reduce_min_i16(i128 &o)
{
  return _mm_reduce_min_epi16(o);
}

inline short
reduce_min_i16(i256 &o)
{
  return _mm256_reduce_min_epi16(o);
}

inline char
reduce_min_i8(i128 &o)
{
  return _mm_reduce_min_epi8(o);
}

inline char
reduce_min_i8(i256 &o)
{
  return _mm256_reduce_min_epi8(o);
}

inline unsigned short
reduce_min_u16(i128 &o)
{
  return _mm_reduce_min_epu16(o);
}

inline unsigned short
reduce_min_u16(i256 &o)
{
  return _mm256_reduce_min_epu16(o);
}

inline unsigned char
reduce_min_u8(i128 &o)
{
  return _mm_reduce_min_epu8(o);
}

inline unsigned char
reduce_min_u8(i256 &o)
{
  return _mm256_reduce_min_epu8(o);
}

template<typename M>
inline float
mask_reduce_min_ps(M k, f512 &o)
{
  return _mm512_mask_reduce_min_ps(k, o);
}

template<typename M>
inline double
mask_reduce_min_pd(M k, d512 &o)
{
  return _mm512_mask_reduce_min_pd(k, o);
}

template<typename M>
inline int
mask_reduce_min_i32(M k, i512 &o)
{
  return _mm512_mask_reduce_min_epi32(k, o);
}

template<typename M>
inline long long
mask_reduce_min_i64(M k, i512 &o)
{
  return _mm512_mask_reduce_min_epi64(k, o);
}

template<typename M>
inline unsigned int
mask_reduce_min_u32(M k, i512 &o)
{
  return _mm512_mask_reduce_min_epu32(k, o);
}

template<typename M>
inline unsigned long long
mask_reduce_min_u64(M k, i512 &o)
{
  return _mm512_mask_reduce_min_epu64(k, o);
}

template<typename M>
inline short
mask_reduce_min_i16(M k, i128 &o)
{
  return _mm_mask_reduce_min_epi16(k, o);
}

template<typename M>
inline short
mask_reduce_min_i16(M k, i256 &o)
{
  return _mm256_mask_reduce_min_epi16(k, o);
}

template<typename M>
inline char
mask_reduce_min_i8(M k, i128 &o)
{
  return _mm_mask_reduce_min_epi8(k, o);
}

template<typename M>
inline char
mask_reduce_min_i8(M k, i256 &o)
{
  return _mm256_mask_reduce_min_epi8(k, o);
}

template<typename M>
inline unsigned short
mask_reduce_min_u16(M k, i128 &o)
{
  return _mm_mask_reduce_min_epu16(k, o);
}

template<typename M>
inline unsigned short
mask_reduce_min_u16(M k, i256 &o)
{
  return _mm256_mask_reduce_min_epu16(k, o);
}

template<typename M>
inline unsigned char
mask_reduce_min_u8(M k, i128 &o)
{
  return _mm_mask_reduce_min_epu8(k, o);
}

template<typename M>
inline unsigned char
mask_reduce_min_u8(M k, i256 &o)
{
  return _mm256_mask_reduce_min_epu8(k, o);
}
#endif      // __micron_simd_svml

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rounds

template<is_simd_type B>
inline B
ceil(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_ceil_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_ceil_pd(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_ceil_ps(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_ceil_pd(o);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_ceil_ps(o);      // AVX-512F
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_ceil_pd(o);      // AVX-512F
#endif
#if defined(__micron_simd_svml)
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_ceil_ph(o);      // AVX-512FP16
#endif
  } else
    static_assert(!sizeof(B), "ceil<B>: unsupported simd type (512-bit needs AVX-512F; _Float16 path needs an SVML/FP16 backend)");
}

inline f128
ceil_ss(f128 a, f128 b)
{
  return _mm_ceil_ss(a, b);
}

inline d128
ceil_sd(d128 a, d128 b)
{
  return _mm_ceil_sd(a, b);
}

// masked ceil (_mm512_mask*) + FP16
#if defined(__micron_simd_svml)
template<is_simd_type B, typename M>
inline B
mask_ceil(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_ceil_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_ceil_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_ceil_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_ceil<B>: only 512-bit ps/pd/ph are supported");
}
#endif      // __micron_simd_svml

template<is_simd_type B>
inline B
floor(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_floor_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_floor_pd(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_floor_ps(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_floor_pd(o);
#if defined(__micron_x86_avx512f)
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_floor_ps(o);      // AVX-512F
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_floor_pd(o);      // AVX-512F
#endif
#if defined(__micron_simd_svml)
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_floor_ph(o);      // AVX-512FP16
#endif
  } else
    static_assert(!sizeof(B), "floor<B>: unsupported simd type (512-bit needs AVX-512F; _Float16 path needs an SVML/FP16 backend)");
}

inline f128
floor_ss(f128 a, f128 b)
{
  return _mm_floor_ss(a, b);
}

inline d128
floor_sd(d128 a, d128 b)
{
  return _mm_floor_sd(a, b);
}

// masked floor (_mm512_mask*) + FP16
#if defined(__micron_simd_svml)
template<is_simd_type B, typename M>
inline B
mask_floor(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_floor_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_floor_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_floor_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_floor<B>: only 512-bit ps/pd/ph are supported");
}
#endif      // __micron_simd_svml

// NB: there is no hardware _mm512_round_ps/_pd
template<is_simd_type B>
inline B
round(B &o, int rounding)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_round_ps(o, rounding);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_round_ps(o, rounding);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_round_pd(o, rounding);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_round_pd(o, rounding);
  } else
    static_assert(!sizeof(B), "round<B>: only 128-bit/256-bit ps/pd are supported (AVX-512 uses roundscale, not provided here)");
}

inline f128
round_ss(f128 a, f128 b, int r)
{
  return _mm_round_ss(a, b, r);
}

inline d128
round_sd(d128 a, d128 b, int r)
{
  return _mm_round_sd(a, b, r);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// everything from here to the end of the namespace is unavailable in micron's freestanding intrin layer
// and is therefore gated behind the SVML/extended feature macro
#if defined(__micron_simd_svml)

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// truncs

template<is_simd_type B>
inline B
trunc(B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_trunc_ps(o);      // AVX-512F
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_trunc_pd(o);      // AVX-512F
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_trunc_ph(o);      // AVX-512FP16
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_trunc_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_trunc_ph(o);
  } else
    static_assert(!sizeof(B), "trunc<B>: only 512-bit ps/pd and FP16 ph are supported");
}

template<is_simd_type B, typename M>
inline B
mask_trunc(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_trunc_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_trunc_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_trunc_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_trunc<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
nearbyint(B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_nearbyint_ps(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_nearbyint_pd(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_nearbyint_ph(o);
  } else
    static_assert(!sizeof(B), "nearbyint<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B, typename M>
inline B
mask_nearbyint(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_nearbyint_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_nearbyint_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_nearbyint_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_nearbyint<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
rint(B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_rint_ps(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_rint_pd(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_rint_ph(o);
  } else
    static_assert(!sizeof(B), "rint<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B, typename M>
inline B
mask_rint(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_rint_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_rint_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_rint_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_rint<B>: only 512-bit ps/pd/ph are supported");
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// short vector math lib (svml) extensions
// needs libsvml
// TODO: implement from 0

template<is_simd_type B>
inline B
exp(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_exp_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_exp_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_exp_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_exp_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_exp_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_exp_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_exp_ph(o);      // AVX-512FP16
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_exp_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_exp_ph(o);
  } else
    static_assert(!sizeof(B), "exp<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_exp(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_exp_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_exp_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_exp_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_exp<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
exp2(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_exp2_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_exp2_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_exp2_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_exp2_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_exp2_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_exp2_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_exp2_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_exp2_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_exp2_ph(o);
  } else
    static_assert(!sizeof(B), "exp2<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_exp2(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_exp2_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_exp2_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_exp2_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_exp2<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
exp10(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_exp10_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_exp10_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_exp10_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_exp10_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_exp10_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_exp10_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_exp10_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_exp10_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_exp10_ph(o);
  } else
    static_assert(!sizeof(B), "exp10<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_exp10(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_exp10_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_exp10_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_exp10_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_exp10<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
expm1(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_expm1_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_expm1_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_expm1_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_expm1_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_expm1_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_expm1_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_expm1_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_expm1_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_expm1_ph(o);
  } else
    static_assert(!sizeof(B), "expm1<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_expm1(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_expm1_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_expm1_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_expm1_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_expm1<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
log(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_log_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_log_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_log_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_log_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_log_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_log_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_log_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_log_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_log_ph(o);
  } else
    static_assert(!sizeof(B), "log<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_log(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_log_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_log_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_log_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_log<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
log2(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_log2_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_log2_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_log2_ps(o);      // vlog2ps
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_log2_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_log2_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_log2_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_log2_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_log2_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_log2_ph(o);
  } else
    static_assert(!sizeof(B), "log2<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_log2(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_log2_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_log2_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_log2_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_log2<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
log10(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_log10_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_log10_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_log10_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_log10_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_log10_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_log10_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_log10_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_log10_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_log10_ph(o);
  } else
    static_assert(!sizeof(B), "log10<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_log10(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_log10_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_log10_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_log10_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_log10<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
log1p(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_log1p_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_log1p_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_log1p_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_log1p_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_log1p_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_log1p_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_log1p_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_log1p_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_log1p_ph(o);
  } else
    static_assert(!sizeof(B), "log1p<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_log1p(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_log1p_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_log1p_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_log1p_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_log1p<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
logb(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_logb_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_logb_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_logb_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_logb_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_logb_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_logb_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_logb_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_logb_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_logb_ph(o);
  } else
    static_assert(!sizeof(B), "logb<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_logb(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_logb_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_logb_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_logb_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_logb<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
pow(B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_pow_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_pow_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_pow_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_pow_pd(o, b);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_pow_pd(o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_pow_pd(o, b);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_pow_ph(o, b);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_pow_ph(o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_pow_ph(o, b);
  } else
    static_assert(!sizeof(B), "pow<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_pow(B src, M k, B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_pow_ps(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_pow_pd(src, k, o, b);
  } else
    static_assert(!sizeof(B), "mask_pow<B>: only 512-bit ps/pd are supported");
}

template<is_simd_type B>
inline B
cbrt(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_cbrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_cbrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_cbrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_cbrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_cbrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_cbrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_cbrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_cbrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_cbrt_ph(o);
  } else
    static_assert(!sizeof(B), "cbrt<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_cbrt(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_cbrt_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_cbrt_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_cbrt_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_cbrt<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
invcbrt(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_invcbrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_invcbrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_invcbrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_invcbrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_invcbrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_invcbrt_ph(o);
  } else
    static_assert(!sizeof(B), "invcbrt<B>: requires an SVML backend; 128/256-bit ps/pd + FP16 ph only");
}

template<is_simd_type B>
inline B
invsqrt(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_invsqrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_invsqrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_invsqrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_invsqrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_invsqrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_invsqrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_invsqrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_invsqrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_invsqrt_ph(o);
  } else
    static_assert(!sizeof(B), "invsqrt<B>: requires an SVML backend; ps/pd 128/256/512 + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_invsqrt(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_invsqrt_ps(src, k, o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_invsqrt_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_invsqrt_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_invsqrt<B>: only 512-bit ps/pd/ph are supported");
}

template<is_simd_type B>
inline B
hypot(B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_hypot_ps(o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_hypot_pd(o, b);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_hypot_ph(o, b);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_hypot_ph(o, b);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_hypot_ph(o, b);
  } else
    static_assert(!sizeof(B), "hypot<B>: requires an SVML backend; 512-bit ps/pd + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_hypot(B src, M k, B &o, B &b)
{
  if constexpr ( micron::is_same_v<B, f512> ) {
    return _mm512_mask_hypot_ps(src, k, o, b);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_hypot_pd(src, k, o, b);
  } else
    static_assert(!sizeof(B), "mask_hypot<B>: only 512-bit ps/pd are supported");
}

inline f128
cexp_ps(f128 a)
{
  return _mm_cexp_ps(a);
}

inline f256
cexp_ps(f256 a)
{
  return _mm256_cexp_ps(a);
}

inline f128
clog_ps(f128 a)
{
  return _mm_clog_ps(a);
}

inline f256
clog_ps(f256 a)
{
  return _mm256_clog_ps(a);
}

inline f128
csqrt_ps(f128 a)
{
  return _mm_csqrt_ps(a);
}

inline f256
csqrt_ps(f256 a)
{
  return _mm256_csqrt_ps(a);
}

template<is_simd_type B>
inline B
svml_sqrt(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_svml_sqrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_svml_sqrt_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_svml_sqrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_svml_sqrt_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_svml_sqrt_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_svml_sqrt_ph(o);
  } else
    static_assert(!sizeof(B), "svml_sqrt<B>: requires an SVML backend; 128/256-bit ps/pd + FP16 ph only");
}

template<is_simd_type B>
inline B
svml_ceil(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_svml_ceil_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_svml_ceil_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_svml_ceil_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_svml_ceil_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_svml_ceil_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_svml_ceil_ph(o);
  } else
    static_assert(!sizeof(B), "svml_ceil<B>: requires an SVML backend; 128/256-bit ps/pd + FP16 ph only");
}

template<is_simd_type B>
inline B
svml_floor(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_svml_floor_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_svml_floor_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_svml_floor_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_svml_floor_pd(o);
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_svml_floor_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_svml_floor_ph(o);
  } else
    static_assert(!sizeof(B), "svml_floor<B>: requires an SVML backend; 128/256-bit ps/pd + FP16 ph only");
}

template<is_simd_type B>
inline B
svml_round(B &o)
{
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_svml_round_ps(o);
  } else if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_svml_round_ps(o);
  } else if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_svml_round_pd(o);
  } else if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_svml_round_pd(o);
  } else if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_svml_round_pd(o);      // 512-bit pd only
  } else if constexpr ( micron::is_same_v<B, h128> ) {
    return _mm_svml_round_ph(o);
  } else if constexpr ( micron::is_same_v<B, h256> ) {
    return _mm256_svml_round_ph(o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_svml_round_ph(o);
  } else
    static_assert(!sizeof(B), "svml_round<B>: requires an SVML backend; 128/256-bit ps/pd, 512-bit pd + FP16 ph only");
}

template<is_simd_type B, typename M>
inline B
mask_svml_round(B src, M k, B &o)
{
  if constexpr ( micron::is_same_v<B, d512> ) {
    return _mm512_mask_svml_round_pd(src, k, o);
  } else if constexpr ( micron::is_same_v<B, h512> ) {
    return _mm512_mask_svml_round_ph(src, k, o);
  } else
    static_assert(!sizeof(B), "mask_svml_round<B>: only 512-bit pd/ph are supported");
}

#endif      // __micron_simd_svml

#pragma GCC diagnostic pop

};      // namespace simd
};      // namespace micron
