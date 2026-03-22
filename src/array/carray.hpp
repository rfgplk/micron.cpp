//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../bits/__container.hpp"
#include "../defs.hpp"
#include "../type_traits.hpp"

#include "../algorithm/algorithm.hpp"
#include "../algorithm/memory.hpp"
#include "../except.hpp"
#include "../math/sqrt.hpp"
#include "../math/trig.hpp"
#include "../memory/addr.hpp"
#include "../memory/memory.hpp"
#include "../slice_forward.hpp"
#include "../tags.hpp"
#include "../types.hpp"

#include "../concepts.hpp"

#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
#include <immintrin.h>
#endif

#if defined(__micron_arch_arm_any) && defined(__micron_arm_neon)
#include <arm_neon.h>
#endif

namespace micron
{

// cache-optimized array class, stack allocated, notthreadsafe, mutable.
// no bounds checks, destroy_fast (no post-destroy zeroing), SIMD-dispatched
// arithmetic at compile time, index_sequence unrolled for small N
// default to 64
template <is_regular_object T, usize N = 64>
  requires(N > 0 and ((N * sizeof(T)) < (1 << 22)))
class carray
{
  alignas(64) T stack[N];

#if defined(__micron_x86_avx2)
  static constexpr bool __have_avx2 = true;
#else
  static constexpr bool __have_avx2 = false;
#endif
#if defined(__micron_x86_avx)
  static constexpr bool __have_avx = true;
#else
  static constexpr bool __have_avx = false;
#endif
#if defined(__micron_x86_sse4_1)
  static constexpr bool __have_sse41 = true;
#else
  static constexpr bool __have_sse41 = false;
#endif
#if defined(__micron_x86_sse2)
  static constexpr bool __have_sse2 = true;
#else
  static constexpr bool __have_sse2 = false;
#endif
#if defined(__micron_arm_neon)
  static constexpr bool __have_neon = true;
#else
  static constexpr bool __have_neon = false;
#endif

  // type classification by size
  static constexpr bool __is_f32 = sizeof(T) == 4 && micron::is_floating_point_v<T>;
  static constexpr bool __is_f64 = sizeof(T) == 8 && micron::is_floating_point_v<T>;
  static constexpr bool __is_i32 = sizeof(T) == 4 && micron::is_integral_v<T>;
  static constexpr bool __is_i16 = sizeof(T) == 2 && micron::is_integral_v<T>;

  // 256-bit vector eligibility
  static constexpr bool __v256 = (__have_avx && (__is_f32 || __is_f64)) || (__have_avx2 && (__is_i32 || __is_i16));

  // 128-bit vector eligibility
  static constexpr bool __v128_only
      = !__v256 && ((__have_sse2 && (__is_f32 || __is_f64 || __is_i32 || __is_i16)) || (__have_neon && (__is_f32 || __is_i32)));

  // i32 multiply needs SSE4.1 (mullo_epi32) or AVX2
  static constexpr bool __can_simd_mul_int = __is_i32 && (__have_avx2 || __have_sse41);
  static constexpr bool __can_simd_mul = __is_f32 || __is_f64 || __can_simd_mul_int;

  // integer division
  static constexpr bool __can_simd_div = __is_f32 || __is_f64;

  // vector width in elements
  static constexpr usize __velems = __v256 ? (32 / sizeof(T)) : (__v128_only ? (16 / sizeof(T)) : 0);

  // largest multiple of __velems <= N
  static constexpr usize __vbulk = __velems > 0 ? (N / __velems) * __velems : 0;
  static constexpr usize __vtail = N - __vbulk;
  static constexpr bool __use_simd = __vbulk > 0;

  // only prefetch when array spans enough cache lines
  static constexpr bool __use_prefetch = (N * sizeof(T)) > 4096;
  static constexpr usize __prefetch_dist = 256 / sizeof(T);     // 4 cache lines ahead (64bytes per line usually)

