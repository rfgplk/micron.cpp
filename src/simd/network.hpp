//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// SIMD sorting-network primitives: lane-wise compare-exchange of two contiguous
// blocks (block min/max), and a vectorised Batcher bitonic sort over a power-of-
// two array

#include "simd.hpp"
#include "types.hpp"

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{
namespace simd
{

template<typename K>
consteval bool
__net_has_simd() noexcept
{
#if defined(__micron_x86_avx2)
  return (micron::is_integral_v<K> && sizeof(K) <= 4) || micron::is_same_v<K, float> || micron::is_same_v<K, double>;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return (micron::is_integral_v<K> && sizeof(K) <= 4) || micron::is_same_v<K, float> || micron::is_same_v<K, double>;
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  return (micron::is_integral_v<K> && sizeof(K) <= 4) || micron::is_same_v<K, float>;      // no f64 vector min on arm32
#else
  (void)0;
  return false;
#endif
}

// SIMD lanes per block for T (full register width / element size)
template<typename K>
consteval usize
__net_lanes() noexcept
{
#if defined(__micron_x86_avx2)
  return 32 / sizeof(K);
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_neon)
  return 16 / sizeof(K);
#else
  return 1;
#endif
}

template<typename T>
[[gnu::always_inline]] inline void
__block_cmpswap(T *lo, T *hi, bool asc) noexcept
{
#if defined(__micron_x86_avx2)
  if constexpr ( micron::is_floating_point_v<T> ) {
    if constexpr ( sizeof(T) == 4 ) {
      __m256 A = _mm256_loadu_ps(lo), B = _mm256_loadu_ps(hi);
      __m256 mn = _mm256_min_ps(A, B), mx = _mm256_max_ps(A, B);
      _mm256_storeu_ps(lo, asc ? mn : mx);
      _mm256_storeu_ps(hi, asc ? mx : mn);
    } else {
      __m256d A = _mm256_loadu_pd(lo), B = _mm256_loadu_pd(hi);
      __m256d mn = _mm256_min_pd(A, B), mx = _mm256_max_pd(A, B);
      _mm256_storeu_pd(lo, asc ? mn : mx);
      _mm256_storeu_pd(hi, asc ? mx : mn);
    }
  } else {
    __m256i A = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(lo));
    __m256i B = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(hi));
    __m256i mn, mx;
    if constexpr ( sizeof(T) == 1 ) {
      mn = micron::is_signed_v<T> ? _mm256_min_epi8(A, B) : _mm256_min_epu8(A, B);
      mx = micron::is_signed_v<T> ? _mm256_max_epi8(A, B) : _mm256_max_epu8(A, B);
    } else if constexpr ( sizeof(T) == 2 ) {
      mn = micron::is_signed_v<T> ? _mm256_min_epi16(A, B) : _mm256_min_epu16(A, B);
      mx = micron::is_signed_v<T> ? _mm256_max_epi16(A, B) : _mm256_max_epu16(A, B);
    } else {
      mn = micron::is_signed_v<T> ? _mm256_min_epi32(A, B) : _mm256_min_epu32(A, B);
      mx = micron::is_signed_v<T> ? _mm256_max_epi32(A, B) : _mm256_max_epu32(A, B);
    }
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(lo), asc ? mn : mx);
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(hi), asc ? mx : mn);
  }
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_neon)
  if constexpr ( micron::is_floating_point_v<T> ) {
    if constexpr ( sizeof(T) == 4 ) {
      float32x4_t A = vld1q_f32(lo), B = vld1q_f32(hi);
      float32x4_t mn = vminq_f32(A, B), mx = vmaxq_f32(A, B);
      vst1q_f32(lo, asc ? mn : mx);
      vst1q_f32(hi, asc ? mx : mn);
    }
#if defined(__micron_arch_arm64)
    else {
      float64x2_t A = vld1q_f64(lo), B = vld1q_f64(hi);
      float64x2_t mn = vminq_f64(A, B), mx = vmaxq_f64(A, B);
      vst1q_f64(lo, asc ? mn : mx);
      vst1q_f64(hi, asc ? mx : mn);
    }
#endif
  } else if constexpr ( sizeof(T) == 1 ) {
    if constexpr ( micron::is_signed_v<T> ) {
      int8x16_t A = vld1q_s8(reinterpret_cast<const signed char *>(lo)), B = vld1q_s8(reinterpret_cast<const signed char *>(hi));
      int8x16_t mn = vminq_s8(A, B), mx = vmaxq_s8(A, B);
      vst1q_s8(reinterpret_cast<signed char *>(lo), asc ? mn : mx);
      vst1q_s8(reinterpret_cast<signed char *>(hi), asc ? mx : mn);
    } else {
      uint8x16_t A = vld1q_u8(reinterpret_cast<const unsigned char *>(lo)), B = vld1q_u8(reinterpret_cast<const unsigned char *>(hi));
      uint8x16_t mn = vminq_u8(A, B), mx = vmaxq_u8(A, B);
      vst1q_u8(reinterpret_cast<unsigned char *>(lo), asc ? mn : mx);
      vst1q_u8(reinterpret_cast<unsigned char *>(hi), asc ? mx : mn);
    }
  } else if constexpr ( sizeof(T) == 2 ) {
    if constexpr ( micron::is_signed_v<T> ) {
      int16x8_t A = vld1q_s16(reinterpret_cast<const signed short *>(lo)), B = vld1q_s16(reinterpret_cast<const signed short *>(hi));
      int16x8_t mn = vminq_s16(A, B), mx = vmaxq_s16(A, B);
      vst1q_s16(reinterpret_cast<signed short *>(lo), asc ? mn : mx);
      vst1q_s16(reinterpret_cast<signed short *>(hi), asc ? mx : mn);
    } else {
      uint16x8_t A = vld1q_u16(reinterpret_cast<const unsigned short *>(lo)), B = vld1q_u16(reinterpret_cast<const unsigned short *>(hi));
      uint16x8_t mn = vminq_u16(A, B), mx = vmaxq_u16(A, B);
      vst1q_u16(reinterpret_cast<unsigned short *>(lo), asc ? mn : mx);
      vst1q_u16(reinterpret_cast<unsigned short *>(hi), asc ? mx : mn);
    }
  } else {      // 4-byte integer
    if constexpr ( micron::is_signed_v<T> ) {
      int32x4_t A = vld1q_s32(reinterpret_cast<const signed int *>(lo)), B = vld1q_s32(reinterpret_cast<const signed int *>(hi));
      int32x4_t mn = vminq_s32(A, B), mx = vmaxq_s32(A, B);
      vst1q_s32(reinterpret_cast<signed int *>(lo), asc ? mn : mx);
      vst1q_s32(reinterpret_cast<signed int *>(hi), asc ? mx : mn);
    } else {
      uint32x4_t A = vld1q_u32(reinterpret_cast<const unsigned int *>(lo)), B = vld1q_u32(reinterpret_cast<const unsigned int *>(hi));
      uint32x4_t mn = vminq_u32(A, B), mx = vmaxq_u32(A, B);
      vst1q_u32(reinterpret_cast<unsigned int *>(lo), asc ? mn : mx);
      vst1q_u32(reinterpret_cast<unsigned int *>(hi), asc ? mx : mn);
    }
  }
