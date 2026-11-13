//  Copyright (c) 2024- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../common.hpp"

namespace micron::math::fft::__impl
{
template<fft_scalar F> struct __scalar_packet {
  using value = complex<F>;
  static constexpr usize width = 1;

  static value
  load(const complex<F> *p) noexcept
  {
    return *p;
  }

  static void
  store(complex<F> *p, value v) noexcept
  {
    *p = v;
  }

  static value
  add(value a, value b) noexcept
  {
    return a + b;
  }

  static value
  sub(value a, value b) noexcept
  {
    return a - b;
  }

  static value
  mul(value a, value b) noexcept
  {
    return a * b;
  }

  static value
  scale(value a, F b) noexcept
  {
    return a * b;
  }

  static value
  conjugate(value a) noexcept
  {
    return conj(a);
  }

  template<bool Inverse>
  static value
  rotate(value a) noexcept
  {
    if constexpr ( Inverse )
      return { -a.imag, a.real };
    else
      return { a.imag, -a.real };
  }
};

template<fft_scalar F> struct __complex_packet: __scalar_packet<F> {
};
}      // namespace micron::math::fft::__impl
