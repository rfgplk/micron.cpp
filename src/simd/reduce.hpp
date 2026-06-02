//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// vectorised horizontal reductions over contiguous arrays

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
concept reducible_key = (micron::is_integral_v<K> || micron::is_floating_point_v<K>)
                        && (sizeof(K) == 1 || sizeof(K) == 2 || sizeof(K) == 4 || sizeof(K) == 8);

template<bool Less, typename K>
[[gnu::always_inline]] inline usize
__arg_extreme_scalar(const K *p, usize n) noexcept
{
  usize bi = 0;
  K best = p[0];
  for ( usize i = 1; i < n; ++i ) {
    if constexpr ( Less ) {
      if ( p[i] < best ) {
        best = p[i];
        bi = i;
      }
    } else {
      if ( p[i] > best ) {
        best = p[i];
        bi = i;
      }
    }
  }
  return bi;
}

template<typename K>
consteval bool
__arg_extreme_has_simd() noexcept
{
#if defined(__micron_x86_avx2)
  return (micron::is_integral_v<K> && sizeof(K) == 4) || micron::is_same_v<K, float> || micron::is_same_v<K, double>;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  return (micron::is_integral_v<K> && sizeof(K) <= 4) || micron::is_same_v<K, float> || micron::is_same_v<K, double>;
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  return (micron::is_integral_v<K> && sizeof(K) == 4) || micron::is_same_v<K, float>;
#else
  (void)0;
  return false;
#endif
}

template<bool Less, typename K>
[[gnu::always_inline]] inline K
__chunk_extreme(const K *p) noexcept
{
#if defined(__micron_x86_avx2)
  if constexpr ( micron::is_integral_v<K> && sizeof(K) == 4 ) {
    __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p));
    __m128i lo = _mm256_castsi256_si128(v);
    __m128i hi = _mm256_extracti128_si256(v, 1);
    __m128i m;
    if constexpr ( micron::is_signed_v<K> ) {
      m = Less ? _mm_min_epi32(lo, hi) : _mm_max_epi32(lo, hi);
      m = Less ? _mm_min_epi32(m, _mm_shuffle_epi32(m, 0x4E)) : _mm_max_epi32(m, _mm_shuffle_epi32(m, 0x4E));
      m = Less ? _mm_min_epi32(m, _mm_shuffle_epi32(m, 0xB1)) : _mm_max_epi32(m, _mm_shuffle_epi32(m, 0xB1));
    } else {
      m = Less ? _mm_min_epu32(lo, hi) : _mm_max_epu32(lo, hi);
      m = Less ? _mm_min_epu32(m, _mm_shuffle_epi32(m, 0x4E)) : _mm_max_epu32(m, _mm_shuffle_epi32(m, 0x4E));
      m = Less ? _mm_min_epu32(m, _mm_shuffle_epi32(m, 0xB1)) : _mm_max_epu32(m, _mm_shuffle_epi32(m, 0xB1));
    }
    return static_cast<K>(_mm_cvtsi128_si32(m));
  } else if constexpr ( micron::is_same_v<K, float> ) {
    __m256 v = _mm256_loadu_ps(p);
    __m128 lo = _mm256_castps256_ps128(v);
    __m128 hi = _mm256_extractf128_ps(v, 1);
    __m128 m = Less ? _mm_min_ps(lo, hi) : _mm_max_ps(lo, hi);
    m = Less ? _mm_min_ps(m, _mm_movehl_ps(m, m)) : _mm_max_ps(m, _mm_movehl_ps(m, m));
    m = Less ? _mm_min_ps(m, _mm_shuffle_ps(m, m, 0x55)) : _mm_max_ps(m, _mm_shuffle_ps(m, m, 0x55));
    return _mm_cvtss_f32(m);
  } else {      // double
    __m256d v = _mm256_loadu_pd(p);
    __m128d lo = _mm256_castpd256_pd128(v);
    __m128d hi = _mm256_extractf128_pd(v, 1);
    __m128d m = Less ? _mm_min_pd(lo, hi) : _mm_max_pd(lo, hi);
    m = Less ? _mm_min_pd(m, _mm_unpackhi_pd(m, m)) : _mm_max_pd(m, _mm_unpackhi_pd(m, m));
    return _mm_cvtsd_f64(m);
  }
