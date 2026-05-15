//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 1D quadrature

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "bits/coeff/gauss_kronrod.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

enum class quad_status : u32 { ok = 0, max_depth = 1, abnormal = 2 };

template<ieee754_floating F> struct quad_result {
  F value{ 0 };
  F abs_err{ 0 };
  usize n_evals{ 0 };
  quad_status status{ quad_status::ok };
};

namespace __impl_quad
{

template<ieee754_floating F, typename Fn>
[[gnu::always_inline]] inline void
gk15_7(Fn &f, F a, F b, F &k_out, F &g_out, usize &n_evals_out) noexcept
{
  using table = coeff::gk::gk_15_7<F>;
  const F half_w = F(0.5) * (b - a);
  const F mid = F(0.5) * (a + b);
  const F fmid = f(mid);
  F sk = table::wk[0] * fmid;
  F sg = table::wg[0] * fmid;
  for ( usize k = 1; k < table::half; ++k ) {
    const F xk = table::nodes[k];
    const F fp = f(math::fma<F>(half_w, xk, mid));
    const F fm = f(math::fma<F>(half_w, -xk, mid));
    const F vsum = fp + fm;
    sk = math::fma<F>(table::wk[k], vsum, sk);
    sg = math::fma<F>(table::wg[k], vsum, sg);
  }
  k_out = half_w * sk;
  g_out = half_w * sg;
  n_evals_out += 15;      // 1 (centre) + 7 pairs
}

};      // namespace __impl_quad

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
quad(Fn f, F a, F b, F abs_tol, F rel_tol, usize max_depth = 50, usize max_func_evals = 0) noexcept
{
  quad_result<F> res{};
  if ( a == b ) return res;
  if ( max_depth > 64 ) max_depth = 64;

  constexpr usize stack_cap = 128;

  struct frame {
    F a, b, tol;
    u32 depth_left;
  };

  frame stack[stack_cap];
  usize sp = 0;
  stack[sp++] = frame{ a, b, abs_tol, u32(max_depth) };

  while ( sp > 0 ) {
    const bool eval_capped = max_func_evals != 0 && res.n_evals >= max_func_evals;
    const frame fr = stack[--sp];
    F kv, gv;
    __impl_quad::gk15_7<F>(f, fr.a, fr.b, kv, gv, res.n_evals);
    const F local_err = mk::manip::fabs<F>(kv - gv);
    const F target = mk::manip::fabs<F>(kv) * rel_tol;
    const F budget = (fr.tol > target) ? fr.tol : target;
    if ( local_err <= budget || fr.depth_left == 0 || eval_capped ) {
      res.value += kv;
      res.abs_err += local_err;
      if ( fr.depth_left == 0 && local_err > budget ) res.status = quad_status::max_depth;
      if ( eval_capped && local_err > budget ) res.status = quad_status::max_depth;
      continue;
    }
    if ( sp + 2 > stack_cap ) {
      res.value += kv;
      res.abs_err += local_err;
      res.status = quad_status::max_depth;
      continue;
    }
    const F mid = F(0.5) * (fr.a + fr.b);
    const F new_tol = F(0.5) * fr.tol;
    stack[sp++] = frame{ fr.a, mid, new_tol, fr.depth_left - 1 };
    stack[sp++] = frame{ mid, fr.b, new_tol, fr.depth_left - 1 };
  }
  return res;
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
