//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 2-D adaptive quadrature

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "concepts.hpp"
#include "quad.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template<ieee754_floating F, typename Fn>
[[nodiscard]] inline quad_result<F>
dblquad(Fn f, F ax, F bx, F ay, F by, F atol, F rtol, usize max_depth = 30) noexcept
{
  quad_result<F> outer{};
  const F half_atol = F(0.5) * atol;
  const F half_rtol = F(0.5) * rtol;
  auto outer_fn = [&](F x) noexcept -> F {
    auto inner_fn = [&](F y) noexcept -> F { return f(x, y); };
    quad_result<F> inner = quad<F>(inner_fn, ay, by, half_atol, half_rtol, max_depth);
    outer.n_evals += inner.n_evals;
    if ( inner.status != quad_status::ok ) outer.status = inner.status;
    return inner.value;
  };
  quad_result<F> outer_q = quad<F>(outer_fn, ax, bx, half_atol, half_rtol, max_depth);
  outer.value = outer_q.value;
  outer.abs_err = outer_q.abs_err;
  outer.n_evals += outer_q.n_evals;
  if ( outer_q.status != quad_status::ok ) outer.status = outer_q.status;
  return outer;
}

template<ieee754_floating F, typename Fn, typename YL, typename YU>
[[nodiscard]] inline quad_result<F>
dblquad(Fn f, F ax, F bx, YL yl, YU yu, F atol, F rtol, usize max_depth = 30) noexcept
{
  quad_result<F> outer{};
  const F half_atol = F(0.5) * atol;
  const F half_rtol = F(0.5) * rtol;
  auto outer_fn = [&](F x) noexcept -> F {
    const F a = yl(x);
    const F b = yu(x);
    auto inner_fn = [&](F y) noexcept -> F { return f(x, y); };
    quad_result<F> inner = quad<F>(inner_fn, a, b, half_atol, half_rtol, max_depth);
    outer.n_evals += inner.n_evals;
    if ( inner.status != quad_status::ok ) outer.status = inner.status;
    return inner.value;
  };
  quad_result<F> outer_q = quad<F>(outer_fn, ax, bx, half_atol, half_rtol, max_depth);
  outer.value = outer_q.value;
  outer.abs_err = outer_q.abs_err;
  outer.n_evals += outer_q.n_evals;
  if ( outer_q.status != quad_status::ok ) outer.status = outer_q.status;
  return outer;
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