#elif defined(__micron_arch_arm64) && defined(__micron_arm_neon)
  if constexpr ( micron::is_integral_v<K> && sizeof(K) == 1 ) {
    if constexpr ( micron::is_signed_v<K> ) {
      int8x16_t v = vld1q_s8(reinterpret_cast<const signed char *>(p));
      return static_cast<K>(Less ? vminvq_s8(v) : vmaxvq_s8(v));
    } else {
      uint8x16_t v = vld1q_u8(reinterpret_cast<const unsigned char *>(p));
      return static_cast<K>(Less ? vminvq_u8(v) : vmaxvq_u8(v));
    }
  } else if constexpr ( micron::is_integral_v<K> && sizeof(K) == 2 ) {
    if constexpr ( micron::is_signed_v<K> ) {
      int16x8_t v = vld1q_s16(reinterpret_cast<const signed short *>(p));
      return static_cast<K>(Less ? vminvq_s16(v) : vmaxvq_s16(v));
    } else {
      uint16x8_t v = vld1q_u16(reinterpret_cast<const unsigned short *>(p));
      return static_cast<K>(Less ? vminvq_u16(v) : vmaxvq_u16(v));
    }
  } else if constexpr ( micron::is_integral_v<K> && sizeof(K) == 4 ) {
    if constexpr ( micron::is_signed_v<K> ) {
      int32x4_t v = vld1q_s32(reinterpret_cast<const signed int *>(p));
      return static_cast<K>(Less ? vminvq_s32(v) : vmaxvq_s32(v));
    } else {
      uint32x4_t v = vld1q_u32(reinterpret_cast<const unsigned int *>(p));
      return static_cast<K>(Less ? vminvq_u32(v) : vmaxvq_u32(v));
    }
  } else if constexpr ( micron::is_same_v<K, float> ) {
    float32x4_t v = vld1q_f32(p);
    return Less ? vminvq_f32(v) : vmaxvq_f32(v);
  } else {      // double : 2-lane fold (no vminvq_f64)
    float64x2_t v = vld1q_f64(p);
    double a = vgetq_lane_f64(v, 0), b = vgetq_lane_f64(v, 1);
    return Less ? (a < b ? a : b) : (a > b ? a : b);
  }
#elif defined(__micron_arch_arm32) && defined(__micron_arm_neon)
  if constexpr ( micron::is_integral_v<K> && sizeof(K) == 4 ) {
    if constexpr ( micron::is_signed_v<K> ) {
      int32x4_t v = vld1q_s32(reinterpret_cast<const signed int *>(p));
      int32x2_t r = Less ? vpmin_s32(vget_low_s32(v), vget_high_s32(v)) : vpmax_s32(vget_low_s32(v), vget_high_s32(v));
      r = Less ? vpmin_s32(r, r) : vpmax_s32(r, r);
      return static_cast<K>(vget_lane_s32(r, 0));
    } else {
      uint32x4_t v = vld1q_u32(reinterpret_cast<const unsigned int *>(p));
      uint32x2_t r = Less ? vpmin_u32(vget_low_u32(v), vget_high_u32(v)) : vpmax_u32(vget_low_u32(v), vget_high_u32(v));
      r = Less ? vpmin_u32(r, r) : vpmax_u32(r, r);
      return static_cast<K>(vget_lane_u32(r, 0));
    }
  } else {      // float
    float32x4_t v = vld1q_f32(p);
    float32x2_t r = Less ? vpmin_f32(vget_low_f32(v), vget_high_f32(v)) : vpmax_f32(vget_low_f32(v), vget_high_f32(v));
    r = Less ? vpmin_f32(r, r) : vpmax_f32(r, r);
    return vget_lane_f32(r, 0);
  }
