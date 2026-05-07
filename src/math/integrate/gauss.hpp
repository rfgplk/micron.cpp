//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// -> gauss_legendre<F, Order>(f, a, b)
// -> clenshaw_curtis<F, N>(f, a, b)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "bits/coeff/gauss_legendre.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template <usize Order, ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline F
gauss_legendre(Fn f, F a, F b) noexcept
{
  using table = coeff::gl::gl_table<F, Order>;
  const F half_w = F(0.5) * (b - a);
  const F mid = F(0.5) * (a + b);
  F s = F(0);
  if constexpr ( table::has_zero ) {
    s = math::fma<F>(table::weights[0], f(mid), s);
    for ( usize k = 1; k < table::half; ++k ) {
      const F xk = table::nodes[k];
      const F vk = f(math::fma<F>(half_w, xk, mid)) + f(math::fma<F>(half_w, -xk, mid));
      s = math::fma<F>(table::weights[k], vk, s);
    }
  } else {
    for ( usize k = 0; k < table::half; ++k ) {
      const F xk = table::nodes[k];
      const F vk = f(math::fma<F>(half_w, xk, mid)) + f(math::fma<F>(half_w, -xk, mid));
      s = math::fma<F>(table::weights[k], vk, s);
    }
  }
  return half_w * s;
}

template <usize N, ieee754_floating F, callable_real<F> Fn>
  requires(N >= 2 and N <= 64)
[[nodiscard]] inline F
clenshaw_curtis(Fn f, F a, F b) noexcept
{
  const F half_w = F(0.5) * (b - a);
  const F mid = F(0.5) * (a + b);
  const F pi_n = micron::math::constant_pi<F> / F(N);

  F y[N + 1];
  for ( usize k = 0; k <= N; ++k ) {
    const F xk = mk::trig::cos<F>(F(k) * pi_n);
    y[k] = f(math::fma<F>(half_w, xk, mid));
  }

  F acc = F(0);
  {
    F a0 = F(0.5) * (y[0] + y[N]);
    for ( usize k = 1; k < N; ++k ) a0 += y[k];
    a0 *= F(2) / F(N);
    acc += F(0.5) * a0;
  }
  for ( usize m = 2; m <= N; m += 2 ) {
    F am = F(0.5) * (y[0] + (m % 2 == 0 ? y[N] : -y[N]));
    for ( usize k = 1; k < N; ++k ) am += y[k] * mk::trig::cos<F>(F(m * k) * pi_n);
    am *= F(2) / F(N);
    const F denom = F(1) - F(m * m);
    acc += am / denom;
  }
  return half_w * F(2) * acc;
}

};     // namespace integrate
};     // namespace math
};     // namespace micron
