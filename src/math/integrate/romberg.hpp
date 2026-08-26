//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Richardson-accelerated trapezoidal integration

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline F
romberg(Fn f, F a, F b, F tol, usize max_levels = 12) noexcept
{
  if ( a == b ) return F(0);
  constexpr usize cap = 16;
  if ( max_levels > cap ) max_levels = cap;
  F previous[cap]{};
  F current[cap]{};

  F h = b - a;
  previous[0] = F(0.5) * h * (f(a) + f(b));
  F best = previous[0];

  for ( usize k = 1; k < max_levels; ++k ) {
    const F h_new = h / F(2);
    const usize n_new_pts = (usize(1) << (k - 1));
    F sum = F(0);
    for ( usize i = 1; i <= n_new_pts; ++i ) {
      const F x = math::fma<F>(F(2 * i - 1), h_new, a);
      sum += f(x);
    }
    current[0] = F(0.5) * previous[0] + h_new * sum;

    F denom = F(4);
    for ( usize j = 1; j <= k; ++j ) {
      current[j] = current[j - 1] + (current[j - 1] - previous[j - 1]) / (denom - F(1));
      denom *= F(4);
    }

    const F prev = best;
    best = current[k];
    if ( k >= 2 && mk::manip::fabs<F>(best - prev) < tol ) return best;
    h = h_new;
    for ( usize j = 0; j <= k; ++j ) previous[j] = current[j];
  }
  return best;
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
