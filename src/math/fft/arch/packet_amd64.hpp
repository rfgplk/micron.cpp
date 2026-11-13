//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../simd/aliases.hpp"
#include "packet_generic.hpp"

namespace micron::math::fft::__impl
{
#if defined(__micron_x86_avx2)
template<> struct __complex_packet<float> {
  using value = __m256;
  static constexpr usize width = 4;

  static value
  load(const complex<float> *p) noexcept
  {
    return simd::avx::cast_i256_to_f32(simd::avx::loadu_i256(reinterpret_cast<const __m256i_u *>(p)));
  }

  static void
  store(complex<float> *p, value v) noexcept
  {
    simd::avx::storeu_i256(reinterpret_cast<__m256i_u *>(p), simd::avx::cast_f32_to_i256(v));
  }

  static value
  add(value a, value b) noexcept
  {
    return simd::avx::add_f32(a, b);
  }

  static value
  sub(value a, value b) noexcept
  {
    return simd::avx::sub_f32(a, b);
  }

  static value
  scale(value a, float b) noexcept
  {
    return simd::avx::mul_f32(a, simd::avx::splat_f32(b));
  }

  template<bool Imaginary>
  static value
  __flip_sign(value a) noexcept
  {
    // Floating zero masks can merge under -fno-signed-zeros before the XOR.
    constexpr int sign = -2147483647 - 1, real = Imaginary ? 0 : sign, imag = Imaginary ? sign : 0;
    auto mask = simd::avx::set_i32(imag, real, imag, real, imag, real, imag, real);
    return simd::avx::cast_i256_to_f32(simd::avx2::xor_i256(simd::avx::cast_f32_to_i256(a), mask));
  }

  static value
  conjugate(value a) noexcept
  {
    return __flip_sign<true>(a);
  }

  static value
  mul(value a, value b) noexcept
  {
    auto re = simd::avx::moveldup_f32(a), im = simd::avx::movehdup_f32(a);
    auto cross = simd::avx::mul_f32(im, simd::avx::permute_f32<0xb1>(b));
#if defined(__micron_x86_fma)
    return simd::fma::fma_addsub_f32(re, b, cross);
#else
    return simd::avx::addsub_f32(simd::avx::mul_f32(re, b), cross);
#endif
  }

  template<bool Inverse>
  static value
  rotate(value a) noexcept
  {
    return __flip_sign<!Inverse>(simd::avx::permute_f32<0xb1>(a));
  }
};

template<> struct __complex_packet<double> {
  using value = __m256d;
  static constexpr usize width = 2;

  static value
  load(const complex<double> *p) noexcept
  {
    return simd::avx::cast_i256_to_f64(simd::avx::loadu_i256(reinterpret_cast<const __m256i_u *>(p)));
  }

  static void
  store(complex<double> *p, value v) noexcept
  {
    simd::avx::storeu_i256(reinterpret_cast<__m256i_u *>(p), simd::avx::cast_f64_to_i256(v));
  }

  static value
  add(value a, value b) noexcept
  {
    return simd::avx::add_f64(a, b);
  }

  static value
  sub(value a, value b) noexcept
  {
    return simd::avx::sub_f64(a, b);
  }

  static value
  scale(value a, double b) noexcept
  {
    return simd::avx::mul_f64(a, simd::avx::splat_f64(b));
  }

  template<bool Imaginary>
  static value
  __flip_sign(value a) noexcept
  {
    constexpr long long sign = -9223372036854775807LL - 1, real = Imaginary ? 0 : sign, imag = Imaginary ? sign : 0;
    auto mask = simd::avx::set_i64(imag, real, imag, real);
    return simd::avx::cast_i256_to_f64(simd::avx2::xor_i256(simd::avx::cast_f64_to_i256(a), mask));
  }

  static value
  conjugate(value a) noexcept
  {
    return __flip_sign<true>(a);
  }

  static value
  mul(value a, value b) noexcept
  {
    auto re = simd::avx::movedup_f64(a), im = simd::avx::permute_f64<15>(a);
    auto cross = simd::avx::mul_f64(im, simd::avx::permute_f64<5>(b));
#if defined(__micron_x86_fma)
    return simd::fma::fma_addsub_f64(re, b, cross);
#else
    return simd::avx::addsub_f64(simd::avx::mul_f64(re, b), cross);
#endif
  }

  template<bool Inverse>
  static value
  rotate(value a) noexcept
  {
    return __flip_sign<!Inverse>(simd::avx::permute_f64<5>(a));
  }
};
#elif defined(__micron_x86_sse2)
// Integer loads avoid extra register moves in GCC's i386 butterflies.
template<> struct __complex_packet<float> {
  using value = __m128;
  static constexpr usize width = 2;

  static value
  load(const complex<float> *p) noexcept
  {
    if constexpr ( __micron_ptr_bits == 32 )
      return simd::sse::cast_i128_to_f32(simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p)));
    return simd::sse::loadu_f32(reinterpret_cast<const float *>(p));
  }

  static void
  store(complex<float> *p, value v) noexcept
  {
    if constexpr ( __micron_ptr_bits == 32 )
      simd::sse::storeu_i128(reinterpret_cast<__m128i_u *>(p), simd::sse::cast_f32_to_i128(v));
    else
      simd::sse::storeu_f32(reinterpret_cast<float *>(p), v);
  }

  static value
  add(value a, value b) noexcept
  {
    return simd::sse::add_f32(a, b);
  }

  static value
  sub(value a, value b) noexcept
  {
    return simd::sse::sub_f32(a, b);
  }

  static value
  scale(value a, float b) noexcept
  {
    return simd::sse::mul_f32(a, simd::sse::splat_f32(b));
  }

  static value
  conjugate(value a) noexcept
  {
    return simd::sse::xor_f32(a, simd::sse::set_f32(-0.f, 0.f, -0.f, 0.f));
  }

  static value
  mul(value a, value b) noexcept
  {
    auto re = simd::sse::shuffle_f32<0xa0>(a, a), im = simd::sse::shuffle_f32<0xf5>(a, a);
    auto cross = simd::sse::mul_f32(im, simd::sse::shuffle_f32<0xb1>(b, b));
    return simd::sse::add_f32(simd::sse::mul_f32(re, b), simd::sse::xor_f32(cross, simd::sse::set_f32(0.f, -0.f, 0.f, -0.f)));
  }

  template<bool Inverse>
  static value
  rotate(value a) noexcept
  {
    auto mask = Inverse ? simd::sse::set_f32(0.f, -0.f, 0.f, -0.f) : simd::sse::set_f32(-0.f, 0.f, -0.f, 0.f);
    return simd::sse::xor_f32(simd::sse::shuffle_f32<0xb1>(a, a), mask);
  }
};