  // index_sequence unroll threshold
  static constexpr usize __unroll_threshold = 32;
  static constexpr bool __use_unroll = !__use_simd && N <= __unroll_threshold;

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_add_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[I] += v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_sub_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[I] -= v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_mul_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[I] *= v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_div_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[I] /= v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_mod_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[I] %= v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_add_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[I] += src[I]), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_sub_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[I] -= src[I]), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_mul_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[I] *= src[I]), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_div_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[I] /= src[I]), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __ur_mod_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[I] %= src[I]), ...);
  }

  // scalar tails
  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_add_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] += v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_sub_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] -= v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_mul_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] *= v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_div_scalar(const T &v, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] /= v), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_add_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] += src[__vbulk + I]), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_sub_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] -= src[__vbulk + I]), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_mul_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] *= src[__vbulk + I]), ...);
  }

  template <usize... I>
  inline __attribute__((always_inline)) void
  __tail_div_array(const T *__restrict src, micron::index_sequence<I...>)
  {
    ((stack[__vbulk + I] /= src[__vbulk + I]), ...);
  }

  inline __attribute__((always_inline)) void
  __v_add_broadcast(const T &scalar)
  {
#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
    if constexpr ( __v256 && __is_f32 ) {
      __m256 sv = _mm256_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems ) {
        if constexpr ( __use_prefetch )
          __builtin_prefetch(stack + i + __prefetch_dist, 0, 1);
        _mm256_store_ps(stack + i, _mm256_add_ps(_mm256_load_ps(stack + i), sv));
      }
    } else if constexpr ( __v256 && __is_f64 ) {
      __m256d sv = _mm256_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems ) {
        if constexpr ( __use_prefetch )
          __builtin_prefetch(stack + i + __prefetch_dist, 0, 1);
        _mm256_store_pd(stack + i, _mm256_add_pd(_mm256_load_pd(stack + i), sv));
      }
    } else if constexpr ( __v256 && __is_i32 ) {
      __m256i sv = _mm256_set1_epi32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_si256(reinterpret_cast<__m256i *>(stack + i),
                           _mm256_add_epi32(_mm256_load_si256(reinterpret_cast<const __m256i *>(stack + i)), sv));
    } else if constexpr ( __v128_only && __is_f32 ) {
      __m128 sv = _mm_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_ps(stack + i, _mm_add_ps(_mm_load_ps(stack + i), sv));
    } else if constexpr ( __v128_only && __is_f64 ) {
      __m128d sv = _mm_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_pd(stack + i, _mm_add_pd(_mm_load_pd(stack + i), sv));
    } else if constexpr ( __v128_only && __is_i32 ) {
      __m128i sv = _mm_set1_epi32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_si128(reinterpret_cast<__m128i *>(stack + i),
                        _mm_add_epi32(_mm_load_si128(reinterpret_cast<const __m128i *>(stack + i)), sv));
    }
