//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "packet_neon.hpp"

namespace micron::math::fft::__impl
{
template<> struct __complex_packet<double> {
  struct value {
    float64x2_t real, imag;
  };

  static constexpr usize width = 2;

  static value
  load(const complex<double> *p) noexcept
  {
    auto lo = simd::neon::load_f64(reinterpret_cast<const double *>(p));
    auto hi = simd::neon::load_f64(reinterpret_cast<const double *>(p + 1));
    return { simd::neon::unzip_lo_f64(lo, hi), simd::neon::unzip_hi_f64(lo, hi) };
  }

  static void
  store(complex<double> *p, value v) noexcept
  {
    auto lo = simd::neon::zip_lo_f64(v.real, v.imag), hi = simd::neon::zip_hi_f64(v.real, v.imag);
    simd::neon::store_f64(reinterpret_cast<double *>(p), lo);
    simd::neon::store_f64(reinterpret_cast<double *>(p + 1), hi);
  }

  static value
  add(value a, value b) noexcept
  {
    return { simd::neon::add(a.real, b.real), simd::neon::add(a.imag, b.imag) };
  }

  static value
  sub(value a, value b) noexcept
  {
    return { simd::neon::sub(a.real, b.real), simd::neon::sub(a.imag, b.imag) };
  }

  static value
  scale(value a, double b) noexcept
  {
    return { simd::neon::mul(a.real, simd::neon::splat_f64(b)), simd::neon::mul(a.imag, simd::neon::splat_f64(b)) };
  }

  static value
  conjugate(value a) noexcept
  {
    return { a.real, simd::neon::neg(a.imag) };
  }

  static value
  mul(value a, value b) noexcept
  {
    return { simd::neon::fms(simd::neon::mul(a.real, b.real), a.imag, b.imag),
             simd::neon::fma(simd::neon::mul(a.real, b.imag), a.imag, b.real) };
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
