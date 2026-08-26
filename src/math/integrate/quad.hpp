//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// globalerror adaptive Gauss-Kronrod quadrature

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "../quants/vec.hpp"
#include "bits/coeff/gauss_kronrod.hpp"
#include "common.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

enum class quad_status : u32 {
  ok = 0,
  max_depth = 1,
  abnormal = 2,
  max_evaluations = 3,
  max_intervals = 4,
  roundoff = 5,
  non_finite = 6,
  divergent = 7,
  invalid_input = 8,
  maximum_evaluations = max_evaluations,
  maximum_intervals = max_intervals,
  roundoff_error = roundoff,
};

enum class gauss_kronrod_rule : u32 { gk15 = 15, gk21 = 21 };
enum class quad_error_norm : u32 { maximum = 0, l2 = 1 };

template<ieee754_floating F> struct quad_result {
  F value{ 0 };
  F abs_err{ 0 };
  usize n_evals{ 0 };
  quad_status status{ quad_status::ok };

  F resabs{ 0 };
  F resasc{ 0 };
  usize n_intervals{ 0 };
  usize n_splits{ 0 };
  usize n_roundoff{ 0 };
};

template<ieee754_floating F> struct quad_options {
  union {
    F abs_tol{ math::default_eps<F>() * F(100) };
    F atol;
  };

  union {
    F rel_tol{ math::default_eps<F>() * F(100) };
    F rtol;
  };

  union {
    usize max_evals{ 0 };
    usize max_func_evals;
  };

  union {
    usize max_intervals{ 128 };
    usize limit;
  };

  usize max_depth{ 64 };
  gauss_kronrod_rule rule{ gauss_kronrod_rule::gk21 };

  union {
    const F *breakpoints{ nullptr };
    const F *points;
  };

  union {
    usize n_breakpoints{ 0 };
    usize n_points;
  };

  accumulation_policy accumulation{ accumulation_policy::fast };
};

template<ieee754_floating F> struct quad_interval {
  F a{ 0 };
  F b{ 0 };
  F value{ 0 };
  F error{ 0 };
  F resabs{ 0 };
  F resasc{ 0 };
  u32 depth{ 0 };
};

template<ieee754_floating F, usize Capacity> struct quad_workspace {
  static_assert(Capacity > 0, "quad_workspace capacity must be non-zero");
  quad_interval<F> heap[Capacity]{};
  usize size{ 0 };

  static constexpr usize capacity = Capacity;

  constexpr void
  reset() noexcept
  {
    size = 0;
  }
};

