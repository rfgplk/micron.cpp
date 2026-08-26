//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// composite Simpson integration
//
// -> simpson(f, a, b, n)
// -> simpson_38(f, a, b, n)
// -> simpson(y, n, dx)

#include "../../concepts.hpp"
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

// n must be % 2 == 0
template<ieee754_floating F, accumulation_policy Policy = accumulation_policy::fast, callable_real<F> Fn>
[[nodiscard]] inline F
simpson(Fn f, F a, F b, usize n) noexcept
{
  if ( n == 0 || a == b ) return F(0);
  if ( n % 2 != 0 ) {
    // fold the trailing panel into a trapezoid and Simpson the rest
    const usize n_simp = n - 1;
    const F h_full = (b - a) / F(n);
    const F b_simp = a + F(n_simp) * h_full;
    const F simp_part = simpson<F, Policy>(f, a, b_simp, n_simp);
    const F trap_part = F(0.5) * h_full * (f(b_simp) + f(b));
    return simp_part + trap_part;
  }
  const F h = (b - a) / F(n);
  __impl_accumulate::sum<F, Policy> s_odd{};
  __impl_accumulate::sum<F, Policy> s_even{};
  for ( usize i = 1; i < n; i += 2 ) s_odd.add(f(math::fma<F>(F(i), h, a)));
  for ( usize i = 2; i < n; i += 2 ) s_even.add(f(math::fma<F>(F(i), h, a)));
  return (h / F(3)) * (f(a) + f(b) + F(4) * s_odd.get() + F(2) * s_even.get());
}

// n must be mul of 3
template<ieee754_floating F, accumulation_policy Policy = accumulation_policy::fast, callable_real<F> Fn>
[[nodiscard]] inline F
simpson_38(Fn f, F a, F b, usize n) noexcept
{
  if ( n == 0 || a == b ) return F(0);
  // round n down to nearest multiple of 3, fold remainder via trapezoid
  const usize r = n % 3;
  if ( r != 0 ) {
    const usize n_main = n - r;
    if ( n_main == 0 ) return trapezoid<F, Policy>(f, a, b, n);
    const F h_full = (b - a) / F(n);
    const F b_main = a + F(n_main) * h_full;
    const F main_part = simpson_38<F, Policy>(f, a, b_main, n_main);
    const F tail_part = trapezoid<F, Policy>(f, b_main, b, r);
    return main_part + tail_part;
  }
  const F h = (b - a) / F(n);
  __impl_accumulate::sum<F, Policy> s_3{};
  __impl_accumulate::sum<F, Policy> s_other{};
  for ( usize i = 1; i < n; ++i ) {
    const F xi = math::fma<F>(F(i), h, a);
    if ( i % 3 == 0 )
      s_3.add(f(xi));
    else
      s_other.add(f(xi));
  }
  return (F(3) * h / F(8)) * (f(a) + f(b) + F(3) * s_other.get() + F(2) * s_3.get());
}

// n must be odd
template<ieee754_floating F, accumulation_policy Policy = accumulation_policy::fast>
[[nodiscard]] inline F
simpson(const F *y, usize n, F dx) noexcept
{
  if ( n < 2 ) return F(0);
  if ( n % 2 == 0 ) {
    const F simp_part = simpson<F, Policy>(y, n - 1, dx);
    const F trap_part = F(0.5) * dx * (y[n - 2] + y[n - 1]);
    return simp_part + trap_part;
  }
  __impl_accumulate::sum<F, Policy> s_odd{};
  __impl_accumulate::sum<F, Policy> s_even{};
  for ( usize i = 1; i < n - 1; i += 2 ) s_odd.add(y[i]);
  for ( usize i = 2; i < n - 1; i += 2 ) s_even.add(y[i]);
  return (dx / F(3)) * (y[0] + y[n - 1] + F(4) * s_odd.get() + F(2) * s_even.get());
}

template<is_iterable_container C>
  requires ieee754_floating<typename C::value_type>
[[nodiscard]] inline typename C::value_type
simpson(const C &y, typename C::value_type dx) noexcept
{
  return simpson<typename C::value_type>(y.cbegin(), y.size(), dx);
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