template<> struct __complex_packet<double> {
  using value = __m128d;
  static constexpr usize width = 1;

  static value
  load(const complex<double> *p) noexcept
  {
    if constexpr ( __micron_ptr_bits == 32 )
      return simd::sse::cast_i128_to_f64(simd::sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p)));
    return simd::sse::loadu_f64(reinterpret_cast<const double *>(p));
  }

  static void
  store(complex<double> *p, value v) noexcept
  {
    if constexpr ( __micron_ptr_bits == 32 )
      simd::sse::storeu_i128(reinterpret_cast<__m128i_u *>(p), simd::sse::cast_f64_to_i128(v));
    else
      simd::sse::storeu_f64(reinterpret_cast<double *>(p), v);
  }

  static value
  add(value a, value b) noexcept
  {
    return simd::sse::add_f64(a, b);
  }

  static value
  sub(value a, value b) noexcept
  {
    return simd::sse::sub_f64(a, b);
  }

  static value
  scale(value a, double b) noexcept
  {
    return simd::sse::mul_f64(a, simd::sse::splat_f64(b));
  }

  static value
  conjugate(value a) noexcept
  {
    return simd::sse::xor_f64(a, simd::sse::set_f64(-0., 0.));
  }

  static value
  mul(value a, value b) noexcept
  {
    auto re = simd::sse::unpack_lo_f64(a, a), im = simd::sse::unpack_hi_f64(a, a);
    auto cross = simd::sse::mul_f64(im, simd::sse::shuffle_f64<1>(b, b));
    return simd::sse::add_f64(simd::sse::mul_f64(re, b), simd::sse::xor_f64(cross, simd::sse::set_f64(0., -0.)));
  }

  template<bool Inverse>
  static value
  rotate(value a) noexcept
  {
    auto mask = Inverse ? simd::sse::set_f64(0., -0.) : simd::sse::set_f64(-0., 0.);
    return simd::sse::xor_f64(simd::sse::shuffle_f64<1>(a, a), mask);
  }
};
#endif
};      // namespace micron::math::fft::__impl