namespace __impl_quad
{

template<ieee754_floating F>
[[gnu::always_inline]] inline void
add(F x, accumulation_policy policy, F &sum, F &correction) noexcept
{
  if ( policy == accumulation_policy::fast ) {
    sum += x;
    return;
  }
  const F next = sum + x;
  if ( mk::manip::fabs<F>(sum) >= mk::manip::fabs<F>(x) )
    correction += (sum - next) + x;
  else
    correction += (x - next) + sum;
  sum = next;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
total(F sum, F correction, accumulation_policy policy) noexcept
{
  return policy == accumulation_policy::accurate ? sum + correction : sum;
}

template<ieee754_floating F, typename Fn, typename Table>
[[gnu::always_inline]] inline bool
gk_rule(Fn &f, F a, F b, quad_interval<F> &out, usize &n_evals) noexcept
{
  const F half_w = F(0.5) * (b - a);
  const F abs_half_w = mk::manip::fabs<F>(half_w);
  const F mid = F(0.5) * (a + b);
  const F fmid = f(mid);
  ++n_evals;
  if ( !ieee::is_finite<F>(fmid) ) return false;

  F fv1[Table::half]{};
  F fv2[Table::half]{};
  fv1[0] = fmid;
  fv2[0] = fmid;
  F resk = Table::wk[0] * fmid;
  F resg = Table::wg[0] * fmid;
  F resabs = Table::wk[0] * mk::manip::fabs<F>(fmid);

  for ( usize k = 1; k < Table::half; ++k ) {
    const F absc = half_w * Table::nodes[k];
    const F fm = f(mid - absc);
    const F fp = f(mid + absc);
    n_evals += 2;
    if ( !ieee::is_finite<F>(fm) || !ieee::is_finite<F>(fp) ) return false;
    fv1[k] = fm;
    fv2[k] = fp;
    const F pair = fm + fp;
    resk = math::fma<F>(Table::wk[k], pair, resk);
    if ( Table::wg[k] != F(0) ) resg = math::fma<F>(Table::wg[k], pair, resg);
    resabs = math::fma<F>(Table::wk[k], mk::manip::fabs<F>(fm) + mk::manip::fabs<F>(fp), resabs);
  }

  const F mean = F(0.5) * resk;
  F resasc = Table::wk[0] * mk::manip::fabs<F>(fmid - mean);
  for ( usize k = 1; k < Table::half; ++k )
    resasc += Table::wk[k] * (mk::manip::fabs<F>(fv1[k] - mean) + mk::manip::fabs<F>(fv2[k] - mean));

  out.a = a;
  out.b = b;
  out.value = half_w * resk;
  out.resabs = abs_half_w * resabs;
  out.resasc = abs_half_w * resasc;
  F error = mk::manip::fabs<F>(half_w * (resk - resg));
  if ( out.resasc != F(0) && error != F(0) ) {
    F ratio = F(200) * error / out.resasc;
    if ( ratio < F(1) ) error = out.resasc * ratio * mk::pow_ns::sqrt<F>(ratio);
  }
  const F floor = F(50) * machine_epsilon<F>() * out.resabs;
  out.error = error > floor ? error : floor;
  return ieee::is_finite<F>(out.value) && ieee::is_finite<F>(out.error);
}

template<ieee754_floating F, typename Fn, typename Table>
[[gnu::always_inline]] inline bool
gk_rule_batch(Fn &f, F a, F b, quad_interval<F> &out, usize &n_evals) noexcept
{
  constexpr usize count = Table::half * 2 - 1;
  const F half_w = F(0.5) * (b - a);
  const F abs_half_w = mk::manip::fabs<F>(half_w);
  const F mid = F(0.5) * (a + b);
  F points[count]{};
  F values[count]{};
  points[0] = mid;
  for ( usize k = 1; k < Table::half; ++k ) {
    const F absc = half_w * Table::nodes[k];
    points[2 * k - 1] = mid - absc;
    points[2 * k] = mid + absc;
  }
  f(points, values, count);
  n_evals += count;
  for ( usize i = 0; i < count; ++i )
    if ( !ieee::is_finite<F>(values[i]) ) return false;

  const F fmid = values[0];
  F resk = Table::wk[0] * fmid;
  F resg = Table::wg[0] * fmid;
  F resabs = Table::wk[0] * mk::manip::fabs<F>(fmid);
  for ( usize k = 1; k < Table::half; ++k ) {
    const F fm = values[2 * k - 1];
    const F fp = values[2 * k];
    const F pair = fm + fp;
    resk = math::fma<F>(Table::wk[k], pair, resk);
    if ( Table::wg[k] != F(0) ) resg = math::fma<F>(Table::wg[k], pair, resg);
    resabs = math::fma<F>(Table::wk[k], mk::manip::fabs<F>(fm) + mk::manip::fabs<F>(fp), resabs);
  }

  const F mean = F(0.5) * resk;
  F resasc = Table::wk[0] * mk::manip::fabs<F>(fmid - mean);
  for ( usize k = 1; k < Table::half; ++k )
    resasc += Table::wk[k] * (mk::manip::fabs<F>(values[2 * k - 1] - mean) + mk::manip::fabs<F>(values[2 * k] - mean));

  out.a = a;
  out.b = b;
  out.value = half_w * resk;
  out.resabs = abs_half_w * resabs;
  out.resasc = abs_half_w * resasc;
  F error = mk::manip::fabs<F>(half_w * (resk - resg));
  if ( out.resasc != F(0) && error != F(0) ) {
    const F ratio = F(200) * error / out.resasc;
    if ( ratio < F(1) ) error = out.resasc * ratio * mk::pow_ns::sqrt<F>(ratio);
  }
  const F floor = F(50) * machine_epsilon<F>() * out.resabs;
  out.error = error > floor ? error : floor;
  return ieee::is_finite<F>(out.value) && ieee::is_finite<F>(out.error);
}

template<ieee754_floating F, typename Fn>
[[gnu::always_inline]] inline bool
evaluate(Fn &f, F a, F b, gauss_kronrod_rule rule, quad_interval<F> &out, usize &n_evals) noexcept
{
  if ( rule == gauss_kronrod_rule::gk15 ) return gk_rule<F, Fn, coeff::gk::gk_15_7<F>>(f, a, b, out, n_evals);
  return gk_rule<F, Fn, coeff::gk::gk_21_10<F>>(f, a, b, out, n_evals);
}

template<ieee754_floating F, typename Fn>
[[gnu::always_inline]] inline bool
evaluate_batch(Fn &f, F a, F b, gauss_kronrod_rule rule, quad_interval<F> &out, usize &n_evals) noexcept
{
  if ( rule == gauss_kronrod_rule::gk15 ) return gk_rule_batch<F, Fn, coeff::gk::gk_15_7<F>>(f, a, b, out, n_evals);
  return gk_rule_batch<F, Fn, coeff::gk::gk_21_10<F>>(f, a, b, out, n_evals);
}

template<ieee754_floating F, usize Capacity>
[[gnu::always_inline]] inline void
heap_push(quad_workspace<F, Capacity> &ws, const quad_interval<F> &v) noexcept
{
  usize i = ws.size++;
  while ( i != 0 ) {
    const usize parent = (i - 1) >> 1;
    if ( ws.heap[parent].error >= v.error ) break;
    ws.heap[i] = ws.heap[parent];
    i = parent;
  }
  ws.heap[i] = v;
}

template<ieee754_floating F, usize Capacity>
[[nodiscard, gnu::always_inline]] inline quad_interval<F>
heap_pop(quad_workspace<F, Capacity> &ws) noexcept
{
  const quad_interval<F> result = ws.heap[0];
  const quad_interval<F> tail = ws.heap[--ws.size];
  if ( ws.size == 0 ) return result;
  usize i = 0;
  while ( true ) {
    const usize left = i * 2 + 1;
    if ( left >= ws.size ) break;
    const usize right = left + 1;
    const usize child = right < ws.size && ws.heap[right].error > ws.heap[left].error ? right : left;
    if ( ws.heap[child].error <= tail.error ) break;
    ws.heap[i] = ws.heap[child];
    i = child;
  }
  ws.heap[i] = tail;
  return result;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
tolerance(const quad_options<F> &options, F value) noexcept
{
  const F rel = options.rel_tol * mk::manip::fabs<F>(value);
  return options.abs_tol > rel ? options.abs_tol : rel;
}

template<ieee754_floating F, typename Fn, usize Capacity, bool Batch = false>
[[nodiscard]] inline quad_result<F>
finite(Fn f, F a, F b, const quad_options<F> &options, quad_workspace<F, Capacity> &ws) noexcept
{
  quad_result<F> result{};
  ws.reset();
  if ( a == b ) return result;
  if ( options.abs_tol < F(0) || options.rel_tol < F(0) || options.max_depth == 0
       || (options.n_breakpoints != 0 && options.breakpoints == nullptr) ) {
    result.status = quad_status::invalid_input;
    return result;
  }
  for ( usize i = 0; i < options.n_breakpoints; ++i ) {
    if ( !ieee::is_finite<F>(options.breakpoints[i]) ) {
      result.status = quad_status::invalid_input;
      return result;
    }
  }

  const usize requested_limit = options.max_intervals == 0 ? Capacity : options.max_intervals;
  const usize interval_limit = requested_limit < Capacity ? requested_limit : Capacity;
  const usize eval_cost = options.rule == gauss_kronrod_rule::gk15 ? 15 : 21;
  F value = F(0), value_c = F(0), error = F(0), error_c = F(0), resabs = F(0), resabs_c = F(0), resasc = F(0), resasc_c = F(0);

  F left = a;
  usize initialized = 0;
  while ( left < b ) {
    F right = b;
    for ( usize i = 0; i < options.n_breakpoints; ++i ) {
      const F p = options.breakpoints[i];
      if ( p > left && p < right && p < b ) right = p;
    }
    if ( initialized >= interval_limit ) {
      result.status = quad_status::max_intervals;
      break;
    }
    if ( options.max_evals != 0 && result.n_evals + eval_cost > options.max_evals ) {
      result.status = quad_status::max_evaluations;
      break;
    }
    quad_interval<F> initial{};
    bool evaluated{};
    if constexpr ( Batch )
      evaluated = evaluate_batch<F>(f, left, right, options.rule, initial, result.n_evals);
    else
      evaluated = evaluate<F>(f, left, right, options.rule, initial, result.n_evals);
    if ( !evaluated ) {
      result.status = quad_status::non_finite;
      ws.reset();
      return result;
    }
    heap_push(ws, initial);
    add(initial.value, options.accumulation, value, value_c);
    add(initial.error, options.accumulation, error, error_c);
    add(initial.resabs, options.accumulation, resabs, resabs_c);
    add(initial.resasc, options.accumulation, resasc, resasc_c);
    ++initialized;
    left = right;
  }

  if ( ws.size == 0 ) return result;
  result.n_intervals = ws.size;
  const F initial_error = total(error, error_c, options.accumulation);

  while ( result.status == quad_status::ok ) {
    const F current_value = total(value, value_c, options.accumulation);
    const F current_error = total(error, error_c, options.accumulation);
    if ( current_error <= tolerance(options, current_value) ) break;
    if ( ws.size >= interval_limit ) {
      result.status = quad_status::max_intervals;
      break;
    }
    if ( options.max_evals != 0 && result.n_evals + 2 * eval_cost > options.max_evals ) {
      result.status = quad_status::max_evaluations;
      break;
    }

    const quad_interval<F> parent = heap_pop(ws);
    if ( parent.depth >= options.max_depth ) {
      heap_push(ws, parent);
      result.status = quad_status::max_depth;
      break;
    }
    const F mid = F(0.5) * (parent.a + parent.b);
    if ( mid == parent.a || mid == parent.b ) {
      heap_push(ws, parent);
      result.status = quad_status::roundoff;
      break;
    }

    quad_interval<F> lo{}, hi{};
    bool lo_ok{}, hi_ok{};
    if constexpr ( Batch ) {
      lo_ok = evaluate_batch<F>(f, parent.a, mid, options.rule, lo, result.n_evals);
      hi_ok = evaluate_batch<F>(f, mid, parent.b, options.rule, hi, result.n_evals);
    } else {
      lo_ok = evaluate<F>(f, parent.a, mid, options.rule, lo, result.n_evals);
      hi_ok = evaluate<F>(f, mid, parent.b, options.rule, hi, result.n_evals);
    }
    if ( !lo_ok || !hi_ok ) {
      heap_push(ws, parent);
      result.status = quad_status::non_finite;
      break;
    }
    lo.depth = parent.depth + 1;
    hi.depth = parent.depth + 1;

    const F child_value = lo.value + hi.value;
    const F child_error = lo.error + hi.error;
    const F scale = mk::manip::fabs<F>(parent.value) + F(1);
    if ( mk::manip::fabs<F>(parent.value - child_value) <= F(100) * machine_epsilon<F>() * scale && child_error >= F(0.99) * parent.error )
      ++result.n_roundoff;

    add(-parent.value, options.accumulation, value, value_c);
    add(child_value, options.accumulation, value, value_c);
    add(-parent.error, options.accumulation, error, error_c);
    add(child_error, options.accumulation, error, error_c);
    add(-parent.resabs, options.accumulation, resabs, resabs_c);
    add(lo.resabs + hi.resabs, options.accumulation, resabs, resabs_c);
    add(-parent.resasc, options.accumulation, resasc, resasc_c);
    add(lo.resasc + hi.resasc, options.accumulation, resasc, resasc_c);
    heap_push(ws, lo);
    heap_push(ws, hi);
    ++result.n_splits;
    result.n_intervals = ws.size;

    if ( result.n_roundoff >= 20 ) result.status = quad_status::roundoff;
    const F now_error = total(error, error_c, options.accumulation);
    if ( result.n_splits > 50 && initial_error > F(0) && now_error > initial_error * F(1000000) ) result.status = quad_status::divergent;
  }

  result.value = total(value, value_c, options.accumulation);
  result.abs_err = total(error, error_c, options.accumulation);
  result.resabs = total(resabs, resabs_c, options.accumulation);
  result.resasc = total(resasc, resasc_c, options.accumulation);
  if ( result.abs_err < F(0) ) result.abs_err = F(0);
  if ( !ieee::is_finite<F>(result.value) || !ieee::is_finite<F>(result.abs_err) ) result.status = quad_status::non_finite;
  return result;
}

template<ieee754_floating F, typename Fn, usize Capacity>
[[nodiscard]] inline quad_result<F>
dispatch_bounds(Fn f, F a, F b, const quad_options<F> &options, quad_workspace<F, Capacity> &ws) noexcept
{
  if ( a != a || b != b ) {
    quad_result<F> result{};
    result.status = quad_status::invalid_input;
    return result;
  }
  const int ai = ieee::inf_sign<F>(a);
  const int bi = ieee::inf_sign<F>(b);
  if ( ai == 0 && bi == 0 ) {
    if ( b < a ) {
      quad_result<F> r = finite<F>(f, b, a, options, ws);
      r.value = -r.value;
      return r;
    }
    return finite<F>(f, a, b, options, ws);
  }

  if ( (ai == 1 && bi == 0) || (ai == 0 && bi == -1) || (ai == 1 && bi == -1) ) {
    quad_result<F> r = dispatch_bounds<F>(f, b, a, options, ws);
    r.value = -r.value;
    return r;
  }

  quad_options<F> transformed = options;
  transformed.breakpoints = nullptr;
  transformed.n_breakpoints = 0;

  if ( ai == 0 && bi == 1 ) {
    auto g = [&](F t) noexcept -> F {
      const F q = F(1) - t;
      const F x = a + t / q;
      return f(x) / (q * q);
    };
    return finite<F>(g, F(0), F(1), transformed, ws);
  }
  if ( ai == -1 && bi == 0 ) {
    auto g = [&](F t) noexcept -> F {
      const F q = F(1) - t;
      const F x = b - t / q;
      return f(x) / (q * q);
    };
    return finite<F>(g, F(0), F(1), transformed, ws);
  }
  if ( ai == -1 && bi == 1 ) {
    auto g = [&](F t) noexcept -> F {
      const F angle = constant_pi<F> * (t - F(0.5));
      const F c = mk::trig::cos<F>(angle);
      return f(mk::trig::tan<F>(angle)) * constant_pi<F> / (c * c);
    };
    return finite<F>(g, F(0), F(1), transformed, ws);
  }

  quad_result<F> result{};
  result.status = quad_status::invalid_input;
  return result;
}

};      // namespace __impl_quad

template<ieee754_floating F, callable_real<F> Fn, usize Capacity>
[[nodiscard]] inline quad_result<F>
quad(Fn f, F a, F b, const quad_options<F> &options, quad_workspace<F, Capacity> &workspace) noexcept
{
  return __impl_quad::dispatch_bounds<F>(f, a, b, options, workspace);
}

template<ieee754_floating F, callable_real<F> Fn, usize Capacity>
[[nodiscard]] inline quad_result<F>
quad(Fn f, F a, F b, quad_workspace<F, Capacity> &workspace, const quad_options<F> &options = {}) noexcept
{
  return quad<F>(f, a, b, options, workspace);
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
quad(Fn f, F a, F b, const quad_options<F> &options) noexcept
{
  quad_workspace<F, 128> workspace{};
  return quad<F>(f, a, b, options, workspace);
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline quad_result<F>
quad(Fn f, F a, F b, F abs_tol, F rel_tol, usize max_depth = 50, usize max_func_evals = 0) noexcept
{
  quad_options<F> options{};
  options.abs_tol = abs_tol;
  options.rel_tol = rel_tol;
  options.max_depth = max_depth > 64 ? 64 : max_depth;
  options.max_evals = max_func_evals;
  options.max_intervals = 128;
  options.rule = gauss_kronrod_rule::gk15;
  quad_workspace<F, 128> workspace{};
  return quad<F>(f, a, b, options, workspace);
}

template<usize M, ieee754_floating F> struct quad_vec_result {
  vec<F, M> value{};
  vec<F, M> abs_err{};
  usize n_evals{ 0 };
  quad_status status{ quad_status::ok };
  F error_norm{ 0 };
  usize n_intervals{ 0 };
};

template<usize M, ieee754_floating F> struct quad_vec_interval {
  F a{ 0 };
  F b{ 0 };
  vec<F, M> value{};
  vec<F, M> error{};
  F priority{ 0 };
  u32 depth{ 0 };
};

template<usize M, ieee754_floating F, usize Capacity> struct quad_vec_workspace {
  static_assert(M >= 2 && M <= 16 && Capacity > 0, "invalid vector quadrature workspace");
  quad_vec_interval<M, F> heap[Capacity]{};
  usize size{ 0 };

  static constexpr usize capacity = Capacity;

  constexpr void
  reset() noexcept
  {
    size = 0;
  }
};

namespace __impl_quad_vec
{

template<usize M, ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
norm(const vec<F, M> &value, quad_error_norm mode) noexcept
{
  F result = F(0);
  if ( mode == quad_error_norm::maximum ) {
    for ( usize component = 0; component < M; ++component ) {
      const F magnitude = mk::manip::fabs<F>(value.data[component]);
      if ( magnitude > result ) result = magnitude;
    }
    return result;
  }
  for ( usize component = 0; component < M; ++component ) result += value.data[component] * value.data[component];
  return mk::pow_ns::sqrt<F>(result);
}

template<usize M, ieee754_floating F, typename Fn, typename Table>
[[nodiscard]] inline bool
evaluate_rule(Fn &f, F a, F b, quad_vec_interval<M, F> &out, usize &n_evals, quad_error_norm mode) noexcept
{
  const F half_w = F(0.5) * (b - a);
  const F abs_half_w = mk::manip::fabs<F>(half_w);
  const F mid = F(0.5) * (a + b);
  vec<F, M> negative[Table::half]{};
  vec<F, M> positive[Table::half]{};
  negative[0] = f(mid);
  positive[0] = negative[0];
  ++n_evals;
  F resk[M]{}, resg[M]{}, resabs[M]{};
  for ( usize component = 0; component < M; ++component ) {
    const F center = negative[0].data[component];
    if ( !ieee::is_finite<F>(center) ) return false;
    resk[component] = Table::wk[0] * center;
    resg[component] = Table::wg[0] * center;
    resabs[component] = Table::wk[0] * mk::manip::fabs<F>(center);
  }
  for ( usize k = 1; k < Table::half; ++k ) {
    const F absc = half_w * Table::nodes[k];
    negative[k] = f(mid - absc);
    positive[k] = f(mid + absc);
    n_evals += 2;
    for ( usize component = 0; component < M; ++component ) {
      const F fm = negative[k].data[component];
      const F fp = positive[k].data[component];
      if ( !ieee::is_finite<F>(fm) || !ieee::is_finite<F>(fp) ) return false;
      const F pair = fm + fp;
      resk[component] = math::fma<F>(Table::wk[k], pair, resk[component]);
      if ( Table::wg[k] != F(0) ) resg[component] = math::fma<F>(Table::wg[k], pair, resg[component]);
      resabs[component] = math::fma<F>(Table::wk[k], mk::manip::fabs<F>(fm) + mk::manip::fabs<F>(fp), resabs[component]);
    }
  }

  out.a = a;
  out.b = b;
  for ( usize component = 0; component < M; ++component ) {
    const F mean = F(0.5) * resk[component];
    F resasc = Table::wk[0] * mk::manip::fabs<F>(negative[0].data[component] - mean);
    for ( usize k = 1; k < Table::half; ++k )
      resasc += Table::wk[k]
                * (mk::manip::fabs<F>(negative[k].data[component] - mean) + mk::manip::fabs<F>(positive[k].data[component] - mean));
    const F scaled_abs = abs_half_w * resabs[component];
    const F scaled_asc = abs_half_w * resasc;
    F error = mk::manip::fabs<F>(half_w * (resk[component] - resg[component]));
    if ( scaled_asc != F(0) && error != F(0) ) {
      const F ratio = F(200) * error / scaled_asc;
      if ( ratio < F(1) ) error = scaled_asc * ratio * mk::pow_ns::sqrt<F>(ratio);
    }
    const F floor = F(50) * machine_epsilon<F>() * scaled_abs;
    out.value.data[component] = half_w * resk[component];
    out.error.data[component] = error > floor ? error : floor;
  }
  out.priority = norm(out.error, mode);
  return ieee::is_finite<F>(out.priority);
}

template<usize M, ieee754_floating F, typename Fn>
[[nodiscard]] inline bool
evaluate(Fn &f, F a, F b, gauss_kronrod_rule rule, quad_vec_interval<M, F> &out, usize &n_evals, quad_error_norm mode) noexcept
{
  if ( rule == gauss_kronrod_rule::gk15 ) return evaluate_rule<M, F, Fn, coeff::gk::gk_15_7<F>>(f, a, b, out, n_evals, mode);
  return evaluate_rule<M, F, Fn, coeff::gk::gk_21_10<F>>(f, a, b, out, n_evals, mode);
}

template<usize M, ieee754_floating F, usize Capacity>
inline void
push(quad_vec_workspace<M, F, Capacity> &workspace, const quad_vec_interval<M, F> &interval) noexcept
{
  usize i = workspace.size++;
  while ( i != 0 ) {
    const usize parent = (i - 1) >> 1;
    if ( workspace.heap[parent].priority >= interval.priority ) break;
    workspace.heap[i] = workspace.heap[parent];
    i = parent;
  }
  workspace.heap[i] = interval;
}

template<usize M, ieee754_floating F, usize Capacity>
[[nodiscard]] inline quad_vec_interval<M, F>
pop(quad_vec_workspace<M, F, Capacity> &workspace) noexcept
{
  const quad_vec_interval<M, F> result = workspace.heap[0];
  const quad_vec_interval<M, F> tail = workspace.heap[--workspace.size];
  if ( workspace.size == 0 ) return result;
  usize i = 0;
  while ( true ) {
    const usize left = i * 2 + 1;
    if ( left >= workspace.size ) break;
    const usize right = left + 1;
    const usize child = right < workspace.size && workspace.heap[right].priority > workspace.heap[left].priority ? right : left;
    if ( workspace.heap[child].priority <= tail.priority ) break;
    workspace.heap[i] = workspace.heap[child];
    i = child;
  }
  workspace.heap[i] = tail;
  return result;
}

template<usize M, ieee754_floating F, typename Fn, usize Capacity>
[[nodiscard]] inline quad_vec_result<M, F>
finite(Fn f, F a, F b, const quad_options<F> &options, quad_vec_workspace<M, F, Capacity> &workspace, quad_error_norm mode) noexcept
{
  quad_vec_result<M, F> result{};
  workspace.reset();
  if ( a == b ) return result;
  if ( options.abs_tol < F(0) || options.rel_tol < F(0) || options.max_depth == 0
       || (options.n_breakpoints != 0 && options.breakpoints == nullptr) ) {
    result.status = quad_status::invalid_input;
    return result;
  }
  for ( usize i = 0; i < options.n_breakpoints; ++i ) {
    if ( !ieee::is_finite<F>(options.breakpoints[i]) ) {
      result.status = quad_status::invalid_input;
      return result;
    }
  }
  const usize requested_limit = options.max_intervals == 0 ? Capacity : options.max_intervals;
  const usize interval_limit = requested_limit < Capacity ? requested_limit : Capacity;
  const usize eval_cost = options.rule == gauss_kronrod_rule::gk15 ? 15 : 21;
  __impl_accumulate::runtime_sum<F> values[M];
  __impl_accumulate::runtime_sum<F> errors[M];
  for ( usize component = 0; component < M; ++component ) {
    values[component].policy = options.accumulation;
    errors[component].policy = options.accumulation;
  }

  F left = a;
  while ( left < b ) {
    F right = b;
    for ( usize i = 0; i < options.n_breakpoints; ++i ) {
      const F point = options.breakpoints[i];
      if ( point > left && point < right && point < b ) right = point;
    }
    if ( workspace.size >= interval_limit ) {
      result.status = quad_status::max_intervals;
      break;
    }
    if ( options.max_evals != 0 && result.n_evals + eval_cost > options.max_evals ) {
      result.status = quad_status::max_evaluations;
      break;
    }
    quad_vec_interval<M, F> interval{};
    if ( !evaluate<M, F>(f, left, right, options.rule, interval, result.n_evals, mode) ) {
      result.status = quad_status::non_finite;
      workspace.reset();
      return result;
    }
    push(workspace, interval);
    for ( usize component = 0; component < M; ++component ) {
      values[component].add(interval.value.data[component]);
      errors[component].add(interval.error.data[component]);
    }
    left = right;
  }
  if ( workspace.size == 0 ) return result;

  while ( result.status == quad_status::ok ) {
    for ( usize component = 0; component < M; ++component ) {
      result.value.data[component] = values[component].get();
      result.abs_err.data[component] = errors[component].get();
    }
    result.error_norm = norm(result.abs_err, mode);
    const F value_norm = norm(result.value, mode);
    const F relative = options.rel_tol * value_norm;
    const F tolerance = options.abs_tol > relative ? options.abs_tol : relative;
    if ( result.error_norm <= tolerance ) break;
    if ( workspace.size >= interval_limit ) {
      result.status = quad_status::max_intervals;
      break;
    }
    if ( options.max_evals != 0 && result.n_evals + 2 * eval_cost > options.max_evals ) {
      result.status = quad_status::max_evaluations;
      break;
    }
    const quad_vec_interval<M, F> parent = pop(workspace);
    if ( parent.depth >= options.max_depth ) {
      push(workspace, parent);
      result.status = quad_status::max_depth;
      break;
    }
    const F mid = F(0.5) * (parent.a + parent.b);
    if ( mid == parent.a || mid == parent.b ) {
      push(workspace, parent);
      result.status = quad_status::roundoff;
      break;
    }
    quad_vec_interval<M, F> lo{}, hi{};
    if ( !evaluate<M, F>(f, parent.a, mid, options.rule, lo, result.n_evals, mode)
         || !evaluate<M, F>(f, mid, parent.b, options.rule, hi, result.n_evals, mode) ) {
      push(workspace, parent);
      result.status = quad_status::non_finite;
      break;
    }
    lo.depth = parent.depth + 1;
    hi.depth = parent.depth + 1;
    for ( usize component = 0; component < M; ++component ) {
      values[component].add(-parent.value.data[component]);
      values[component].add(lo.value.data[component] + hi.value.data[component]);
      errors[component].add(-parent.error.data[component]);
      errors[component].add(lo.error.data[component] + hi.error.data[component]);
    }
    push(workspace, lo);
    push(workspace, hi);
  }
  for ( usize component = 0; component < M; ++component ) {
    result.value.data[component] = values[component].get();
    result.abs_err.data[component] = errors[component].get();
    if ( result.abs_err.data[component] < F(0) ) result.abs_err.data[component] = F(0);
  }
  result.error_norm = norm(result.abs_err, mode);
  result.n_intervals = workspace.size;
  return result;
}

};      // namespace __impl_quad_vec

template<usize M, ieee754_floating F, typename Fn, usize Capacity>
  requires(M >= 2 && M <= 16)
[[nodiscard]] inline quad_vec_result<M, F>
quad_vec(Fn f, F a, F b, const quad_options<F> &options, quad_vec_workspace<M, F, Capacity> &workspace,
         quad_error_norm norm = quad_error_norm::maximum) noexcept
{
  if ( a != a || b != b ) {
    quad_vec_result<M, F> result{};
    result.status = quad_status::invalid_input;
    return result;
  }
  const int ai = ieee::inf_sign<F>(a);
  const int bi = ieee::inf_sign<F>(b);
  if ( ai == 0 && bi == 0 ) {
    if ( b < a ) {
      quad_vec_result<M, F> result = __impl_quad_vec::finite<M, F>(f, b, a, options, workspace, norm);
      for ( usize component = 0; component < M; ++component ) result.value.data[component] = -result.value.data[component];
      return result;
    }
    return __impl_quad_vec::finite<M, F>(f, a, b, options, workspace, norm);
  }
  if ( (ai == 1 && bi == 0) || (ai == 0 && bi == -1) || (ai == 1 && bi == -1) ) {
    quad_vec_result<M, F> result = quad_vec<M, F>(f, b, a, options, workspace, norm);
    for ( usize component = 0; component < M; ++component ) result.value.data[component] = -result.value.data[component];
    return result;
  }
  quad_options<F> transformed = options;
  transformed.breakpoints = nullptr;
  transformed.n_breakpoints = 0;
  if ( ai == 0 && bi == 1 ) {
    auto g = [&](F t) noexcept {
      const F q = F(1) - t;
      auto value = f(a + t / q);
      for ( usize component = 0; component < M; ++component ) value.data[component] /= q * q;
      return value;
    };
    return __impl_quad_vec::finite<M, F>(g, F(0), F(1), transformed, workspace, norm);
  }
  if ( ai == -1 && bi == 0 ) {
    auto g = [&](F t) noexcept {
      const F q = F(1) - t;
      auto value = f(b - t / q);
      for ( usize component = 0; component < M; ++component ) value.data[component] /= q * q;
      return value;
    };
    return __impl_quad_vec::finite<M, F>(g, F(0), F(1), transformed, workspace, norm);
  }
  if ( ai == -1 && bi == 1 ) {
    auto g = [&](F t) noexcept {
      const F angle = constant_pi<F> * (t - F(0.5));
      const F c = mk::trig::cos<F>(angle);
      auto value = f(mk::trig::tan<F>(angle));
      const F jacobian = constant_pi<F> / (c * c);
      for ( usize component = 0; component < M; ++component ) value.data[component] *= jacobian;
      return value;
    };
    return __impl_quad_vec::finite<M, F>(g, F(0), F(1), transformed, workspace, norm);
  }
  quad_vec_result<M, F> result{};
  result.status = quad_status::invalid_input;
  return result;
}

template<usize M, ieee754_floating F, typename Fn, usize Capacity>
  requires(M >= 2 && M <= 16)
[[nodiscard]] inline quad_vec_result<M, F>
quad_vec(Fn f, F a, F b, quad_vec_workspace<M, F, Capacity> &workspace, const quad_options<F> &options = {},
         quad_error_norm norm = quad_error_norm::maximum) noexcept
{
  return quad_vec<M, F>(f, a, b, options, workspace, norm);
}

template<usize M, ieee754_floating F, typename Fn, usize Capacity>
  requires(M >= 2 && M <= 16)
[[nodiscard]] inline quad_vec_result<M, F>
quad_vec(Fn f, F a, F b, const quad_options<F> &options, quad_workspace<F, Capacity> &workspace,
         quad_error_norm norm = quad_error_norm::maximum) noexcept
{
  quad_vec_result<M, F> result{};
  F norm_acc = F(0);
  for ( usize component = 0; component < M; ++component ) {
    auto scalar = [&](F x) noexcept -> F { return f(x).data[component]; };
    quad_result<F> r = quad<F>(scalar, a, b, options, workspace);
    result.value.data[component] = r.value;
    result.abs_err.data[component] = r.abs_err;
    result.n_evals += r.n_evals;
    if ( r.n_intervals > result.n_intervals ) result.n_intervals = r.n_intervals;
    if ( r.status != quad_status::ok && result.status == quad_status::ok ) result.status = r.status;
    if ( norm == quad_error_norm::maximum ) {
      if ( r.abs_err > norm_acc ) norm_acc = r.abs_err;
    } else {
      norm_acc += r.abs_err * r.abs_err;
    }
  }
  result.error_norm = norm == quad_error_norm::maximum ? norm_acc : mk::pow_ns::sqrt<F>(norm_acc);
  return result;
}

template<ieee754_floating F, callable_real_batch<F> Fn, usize Capacity>
[[nodiscard]] inline quad_result<F>
quad_batch(Fn f, F a, F b, const quad_options<F> &options, quad_workspace<F, Capacity> &workspace) noexcept
{
  if ( ieee::inf_sign<F>(a) == 0 && ieee::inf_sign<F>(b) == 0 ) {
    if ( b < a ) {
      quad_result<F> result = __impl_quad::finite<F, Fn, Capacity, true>(f, b, a, options, workspace);
      result.value = -result.value;
      return result;
    }
    return __impl_quad::finite<F, Fn, Capacity, true>(f, a, b, options, workspace);
  }
  auto scalar = [&](F x) noexcept -> F {
    F y{};
    f(&x, &y, 1);
    return y;
  };
  return quad<F>(scalar, a, b, options, workspace);
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
