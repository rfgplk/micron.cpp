//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "common.hpp"
#include "concepts.hpp"
#include "trapezoid.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

namespace __impl_newton_cotes
{

template<usize Order, ieee754_floating F, bool Open>
[[nodiscard]] inline constexpr F
weight(usize index) noexcept
{
  static_assert(Order >= 1 && Order <= 8, "Newton-Cotes order must be in [1, 8]");
  constexpr usize points = Order + 1;
  F coefficients[points]{};
  coefficients[0] = F(1);
  usize degree = 0;
  const F xi = F(index + (Open ? 1 : 0));
  for ( usize j = 0; j < points; ++j ) {
    if ( j == index ) continue;
    const F xj = F(j + (Open ? 1 : 0));
    const F divisor = xi - xj;
    F next[points]{};
    for ( usize k = 0; k <= degree; ++k ) {
      next[k] -= xj * coefficients[k] / divisor;
      next[k + 1] += coefficients[k] / divisor;
    }
    ++degree;
    for ( usize k = 0; k <= degree; ++k ) coefficients[k] = next[k];
  }
  const F upper = F(Open ? Order + 2 : Order);
  F power = upper;
  F result = F(0);
  for ( usize k = 0; k < points; ++k ) {
    result += coefficients[k] * power / F(k + 1);
    power *= upper;
  }
  return result;
}

template<usize Order, ieee754_floating F, bool Open>
[[nodiscard]] inline constexpr F
table_weight(usize index) noexcept
{
  switch ( index ) {
  case 0:
    return weight<Order, F, Open>(0);
  case 1:
    return weight<Order, F, Open>(1);
  case 2:
    if constexpr ( Order >= 2 ) return weight<Order, F, Open>(2);
    break;
  case 3:
    if constexpr ( Order >= 3 ) return weight<Order, F, Open>(3);
    break;
  case 4:
    if constexpr ( Order >= 4 ) return weight<Order, F, Open>(4);
    break;
  case 5:
    if constexpr ( Order >= 5 ) return weight<Order, F, Open>(5);
    break;
  case 6:
    if constexpr ( Order >= 6 ) return weight<Order, F, Open>(6);
    break;
  case 7:
    if constexpr ( Order >= 7 ) return weight<Order, F, Open>(7);
    break;
  case 8:
    if constexpr ( Order >= 8 ) return weight<Order, F, Open>(8);
    break;
  }
  return F(0);
}

};      // namespace __impl_newton_cotes

template<usize Order, ieee754_floating F>
[[nodiscard]] inline constexpr F
closed_newton_cotes_weight(usize index) noexcept
{
  return __impl_newton_cotes::table_weight<Order, F, false>(index);
}

template<usize Order, ieee754_floating F>
[[nodiscard]] inline constexpr F
open_newton_cotes_weight(usize index) noexcept
{
  return __impl_newton_cotes::table_weight<Order, F, true>(index);
}

template<usize Order, ieee754_floating F, callable_real<F> Fn>
  requires(Order >= 1 && Order <= 8)
[[nodiscard]] inline F
newton_cotes(Fn f, F a, F b, usize panels = Order, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  if ( panels == 0 || a == b ) return F(0);
  const F h = (b - a) / F(panels);
  __impl_accumulate::runtime_sum<F> total{ policy };
  usize first = 0;
  for ( ; first + Order <= panels; first += Order ) {
    F panel = F(0);
    for ( usize j = 0; j <= Order; ++j ) {
      const F x = math::fma<F>(F(first + j), h, a);
      panel += closed_newton_cotes_weight<Order, F>(j) * f(x);
    }
    total.add(h * panel);
  }
  if ( first < panels ) {
    const F tail_a = math::fma<F>(F(first), h, a);
    total.add(trapezoid<F>(f, tail_a, b, panels - first));
  }
  return total.get();
}

template<usize Order, ieee754_floating F, callable_real<F> Fn>
  requires(Order >= 1 && Order <= 8)
[[nodiscard]] inline F
newton_cotes_open(Fn f, F a, F b, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  const F h = (b - a) / F(Order + 2);
  __impl_accumulate::runtime_sum<F> total{ policy };
  for ( usize j = 0; j <= Order; ++j ) {
    const F x = math::fma<F>(F(j + 1), h, a);
    total.add(open_newton_cotes_weight<Order, F>(j) * f(x));
  }
  return h * total.get();
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline F
boole(Fn f, F a, F b, usize panels = 4, accumulation_policy policy = accumulation_policy::fast) noexcept
{
  return newton_cotes<4, F>(f, a, b, panels, policy);
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
