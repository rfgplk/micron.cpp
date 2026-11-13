//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../simd/aliases/neon.hpp"
#include "packet_generic.hpp"

namespace micron::math::fft::__impl
{
// GCC scalarizes generic vector<float> arithmetic on ARMv7 at strict -O3.
// Explicit NEON operations avoid that expansion and its register spills. They
// have ARMv7 NEON's native denormal behavior; no FPSCR state is changed.
#if defined(__micron_arch_arm32)
[[gnu::always_inline]] inline float32x4_t
__neon_add(float32x4_t a, float32x4_t b) noexcept
{
  float32x4_t r;
  asm("vadd.f32 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

[[gnu::always_inline]] inline float32x4_t
__neon_sub(float32x4_t a, float32x4_t b) noexcept
{
  float32x4_t r;
  asm("vsub.f32 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

[[gnu::always_inline]] inline float32x4_t
__neon_mul(float32x4_t a, float32x4_t b) noexcept
{
  float32x4_t r;
  asm("vmul.f32 %q0, %q1, %q2" : "=w"(r) : "w"(a), "w"(b));
  return r;
}

[[gnu::always_inline]] inline float32x4_t
__neon_mla(float32x4_t acc, float32x4_t a, float32x4_t b) noexcept
{
  asm("vmla.f32 %q0, %q1, %q2" : "+w"(acc) : "w"(a), "w"(b));
  return acc;
}

[[gnu::always_inline]] inline float32x4_t
__neon_mls(float32x4_t acc, float32x4_t a, float32x4_t b) noexcept
{
  asm("vmls.f32 %q0, %q1, %q2" : "+w"(acc) : "w"(a), "w"(b));
  return acc;
}
#endif

template<> struct __complex_packet<float> {
  struct value {
    float32x4_t real, imag;
  };

  static constexpr usize width = 4;

  static value
  load(const complex<float> *p) noexcept
  {
    auto lo = simd::neon::load_f32(reinterpret_cast<const float *>(p));
    auto hi = simd::neon::load_f32(reinterpret_cast<const float *>(p + 2));
    return { simd::neon::unzip_lo_f32(lo, hi), simd::neon::unzip_hi_f32(lo, hi) };
  }

  static void
  store(complex<float> *p, value v) noexcept
  {
    auto lo = simd::neon::zip_lo_f32(v.real, v.imag), hi = simd::neon::zip_hi_f32(v.real, v.imag);
    simd::neon::store_f32(reinterpret_cast<float *>(p), lo);
    simd::neon::store_f32(reinterpret_cast<float *>(p + 2), hi);
  }

  static value
  add(value a, value b) noexcept
  {
#if defined(__micron_arch_arm32)
    return { __neon_add(a.real, b.real), __neon_add(a.imag, b.imag) };
#else
    return { simd::neon::add(a.real, b.real), simd::neon::add(a.imag, b.imag) };
#endif
  }

  static value
  sub(value a, value b) noexcept
  {
#if defined(__micron_arch_arm32)
    return { __neon_sub(a.real, b.real), __neon_sub(a.imag, b.imag) };
#else
    return { simd::neon::sub(a.real, b.real), simd::neon::sub(a.imag, b.imag) };
#endif
  }

  static value
  scale(value a, float b) noexcept
  {
#if defined(__micron_arch_arm32)
    return { __neon_mul(a.real, simd::neon::splat_f32(b)), __neon_mul(a.imag, simd::neon::splat_f32(b)) };
#else
    return { simd::neon::mul(a.real, simd::neon::splat_f32(b)), simd::neon::mul(a.imag, simd::neon::splat_f32(b)) };
#endif
  }

  static value
  conjugate(value a) noexcept
  {
    return { a.real, simd::neon::neg(a.imag) };
  }

  static value
  mul(value a, value b) noexcept
  {
#if defined(__micron_arch_arm32)
    return { __neon_mls(__neon_mul(a.real, b.real), a.imag, b.imag), __neon_mla(__neon_mul(a.real, b.imag), a.imag, b.real) };
#elif defined(__micron_arm_fma)
    return { simd::neon::fms(simd::neon::mul(a.real, b.real), a.imag, b.imag),
             simd::neon::fma(simd::neon::mul(a.real, b.imag), a.imag, b.real) };
#else
    return { simd::neon::sub(simd::neon::mul(a.real, b.real), simd::neon::mul(a.imag, b.imag)),
             simd::neon::add(simd::neon::mul(a.real, b.imag), simd::neon::mul(a.imag, b.real)) };
#endif
  }

  template<bool Inverse>
  static value
  rotate(value a) noexcept
  {
    if constexpr ( Inverse )
      return { simd::neon::neg(a.imag), a.real };
    else
      return { a.imag, simd::neon::neg(a.real) };
  }
};

}      // namespace micron::math::fft::__impl
