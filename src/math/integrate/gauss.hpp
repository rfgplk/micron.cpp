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
#include "bits/kernels.hpp"
#include "common.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template<usize Order, ieee754_floating F, callable_real<F> Fn>
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

template<usize Order, ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline F
gauss_legendre(Fn f, F a, F b, accumulation_policy policy) noexcept
{
  using table = coeff::gl::gl_table<F, Order>;
  const F half_w = F(0.5) * (b - a);
  const F mid = F(0.5) * (a + b);
  __impl_accumulate::runtime_sum<F> sum{ policy };
  if constexpr ( table::has_zero ) {
    sum.add(table::weights[0] * f(mid));
    for ( usize k = 1; k < table::half; ++k ) {
      const F xk = table::nodes[k];
      sum.add(table::weights[k] * (f(math::fma<F>(half_w, xk, mid)) + f(math::fma<F>(half_w, -xk, mid))));
    }
  } else {
    for ( usize k = 0; k < table::half; ++k ) {
      const F xk = table::nodes[k];
      sum.add(table::weights[k] * (f(math::fma<F>(half_w, xk, mid)) + f(math::fma<F>(half_w, -xk, mid))));
    }
  }
  return half_w * sum.get();
}

template<usize Order, ieee754_floating F, callable_real_batch<F> Fn>
[[nodiscard]] inline F
gauss_legendre_batch(Fn f, F a, F b, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  using table = coeff::gl::gl_table<F, Order>;
  const F half_w = F(0.5) * (b - a);
  const F mid = F(0.5) * (a + b);
  F points[Order]{};
  F values[Order]{};
  F weights[Order]{};
  usize count = 0;
  if constexpr ( table::has_zero ) {
    points[count] = mid;
    weights[count++] = table::weights[0];
    for ( usize k = 1; k < table::half; ++k ) {
      points[count] = math::fma<F>(half_w, -table::nodes[k], mid);
      weights[count++] = table::weights[k];
      points[count] = math::fma<F>(half_w, table::nodes[k], mid);
      weights[count++] = table::weights[k];
    }
  } else {
    for ( usize k = 0; k < table::half; ++k ) {
      points[count] = math::fma<F>(half_w, -table::nodes[k], mid);
      weights[count++] = table::weights[k];
      points[count] = math::fma<F>(half_w, table::nodes[k], mid);
      weights[count++] = table::weights[k];
    }
  }
  f(points, values, count);
  if ( policy == accumulation_policy::fast ) return half_w * __integrate_arch::weighted_sum_fast(weights, values, count);
  __impl_accumulate::runtime_sum<F> sum{ policy };
  for ( usize i = 0; i < count; ++i ) sum.add(weights[i] * values[i]);
  return half_w * sum.get();
}

template<usize N, ieee754_floating F, callable_real<F> Fn>
  requires(N >= 2 and N <= 64)
[[nodiscard]] inline F
clenshaw_curtis(Fn f, F a, F b) noexcept
{
  const F half_w = F(0.5) * (b - a);
  const F mid = F(0.5) * (a + b);
  using fixed_table = coeff::cc::cc_table<F, N>;
  if constexpr ( fixed_table::available ) {
    F result = F(0);
    for ( usize k = 0; k <= N; ++k )
      result = math::fma<F>(fixed_table::weights[k], f(math::fma<F>(half_w, fixed_table::nodes[k], mid)), result);
    return half_w * result;
  }
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
    F am = F(0.5) * (y[0] + y[N]);
    for ( usize k = 1; k < N; ++k ) am += y[k] * mk::trig::cos<F>(F(m * k) * pi_n);
    am *= F(2) / F(N);
    if ( m == N ) am *= F(0.5);
    const F denom = F(1) - F(m * m);
    acc += am / denom;
  }
  return half_w * F(2) * acc;
}

template<usize N, ieee754_floating F, callable_real_batch<F> Fn>
  requires(N >= 2 and N <= 64)
[[nodiscard]] inline F
clenshaw_curtis_batch(Fn f, F a, F b, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  const F half_w = F(0.5) * (b - a);
  const F mid = F(0.5) * (a + b);
  F points[N + 1]{};
  F values[N + 1]{};
  F weights[N + 1]{};
  using fixed_table = coeff::cc::cc_table<F, N>;
  if constexpr ( fixed_table::available ) {
    for ( usize k = 0; k <= N; ++k ) {
      points[k] = math::fma<F>(half_w, fixed_table::nodes[k], mid);
      weights[k] = fixed_table::weights[k];
    }
  } else {
    const F pi_n = constant_pi<F> / F(N);
    for ( usize k = 0; k <= N; ++k ) {
      const F node = mk::trig::cos<F>(F(k) * pi_n);
      points[k] = math::fma<F>(half_w, node, mid);
      F bracket = F(1);
      for ( usize j = 1; j <= N / 2; ++j ) {
        const F multiplier = 2 * j == N ? F(1) : F(2);
        bracket -= multiplier * mk::trig::cos<F>(F(2 * j * k) * pi_n) / F(4 * j * j - 1);
      }
      weights[k] = F(2) * bracket / F(N);
      if ( k == 0 || k == N ) weights[k] *= F(0.5);
    }
  }
  f(points, values, N + 1);
  if ( policy == accumulation_policy::fast ) return half_w * __integrate_arch::weighted_sum_fast(weights, values, N + 1);
  __impl_accumulate::runtime_sum<F> sum{ policy };
  for ( usize k = 0; k <= N; ++k ) sum.add(weights[k] * values[k]);
  return half_w * sum.get();
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