#elif defined(__micron_arm_neon)
    if constexpr ( __have_neon && __is_f32 ) {
      float32x4_t sv = vdupq_n_f32(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        vst1q_f32(stack + i, vaddq_f32(vld1q_f32(stack + i), sv));
    } else if constexpr ( __have_neon && __is_i32 ) {
      int32x4_t sv = vdupq_n_s32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        vst1q_s32(reinterpret_cast<int *>(stack + i), vaddq_s32(vld1q_s32(reinterpret_cast<const int *>(stack + i)), sv));
    }
#endif
    if constexpr ( __vtail > 0 )
      __tail_add_scalar(scalar, micron::make_index_sequence<__vtail>{});
  }

  inline __attribute__((always_inline)) void
  __v_sub_broadcast(const T &scalar)
  {
#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
    if constexpr ( __v256 && __is_f32 ) {
      __m256 sv = _mm256_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_ps(stack + i, _mm256_sub_ps(_mm256_load_ps(stack + i), sv));
    } else if constexpr ( __v256 && __is_f64 ) {
      __m256d sv = _mm256_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_pd(stack + i, _mm256_sub_pd(_mm256_load_pd(stack + i), sv));
    } else if constexpr ( __v256 && __is_i32 ) {
      __m256i sv = _mm256_set1_epi32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_si256(reinterpret_cast<__m256i *>(stack + i),
                           _mm256_sub_epi32(_mm256_load_si256(reinterpret_cast<const __m256i *>(stack + i)), sv));
    } else if constexpr ( __v128_only && __is_f32 ) {
      __m128 sv = _mm_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_ps(stack + i, _mm_sub_ps(_mm_load_ps(stack + i), sv));
    } else if constexpr ( __v128_only && __is_f64 ) {
      __m128d sv = _mm_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_pd(stack + i, _mm_sub_pd(_mm_load_pd(stack + i), sv));
    } else if constexpr ( __v128_only && __is_i32 ) {
      __m128i sv = _mm_set1_epi32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_si128(reinterpret_cast<__m128i *>(stack + i),
                        _mm_sub_epi32(_mm_load_si128(reinterpret_cast<const __m128i *>(stack + i)), sv));
    }
#elif defined(__micron_arm_neon)
    if constexpr ( __have_neon && __is_f32 ) {
      float32x4_t sv = vdupq_n_f32(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        vst1q_f32(stack + i, vsubq_f32(vld1q_f32(stack + i), sv));
    } else if constexpr ( __have_neon && __is_i32 ) {
      int32x4_t sv = vdupq_n_s32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        vst1q_s32(reinterpret_cast<int *>(stack + i), vsubq_s32(vld1q_s32(reinterpret_cast<const int *>(stack + i)), sv));
    }
#endif
    if constexpr ( __vtail > 0 )
      __tail_sub_scalar(scalar, micron::make_index_sequence<__vtail>{});
  }

  inline __attribute__((always_inline)) void
  __v_mul_broadcast(const T &scalar)
  {
#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
    if constexpr ( __v256 && __is_f32 ) {
      __m256 sv = _mm256_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_ps(stack + i, _mm256_mul_ps(_mm256_load_ps(stack + i), sv));
    } else if constexpr ( __v256 && __is_f64 ) {
      __m256d sv = _mm256_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_pd(stack + i, _mm256_mul_pd(_mm256_load_pd(stack + i), sv));
    } else if constexpr ( __v256 && __is_i32 && __can_simd_mul_int ) {
      __m256i sv = _mm256_set1_epi32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_si256(reinterpret_cast<__m256i *>(stack + i),
                           _mm256_mullo_epi32(_mm256_load_si256(reinterpret_cast<const __m256i *>(stack + i)), sv));
    } else if constexpr ( __v128_only && __is_f32 ) {
      __m128 sv = _mm_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_ps(stack + i, _mm_mul_ps(_mm_load_ps(stack + i), sv));
    } else if constexpr ( __v128_only && __is_f64 ) {
      __m128d sv = _mm_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_pd(stack + i, _mm_mul_pd(_mm_load_pd(stack + i), sv));
    } else if constexpr ( __v128_only && __is_i32 && __can_simd_mul_int ) {
      __m128i sv = _mm_set1_epi32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_si128(reinterpret_cast<__m128i *>(stack + i),
                        _mm_mullo_epi32(_mm_load_si128(reinterpret_cast<const __m128i *>(stack + i)), sv));
    }
#elif defined(__micron_arm_neon)
    if constexpr ( __have_neon && __is_f32 ) {
      float32x4_t sv = vdupq_n_f32(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        vst1q_f32(stack + i, vmulq_f32(vld1q_f32(stack + i), sv));
    } else if constexpr ( __have_neon && __is_i32 ) {
      int32x4_t sv = vdupq_n_s32(static_cast<int>(scalar));
      for ( usize i = 0; i < __vbulk; i += __velems )
        vst1q_s32(reinterpret_cast<int *>(stack + i), vmulq_s32(vld1q_s32(reinterpret_cast<const int *>(stack + i)), sv));
    }
#endif
    if constexpr ( __vtail > 0 )
      __tail_mul_scalar(scalar, micron::make_index_sequence<__vtail>{});
  }

  inline __attribute__((always_inline)) void
  __v_div_broadcast(const T &scalar)
  {
    // integer div has no SIMD insts
#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
    if constexpr ( __v256 && __is_f32 ) {
      __m256 sv = _mm256_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_ps(stack + i, _mm256_div_ps(_mm256_load_ps(stack + i), sv));
    } else if constexpr ( __v256 && __is_f64 ) {
      __m256d sv = _mm256_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm256_store_pd(stack + i, _mm256_div_pd(_mm256_load_pd(stack + i), sv));
    } else if constexpr ( __v128_only && __is_f32 ) {
      __m128 sv = _mm_set1_ps(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_ps(stack + i, _mm_div_ps(_mm_load_ps(stack + i), sv));
    } else if constexpr ( __v128_only && __is_f64 ) {
      __m128d sv = _mm_set1_pd(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        _mm_store_pd(stack + i, _mm_div_pd(_mm_load_pd(stack + i), sv));
    }
#elif defined(__micron_arm_neon)
    if constexpr ( __have_neon && __is_f32 ) {
      float32x4_t sv = vdupq_n_f32(scalar);
      for ( usize i = 0; i < __vbulk; i += __velems )
        vst1q_f32(stack + i, vdivq_f32(vld1q_f32(stack + i), sv));
    }
#endif
    if constexpr ( __vtail > 0 )
      __tail_div_scalar(scalar, micron::make_index_sequence<__vtail>{});
  }

  template <usize M>
  inline __attribute__((always_inline)) void
  __v_add_elements(const T *__restrict src)
  {
    static constexpr usize bulk = __velems > 0 ? (M / __velems) * __velems : 0;
    static constexpr usize tail = M - bulk;
#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
    if constexpr ( __v256 && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_ps(stack + i, _mm256_add_ps(_mm256_load_ps(stack + i), _mm256_load_ps(src + i)));
    } else if constexpr ( __v256 && __is_f64 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_pd(stack + i, _mm256_add_pd(_mm256_load_pd(stack + i), _mm256_load_pd(src + i)));
    } else if constexpr ( __v256 && __is_i32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_si256(reinterpret_cast<__m256i *>(stack + i),
                           _mm256_add_epi32(_mm256_load_si256(reinterpret_cast<const __m256i *>(stack + i)),
                                            _mm256_load_si256(reinterpret_cast<const __m256i *>(src + i))));
    } else if constexpr ( __v128_only && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_ps(stack + i, _mm_add_ps(_mm_load_ps(stack + i), _mm_load_ps(src + i)));
    } else if constexpr ( __v128_only && __is_f64 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_pd(stack + i, _mm_add_pd(_mm_load_pd(stack + i), _mm_load_pd(src + i)));
    } else if constexpr ( __v128_only && __is_i32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_si128(reinterpret_cast<__m128i *>(stack + i), _mm_add_epi32(_mm_load_si128(reinterpret_cast<const __m128i *>(stack + i)),
                                                                              _mm_load_si128(reinterpret_cast<const __m128i *>(src + i))));
    }
#elif defined(__micron_arm_neon)
    if constexpr ( __have_neon && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        vst1q_f32(stack + i, vaddq_f32(vld1q_f32(stack + i), vld1q_f32(src + i)));
    } else if constexpr ( __have_neon && __is_i32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        vst1q_s32(reinterpret_cast<int *>(stack + i),
                  vaddq_s32(vld1q_s32(reinterpret_cast<const int *>(stack + i)), vld1q_s32(reinterpret_cast<const int *>(src + i))));
    }
#endif
    if constexpr ( tail > 0 )
      for ( usize i = bulk; i < M; i++ )
        stack[i] += src[i];
  }

  template <usize M>
  inline __attribute__((always_inline)) void
  __v_sub_elements(const T *__restrict src)
  {
    static constexpr usize bulk = __velems > 0 ? (M / __velems) * __velems : 0;
    static constexpr usize tail = M - bulk;
#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
    if constexpr ( __v256 && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_ps(stack + i, _mm256_sub_ps(_mm256_load_ps(stack + i), _mm256_load_ps(src + i)));
    } else if constexpr ( __v256 && __is_f64 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_pd(stack + i, _mm256_sub_pd(_mm256_load_pd(stack + i), _mm256_load_pd(src + i)));
    } else if constexpr ( __v256 && __is_i32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_si256(reinterpret_cast<__m256i *>(stack + i),
                           _mm256_sub_epi32(_mm256_load_si256(reinterpret_cast<const __m256i *>(stack + i)),
                                            _mm256_load_si256(reinterpret_cast<const __m256i *>(src + i))));
    } else if constexpr ( __v128_only && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_ps(stack + i, _mm_sub_ps(_mm_load_ps(stack + i), _mm_load_ps(src + i)));
    } else if constexpr ( __v128_only && __is_f64 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_pd(stack + i, _mm_sub_pd(_mm_load_pd(stack + i), _mm_load_pd(src + i)));
    } else if constexpr ( __v128_only && __is_i32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_si128(reinterpret_cast<__m128i *>(stack + i), _mm_sub_epi32(_mm_load_si128(reinterpret_cast<const __m128i *>(stack + i)),
                                                                              _mm_load_si128(reinterpret_cast<const __m128i *>(src + i))));
    }
#elif defined(__micron_arm_neon)
    if constexpr ( __have_neon && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        vst1q_f32(stack + i, vsubq_f32(vld1q_f32(stack + i), vld1q_f32(src + i)));
    } else if constexpr ( __have_neon && __is_i32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        vst1q_s32(reinterpret_cast<int *>(stack + i),
                  vsubq_s32(vld1q_s32(reinterpret_cast<const int *>(stack + i)), vld1q_s32(reinterpret_cast<const int *>(src + i))));
    }
#endif
    if constexpr ( tail > 0 )
      for ( usize i = bulk; i < M; i++ )
        stack[i] -= src[i];
  }

  template <usize M>
  inline __attribute__((always_inline)) void
  __v_mul_elements(const T *__restrict src)
  {
    static constexpr usize bulk = (__velems > 0 && __can_simd_mul) ? (M / __velems) * __velems : 0;
    static constexpr usize tail = M - bulk;
#if defined(__micron_arch_x86_any) && __micron_x86_simd_width >= 128
    if constexpr ( __v256 && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_ps(stack + i, _mm256_mul_ps(_mm256_load_ps(stack + i), _mm256_load_ps(src + i)));
    } else if constexpr ( __v256 && __is_f64 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_pd(stack + i, _mm256_mul_pd(_mm256_load_pd(stack + i), _mm256_load_pd(src + i)));
    } else if constexpr ( __v256 && __is_i32 && __can_simd_mul_int ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm256_store_si256(reinterpret_cast<__m256i *>(stack + i),
                           _mm256_mullo_epi32(_mm256_load_si256(reinterpret_cast<const __m256i *>(stack + i)),
                                              _mm256_load_si256(reinterpret_cast<const __m256i *>(src + i))));
    } else if constexpr ( __v128_only && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_ps(stack + i, _mm_mul_ps(_mm_load_ps(stack + i), _mm_load_ps(src + i)));
    } else if constexpr ( __v128_only && __is_f64 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_pd(stack + i, _mm_mul_pd(_mm_load_pd(stack + i), _mm_load_pd(src + i)));
    } else if constexpr ( __v128_only && __is_i32 && __can_simd_mul_int ) {
      for ( usize i = 0; i < bulk; i += __velems )
        _mm_store_si128(reinterpret_cast<__m128i *>(stack + i),
                        _mm_mullo_epi32(_mm_load_si128(reinterpret_cast<const __m128i *>(stack + i)),
                                        _mm_load_si128(reinterpret_cast<const __m128i *>(src + i))));
    }
#elif defined(__micron_arm_neon)
    if constexpr ( __have_neon && __is_f32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        vst1q_f32(stack + i, vmulq_f32(vld1q_f32(stack + i), vld1q_f32(src + i)));
    } else if constexpr ( __have_neon && __is_i32 ) {
      for ( usize i = 0; i < bulk; i += __velems )
        vst1q_s32(reinterpret_cast<int *>(stack + i),
                  vmulq_s32(vld1q_s32(reinterpret_cast<const int *>(stack + i)), vld1q_s32(reinterpret_cast<const int *>(src + i))));
    }
#endif
    if constexpr ( tail > 0 )
      for ( usize i = bulk; i < M; i++ )
        stack[i] *= src[i];
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_add(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      if constexpr ( __use_simd )
        __v_add_broadcast(static_cast<T>(v));
      else if constexpr ( __use_unroll )
        __ur_add_scalar(static_cast<T>(v), micron::make_index_sequence<N>{});
      else {
        T *__restrict dst = stack;
#pragma GCC ivdep
        for ( usize i = 0; i < N; i++ )
          dst[i] += static_cast<T>(v);
      }
    } else {
      const auto *__restrict src = v.data();
      const usize bound = v.size() < N ? v.size() : N;
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( usize i = 0; i < bound; i++ )
        dst[i] += src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_sub(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      if constexpr ( __use_simd )
        __v_sub_broadcast(static_cast<T>(v));
      else if constexpr ( __use_unroll )
        __ur_sub_scalar(static_cast<T>(v), micron::make_index_sequence<N>{});
      else {
        T *__restrict dst = stack;
#pragma GCC ivdep
        for ( usize i = 0; i < N; i++ )
          dst[i] -= static_cast<T>(v);
      }
    } else {
      const auto *__restrict src = v.data();
      const usize bound = v.size() < N ? v.size() : N;
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( usize i = 0; i < bound; i++ )
        dst[i] -= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_mul(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      if constexpr ( __use_simd && __can_simd_mul )
        __v_mul_broadcast(static_cast<T>(v));
      else if constexpr ( __use_unroll )
        __ur_mul_scalar(static_cast<T>(v), micron::make_index_sequence<N>{});
      else {
        T *__restrict dst = stack;
#pragma GCC ivdep
        for ( usize i = 0; i < N; i++ )
          dst[i] *= static_cast<T>(v);
      }
    } else {
      const auto *__restrict src = v.data();
      const usize bound = v.size() < N ? v.size() : N;
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( usize i = 0; i < bound; i++ )
        dst[i] *= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_div(const U &v)
  {
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      if constexpr ( __use_simd && __can_simd_div )
        __v_div_broadcast(static_cast<T>(v));
      else if constexpr ( __use_unroll )
        __ur_div_scalar(static_cast<T>(v), micron::make_index_sequence<N>{});
      else {
        T *__restrict dst = stack;
#pragma GCC ivdep
        for ( usize i = 0; i < N; i++ )
          dst[i] /= static_cast<T>(v);
      }
    } else {
      const auto *__restrict src = v.data();
      const usize bound = v.size() < N ? v.size() : N;
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( usize i = 0; i < bound; i++ )
        dst[i] /= src[i];
    }
  }

  template <typename U>
  inline __attribute__((always_inline)) void
  __apply_mod(const U &v)
  {
    // SIMD doesn't support modulos
    if constexpr ( micron::is_arithmetic_v<micron::remove_cv_t<U>> ) {
      if constexpr ( __use_unroll )
        __ur_mod_scalar(static_cast<T>(v), micron::make_index_sequence<N>{});
      else {
        T *__restrict dst = stack;
#pragma GCC ivdep
        for ( usize i = 0; i < N; i++ )
          dst[i] %= static_cast<T>(v);
      }
    } else {
      const auto *__restrict src = v.data();
      const usize bound = v.size() < N ? v.size() : N;
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( usize i = 0; i < bound; i++ )
        dst[i] %= src[i];
    }
  }

#define __carray_dispatch_scalar(OP, SIMD_FN, UR_FN, SIMD_GUARD)                                                                           \
  do {                                                                                                                                     \
    if constexpr ( __use_simd && (SIMD_GUARD) )                                                                                            \
      SIMD_FN(o);                                                                                                                          \
    else if constexpr ( __use_unroll )                                                                                                     \
      UR_FN(o, micron::make_index_sequence<N>{});                                                                                          \
    else {                                                                                                                                 \
      T *__restrict dst = stack;                                                                                                           \
      _Pragma("GCC ivdep") for ( usize i = 0; i < N; i++ ) dst[i] OP o;                                                                    \
    }                                                                                                                                      \
  } while ( 0 )

#define __carray_dispatch_array(OP, SIMD_FN, UR_FN, SIMD_GUARD, M)                                                                         \
  do {                                                                                                                                     \
    if constexpr ( __use_simd && (SIMD_GUARD) )                                                                                            \
      SIMD_FN<M>(o.stack);                                                                                                                 \
    else if constexpr ( M <= __unroll_threshold )                                                                                          \
      UR_FN(o.stack, micron::make_index_sequence<M>{});                                                                                    \
    else {                                                                                                                                 \
      T *__restrict dst = stack;                                                                                                           \
      const T *__restrict src = o.stack;                                                                                                   \
      _Pragma("GCC ivdep") for ( usize i = 0; i < M; i++ ) dst[i] OP src[i];                                                               \
    }                                                                                                                                      \
  } while ( 0 )

public:
  using category_type = array_tag;
  using mutability_type = mutable_tag;
  using memory_type = stack_tag;
  typedef usize size_type;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;
  static constexpr size_type length = N;

  // NOTE: destroy_fast
  ~carray() { __impl_container::destroy_fast<N, T>(micron::addr(stack[0])); }

  carray() { __impl_container::construct<N, T>(micron::addr(stack[0]), T{}); }

  template <typename Fn>
    requires(micron::is_function_v<Fn> or micron::is_invocable_v<Fn>)
  carray(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::generate(begin(), end(), fn);
  }

  template <typename Fn>
    requires(micron::is_invocable_v<Fn, T *> or micron::is_invocable_v<Fn, T>)
  carray(Fn &&fn)
  {
    __impl_container::construct<N, T>(micron::addr(stack[0]), T{});
    micron::transform(begin(), end(), fn);
  }

  carray(const T &o) { __impl_container::construct<N, T>(micron::addr(stack[0]), o); }

  carray(const std::initializer_list<T> &&lst)
  {
    if ( lst.size() > N )
      exc<except::runtime_error>("micron::carray init_list too large.");
    size_type i = 0;
    for ( auto &&value : lst )
      stack[i++] = micron::move(value);
    if ( lst.size() < N )
      __impl_container::construct(micron::addr(stack[lst.size()]), T{}, N - lst.size());
  }

  template <is_container A>
    requires(!micron::is_same_v<A, carray>)
  carray(A &&o)
  {
    if ( o.size() < N )
      exc<except::runtime_error>("micron::carray carray(A&&) invalid size");
    if constexpr ( micron::is_rvalue_reference_v<A &&> )
      __impl_container::move<N, T>(micron::addr(stack[0]), o.begin());
    else
      __impl_container::copy<N, T>(micron::addr(stack[0]), o.begin());
  }

  template <class C> carray(const slice<T, C> &o)
  {
    const size_type bound = o.size() < N ? o.size() : N;
    __impl_container::copy(micron::addr(stack[0]), o.begin(), bound);
    if ( bound < N )
      __impl_container::construct(micron::addr(stack[bound]), T{}, N - bound);
  }

  carray(const carray &o) { __impl_container::copy<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0])); }

  carray(carray &&o) { __impl_container::move<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0])); }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // iterators

  [[nodiscard]] iterator
  begin() noexcept
  {
    return micron::addr(stack[0]);
  }

  [[nodiscard]] const_iterator
  cbegin() const noexcept
  {
    return micron::addr(stack[0]);
  }

  [[nodiscard]] iterator
  end() noexcept
  {
    return micron::addr(stack[N]);
  }

  [[nodiscard]] const_iterator
  cend() const noexcept
  {
    return micron::addr(stack[N]);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // capacity

  size_type
  size() const noexcept
  {
    return N;
  }

  size_type
  max_size() const noexcept
  {
    return N;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // observers

  auto *
  addr() noexcept
  {
    return this;
  }

  const auto *
  addr() const noexcept
  {
    return this;
  }

  [[nodiscard]] iterator
  data() noexcept
  {
    return stack;
  }

  [[nodiscard]] const_iterator
  data() const noexcept
  {
    return stack;
  }

  void
  clear()
  {
    // NOTE: destroy zeroes by default, carray cant hold objects always valid
    __impl_container::destroy<N, T>(micron::addr(stack[0]));
    //__impl_container::construct<N, T>(micron::addr(stack[0]), T{});
  }

  inline T &
  at(const size_type i) noexcept
  {
    return stack[i];
  }

  inline const T &
  at(const size_type i) const noexcept
  {
    return stack[i];
  }

  inline iterator
  get(const size_type n) noexcept
  {
    return (stack + n);
  }

  inline const_iterator
  get(const size_type n) const noexcept
  {
    return (stack + n);
  }

  inline const_iterator
  cget(const size_type n) const noexcept
  {
    return (stack + n);
  }

  inline T &
  operator[](const size_type i) noexcept
  {
    return stack[i];
  }

  inline const T &
  operator[](const size_type i) const noexcept
  {
    return stack[i];
  }

  template <class C>
  inline slice<T, C>
  operator[]()
  {
    return slice<T, C>(begin(), end());
  }

  template <class C>
  inline const slice<T, C>
  operator[]() const
  {
    return slice<T, C>(cbegin(), cend());
  }

  template <class C>
  inline __attribute__((always_inline)) const slice<T, C>
  operator[](size_type from, size_type to) const
  {
    return slice<T, C>(get(from), get(to));
  }

  template <class C>
  inline __attribute__((always_inline)) slice<T, C>
  operator[](size_type from, size_type to)
  {
    return slice<T, C>(get(from), get(to));
  }

  byte *
  operator&() noexcept
  {
    return reinterpret_cast<byte *>(stack);
  }

  const byte *
  operator&() const noexcept
  {
    return reinterpret_cast<const byte *>(stack);
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // assignment

  template <typename F, size_type M>
  carray &
  operator=(T (&o)[M])
    requires micron::is_array_v<F> && (M <= N)
  {
    __impl_container::copy_assign<M, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }

  template <typename F>
  carray &
  operator=(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::ctypeset<N>(micron::addr(stack[0]), o);
    return *this;
  }

  template <is_constexpr_container A>
  carray &
  operator=(const A &o)
  {
    if constexpr ( N <= A::length )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign<A::length, T>(micron::addr(stack[0]), micron::addr(o[0]));
    return *this;
  }

  template <is_container A>
    requires(!micron::is_same_v<A, carray>)
  carray &
  operator=(const A &o)
  {
    if ( N <= o.size() )
      __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o[0]));
    else
      __impl_container::copy_assign(micron::addr(stack[0]), micron::addr(o[0]), o.size());
    return *this;
  }

  carray &
  operator=(const carray &o)
  {
    __impl_container::copy_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  carray &
  operator=(carray &&o)
  {
    __impl_container::move_assign<N, T>(micron::addr(stack[0]), micron::addr(o.stack[0]));
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // binary arithmetic

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray
  operator+(const carray<T, M> &o) const
  {
    carray arr(*this);
    __carray_dispatch_array(+=, arr.template __v_add_elements, arr.__ur_add_array, true, M);
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray
  operator-(const carray<T, M> &o) const
  {
    carray arr(*this);
    __carray_dispatch_array(-=, arr.template __v_sub_elements, arr.__ur_sub_array, true, M);
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray
  operator*(const carray<T, M> &o) const
  {
    carray arr(*this);
    __carray_dispatch_array(*=, arr.template __v_mul_elements, arr.__ur_mul_array, __can_simd_mul, M);
    return arr;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray
  operator/(const carray<T, M> &o) const
  {
    carray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
    if constexpr ( M <= __unroll_threshold )
      arr.__ur_div_array(src, micron::make_index_sequence<M>{});
    else {
#pragma GCC ivdep
      for ( size_type i = 0; i < M; i++ )
        dst[i] /= src[i];
    }
    return arr;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) carray
  operator%(const carray<T, M> &o) const
  {
    carray arr(*this);
    T *__restrict dst = arr.stack;
    const T *__restrict src = o.stack;
    if constexpr ( M <= __unroll_threshold )
      arr.__ur_mod_array(src, micron::make_index_sequence<M>{});
    else {
#pragma GCC ivdep
      for ( size_type i = 0; i < M; i++ )
        dst[i] %= src[i];
    }
    return arr;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // reductions

  T
  sum(void) const noexcept
  {
    const T *__restrict src = stack;
    T sm = T{};
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      sm += src[i];
    return sm;
  }

  T
  mul_reduce(void) const noexcept
  {
    const T *__restrict src = stack;
    T m = src[0];
#pragma GCC ivdep
    for ( size_type i = 1; i < N; i++ )
      m *= src[i];
    return m;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignments

  inline __attribute__((always_inline)) carray &
  operator+=(const T &o)
  {
    __carray_dispatch_scalar(+=, __v_add_broadcast, __ur_add_scalar, true);
    return *this;
  }

  inline __attribute__((always_inline)) carray &
  operator-=(const T &o)
  {
    __carray_dispatch_scalar(-=, __v_sub_broadcast, __ur_sub_scalar, true);
    return *this;
  }

  inline __attribute__((always_inline)) carray &
  operator*=(const T &o)
  {
    __carray_dispatch_scalar(*=, __v_mul_broadcast, __ur_mul_scalar, __can_simd_mul);
    return *this;
  }

  inline __attribute__((always_inline)) carray &
  operator/=(const T &o)
  {
    __carray_dispatch_scalar(/=, __v_div_broadcast, __ur_div_scalar, __can_simd_div);
    return *this;
  }

  inline __attribute__((always_inline)) carray &
  operator%=(const T &o)
    requires micron::is_integral_v<T>
  {
    // no SIMD for modulo
    if constexpr ( __use_unroll )
      __ur_mod_scalar(o, micron::make_index_sequence<N>{});
    else {
      T *__restrict dst = stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < N; i++ )
        dst[i] %= o;
    }
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // compound assignments

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray &
  operator+=(const carray<T, M> &o)
  {
    __carray_dispatch_array(+=, this->template __v_add_elements, __ur_add_array, true, M);
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray &
  operator-=(const carray<T, M> &o)
  {
    __carray_dispatch_array(-=, this->template __v_sub_elements, __ur_sub_array, true, M);
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray &
  operator*=(const carray<T, M> &o)
  {
    __carray_dispatch_array(*=, this->template __v_mul_elements, __ur_mul_array, __can_simd_mul, M);
    return *this;
  }

  template <size_type M>
    requires(M <= N)
  inline __attribute__((always_inline)) carray &
  operator/=(const carray<T, M> &o)
  {
    if constexpr ( M <= __unroll_threshold )
      __ur_div_array(o.stack, micron::make_index_sequence<M>{});
    else {
      T *__restrict dst = stack;
      const T *__restrict src = o.stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < M; i++ )
        dst[i] /= src[i];
    }
    return *this;
  }

  template <size_type M>
    requires(M <= N && micron::is_integral_v<T>)
  inline __attribute__((always_inline)) carray &
  operator%=(const carray<T, M> &o)
  {
    if constexpr ( M <= __unroll_threshold )
      __ur_mod_array(o.stack, micron::make_index_sequence<M>{});
    else {
      T *__restrict dst = stack;
      const T *__restrict src = o.stack;
#pragma GCC ivdep
      for ( size_type i = 0; i < M; i++ )
        dst[i] %= src[i];
    }
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // named in-place arithmetic

  void
  mul(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] *= n;
  }

  void
  div(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] /= n;
  }

  void
  sub(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] -= n;
  }

  void
  add(const size_type n)
  {
    T *__restrict dst = stack;
#pragma GCC ivdep
    for ( size_type i = 0; i < N; i++ )
      dst[i] += n;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // queries

  bool
  all(const T &o) const noexcept
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] != o )
        return false;
    return true;
  }

  bool
  any(const T &o) const noexcept
  {
    for ( size_type i = 0; i < N; i++ )
      if ( stack[i] == o )
        return true;
    return false;
  }

  void
  sqrt(void)
  {
    T *__restrict dst = stack;
    for ( size_type i = 0; i < N; i++ )
      dst[i] = math::sqrt(static_cast<float>(dst[i]));
  }

  template <typename F>
  carray &
  fill(const F &o)
    requires micron::is_fundamental_v<F>
  {
    micron::ctypeset<N>(micron::addr(stack[0]), o);
    return *this;
  }

  template <typename F>
  carray &
  fill(const F &o)
  {
    __impl_container::set<N, T>(micron::addr(stack[0]), o);
    return *this;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // type queries

  static constexpr bool
  is_pod() noexcept
  {
    return micron::is_pod_v<T>;
  }

  static constexpr bool
  is_class() noexcept
  {
    return micron::is_class_v<T>;
  }

  static constexpr bool
  is_trivial() noexcept
  {
    return micron::is_trivial_v<T>;
  }

#undef __carray_dispatch_scalar
#undef __carray_dispatch_array
};

};     // namespace micron
