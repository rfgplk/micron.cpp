//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "bits/kernels.hpp"
#include "common.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

namespace __impl_gaussian
{

template<ieee754_floating F, usize N>
[[nodiscard]] inline usize
sturm_count(const F (&diagonal)[N], const F (&off_diagonal)[N], F x) noexcept
{
  const F tiny = math::default_eps<F>() * math::default_eps<F>();
  F q = diagonal[0] - x;
  usize count = q < F(0) ? 1 : 0;
  if ( mk::manip::fabs<F>(q) < tiny ) q = q < F(0) ? -tiny : tiny;
  for ( usize i = 1; i < N; ++i ) {
    q = diagonal[i] - x - off_diagonal[i - 1] * off_diagonal[i - 1] / q;
    if ( q < F(0) ) ++count;
    if ( mk::manip::fabs<F>(q) < tiny ) q = q < F(0) ? -tiny : tiny;
  }
  return count;
}

template<ieee754_floating F, usize N>
inline void
build(const F (&diagonal)[N], const F (&off_diagonal)[N], F moment0, F (&nodes)[N], F (&weights)[N]) noexcept
{
  F lower = diagonal[0];
  F upper = diagonal[0];
  for ( usize i = 0; i < N; ++i ) {
    F radius = F(0);
    if ( i != 0 ) radius += mk::manip::fabs<F>(off_diagonal[i - 1]);
    if ( i + 1 < N ) radius += mk::manip::fabs<F>(off_diagonal[i]);
    const F lo = diagonal[i] - radius;
    const F hi = diagonal[i] + radius;
    if ( lo < lower ) lower = lo;
    if ( hi > upper ) upper = hi;
  }
  const F pad = (upper - lower) * math::default_eps<F>() * F(16) + math::default_eps<F>();
  lower -= pad;
  upper += pad;

  constexpr usize iterations = sizeof(F) <= 4 ? 64 : 112;
  for ( usize root = 0; root < N; ++root ) {
    F lo = lower;
    F hi = upper;
    for ( usize iteration = 0; iteration < iterations; ++iteration ) {
      const F mid = F(0.5) * (lo + hi);
      if ( sturm_count<F, N>(diagonal, off_diagonal, mid) <= root )
        lo = mid;
      else
        hi = mid;
    }
    nodes[root] = F(0.5) * (lo + hi);

    F first = F(1);
    F previous = F(1);
    F sumsq = F(1);
    if constexpr ( N > 1 ) {
      F current = (nodes[root] - diagonal[0]) / off_diagonal[0];
      sumsq += current * current;
      for ( usize i = 1; i + 1 < N; ++i ) {
        F next = ((nodes[root] - diagonal[i]) * current - off_diagonal[i - 1] * previous) / off_diagonal[i];
        if ( mk::manip::fabs<F>(next) > F(1000000) ) {
          const F scale = F(0.000001);
          first *= scale;
          previous *= scale;
          current *= scale;
          next *= scale;
          sumsq *= scale * scale;
        }
        sumsq += next * next;
        previous = current;
        current = next;
      }
    }
    weights[root] = moment0 * first * first / sumsq;
  }
}

};      // namespace __impl_gaussian

template<ieee754_floating F, usize N> struct gaussian_rule {
  static_assert(N > 0 && N <= 64, "Gaussian rule order must be in [1, 64]");
  F nodes[N]{};
  F weights[N]{};
  bool valid{ false };

  template<typename Fn>
  [[nodiscard]] inline F
  apply(Fn f, accumulation_policy policy = accumulation_policy::fast) const noexcept
  {
    __impl_accumulate::runtime_sum<F> sum{ policy };
    for ( usize i = 0; i < N; ++i ) sum.add(weights[i] * f(nodes[i]));
    return sum.get();
  }

  template<callable_real_batch<F> Fn>
  [[nodiscard]] inline F
  apply_batch(Fn f, accumulation_policy policy = accumulation_policy::fast) const noexcept
  {
    F values[N]{};
    f(nodes, values, N);
    if ( policy == accumulation_policy::fast ) return __integrate_arch::weighted_sum_fast(weights, values, N);
    __impl_accumulate::runtime_sum<F> sum{ policy };
    for ( usize i = 0; i < N; ++i ) sum.add(weights[i] * values[i]);
    return sum.get();
  }
};

