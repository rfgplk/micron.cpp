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
#error "__avx512dq.hpp included on a non-x86 build"
#endif

// freestanding AVX-512DQ [avx512dqintrin.h]

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"

#define __inline_g [[gnu::always_inline, gnu::artificial, gnu::target("avx512dq,avx512f")]] static inline

__inline_g __m512i
_mm512_mullo_epi64(__m512i a, __m512i b) noexcept
{
  return (__m512i)((__v8du)a * (__v8du)b);
}

__inline_g __m512
_mm512_and_ps_dq(__m512 a, __m512 b) noexcept
{
  return (__m512)__builtin_ia32_andps512_mask((__v16sf)a, (__v16sf)b, (__v16sf){ 0 }, (__mmask16)-1);
}

__inline_g __m512d
_mm512_and_pd_dq(__m512d a, __m512d b) noexcept
{
  return (__m512d)__builtin_ia32_andpd512_mask((__v8df)a, (__v8df)b, (__v8df){ 0 }, (__mmask8)-1);
}

__inline_g __m512
_mm512_broadcast_f32x2(__m128 a) noexcept
{
  return (__m512)__builtin_ia32_broadcastf32x2_512_mask((__v4sf)a, (__v16sf){ 0 }, (__mmask16)-1);
}

__inline_g __m512i
_mm512_broadcast_i32x2(__m128i a) noexcept
{
  return (__m512i)__builtin_ia32_broadcasti32x2_512_mask((__v4si)a, (__v16si){ 0 }, (__mmask16)-1);
}

__inline_g __m512d
_mm512_broadcast_f64x2(__m128d a) noexcept
{
  return (__m512d)__builtin_ia32_broadcastf64x2_512_mask((__v2df)a, (__v8df){ 0 }, (__mmask8)-1);
}

#undef __inline_g

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron
