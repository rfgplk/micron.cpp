//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../intrin.hpp"

#if !defined(__micron_arch_x86_any)
#error "fma.hpp included on a non-x86 build"
#endif

namespace micron
{
namespace simd
{
namespace fma
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_fma [[gnu::always_inline, gnu::artificial, gnu::target("fma")]] static inline

__inline_fma __m128
fma_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fmadd_ps(a, b, c);
}

__inline_fma __m128
fms_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fmsub_ps(a, b, c);
}

__inline_fma __m128
fnma_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fnmadd_ps(a, b, c);
}

__inline_fma __m128
fnms_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fnmsub_ps(a, b, c);
}

__inline_fma __m128
fma_addsub_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fmaddsub_ps(a, b, c);
}

__inline_fma __m128
fms_addsub_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fmsubadd_ps(a, b, c);
}

__inline_fma __m128d
fma_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fmadd_pd(a, b, c);
}

__inline_fma __m128d
fms_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fmsub_pd(a, b, c);
}

__inline_fma __m128d
fnma_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fnmadd_pd(a, b, c);
}

__inline_fma __m128d
fnms_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fnmsub_pd(a, b, c);
}

__inline_fma __m128d
fma_addsub_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fmaddsub_pd(a, b, c);
}

__inline_fma __m128d
fms_addsub_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fmsubadd_pd(a, b, c);
}

__inline_fma __m128
fma_scalar_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fmadd_ss(a, b, c);
}

__inline_fma __m128
fms_scalar_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fmsub_ss(a, b, c);
}

__inline_fma __m128
fnma_scalar_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fnmadd_ss(a, b, c);
}

__inline_fma __m128
fnms_scalar_f32(__m128 a, __m128 b, __m128 c) noexcept
{
  return _mm_fnmsub_ss(a, b, c);
}

__inline_fma __m128d
fma_scalar_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fmadd_sd(a, b, c);
}

__inline_fma __m128d
fms_scalar_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fmsub_sd(a, b, c);
}

__inline_fma __m128d
fnma_scalar_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fnmadd_sd(a, b, c);
}

__inline_fma __m128d
fnms_scalar_f64(__m128d a, __m128d b, __m128d c) noexcept
{
  return _mm_fnmsub_sd(a, b, c);
}

__inline_fma __m256
fma_f32(__m256 a, __m256 b, __m256 c) noexcept
{
  return _mm256_fmadd_ps(a, b, c);
}

__inline_fma __m256
fms_f32(__m256 a, __m256 b, __m256 c) noexcept
{
  return _mm256_fmsub_ps(a, b, c);
}

__inline_fma __m256
fnma_f32(__m256 a, __m256 b, __m256 c) noexcept
{
  return _mm256_fnmadd_ps(a, b, c);
}

__inline_fma __m256
fnms_f32(__m256 a, __m256 b, __m256 c) noexcept
{
  return _mm256_fnmsub_ps(a, b, c);
}

__inline_fma __m256
fma_addsub_f32(__m256 a, __m256 b, __m256 c) noexcept
{
  return _mm256_fmaddsub_ps(a, b, c);
}

__inline_fma __m256
fms_addsub_f32(__m256 a, __m256 b, __m256 c) noexcept
{
  return _mm256_fmsubadd_ps(a, b, c);
}

__inline_fma __m256d
fma_f64(__m256d a, __m256d b, __m256d c) noexcept
{
  return _mm256_fmadd_pd(a, b, c);
}

__inline_fma __m256d
fms_f64(__m256d a, __m256d b, __m256d c) noexcept
{
  return _mm256_fmsub_pd(a, b, c);
}

__inline_fma __m256d
fnma_f64(__m256d a, __m256d b, __m256d c) noexcept
{
  return _mm256_fnmadd_pd(a, b, c);
}

__inline_fma __m256d
fnms_f64(__m256d a, __m256d b, __m256d c) noexcept
{
  return _mm256_fnmsub_pd(a, b, c);
}

__inline_fma __m256d
fma_addsub_f64(__m256d a, __m256d b, __m256d c) noexcept
{
  return _mm256_fmaddsub_pd(a, b, c);
}

__inline_fma __m256d
fms_addsub_f64(__m256d a, __m256d b, __m256d c) noexcept
{
  return _mm256_fmsubadd_pd(a, b, c);
}

#undef __inline_fma

[[gnu::always_inline, gnu::artificial]] static inline __m512
fma_f32_v512(__m512 a, __m512 b, __m512 c) noexcept
{
  return _mm512_fmadd_ps(a, b, c);
}

[[gnu::always_inline, gnu::artificial]] static inline __m512
fms_f32_v512(__m512 a, __m512 b, __m512 c) noexcept
{
  return _mm512_fmsub_ps(a, b, c);
}

[[gnu::always_inline, gnu::artificial]] static inline __m512
fnma_f32_v512(__m512 a, __m512 b, __m512 c) noexcept
{
  return _mm512_fnmadd_ps(a, b, c);
}

[[gnu::always_inline, gnu::artificial]] static inline __m512
fnms_f32_v512(__m512 a, __m512 b, __m512 c) noexcept
{
  return _mm512_fnmsub_ps(a, b, c);
}

[[gnu::always_inline, gnu::artificial]] static inline __m512d
fma_f64_v512(__m512d a, __m512d b, __m512d c) noexcept
{
  return _mm512_fmadd_pd(a, b, c);
}

[[gnu::always_inline, gnu::artificial]] static inline __m512d
fms_f64_v512(__m512d a, __m512d b, __m512d c) noexcept
{
  return _mm512_fmsub_pd(a, b, c);
}

[[gnu::always_inline, gnu::artificial]] static inline __m512d
fnma_f64_v512(__m512d a, __m512d b, __m512d c) noexcept
{
  return _mm512_fnmadd_pd(a, b, c);
}

[[gnu::always_inline, gnu::artificial]] static inline __m512d
fnms_f64_v512(__m512d a, __m512d b, __m512d c) noexcept
{
  return _mm512_fnmsub_pd(a, b, c);
}

#pragma GCC diagnostic pop

};     // namespace fma
};     // namespace simd
};     // namespace micron
