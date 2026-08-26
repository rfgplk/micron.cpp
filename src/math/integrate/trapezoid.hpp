//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// composite trapezoidal integration
//
// -> trapezoid(f, a, b, n)
// -> trapezoid(y, n, dx)
// -> trapezoid(xs, ys, n)
// -> trapezoid(C ys, dx)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
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

template<ieee754_floating F, accumulation_policy Policy = accumulation_policy::fast, callable_real<F> Fn>
[[nodiscard]] inline F
trapezoid(Fn f, F a, F b, usize n) noexcept
{
  if ( n == 0 || a == b ) return F(0);
  const F h = (b - a) / F(n);
  // sum the interior points fully, half-weight the endpoints
  __impl_accumulate::sum<F, Policy> s{};
  s.add(F(0.5) * (f(a) + f(b)));
  for ( usize i = 1; i < n; ++i ) s.add(f(math::fma<F>(F(i), h, a)));
  return h * s.get();
}

// with step dx
template<ieee754_floating F, accumulation_policy Policy = accumulation_policy::fast>
[[nodiscard]] inline F
trapezoid(const F *y, usize n, F dx) noexcept
{
  if ( n < 2 ) return F(0);
  if constexpr ( Policy == accumulation_policy::fast ) return dx * (F(0.5) * (y[0] + y[n - 1]) + __integrate_arch::sum_fast(y + 1, n - 2));
  __impl_accumulate::sum<F, Policy> s{};
  s.add(F(0.5) * (y[0] + y[n - 1]));
  for ( usize i = 1; i < n - 1; ++i ) s.add(y[i]);
  return dx * s.get();
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard]] inline typename C::value_type
trapezoid(const C &y, typename C::value_type dx) noexcept
{
  return trapezoid<typename C::value_type>(y.cbegin(), y.size(), dx);
}

template<ieee754_floating F, accumulation_policy Policy = accumulation_policy::fast>
[[nodiscard]] inline F
trapezoid(const F *xs, const F *ys, usize n) noexcept
{
  if ( n < 2 ) return F(0);
  __impl_accumulate::sum<F, Policy> s{};
  for ( usize i = 1; i < n; ++i ) {
    const F dx = xs[i] - xs[i - 1];
    s.add(F(0.5) * dx * (ys[i] + ys[i - 1]));
  }
  return s.get();
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
