//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  tplquad(f, ax, bx, ay, by, az, bz, atol, rtol)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "concepts.hpp"
#include "dblquad.hpp"
#include "quad.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template<ieee754_floating F, typename Fn>
[[nodiscard]] inline quad_result<F>
tplquad(Fn f, F ax, F bx, F ay, F by, F az, F bz, F atol, F rtol, usize max_depth = 30) noexcept
{
  quad_result<F> total{};
  const F third_atol = atol / F(3);
  const F third_rtol = rtol / F(3);
  auto outer_fn = [&](F x) noexcept -> F {
    auto inner_fn = [&](F y, F z) noexcept -> F { return f(x, y, z); };
    quad_result<F> inner = dblquad<F>(inner_fn, ay, by, az, bz, third_atol, third_rtol, max_depth);
    total.n_evals += inner.n_evals;
    if ( inner.status != quad_status::ok ) total.status = inner.status;
    return inner.value;
  };
  quad_result<F> outer = quad<F>(outer_fn, ax, bx, third_atol, third_rtol, max_depth);
  total.value = outer.value;
  total.abs_err = outer.abs_err;
  total.n_evals += outer.n_evals;
  if ( outer.status != quad_status::ok ) total.status = outer.status;
  return total;
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
