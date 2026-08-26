//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../constants.hpp"
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

template<ieee754_floating F> struct tanh_sinh_options {
  F abs_tol{ math::default_eps<F>() * F(100) };
  F rel_tol{ math::default_eps<F>() * F(100) };
  usize max_levels{ 12 };
  usize max_terms{ 128 };
};

namespace __impl_weighted
{

template<ieee754_floating F>
inline void
merge(quad_result<F> &into, const quad_result<F> &part) noexcept
{
  into.value += part.value;
  into.abs_err += part.abs_err;
  into.n_evals += part.n_evals;
  into.resabs += part.resabs;
  into.resasc += part.resasc;
  into.n_intervals += part.n_intervals;
  into.n_splits += part.n_splits;
  into.n_roundoff += part.n_roundoff;
  if ( into.status == quad_status::ok && part.status != quad_status::ok ) into.status = part.status;
}

template<ieee754_floating F, typename Fn>
[[nodiscard]] inline quad_result<F>
tanh_sinh_finite(Fn f, F a, F b, const tanh_sinh_options<F> &options) noexcept
{
  using work_type = micron::conditional_t<(sizeof(F) < sizeof(f64)), f64, F>;
  quad_result<F> result{};
  if ( a == b ) return result;
  if ( b < a ) {
    quad_result<F> r = tanh_sinh_finite<F>(f, b, a, options);
    r.value = -r.value;
    return r;
  }
  if ( options.max_levels == 0 || options.max_levels >= sizeof(usize) * 8 || options.max_terms == 0 || options.abs_tol < F(0)
       || options.rel_tol < F(0) ) {
    result.status = quad_status::invalid_input;
    return result;
  }

  const work_type midpoint = work_type(0.5) * (work_type(a) + work_type(b));
  const work_type halfwidth = work_type(0.5) * (work_type(b) - work_type(a));
  const work_type pi2 = work_type(0.5) * constant_pi<work_type>;
  work_type previous = work_type(0);
  bool have_previous = false;

  for ( usize level = 0; level < options.max_levels; ++level ) {
    const work_type h = work_type(1) / work_type(usize(1) << level);
    const usize refinement = level < sizeof(usize) * 8 - 1 ? (usize(1) << level) : usize(-1) / options.max_terms;
    const usize term_limit = options.max_terms <= usize(-1) / refinement ? options.max_terms * refinement : usize(-1);
    work_type sum = work_type(0);
    usize quiet_tail = 0;
    for ( usize k = 0; k < term_limit; ++k ) {
      const work_type t = work_type(k) * h;
      const work_type sh = mk::hyp::sinh<work_type>(t);
      const work_type ch = mk::hyp::cosh<work_type>(t);
      const work_type u = pi2 * sh;
      const work_type z = mk::hyp::tanh<work_type>(u);
      const work_type cu = mk::hyp::cosh<work_type>(u);
      if ( !ieee::is_finite<work_type>(cu) || z >= work_type(1) ) break;
      const work_type weight = pi2 * ch / (cu * cu);
      work_type term{};
      if ( k == 0 ) {
        const F value = f(F(midpoint));
        ++result.n_evals;
        if ( !ieee::is_finite<F>(value) ) {
          result.status = quad_status::non_finite;
          return result;
        }
        term = weight * value;
      } else {
        const work_type endpoint_distance = halfwidth * (work_type(1) - z);
        F xp = F(work_type(b) - endpoint_distance);
        F xm = F(work_type(a) + endpoint_distance);
        if ( xp >= b ) xp = mk::manip::nextafter<F>(b, a);
        if ( xm <= a ) xm = mk::manip::nextafter<F>(a, b);
        const F fp = f(xp);
        const F fm = f(xm);
        result.n_evals += 2;
        if ( !ieee::is_finite<F>(fp) || !ieee::is_finite<F>(fm) ) {
          result.status = quad_status::non_finite;
          return result;
        }
        term = weight * (fp + fm);
      }
      sum += term;
      if ( k > 4 && mk::manip::fabs<work_type>(h * halfwidth * term) <= work_type(options.abs_tol) * work_type(0.001) )
        ++quiet_tail;
      else
        quiet_tail = 0;
      if ( quiet_tail == 3 ) break;
      if ( k + 1 == term_limit ) result.status = quad_status::max_evaluations;
    }

    const work_type current = halfwidth * h * sum;
    if ( have_previous ) {
      const work_type delta = mk::manip::fabs<work_type>(current - previous);
      const F error = F(work_type(16) * delta);
      result.value = F(current);
      result.abs_err = error;
      result.resabs = mk::manip::fabs<F>(result.value);
      result.n_intervals = level + 1;
      const F rel = options.rel_tol * mk::manip::fabs<F>(result.value);
      const F tolerance = options.abs_tol > rel ? options.abs_tol : rel;
      if ( error <= tolerance ) {
        result.status = quad_status::ok;
        return result;
      }
    }
    previous = current;
    have_previous = true;
  }

  result.value = F(previous);
  result.resabs = mk::manip::fabs<F>(result.value);
  result.n_intervals = options.max_levels;
  if ( result.status == quad_status::ok ) result.status = quad_status::max_depth;
  return result;
}

};      // namespace __impl_weighted

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
tanh_sinh(Fn f, F a, F b, const tanh_sinh_options<F> &options = {}) noexcept
{
  const int ai = ieee::inf_sign<F>(a);
  const int bi = ieee::inf_sign<F>(b);
  if ( ai == 0 && bi == 0 ) return __impl_weighted::tanh_sinh_finite<F>(f, a, b, options);
  if ( (ai == 1 && bi == 0) || (ai == 0 && bi == -1) || (ai == 1 && bi == -1) ) {
    quad_result<F> r = tanh_sinh<F>(f, b, a, options);
    r.value = -r.value;
    return r;
  }
  if ( ai == 0 && bi == 1 ) {
    auto g = [&](F t) noexcept -> F {
      const F q = F(1) - t;
      return f(a + t / q) / (q * q);
    };
    return __impl_weighted::tanh_sinh_finite<F>(g, F(0), F(1), options);
  }
  if ( ai == -1 && bi == 0 ) {
    auto g = [&](F t) noexcept -> F {
      const F q = F(1) - t;
      return f(b - t / q) / (q * q);
    };
    return __impl_weighted::tanh_sinh_finite<F>(g, F(0), F(1), options);
  }
  if ( ai == -1 && bi == 1 ) {
    auto g = [&](F t) noexcept -> F {
      const F angle = constant_pi<F> * (t - F(0.5));
      const F c = mk::trig::cos<F>(angle);
      return f(mk::trig::tan<F>(angle)) * constant_pi<F> / (c * c);
    };
    return __impl_weighted::tanh_sinh_finite<F>(g, F(0), F(1), options);
  }
  quad_result<F> result{};
  result.status = quad_status::invalid_input;
  return result;
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
tanh_sinh(Fn f, F a, F b, F abs_tol, F rel_tol, usize max_levels = 12) noexcept
{
  tanh_sinh_options<F> options{};
  options.abs_tol = abs_tol;
  options.rel_tol = rel_tol;
  options.max_levels = max_levels;
  return tanh_sinh<F>(f, a, b, options);
}

template<ieee754_floating F, callable_real<F> Fn, usize Capacity>
[[nodiscard]] inline quad_result<F>
quad_sin(Fn f, F a, F b, F omega, const quad_options<F> &options, quad_workspace<F, Capacity> &workspace) noexcept
{
  auto weighted = [&](F x) noexcept -> F { return f(x) * mk::trig::sin<F>(omega * x); };
  return quad<F>(weighted, a, b, options, workspace);
}

template<ieee754_floating F, callable_real<F> Fn, usize Capacity>
[[nodiscard]] inline quad_result<F>
quad_cos(Fn f, F a, F b, F omega, const quad_options<F> &options, quad_workspace<F, Capacity> &workspace) noexcept
{
  auto weighted = [&](F x) noexcept -> F { return f(x) * mk::trig::cos<F>(omega * x); };
  return quad<F>(weighted, a, b, options, workspace);
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
quad_sin(Fn f, F a, F b, F omega, F abs_tol, F rel_tol) noexcept
{
  quad_options<F> options{};
  options.abs_tol = abs_tol;
  options.rel_tol = rel_tol;
  quad_workspace<F, 128> workspace{};
  return quad_sin<F>(f, a, b, omega, options, workspace);
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
quad_cos(Fn f, F a, F b, F omega, F abs_tol, F rel_tol) noexcept
{
  quad_options<F> options{};
  options.abs_tol = abs_tol;
  options.rel_tol = rel_tol;
  quad_workspace<F, 128> workspace{};
  return quad_cos<F>(f, a, b, omega, options, workspace);
}

template<ieee754_floating F, callable_real<F> Fn, usize Capacity>
[[nodiscard]] inline quad_result<F>
quad_cauchy(Fn f, F a, F b, F pole, const quad_options<F> &options, quad_workspace<F, Capacity> &workspace) noexcept
{
  if ( b < a ) {
    quad_result<F> r = quad_cauchy<F>(f, b, a, pole, options, workspace);
    r.value = -r.value;
    return r;
  }
  if ( pole <= a || pole >= b ) {
    auto weighted = [&](F x) noexcept -> F { return f(x) / (x - pole); };
    return quad<F>(weighted, a, b, options, workspace);
  }

  const F dl = pole - a;
  const F dr = b - pole;
  const F symmetric = dl < dr ? dl : dr;
  auto cancelled = [&](F t) noexcept -> F { return (f(pole + t) - f(pole - t)) / t; };
  quad_result<F> result = quad<F>(cancelled, F(0), symmetric, options, workspace);
  if ( dl > symmetric ) {
    auto left = [&](F x) noexcept -> F { return f(x) / (x - pole); };
    quad_result<F> part = quad<F>(left, a, pole - symmetric, options, workspace);
    __impl_weighted::merge(result, part);
  }
  if ( dr > symmetric ) {
    auto right = [&](F x) noexcept -> F { return f(x) / (x - pole); };
    quad_result<F> part = quad<F>(right, pole + symmetric, b, options, workspace);
    __impl_weighted::merge(result, part);
  }
  return result;
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
quad_algebraic(Fn f, F a, F b, F alpha, F beta, const tanh_sinh_options<F> &options = {}) noexcept
{
  if ( alpha <= F(-1) || beta <= F(-1) ) {
    quad_result<F> result{};
    result.status = quad_status::invalid_input;
    return result;
  }
  auto weighted = [&](F x) noexcept -> F { return f(x) * mk::pow_ns::pow<F>(x - a, alpha) * mk::pow_ns::pow<F>(b - x, beta); };
  return tanh_sinh<F>(weighted, a, b, options);
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
quad_algebraic_log(Fn f, F a, F b, F alpha, F beta, bool log_left, bool log_right, const tanh_sinh_options<F> &options = {}) noexcept
{
  if ( alpha <= F(-1) || beta <= F(-1) ) {
    quad_result<F> result{};
    result.status = quad_status::invalid_input;
    return result;
  }
  auto weighted = [&](F x) noexcept -> F {
    F w = mk::pow_ns::pow<F>(x - a, alpha) * mk::pow_ns::pow<F>(b - x, beta);
    if ( log_left ) w *= mk::log_ns::log<F>(x - a);
    if ( log_right ) w *= mk::log_ns::log<F>(b - x);
    return f(x) * w;
  };
  return tanh_sinh<F>(weighted, a, b, options);
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
