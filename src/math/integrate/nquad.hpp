//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "concepts.hpp"
#include "quad.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

namespace __impl_nquad
{

template <usize D, usize Fixed, ieee754_floating F, typename Fn>
inline quad_result<F>
nquad_impl(Fn &f, F (&x)[D], const F (&lo)[D], const F (&hi)[D], F atol, F rtol, usize max_depth, quad_result<F> &acc) noexcept
{
  if constexpr ( Fixed == D ) {
    quad_result<F> r{};
    r.value = f(x);
    return r;
  } else {
    auto outer_fn = [&](F xi) noexcept -> F {
      x[Fixed] = xi;
      if constexpr ( Fixed + 1 == D ) {
        return f(x);
      } else {
        quad_result<F> inner = nquad_impl<D, Fixed + 1, F>(f, x, lo, hi, atol, rtol, max_depth, acc);
        acc.n_evals += inner.n_evals;
        if ( inner.status != quad_status::ok ) acc.status = inner.status;
        return inner.value;
      }
    };
    return quad<F>(outer_fn, lo[Fixed], hi[Fixed], atol, rtol, max_depth);
  }
}

};     // namespace __impl_nquad

template <usize D, ieee754_floating F, typename Fn>
  requires(D >= 1)
[[nodiscard]] inline quad_result<F>
nquad(Fn f, const F (&lo)[D], const F (&hi)[D], F atol, F rtol, usize max_depth = 30) noexcept
{
  quad_result<F> total{};
  const F per_atol = atol / F(D);
  const F per_rtol = rtol / F(D);

  if constexpr ( D == 1 ) {
    auto fn1 = [&](F xi) noexcept -> F {
      F xs[1] = { xi };
      return f(xs);
    };
    return quad<F>(fn1, lo[0], hi[0], atol, rtol, max_depth);
  } else {
    F x[D]{};
    quad_result<F> q = __impl_nquad::nquad_impl<D, 0, F>(f, x, lo, hi, per_atol, per_rtol, max_depth, total);
    total.value = q.value;
    total.abs_err = q.abs_err;
    total.n_evals += q.n_evals;
    if ( q.status != quad_status::ok ) total.status = q.status;
    return total;
  }
}

};     // namespace integrate
};     // namespace math
};     // namespace micron
