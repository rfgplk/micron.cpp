//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Richardson extrapolation tbl

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"

namespace micron
{
namespace math
{
namespace integrate
{
namespace richardson
{

template<ieee754_floating F, typename Fn>
[[nodiscard]] inline F
extrapolate(Fn step, F h0, F ratio, usize order, usize max_levels, F tol, usize *n_levels_out = nullptr) noexcept
{
  constexpr usize cap = 16;
  if ( max_levels > cap ) max_levels = cap;
  F A[cap][cap]{};

  F h = h0;
  A[0][0] = step(h);
  F best = A[0][0];

  for ( usize i = 1; i < max_levels; ++i ) {
    h = h / ratio;
    A[i][0] = step(h);
    F factor = ratio;
    for ( usize p = 0; p < order; ++p ) factor *= ratio;
    F denom_pow = F(1);
    for ( usize p = 0; p < order; ++p ) denom_pow *= ratio;
    for ( usize j = 1; j <= i; ++j ) {
      A[i][j] = A[i][j - 1] + (A[i][j - 1] - A[i - 1][j - 1]) / (denom_pow - F(1));
      F next_pow = denom_pow;
      for ( usize p = 0; p < order; ++p ) next_pow *= ratio;
      denom_pow = next_pow;
    }
    const F prev = best;
    best = A[i][i];
    if ( i >= 2 && mk::manip::fabs<F>(best - prev) < tol ) {
      if ( n_levels_out ) *n_levels_out = i + 1;
      return best;
    }
  }
  if ( n_levels_out ) *n_levels_out = max_levels;
  return best;
}

};      // namespace richardson
};      // namespace integrate
};      // namespace math
};      // namespace micron
