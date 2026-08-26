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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Adaptive Genz-Malik degree-7 cubature with an embedded degree-5 estimate

namespace micron
{
namespace math
{
namespace integrate
{

template<ieee754_floating F> struct cubature_options {
  union {
    F abs_tol{ math::default_eps<F>() * F(1000) };
    F atol;
  };

  union {
    F rel_tol{ math::default_eps<F>() * F(1000) };
    F rtol;
  };

  union {
    usize max_evals{ 0 };
    usize max_func_evals;
  };

  union {
    usize max_regions{ 128 };
    usize limit;
  };

  accumulation_policy accumulation{ accumulation_policy::fast };
};

template<ieee754_floating F> struct cubature_result {
  F value{ 0 };
  F abs_err{ 0 };
  usize n_evals{ 0 };
  quad_status status{ quad_status::ok };
  usize n_regions{ 0 };
  usize n_splits{ 0 };
};

template<usize D, ieee754_floating F> struct cubature_region {
  F center[D]{};
  F halfwidth[D]{};
  F value{ 0 };
  F error{ 0 };
  usize split_dimension{ 0 };
};

template<usize D, ieee754_floating F, usize Capacity> struct cubature_workspace {
  static_assert(D >= 2 && D <= 16, "Genz-Malik cubature supports dimensions 2 through 16");
  static_assert(Capacity > 0, "cubature workspace capacity must be non-zero");
  cubature_region<D, F> heap[Capacity]{};
  usize size{ 0 };

  static constexpr usize capacity = Capacity;

  constexpr void
  reset() noexcept
  {
    size = 0;
  }
};

template<usize D, ieee754_floating F, usize Capacity, usize BatchSize = 64> struct cubature_batch_workspace {
  static_assert(D >= 2 && D <= 16, "Genz-Malik cubature supports dimensions 2 through 16");
  static_assert(Capacity > 0 && BatchSize > 0, "cubature workspace capacities must be non-zero");
  cubature_region<D, F> heap[Capacity]{};
  usize size{ 0 };
  alignas(64) F coordinates[D][BatchSize]{};
  alignas(64) F values[BatchSize]{};
  const F *coordinate_views[D]{};

  static constexpr usize capacity = Capacity;
  static constexpr usize batch_size = BatchSize;

  constexpr void
  reset() noexcept
  {
    size = 0;
  }
};

namespace __impl_cubature
{

template<usize D> inline constexpr usize point_count = usize(1) + usize(4) * D + usize(2) * D * (D - 1) + (usize(1) << D);

template<usize D, ieee754_floating F, typename Fn>
[[nodiscard]] inline bool
evaluate(Fn &f, cubature_region<D, F> &region, usize &n_evals) noexcept
{
  constexpr F lambda2 = F(0.358568582800318091990645153907937L);
  constexpr F lambda4 = F(0.948683298050513799599668063329816L);
  constexpr F lambda5 = F(0.688247201611685297721628734293623L);
  constexpr F weight2 = F(980.0L / 6561.0L);
  constexpr F weight4 = F(200.0L / 19683.0L);
  constexpr F weight_e2 = F(245.0L / 486.0L);
  constexpr F weight_e4 = F(25.0L / 729.0L);
  constexpr F ratio = F((9.0L / 70.0L) / (9.0L / 10.0L));

  F point[D]{};
  for ( usize d = 0; d < D; ++d ) point[d] = region.center[d];
  const F center_value = f(point);
  ++n_evals;
  if ( !ieee::is_finite<F>(center_value) ) return false;

  F sum2 = F(0), sum3 = F(0), sum4 = F(0), sum5 = F(0);
  F difference[D]{};
  for ( usize d = 0; d < D; ++d ) {
    const F delta2 = lambda2 * region.halfwidth[d];
    point[d] = region.center[d] - delta2;
    const F m2 = f(point);
    point[d] = region.center[d] + delta2;
    const F p2 = f(point);
    const F delta4 = lambda4 * region.halfwidth[d];
    point[d] = region.center[d] - delta4;
    const F m4 = f(point);
    point[d] = region.center[d] + delta4;
    const F p4 = f(point);
    point[d] = region.center[d];
    n_evals += 4;
    if ( !ieee::is_finite<F>(m2) || !ieee::is_finite<F>(p2) || !ieee::is_finite<F>(m4) || !ieee::is_finite<F>(p4) ) return false;
    sum2 += m2 + p2;
    sum3 += m4 + p4;
    difference[d] = mk::manip::fabs<F>(m2 + p2 - F(2) * center_value - ratio * (m4 + p4 - F(2) * center_value));
  }

  for ( usize i = 0; i + 1 < D; ++i ) {
    for ( usize j = i + 1; j < D; ++j ) {
      const F di = lambda4 * region.halfwidth[i];
      const F dj = lambda4 * region.halfwidth[j];
      for ( usize signs = 0; signs < 4; ++signs ) {
        point[i] = region.center[i] + ((signs & 1) ? di : -di);
        point[j] = region.center[j] + ((signs & 2) ? dj : -dj);
        const F value = f(point);
        ++n_evals;
        if ( !ieee::is_finite<F>(value) ) return false;
        sum4 += value;
      }
      point[i] = region.center[i];
      point[j] = region.center[j];
    }
  }

  for ( usize mask = 0; mask < (usize(1) << D); ++mask ) {
    for ( usize d = 0; d < D; ++d ) {
      const F delta = lambda5 * region.halfwidth[d];
      point[d] = region.center[d] + ((mask & (usize(1) << d)) ? delta : -delta);
    }
    const F value = f(point);
    ++n_evals;
    if ( !ieee::is_finite<F>(value) ) return false;
    sum5 += value;
  }

  F volume = F(1);
  for ( usize d = 0; d < D; ++d ) volume *= F(2) * region.halfwidth[d];
  const F dd = F(D);
  const F weight1 = (F(12824) - F(9120) * dd + F(400) * dd * dd) / F(19683);
  const F weight3 = (F(1820) - F(400) * dd) / F(19683);
  const F weight5 = F(6859) / F(19683) / F(usize(1) << D);
  const F weight_e1 = (F(729) - F(950) * dd + F(50) * dd * dd) / F(729);
  const F weight_e3 = (F(265) - F(100) * dd) / F(1458);
  const F seventh = volume * (weight1 * center_value + weight2 * sum2 + weight3 * sum3 + weight4 * sum4 + weight5 * sum5);
  const F fifth = volume * (weight_e1 * center_value + weight_e2 * sum2 + weight_e3 * sum3 + weight_e4 * sum4);
  region.value = seventh;
  region.error = mk::manip::fabs<F>(seventh - fifth);
  const F floor = F(50) * machine_epsilon<F>() * mk::manip::fabs<F>(seventh);
  if ( region.error < floor ) region.error = floor;

  region.split_dimension = 0;
  for ( usize d = 1; d < D; ++d ) {
    if ( difference[d] > difference[region.split_dimension]
         || (difference[d] == difference[region.split_dimension] && region.halfwidth[d] > region.halfwidth[region.split_dimension]) )
      region.split_dimension = d;
  }
  return true;
}

template<usize D, ieee754_floating F, typename Fn, usize Capacity, usize BatchSize>
[[nodiscard]] inline bool
evaluate_batch(Fn &f, cubature_region<D, F> &region, usize &n_evals,
               cubature_batch_workspace<D, F, Capacity, BatchSize> &workspace) noexcept
{
  constexpr F lambda2 = F(0.358568582800318091990645153907937L);
  constexpr F lambda4 = F(0.948683298050513799599668063329816L);
  constexpr F lambda5 = F(0.688247201611685297721628734293623L);
  constexpr F weight2 = F(980.0L / 6561.0L);
  constexpr F weight4 = F(200.0L / 19683.0L);
  constexpr F weight_e2 = F(245.0L / 486.0L);
  constexpr F weight_e4 = F(25.0L / 729.0L);
  constexpr F ratio = F((9.0L / 70.0L) / (9.0L / 10.0L));
  constexpr usize axis_count = 1 + 4 * D;

  for ( usize d = 0; d < D; ++d ) workspace.coordinate_views[d] = workspace.coordinates[d];
  F point[D]{};
  for ( usize d = 0; d < D; ++d ) point[d] = region.center[d];
  usize queued = 0;
  auto emit = [&](const F(&p)[D]) noexcept {
    for ( usize d = 0; d < D; ++d ) workspace.coordinates[d][queued] = p[d];
    ++queued;
  };
  auto evaluate_queued = [&](F *capture, usize &captured, F &sum) noexcept -> bool {
    if ( queued == 0 ) return true;
    f(workspace.coordinate_views, workspace.values, queued);
    n_evals += queued;
    for ( usize i = 0; i < queued; ++i ) {
      if ( !ieee::is_finite<F>(workspace.values[i]) ) return false;
      if ( capture != nullptr )
        capture[captured++] = workspace.values[i];
      else
        sum += workspace.values[i];
    }
    queued = 0;
    return true;
  };

  F axis_values[axis_count]{};
  usize captured = 0;
  F ignored = F(0);
  emit(point);
  if ( queued == BatchSize && !evaluate_queued(axis_values, captured, ignored) ) return false;
  for ( usize d = 0; d < D; ++d ) {
    const F delta2 = lambda2 * region.halfwidth[d];
    point[d] = region.center[d] - delta2;
    emit(point);
    if ( queued == BatchSize && !evaluate_queued(axis_values, captured, ignored) ) return false;
    point[d] = region.center[d] + delta2;
    emit(point);
    if ( queued == BatchSize && !evaluate_queued(axis_values, captured, ignored) ) return false;
    const F delta4 = lambda4 * region.halfwidth[d];
    point[d] = region.center[d] - delta4;
    emit(point);
    if ( queued == BatchSize && !evaluate_queued(axis_values, captured, ignored) ) return false;
    point[d] = region.center[d] + delta4;
    emit(point);
    if ( queued == BatchSize && !evaluate_queued(axis_values, captured, ignored) ) return false;
    point[d] = region.center[d];
  }
  if ( !evaluate_queued(axis_values, captured, ignored) ) return false;

  const F center_value = axis_values[0];
  F sum2 = F(0), sum3 = F(0), sum4 = F(0), sum5 = F(0);
  F difference[D]{};
  for ( usize d = 0; d < D; ++d ) {
    const usize base = 1 + 4 * d;
    const F m2 = axis_values[base], p2 = axis_values[base + 1];
    const F m4 = axis_values[base + 2], p4 = axis_values[base + 3];
    sum2 += m2 + p2;
    sum3 += m4 + p4;
    difference[d] = mk::manip::fabs<F>(m2 + p2 - F(2) * center_value - ratio * (m4 + p4 - F(2) * center_value));
  }

  for ( usize i = 0; i + 1 < D; ++i ) {
    for ( usize j = i + 1; j < D; ++j ) {
      const F di = lambda4 * region.halfwidth[i];
      const F dj = lambda4 * region.halfwidth[j];
      for ( usize signs = 0; signs < 4; ++signs ) {
        point[i] = region.center[i] + ((signs & 1) ? di : -di);
        point[j] = region.center[j] + ((signs & 2) ? dj : -dj);
        emit(point);
        if ( queued == BatchSize && !evaluate_queued(nullptr, captured, sum4) ) return false;
      }
      point[i] = region.center[i];
      point[j] = region.center[j];
    }
  }
  if ( !evaluate_queued(nullptr, captured, sum4) ) return false;

  for ( usize mask = 0; mask < (usize(1) << D); ++mask ) {
    for ( usize d = 0; d < D; ++d ) {
      const F delta = lambda5 * region.halfwidth[d];
      point[d] = region.center[d] + ((mask & (usize(1) << d)) ? delta : -delta);
    }
    emit(point);
    if ( queued == BatchSize && !evaluate_queued(nullptr, captured, sum5) ) return false;
  }
  if ( !evaluate_queued(nullptr, captured, sum5) ) return false;

  F volume = F(1);
  for ( usize d = 0; d < D; ++d ) volume *= F(2) * region.halfwidth[d];
  const F dd = F(D);
  const F weight1 = (F(12824) - F(9120) * dd + F(400) * dd * dd) / F(19683);
  const F weight3 = (F(1820) - F(400) * dd) / F(19683);
  const F weight5 = F(6859) / F(19683) / F(usize(1) << D);
  const F weight_e1 = (F(729) - F(950) * dd + F(50) * dd * dd) / F(729);
  const F weight_e3 = (F(265) - F(100) * dd) / F(1458);
  const F seventh = volume * (weight1 * center_value + weight2 * sum2 + weight3 * sum3 + weight4 * sum4 + weight5 * sum5);
  const F fifth = volume * (weight_e1 * center_value + weight_e2 * sum2 + weight_e3 * sum3 + weight_e4 * sum4);
  region.value = seventh;
  region.error = mk::manip::fabs<F>(seventh - fifth);
  const F floor = F(50) * machine_epsilon<F>() * mk::manip::fabs<F>(seventh);
  if ( region.error < floor ) region.error = floor;
  region.split_dimension = 0;
  for ( usize d = 1; d < D; ++d ) {
    if ( difference[d] > difference[region.split_dimension]
         || (difference[d] == difference[region.split_dimension] && region.halfwidth[d] > region.halfwidth[region.split_dimension]) )
      region.split_dimension = d;
  }
  return true;
}

template<usize D, ieee754_floating F, usize Capacity>
inline void
push(cubature_workspace<D, F, Capacity> &workspace, const cubature_region<D, F> &region) noexcept
{
  usize i = workspace.size++;
  while ( i != 0 ) {
    const usize parent = (i - 1) >> 1;
    if ( workspace.heap[parent].error >= region.error ) break;
    workspace.heap[i] = workspace.heap[parent];
    i = parent;
  }
  workspace.heap[i] = region;
}

template<usize D, ieee754_floating F, usize Capacity>
[[nodiscard]] inline cubature_region<D, F>
pop(cubature_workspace<D, F, Capacity> &workspace) noexcept
{
  const cubature_region<D, F> result = workspace.heap[0];
  const cubature_region<D, F> tail = workspace.heap[--workspace.size];
  if ( workspace.size == 0 ) return result;
  usize i = 0;
  while ( true ) {
    const usize left = i * 2 + 1;
    if ( left >= workspace.size ) break;
    const usize right = left + 1;
    const usize child = right < workspace.size && workspace.heap[right].error > workspace.heap[left].error ? right : left;
    if ( workspace.heap[child].error <= tail.error ) break;
    workspace.heap[i] = workspace.heap[child];
    i = child;
  }
  workspace.heap[i] = tail;
  return result;
}

template<usize D, ieee754_floating F, usize Capacity, usize BatchSize>
inline void
push(cubature_batch_workspace<D, F, Capacity, BatchSize> &workspace, const cubature_region<D, F> &region) noexcept
{
  usize i = workspace.size++;
  while ( i != 0 ) {
    const usize parent = (i - 1) >> 1;
    if ( workspace.heap[parent].error >= region.error ) break;
    workspace.heap[i] = workspace.heap[parent];
    i = parent;
  }
  workspace.heap[i] = region;
}

template<usize D, ieee754_floating F, usize Capacity, usize BatchSize>
[[nodiscard]] inline cubature_region<D, F>
pop(cubature_batch_workspace<D, F, Capacity, BatchSize> &workspace) noexcept
{
  const cubature_region<D, F> result = workspace.heap[0];
  const cubature_region<D, F> tail = workspace.heap[--workspace.size];
  if ( workspace.size == 0 ) return result;
  usize i = 0;
  while ( true ) {
    const usize left = i * 2 + 1;
    if ( left >= workspace.size ) break;
    const usize right = left + 1;
    const usize child = right < workspace.size && workspace.heap[right].error > workspace.heap[left].error ? right : left;
    if ( workspace.heap[child].error <= tail.error ) break;
    workspace.heap[i] = workspace.heap[child];
    i = child;
  }
  workspace.heap[i] = tail;
  return result;
}

};      // namespace __impl_cubature

template<usize D, ieee754_floating F, callable_real_d<F, D> Fn, usize Capacity>
  requires(D >= 2 && D <= 16)
[[nodiscard]] inline cubature_result<F>
cubature(Fn f, const F (&lower)[D], const F (&upper)[D], const cubature_options<F> &options,
         cubature_workspace<D, F, Capacity> &workspace) noexcept
{
  cubature_result<F> result{};
  workspace.reset();
  if ( options.abs_tol < F(0) || options.rel_tol < F(0) ) {
    result.status = quad_status::invalid_input;
    return result;
  }
  const usize requested_limit = options.max_regions == 0 ? Capacity : options.max_regions;
  const usize region_limit = requested_limit < Capacity ? requested_limit : Capacity;
  constexpr usize eval_cost = __impl_cubature::point_count<D>;
  if ( options.max_evals != 0 && options.max_evals < eval_cost ) {
    result.status = quad_status::max_evaluations;
    return result;
  }

  cubature_region<D, F> initial{};
  F orientation = F(1);
  for ( usize d = 0; d < D; ++d ) {
    F lo = lower[d];
    F hi = upper[d];
    if ( !ieee::is_finite<F>(lo) || !ieee::is_finite<F>(hi) ) {
      result.status = quad_status::invalid_input;
      return result;
    }
    if ( hi < lo ) {
      const F swap = lo;
      lo = hi;
      hi = swap;
      orientation = -orientation;
    }
    if ( lo == hi ) return result;
    initial.center[d] = F(0.5) * (lo + hi);
    initial.halfwidth[d] = F(0.5) * (hi - lo);
  }
  if ( !__impl_cubature::evaluate<D, F>(f, initial, result.n_evals) ) {
    result.status = quad_status::non_finite;
    return result;
  }
  __impl_cubature::push(workspace, initial);
  __impl_accumulate::runtime_sum<F> value_sum{ options.accumulation };
  __impl_accumulate::runtime_sum<F> error_sum{ options.accumulation };
  value_sum.add(initial.value);
  error_sum.add(initial.error);
  result.n_regions = 1;

  while ( true ) {
    const F value = value_sum.get();
    F error = error_sum.get();
    if ( error < F(0) ) error = F(0);
    const F tolerance
        = options.abs_tol > options.rel_tol * mk::manip::fabs<F>(value) ? options.abs_tol : options.rel_tol * mk::manip::fabs<F>(value);
    if ( error <= tolerance ) break;
    if ( workspace.size >= region_limit ) {
      result.status = quad_status::max_intervals;
      break;
    }
    if ( options.max_evals != 0 && result.n_evals + 2 * eval_cost > options.max_evals ) {
      result.status = quad_status::max_evaluations;
      break;
    }
    const cubature_region<D, F> parent = __impl_cubature::pop(workspace);
    cubature_region<D, F> left = parent;
    cubature_region<D, F> right = parent;
    const usize split = parent.split_dimension;
    left.halfwidth[split] *= F(0.5);
    right.halfwidth[split] = left.halfwidth[split];
    left.center[split] -= left.halfwidth[split];
    right.center[split] += right.halfwidth[split];
    if ( !__impl_cubature::evaluate<D, F>(f, left, result.n_evals) || !__impl_cubature::evaluate<D, F>(f, right, result.n_evals) ) {
      __impl_cubature::push(workspace, parent);
      result.status = quad_status::non_finite;
      break;
    }
    value_sum.add(-parent.value);
    value_sum.add(left.value);
    value_sum.add(right.value);
    error_sum.add(-parent.error);
    error_sum.add(left.error);
    error_sum.add(right.error);
    __impl_cubature::push(workspace, left);
    __impl_cubature::push(workspace, right);
    ++result.n_splits;
    result.n_regions = workspace.size;
  }
  result.value = orientation * value_sum.get();
  result.abs_err = error_sum.get();
  if ( result.abs_err < F(0) ) result.abs_err = F(0);
  return result;
}

template<usize D, ieee754_floating F, callable_real_d<F, D> Fn>
  requires(D >= 2 && D <= 16)
[[nodiscard]] inline cubature_result<F>
cubature(Fn f, const F (&lower)[D], const F (&upper)[D], F abs_tol, F rel_tol) noexcept
{
  cubature_options<F> options{};
  options.abs_tol = abs_tol;
  options.rel_tol = rel_tol;
  cubature_workspace<D, F, 128> workspace{};
  return cubature<D, F>(f, lower, upper, options, workspace);
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, usize Capacity>
  requires(D >= 2 && D <= 16)
[[nodiscard]] inline cubature_result<F>
cubature_batch(Fn f, const F (&lower)[D], const F (&upper)[D], const cubature_options<F> &options,
               cubature_workspace<D, F, Capacity> &workspace) noexcept
{
  auto scalar = [&](const F(&point)[D]) noexcept -> F {
    const F *coordinates[D];
    for ( usize d = 0; d < D; ++d ) coordinates[d] = point + d;
    F value{};
    f(coordinates, &value, 1);
    return value;
  };
  return cubature<D, F>(scalar, lower, upper, options, workspace);
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, usize Capacity, usize BatchSize>
  requires(D >= 2 && D <= 16)
[[nodiscard]] inline cubature_result<F>
cubature_batch(Fn f, const F (&lower)[D], const F (&upper)[D], const cubature_options<F> &options,
               cubature_batch_workspace<D, F, Capacity, BatchSize> &workspace) noexcept
{
  cubature_result<F> result{};
  workspace.reset();
  if ( options.abs_tol < F(0) || options.rel_tol < F(0) ) {
    result.status = quad_status::invalid_input;
    return result;
  }
  const usize requested_limit = options.max_regions == 0 ? Capacity : options.max_regions;
  const usize region_limit = requested_limit < Capacity ? requested_limit : Capacity;
  constexpr usize eval_cost = __impl_cubature::point_count<D>;
  if ( options.max_evals != 0 && options.max_evals < eval_cost ) {
    result.status = quad_status::max_evaluations;
    return result;
  }

  cubature_region<D, F> initial{};
  F orientation = F(1);
  for ( usize d = 0; d < D; ++d ) {
    F lo = lower[d];
    F hi = upper[d];
    if ( !ieee::is_finite<F>(lo) || !ieee::is_finite<F>(hi) ) {
      result.status = quad_status::invalid_input;
      return result;
    }
    if ( hi < lo ) {
      const F swap = lo;
      lo = hi;
      hi = swap;
      orientation = -orientation;
    }
    if ( lo == hi ) return result;
    initial.center[d] = F(0.5) * (lo + hi);
    initial.halfwidth[d] = F(0.5) * (hi - lo);
  }
  if ( !__impl_cubature::evaluate_batch<D, F>(f, initial, result.n_evals, workspace) ) {
    result.status = quad_status::non_finite;
    return result;
  }
  __impl_cubature::push(workspace, initial);
  __impl_accumulate::runtime_sum<F> value_sum{ options.accumulation };
  __impl_accumulate::runtime_sum<F> error_sum{ options.accumulation };
  value_sum.add(initial.value);
  error_sum.add(initial.error);
  result.n_regions = 1;

  while ( true ) {
    const F value = value_sum.get();
    F error = error_sum.get();
    if ( error < F(0) ) error = F(0);
    const F tolerance
        = options.abs_tol > options.rel_tol * mk::manip::fabs<F>(value) ? options.abs_tol : options.rel_tol * mk::manip::fabs<F>(value);
    if ( error <= tolerance ) break;
    if ( workspace.size >= region_limit ) {
      result.status = quad_status::max_intervals;
      break;
    }
    if ( options.max_evals != 0 && result.n_evals + 2 * eval_cost > options.max_evals ) {
      result.status = quad_status::max_evaluations;
      break;
    }
    const cubature_region<D, F> parent = __impl_cubature::pop(workspace);
    cubature_region<D, F> left = parent;
    cubature_region<D, F> right = parent;
    const usize split = parent.split_dimension;
    left.halfwidth[split] *= F(0.5);
    right.halfwidth[split] = left.halfwidth[split];
    left.center[split] -= left.halfwidth[split];
    right.center[split] += right.halfwidth[split];
    if ( !__impl_cubature::evaluate_batch<D, F>(f, left, result.n_evals, workspace)
         || !__impl_cubature::evaluate_batch<D, F>(f, right, result.n_evals, workspace) ) {
      __impl_cubature::push(workspace, parent);
      result.status = quad_status::non_finite;
      break;
    }
    value_sum.add(-parent.value);
    value_sum.add(left.value);
    value_sum.add(right.value);
    error_sum.add(-parent.error);
    error_sum.add(left.error);
    error_sum.add(right.error);
    __impl_cubature::push(workspace, left);
    __impl_cubature::push(workspace, right);
    ++result.n_splits;
    result.n_regions = workspace.size;
  }
  result.value = orientation * value_sum.get();
  result.abs_err = error_sum.get();
  if ( result.abs_err < F(0) ) result.abs_err = F(0);
  return result;
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
