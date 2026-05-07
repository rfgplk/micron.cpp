//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// discrete-data quadrature for measurement series
//
//  -> integrate_samples(xs, ys, n)
//  -> cum_trapezoid(xs, ys, out, n)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "trapezoid.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template <ieee754_floating F>
[[nodiscard]] inline F
integrate_samples(const F *xs, const F *ys, usize n) noexcept
{
  return trapezoid<F>(xs, ys, n);
}

template <is_iterable_container C, is_iterable_container D>
  requires ieee754_floating<typename C::value_type>
[[nodiscard]] inline typename C::value_type
integrate_samples(const C &xs, const D &ys) noexcept
{
  return trapezoid<typename C::value_type>(xs.cbegin(), ys.cbegin(), xs.size());
}

template <ieee754_floating F>
inline void
cum_trapezoid(const F *xs, const F *ys, F *out, usize n) noexcept
{
  if ( n == 0 ) return;
  out[0] = F(0);
  for ( usize i = 1; i < n; ++i ) {
    const F dx = xs[i] - xs[i - 1];
    out[i] = math::fma<F>(F(0.5) * dx, ys[i] + ys[i - 1], out[i - 1]);
  }
}

};     // namespace integrate
};     // namespace math
};     // namespace micron