template<ieee754_floating F, usize N> struct jacobi_rule: gaussian_rule<F, N> {
  F alpha{ 0 };
  F beta{ 0 };

  [[nodiscard]] inline bool
  generate(F a, F b) noexcept
  {
    this->valid = false;
    if ( a <= F(-1) || b <= F(-1) ) return false;
    alpha = a;
    beta = b;
    F diagonal[N]{};
    F off_diagonal[N]{};
    diagonal[0] = (b - a) / (a + b + F(2));
    for ( usize i = 1; i < N; ++i ) {
      const F ii = F(i);
      const F s = F(2) * ii + a + b;
      diagonal[i] = (b * b - a * a) / (s * (s + F(2)));
    }
    for ( usize i = 0; i + 1 < N; ++i ) {
      const F n = F(i + 1);
      const F s = F(2) * n + a + b;
      const F removable = n + a + b;
      const F q = removable == F(0) && s == F(1) ? n * (n + a) * (n + b) / (s + F(1))
                                                 : n * (n + a) * (n + b) * removable / ((s - F(1)) * (s + F(1)));
      off_diagonal[i] = F(2) * mk::pow_ns::sqrt<F>(q) / s;
    }
    const F moment0 = mk::pow_ns::pow<F>(F(2), a + b + F(1)) * mk::special::tgamma<F>(a + F(1)) * mk::special::tgamma<F>(b + F(1))
                      / mk::special::tgamma<F>(a + b + F(2));
    __impl_gaussian::build<F, N>(diagonal, off_diagonal, moment0, this->nodes, this->weights);
    this->valid = true;
    return true;
  }
};

template<ieee754_floating F, usize N> using gauss_jacobi_rule = jacobi_rule<F, N>;

template<ieee754_floating F, usize N> struct hermite_rule: gaussian_rule<F, N> {
  inline void
  generate() noexcept
  {
    F diagonal[N]{};
    F off_diagonal[N]{};
    for ( usize i = 0; i + 1 < N; ++i ) off_diagonal[i] = mk::pow_ns::sqrt<F>(F(i + 1) / F(2));
    __impl_gaussian::build<F, N>(diagonal, off_diagonal, mk::pow_ns::sqrt<F>(constant_pi<F>), this->nodes, this->weights);
    this->valid = true;
  }
};

template<ieee754_floating F, usize N> struct laguerre_rule: gaussian_rule<F, N> {
  F alpha{ 0 };

  [[nodiscard]] inline bool
  generate(F a = F(0)) noexcept
  {
    this->valid = false;
    if ( a <= F(-1) ) return false;
    alpha = a;
    F diagonal[N]{};
    F off_diagonal[N]{};
    for ( usize i = 0; i < N; ++i ) diagonal[i] = F(2 * i + 1) + a;
    for ( usize i = 0; i + 1 < N; ++i ) off_diagonal[i] = mk::pow_ns::sqrt<F>(F(i + 1) * (F(i + 1) + a));
    __impl_gaussian::build<F, N>(diagonal, off_diagonal, mk::special::tgamma<F>(a + F(1)), this->nodes, this->weights);
    this->valid = true;
    return true;
  }
};

template<usize Order, ieee754_floating F, typename Fn>
[[nodiscard]] inline F
gauss_hermite(Fn f, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  hermite_rule<F, Order> rule{};
  rule.generate();
  return rule.apply(f, policy);
}

template<usize Order, ieee754_floating F, typename Fn>
[[nodiscard]] inline F
gauss_laguerre(Fn f, F alpha = F(0), accumulation_policy policy = accumulation_policy::fast) noexcept
{
  laguerre_rule<F, Order> rule{};
  if ( !rule.generate(alpha) ) return F(0);
  return rule.apply(f, policy);
}

template<usize Order, ieee754_floating F, typename Fn>
[[nodiscard]] inline F
gauss_chebyshev_1(Fn f, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  __impl_accumulate::runtime_sum<F> sum{ policy };
  for ( usize k = 1; k <= Order; ++k ) {
    const F x = mk::trig::cos<F>(F(2 * k - 1) * constant_pi<F> / F(2 * Order));
    sum.add(f(x));
  }
  return constant_pi<F> * sum.get() / F(Order);
}

template<usize Order, ieee754_floating F, typename Fn>
[[nodiscard]] inline F
gauss_chebyshev_2(Fn f, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  __impl_accumulate::runtime_sum<F> sum{ policy };
  for ( usize k = 1; k <= Order; ++k ) {
    const F angle = F(k) * constant_pi<F> / F(Order + 1);
    const F s = mk::trig::sin<F>(angle);
    sum.add(s * s * f(mk::trig::cos<F>(angle)));
  }
  return constant_pi<F> * sum.get() / F(Order + 1);
}

template<usize Order, ieee754_floating F, typename Fn>
[[nodiscard]] inline F
gauss_chebyshev_i(Fn f, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  return gauss_chebyshev_1<Order, F>(f, policy);
}

template<usize Order, ieee754_floating F, typename Fn>
[[nodiscard]] inline F
gauss_chebyshev_ii(Fn f, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  return gauss_chebyshev_2<Order, F>(f, policy);
}

template<usize Order, ieee754_floating F, typename Fn>
[[nodiscard]] inline F
gauss_jacobi(Fn f, F alpha, F beta, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  jacobi_rule<F, Order> rule{};
  if ( !rule.generate(alpha, beta) ) return F(0);
  return rule.apply(f, policy);
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