#else
  return p[0];
#endif
}

template<typename K>
consteval usize
__chunk_lanes() noexcept
{
#if defined(__micron_x86_avx2)
  return 32 / sizeof(K);
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_neon)
  return 16 / sizeof(K);
#else
  return 1;
#endif
}

template<bool Less, typename K>
inline usize
arg_extreme(const K *p, usize n) noexcept
{
  if ( n <= 1 ) return 0;

  if constexpr ( reducible_key<K> && __arg_extreme_has_simd<K>() ) {
    {
      constexpr usize L = __chunk_lanes<K>();
      usize bi = 0;
      K best = p[0];
      usize i = 0;
      for ( ; i + L <= n; i += L ) {
        K cv = __chunk_extreme<Less, K>(p + i);
        const bool better = Less ? (cv < best) : (cv > best);
        if ( better ) {
          for ( usize j = i; j < i + L; ++j ) {
            if constexpr ( Less ) {
              if ( p[j] < best ) {
                best = p[j];
                bi = j;
              }
            } else {
              if ( p[j] > best ) {
                best = p[j];
                bi = j;
              }
            }
          }
        }
      }
      for ( ; i < n; ++i ) {
        if constexpr ( Less ) {
          if ( p[i] < best ) {
            best = p[i];
            bi = i;
          }
        } else {
          if ( p[i] > best ) {
            best = p[i];
            bi = i;
          }
        }
      }
      return bi;
    }
  }
  return __arg_extreme_scalar<Less, K>(p, n);
}

// convenience wrappers
template<typename K>
[[gnu::always_inline]] inline usize
argmin(const K *p, usize n) noexcept
{
  return arg_extreme<true, K>(p, n);
}

template<typename K>
[[gnu::always_inline]] inline usize
argmax(const K *p, usize n) noexcept
{
  return arg_extreme<false, K>(p, n);
}

// quake specific code
// TODO: move out

inline usize
first_quake_violation(const u32 *nc, usize H, u32 an, u32 ad) noexcept
{
  if ( H < 2 ) return H;
  const usize lim = H - 1;      // i ranges over [0, lim)
  usize i = 0;

#if defined(__micron_x86_avx2)
  {
    const __m256i van = _mm256_set1_epi32(static_cast<int>(an));
    const __m256i vad = _mm256_set1_epi32(static_cast<int>(ad));
    for ( ; i + 8 <= lim; i += 8 ) {
      __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(nc + i));
      __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(nc + i + 1));
      __m256i lhs = _mm256_mullo_epi32(b, vad);
      __m256i rhs = _mm256_mullo_epi32(a, van);
      __m256i gt = _mm256_cmpgt_epi32(lhs, rhs);
      unsigned m = static_cast<unsigned>(_mm256_movemask_epi8(gt));
      if ( m ) return i + (static_cast<usize>(__builtin_ctz(m)) / 4);
    }
  }
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_neon)
  {
    const uint32x4_t van = vdupq_n_u32(an);
    const uint32x4_t vad = vdupq_n_u32(ad);
    for ( ; i + 4 <= lim; i += 4 ) {
      uint32x4_t a = vld1q_u32(nc + i);
      uint32x4_t b = vld1q_u32(nc + i + 1);
      uint32x4_t gt = vcgtq_u32(vmulq_u32(b, vad), vmulq_u32(a, van));
      u32 buf[4];
      vst1q_u32(buf, gt);
      for ( int k = 0; k < 4; ++k )
        if ( buf[k] ) return i + static_cast<usize>(k);
    }
  }
#endif

  for ( ; i < lim; ++i )
    if ( static_cast<u64>(ad) * nc[i + 1] > static_cast<u64>(an) * nc[i] ) return i;
  return H;
}

};      // namespace simd
};      // namespace micron