#else
  // scalar fallback (also used when __net_has_simd is false): element-wise
  constexpr usize L = __net_lanes<T>() > 1 ? __net_lanes<T>() : 1;
  for ( usize i = 0; i < L; ++i ) {
    T x = lo[i], y = hi[i];
    T mn = x < y ? x : y, mx = x < y ? y : x;
    lo[i] = asc ? mn : mx;
    hi[i] = asc ? mx : mn;
  }
#endif
}

template<typename T>
[[gnu::always_inline]] inline void
__scalar_stage(T *a, usize n, usize j, usize k) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    const usize l = i ^ j;
    if ( l > i ) {
      const bool asc = ((i & k) == 0);
      if ( (asc && a[l] < a[i]) || (!asc && a[i] < a[l]) ) {
        T t = a[i];
        a[i] = a[l];
        a[l] = t;
      }
    }
  }
}

template<typename T>
inline void
bitonic_sort_pow2(T *a, usize n) noexcept
{
  if ( n < 2 ) return;
  if constexpr ( __net_has_simd<T>() ) {
    constexpr usize W = __net_lanes<T>();
    for ( usize k = 2; k <= n; k <<= 1 ) {
      for ( usize j = k >> 1; j > 0; j >>= 1 ) {
        if ( j >= W ) {
          for ( usize i = 0; i < n; i += W ) {
            if ( i & j ) continue;      // high side of the pair; handled from the low side
            __block_cmpswap<T>(a + i, a + (i + j), (i & k) == 0);
          }
        } else {
          __scalar_stage<T>(a, n, j, k);
        }
      }
    }
  } else {
    for ( usize k = 2; k <= n; k <<= 1 )
      for ( usize j = k >> 1; j > 0; j >>= 1 ) __scalar_stage<T>(a, n, j, k);
  }
}

};      // namespace simd
};      // namespace micron
