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
#error "__fma.hpp included on a non-x86 build"
#endif

// freestanding FMA [fmaintrin.h] & AVX-512 FMA

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

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("fma")]] static inline

// 128bits
__inline_g __m128
_mm_fmadd_ps(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfmaddps((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fmsub_ps(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfmsubps((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fnmadd_ps(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfnmaddps((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fnmsub_ps(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfnmsubps((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fmaddsub_ps(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfmaddsubps((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fmsubadd_ps(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfmaddsubps((__v4sf)a, (__v4sf)b, -(__v4sf)c);
}

__inline_g __m128d
_mm_fmadd_pd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfmaddpd((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fmsub_pd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfmsubpd((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fnmadd_pd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfnmaddpd((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fnmsub_pd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfnmsubpd((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fmaddsub_pd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfmaddsubpd((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fmsubadd_pd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfmaddsubpd((__v2df)a, (__v2df)b, -(__v2df)c);
}

__inline_g __m128
_mm_fmadd_ss(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfmaddss3((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fmsub_ss(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfmsubss3((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fnmadd_ss(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfnmaddss3((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128
_mm_fnmsub_ss(__m128 a, __m128 b, __m128 c) noexcept
{
  return (__m128)__builtin_ia32_vfnmsubss3((__v4sf)a, (__v4sf)b, (__v4sf)c);
}

__inline_g __m128d
_mm_fmadd_sd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfmaddsd3((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fmsub_sd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfmsubsd3((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fnmadd_sd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfnmaddsd3((__v2df)a, (__v2df)b, (__v2df)c);
}

__inline_g __m128d
_mm_fnmsub_sd(__m128d a, __m128d b, __m128d c) noexcept
{
  return (__m128d)__builtin_ia32_vfnmsubsd3((__v2df)a, (__v2df)b, (__v2df)c);
}

// 256 bits
__inline_g __m256
_mm256_fmadd_ps(__m256 a, __m256 b, __m256 c) noexcept
{
  return (__m256)__builtin_ia32_vfmaddps256((__v8sf)a, (__v8sf)b, (__v8sf)c);
}

__inline_g __m256
_mm256_fmsub_ps(__m256 a, __m256 b, __m256 c) noexcept
{
  return (__m256)__builtin_ia32_vfmsubps256((__v8sf)a, (__v8sf)b, (__v8sf)c);
}

__inline_g __m256
_mm256_fnmadd_ps(__m256 a, __m256 b, __m256 c) noexcept
{
  return (__m256)__builtin_ia32_vfnmaddps256((__v8sf)a, (__v8sf)b, (__v8sf)c);
}

__inline_g __m256
_mm256_fnmsub_ps(__m256 a, __m256 b, __m256 c) noexcept
{
  return (__m256)__builtin_ia32_vfnmsubps256((__v8sf)a, (__v8sf)b, (__v8sf)c);
}

__inline_g __m256
_mm256_fmaddsub_ps(__m256 a, __m256 b, __m256 c) noexcept
{
  return (__m256)__builtin_ia32_vfmaddsubps256((__v8sf)a, (__v8sf)b, (__v8sf)c);
}

__inline_g __m256
_mm256_fmsubadd_ps(__m256 a, __m256 b, __m256 c) noexcept
{
  return (__m256)__builtin_ia32_vfmaddsubps256((__v8sf)a, (__v8sf)b, -(__v8sf)c);
}

__inline_g __m256d
_mm256_fmadd_pd(__m256d a, __m256d b, __m256d c) noexcept
{
  return (__m256d)__builtin_ia32_vfmaddpd256((__v4df)a, (__v4df)b, (__v4df)c);
}

__inline_g __m256d
_mm256_fmsub_pd(__m256d a, __m256d b, __m256d c) noexcept
{
  return (__m256d)__builtin_ia32_vfmsubpd256((__v4df)a, (__v4df)b, (__v4df)c);
}

__inline_g __m256d
_mm256_fnmadd_pd(__m256d a, __m256d b, __m256d c) noexcept
{
  return (__m256d)__builtin_ia32_vfnmaddpd256((__v4df)a, (__v4df)b, (__v4df)c);
}

__inline_g __m256d
_mm256_fnmsub_pd(__m256d a, __m256d b, __m256d c) noexcept
{
  return (__m256d)__builtin_ia32_vfnmsubpd256((__v4df)a, (__v4df)b, (__v4df)c);
}

__inline_g __m256d
_mm256_fmaddsub_pd(__m256d a, __m256d b, __m256d c) noexcept
{
  return (__m256d)__builtin_ia32_vfmaddsubpd256((__v4df)a, (__v4df)b, (__v4df)c);
}

__inline_g __m256d
_mm256_fmsubadd_pd(__m256d a, __m256d b, __m256d c) noexcept
{
  return (__m256d)__builtin_ia32_vfmaddsubpd256((__v4df)a, (__v4df)b, -(__v4df)c);
}

#undef __inline_g

// 512 bits

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("avx512f")]] static inline

__inline_g __m512
_mm512_fmadd_ps(__m512 a, __m512 b, __m512 c) noexcept
{
  return (__m512)__builtin_ia32_vfmaddps512_mask((__v16sf)a, (__v16sf)b, (__v16sf)c, (__mmask16)-1,
                                                 /*round=*/4 /*_MM_FROUND_CUR_DIRECTION*/);
}

__inline_g __m512
_mm512_fmsub_ps(__m512 a, __m512 b, __m512 c) noexcept
{
  return (__m512)__builtin_ia32_vfmsubps512_mask((__v16sf)a, (__v16sf)b, (__v16sf)c, (__mmask16)-1, 4);
}

__inline_g __m512
_mm512_fnmadd_ps(__m512 a, __m512 b, __m512 c) noexcept
{
  return (__m512)__builtin_ia32_vfnmaddps512_mask((__v16sf)a, (__v16sf)b, (__v16sf)c, (__mmask16)-1, 4);
}

__inline_g __m512
_mm512_fnmsub_ps(__m512 a, __m512 b, __m512 c) noexcept
{
  return (__m512)__builtin_ia32_vfnmsubps512_mask((__v16sf)a, (__v16sf)b, (__v16sf)c, (__mmask16)-1, 4);
}

__inline_g __m512
_mm512_fmaddsub_ps(__m512 a, __m512 b, __m512 c) noexcept
{
  return (__m512)__builtin_ia32_vfmaddsubps512_mask((__v16sf)a, (__v16sf)b, (__v16sf)c, (__mmask16)-1, 4);
}

__inline_g __m512
_mm512_fmsubadd_ps(__m512 a, __m512 b, __m512 c) noexcept
{
  return (__m512)__builtin_ia32_vfmaddsubps512_mask((__v16sf)a, (__v16sf)b, -(__v16sf)c, (__mmask16)-1, 4);
}

__inline_g __m512d
_mm512_fmadd_pd(__m512d a, __m512d b, __m512d c) noexcept
{
  return (__m512d)__builtin_ia32_vfmaddpd512_mask((__v8df)a, (__v8df)b, (__v8df)c, (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_fmsub_pd(__m512d a, __m512d b, __m512d c) noexcept
{
  return (__m512d)__builtin_ia32_vfmsubpd512_mask((__v8df)a, (__v8df)b, (__v8df)c, (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_fnmadd_pd(__m512d a, __m512d b, __m512d c) noexcept
{
  return (__m512d)__builtin_ia32_vfnmaddpd512_mask((__v8df)a, (__v8df)b, (__v8df)c, (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_fnmsub_pd(__m512d a, __m512d b, __m512d c) noexcept
{
  return (__m512d)__builtin_ia32_vfnmsubpd512_mask((__v8df)a, (__v8df)b, (__v8df)c, (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_fmaddsub_pd(__m512d a, __m512d b, __m512d c) noexcept
{
  return (__m512d)__builtin_ia32_vfmaddsubpd512_mask((__v8df)a, (__v8df)b, (__v8df)c, (__mmask8)-1, 4);
}

__inline_g __m512d
_mm512_fmsubadd_pd(__m512d a, __m512d b, __m512d c) noexcept
{
  return (__m512d)__builtin_ia32_vfmaddsubpd512_mask((__v8df)a, (__v8df)b, -(__v8df)c, (__mmask8)-1, 4);
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_mm_fmadd_ps);
__inject_i(_mm_fmsub_ps);
__inject_i(_mm_fnmadd_ps);
__inject_i(_mm_fnmsub_ps);
__inject_i(_mm_fmaddsub_ps);
__inject_i(_mm_fmsubadd_ps);
__inject_i(_mm_fmadd_pd);
__inject_i(_mm_fmsub_pd);
__inject_i(_mm_fnmadd_pd);
__inject_i(_mm_fnmsub_pd);
__inject_i(_mm_fmaddsub_pd);
__inject_i(_mm_fmsubadd_pd);
__inject_i(_mm_fmadd_ss);
__inject_i(_mm_fmsub_ss);
__inject_i(_mm_fnmadd_ss);
__inject_i(_mm_fnmsub_ss);
__inject_i(_mm_fmadd_sd);
__inject_i(_mm_fmsub_sd);
__inject_i(_mm_fnmadd_sd);
__inject_i(_mm_fnmsub_sd);
__inject_i(_mm256_fmadd_ps);
__inject_i(_mm256_fmsub_ps);
__inject_i(_mm256_fnmadd_ps);
__inject_i(_mm256_fnmsub_ps);
__inject_i(_mm256_fmaddsub_ps);
__inject_i(_mm256_fmsubadd_ps);
__inject_i(_mm256_fmadd_pd);
__inject_i(_mm256_fmsub_pd);
__inject_i(_mm256_fnmadd_pd);
__inject_i(_mm256_fnmsub_pd);
__inject_i(_mm256_fmaddsub_pd);
__inject_i(_mm256_fmsubadd_pd);
__inject_i(_mm512_fmadd_ps);
__inject_i(_mm512_fmsub_ps);
__inject_i(_mm512_fnmadd_ps);
__inject_i(_mm512_fnmsub_ps);
__inject_i(_mm512_fmaddsub_ps);
__inject_i(_mm512_fmsubadd_ps);
__inject_i(_mm512_fmadd_pd);
__inject_i(_mm512_fmsub_pd);
__inject_i(_mm512_fnmadd_pd);
__inject_i(_mm512_fnmsub_pd);
__inject_i(_mm512_fmaddsub_pd);
__inject_i(_mm512_fmsubadd_pd);
#undef __inject_i
#endif
