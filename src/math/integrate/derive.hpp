//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// finite difference derivatives
//
// -> diff(y, dy, n, dx)
// -> diff(xs, ys, dy, n)
// -> diff2(xs, ys, d2y, n)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../constants.hpp"
#include "../ieee.hpp"
#include "../matrix/mat.hpp"
#include "../mk.hpp"
#include "../quants/vec.hpp"
#include "bits/kernels.hpp"
#include "concepts.hpp"
#include "richardson.hpp"

namespace micron
{
namespace math
{
namespace integrate
{
namespace derive
{

enum class derivative_status : u32 { ok = 0, max_levels = 1, non_finite = 2, invalid_input = 3 };

template<ieee754_floating F> struct derivative_result {
  F value{ 0 };
  F abs_err{ 0 };
  F step{ 0 };
  usize n_evals{ 0 };
  usize levels{ 0 };
  derivative_status status{ derivative_status::ok };
};

template<ieee754_floating F> using adaptive_result = derivative_result<F>;

template<ieee754_floating F, usize MaxPoints = 25, usize MaxOrder = 12> struct fornberg_workspace {
  static_assert(MaxPoints > 0 && MaxOrder > 0, "Fornberg workspace dimensions must be non-zero");
  F coefficients[MaxPoints][MaxOrder + 1]{};
};

template<usize D, ieee754_floating F, usize Capacity> struct derivative_batch_workspace {
  static_assert(D >= 1 && D <= 16 && Capacity > 0, "invalid differentiation batch workspace");
  alignas(64) F coordinates[D][Capacity]{};
  alignas(64) F values[Capacity]{};
  const F *coordinate_views[D]{};

  static constexpr usize capacity = Capacity;
};

template<usize D, usize R, ieee754_floating F, usize Capacity> struct jacobian_batch_workspace {
  static_assert(D >= 1 && D <= 16 && R >= 1 && R <= 16 && Capacity > 0, "invalid Jacobian batch workspace");
  alignas(64) F coordinates[D][Capacity]{};
  alignas(64) F values[R][Capacity]{};
  const F *coordinate_views[D]{};
  F *value_views[R]{};

  static constexpr usize capacity = Capacity;
};

template<ieee754_floating F, usize MaxPoints, usize MaxOrder>
[[nodiscard]] inline bool
fornberg_weights(const F *nodes, usize count, F x0, usize derivative, F *weights,
                 fornberg_workspace<F, MaxPoints, MaxOrder> &workspace) noexcept
{
  if ( count == 0 || count > MaxPoints || derivative > MaxOrder || derivative >= count ) return false;
  for ( usize i = 0; i < count; ++i )
    for ( usize order = 0; order <= derivative; ++order ) workspace.coefficients[i][order] = F(0);
  workspace.coefficients[0][0] = F(1);
  F c1 = F(1);
  F c4 = nodes[0] - x0;
  for ( usize i = 1; i < count; ++i ) {
    const usize highest = i < derivative ? i : derivative;
    F c2 = F(1);
    const F c5 = c4;
    c4 = nodes[i] - x0;
    for ( usize j = 0; j < i; ++j ) {
      const F c3 = nodes[i] - nodes[j];
      if ( c3 == F(0) ) return false;
      c2 *= c3;
      if ( j + 1 == i ) {
        for ( usize order = highest; order > 0; --order )
          workspace.coefficients[i][order]
              = c1 * (F(order) * workspace.coefficients[i - 1][order - 1] - c5 * workspace.coefficients[i - 1][order]) / c2;
        workspace.coefficients[i][0] = -c1 * c5 * workspace.coefficients[i - 1][0] / c2;
      }
      for ( usize order = highest; order > 0; --order )
        workspace.coefficients[j][order] = (c4 * workspace.coefficients[j][order] - F(order) * workspace.coefficients[j][order - 1]) / c3;
      workspace.coefficients[j][0] = c4 * workspace.coefficients[j][0] / c3;
    }
    c1 = c2;
  }
  for ( usize i = 0; i < count; ++i ) weights[i] = workspace.coefficients[i][derivative];
  return true;
}

template<usize Derivative, usize Points, ieee754_floating F, callable_real<F> Fn>
  requires(Derivative >= 1 && Derivative <= 12 && Points > Derivative && Points <= 25 && (Points % 2 == 1))
[[nodiscard]] inline F
central_nth(Fn f, F x, F h) noexcept
{
  F nodes[Points]{};
  F weights[Points]{};
  constexpr usize radius = Points / 2;
  for ( usize i = 0; i < Points; ++i ) nodes[i] = F(i) - F(radius);
  fornberg_workspace<F, Points, Derivative> workspace{};
  (void)fornberg_weights(nodes, Points, F(0), Derivative, weights, workspace);
  F sum = F(0);
  for ( usize i = 0; i < Points; ++i ) sum += weights[i] * f(math::fma<F>(nodes[i], h, x));
  F scale = F(1);
  for ( usize order = 0; order < Derivative; ++order ) scale *= h;
  return sum / scale;
}

template<usize Derivative, usize Points = Derivative + 4, ieee754_floating F, callable_real<F> Fn>
  requires(Derivative >= 1 && Derivative <= 12 && Points > Derivative && Points <= 25)
[[nodiscard]] inline F
forward_nth(Fn f, F x, F h) noexcept
{
  F nodes[Points]{};
  F weights[Points]{};
  for ( usize i = 0; i < Points; ++i ) nodes[i] = F(i);
  fornberg_workspace<F, Points, Derivative> workspace{};
  (void)fornberg_weights(nodes, Points, F(0), Derivative, weights, workspace);
  F sum = F(0);
  for ( usize i = 0; i < Points; ++i ) sum += weights[i] * f(math::fma<F>(nodes[i], h, x));
  F scale = F(1);
  for ( usize order = 0; order < Derivative; ++order ) scale *= h;
  return sum / scale;
}

template<usize Derivative, usize Points = Derivative + 4, ieee754_floating F, callable_real<F> Fn>
  requires(Derivative >= 1 && Derivative <= 12 && Points > Derivative && Points <= 25)
[[nodiscard]] inline F
backward_nth(Fn f, F x, F h) noexcept
{
  F nodes[Points]{};
  F weights[Points]{};
  for ( usize i = 0; i < Points; ++i ) nodes[i] = -F(i);
  fornberg_workspace<F, Points, Derivative> workspace{};
  (void)fornberg_weights(nodes, Points, F(0), Derivative, weights, workspace);
  F sum = F(0);
  for ( usize i = 0; i < Points; ++i ) sum += weights[i] * f(math::fma<F>(nodes[i], h, x));
  F scale = F(1);
  for ( usize order = 0; order < Derivative; ++order ) scale *= h;
  return sum / scale;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
h_central(F x = F(0)) noexcept
{
  const F ax = mk::manip::fabs<F>(x);
  const F scale = ax > F(1) ? ax : F(1);
  if constexpr ( sizeof(F) == sizeof(float) )
    return F(1e-2L) * scale;
  else if constexpr ( sizeof(F) == sizeof(double) )
    return F(1e-5L) * scale;
  else
    return F(1e-6L) * scale;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
h_forward(F x = F(0)) noexcept
{
  const F ax = mk::manip::fabs<F>(x);
  const F scale = ax > F(1) ? ax : F(1);
  if constexpr ( sizeof(F) == sizeof(float) )
    return F(1e-3L) * scale;
  else if constexpr ( sizeof(F) == sizeof(double) )
    return F(1e-7L) * scale;
  else
    return F(1e-9L) * scale;
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard, gnu::always_inline]] inline constexpr F
forward(Fn f, F x, F h) noexcept
{
  return (f(x + h) - f(x)) / h;
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard, gnu::always_inline]] inline constexpr F
backward(Fn f, F x, F h) noexcept
{
  return (f(x) - f(x - h)) / h;
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard, gnu::always_inline]] inline constexpr F
central(Fn f, F x, F h) noexcept
{
  return (f(x + h) - f(x - h)) / (F(2) * h);
}

// f'(x) ≈ (-f(x+2h) + 8 f(x+h) - 8 f(x-h) + f(x-2h)) / (12 h)
template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard, gnu::always_inline]] inline constexpr F
central4(Fn f, F x, F h) noexcept
{
  const F h2 = F(2) * h;
  const F p2 = f(x + h2);
  const F p1 = f(x + h);
  const F m1 = f(x - h);
  const F m2 = f(x - h2);
  return (-p2 + F(8) * p1 - F(8) * m1 + m2) / (F(12) * h);
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline F
adaptive(Fn f, F x, F h0 = F(0), F tol = F(0), usize max_levels = 12) noexcept
{
  if ( h0 == F(0) ) h0 = h_central<F>(x);
  if ( tol == F(0) ) tol = math::default_eps<F>() * F(100);
  auto step = [&](F h) noexcept -> F { return central<F>(f, x, h); };
  return richardson::extrapolate<F>(step, h0, F(2), 2, max_levels, tol);
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline derivative_result<F>
adaptive_diagnostic(Fn f, F x, F h0 = F(0), F tol = F(0), usize max_levels = 12) noexcept
{
  derivative_result<F> result{};
  if ( h0 == F(0) ) h0 = h_central<F>(x);
  if ( tol == F(0) ) tol = math::default_eps<F>() * F(100);
  if ( h0 <= F(0) || tol < F(0) || max_levels == 0 ) {
    result.status = derivative_status::invalid_input;
    return result;
  }
  F h = h0;
  F previous = central4<F>(f, x, h);
  result.n_evals = 4;
  for ( usize level = 1; level < max_levels; ++level ) {
    h *= F(0.5);
    const F current = central4<F>(f, x, h);
    result.n_evals += 4;
    result.value = current;
    result.abs_err = mk::manip::fabs<F>(current - previous) / F(15);
    result.step = h;
    result.levels = level + 1;
    if ( !ieee::is_finite<F>(current) ) {
      result.status = derivative_status::non_finite;
      return result;
    }
    if ( result.abs_err <= tol ) return result;
    previous = current;
  }
  result.status = derivative_status::max_levels;
  return result;
}

template<ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline derivative_result<F>
adaptive_ex(Fn f, F x, F h0 = F(0), F tol = F(0), usize max_levels = 12) noexcept
{
  return adaptive_diagnostic<F>(f, x, h0, tol, max_levels);
}

template<usize Order, ieee754_floating F, callable_real<F> Fn>
  requires(Order >= 1 and Order <= 12)
[[nodiscard]] inline F
nth(Fn f, F x, F h) noexcept
{
  constexpr usize points = Order + (Order % 2 == 0 ? 5 : 4);
  return central_nth<Order, points, F>(f, x, h);
}

template<usize Derivative, usize Points, ieee754_floating F, callable_real_batch<F> Fn>
  requires(Derivative >= 1 && Derivative <= 12 && Points > Derivative && Points <= 25 && (Points % 2 == 1))
[[nodiscard]] inline F
central_nth_batch(Fn f, F x, F h) noexcept
{
  F offsets[Points]{};
  F nodes[Points]{};
  F values[Points]{};
  F weights[Points]{};
  constexpr usize radius = Points / 2;
  for ( usize i = 0; i < Points; ++i ) {
    offsets[i] = F(i) - F(radius);
    nodes[i] = math::fma<F>(offsets[i], h, x);
  }
  fornberg_workspace<F, Points, Derivative> workspace{};
  (void)fornberg_weights(offsets, Points, F(0), Derivative, weights, workspace);
  f(nodes, values, Points);
  const F sum = __integrate_arch::weighted_sum_fast(weights, values, Points);
  F scale = F(1);
  for ( usize order = 0; order < Derivative; ++order ) scale *= h;
  return sum / scale;
}

template<usize D, ieee754_floating F, callable_real_d<F, D> Fn>
  requires(D >= 2 and D <= 16)
[[nodiscard]] inline vec<F, D>
gradient(Fn f, const F (&x)[D], F h) noexcept
{
  vec<F, D> g{};
  F xp[D];
  for ( usize d = 0; d < D; ++d ) xp[d] = x[d];
  for ( usize d = 0; d < D; ++d ) {
    xp[d] = x[d] + h;
    const F fph = f(xp);
    xp[d] = x[d] - h;
    const F fmh = f(xp);
    xp[d] = x[d];
    g.data[d] = (fph - fmh) / (F(2) * h);
  }
  return g;
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, usize Capacity>
  requires(D >= 1 && D <= 16 && Capacity >= 2 * D)
[[nodiscard]] inline vec<F, D>
gradient_batch(Fn f, const F (&x)[D], F h, derivative_batch_workspace<D, F, Capacity> &workspace) noexcept
{
  vec<F, D> result{};
  for ( usize d = 0; d < D; ++d ) workspace.coordinate_views[d] = workspace.coordinates[d];
  for ( usize sample = 0; sample < 2 * D; ++sample )
    for ( usize d = 0; d < D; ++d ) workspace.coordinates[d][sample] = x[d];
  for ( usize d = 0; d < D; ++d ) {
    workspace.coordinates[d][2 * d] += h;
    workspace.coordinates[d][2 * d + 1] -= h;
  }
  f(workspace.coordinate_views, workspace.values, 2 * D);
  const F scale = F(0.5) / h;
  for ( usize d = 0; d < D; ++d ) result.data[d] = (workspace.values[2 * d] - workspace.values[2 * d + 1]) * scale;
  return result;
}

template<usize D, usize R, ieee754_floating F, typename Fn>
  requires(D >= 1 and D <= 16 and R >= 1 and R <= 16)
[[nodiscard]] inline mat<F, R, D>
jacobian(Fn f, const F (&x)[D], F h) noexcept
{
  mat<F, R, D> J{};
  F xp[D];
  for ( usize d = 0; d < D; ++d ) xp[d] = x[d];
  for ( usize d = 0; d < D; ++d ) {
    xp[d] = x[d] + h;
    auto fp = f(xp);
    xp[d] = x[d] - h;
    auto fm = f(xp);
    xp[d] = x[d];
    for ( usize r = 0; r < R; ++r ) J.data[r * D + d] = (fp.data[r] - fm.data[r]) / (F(2) * h);
  }
  return J;
}

template<usize D, usize R, ieee754_floating F, callable_vector_batch_d<F> Fn, usize Capacity>
  requires(D >= 1 && D <= 16 && R >= 1 && R <= 16 && Capacity >= 2 * D)
[[nodiscard]] inline mat<F, R, D>
jacobian_batch(Fn f, const F (&x)[D], F h, jacobian_batch_workspace<D, R, F, Capacity> &workspace) noexcept
{
  mat<F, R, D> result{};
  for ( usize d = 0; d < D; ++d ) workspace.coordinate_views[d] = workspace.coordinates[d];
  for ( usize r = 0; r < R; ++r ) workspace.value_views[r] = workspace.values[r];
  for ( usize sample = 0; sample < 2 * D; ++sample )
    for ( usize d = 0; d < D; ++d ) workspace.coordinates[d][sample] = x[d];
  for ( usize d = 0; d < D; ++d ) {
    workspace.coordinates[d][2 * d] += h;
    workspace.coordinates[d][2 * d + 1] -= h;
  }
  f(workspace.coordinate_views, workspace.value_views, 2 * D);
  const F scale = F(0.5) / h;
  for ( usize r = 0; r < R; ++r )
    for ( usize d = 0; d < D; ++d ) result.data[r * D + d] = (workspace.values[r][2 * d] - workspace.values[r][2 * d + 1]) * scale;
  return result;
}

// (f(x+he_i+he_j) - f(x+he_i-he_j) - f(x-he_i+he_j) + f(x-he_i-he_j)) / (4 h**2)
template<usize D, ieee754_floating F, callable_real_d<F, D> Fn>
  requires(D >= 1 and D <= 16)
[[nodiscard]] inline mat<F, D, D>
hessian(Fn f, const F (&x)[D], F h) noexcept
{
  mat<F, D, D> H{};
  F xp[D];
  for ( usize d = 0; d < D; ++d ) xp[d] = x[d];
  const F f0 = f(x);
  const F h2 = h * h;

  // diagonal first
  for ( usize i = 0; i < D; ++i ) {
    xp[i] = x[i] + h;
    const F fp = f(xp);
    xp[i] = x[i] - h;
    const F fm = f(xp);
    xp[i] = x[i];
    H.data[i * D + i] = (fp - F(2) * f0 + fm) / h2;
  }
  // off-diagonal
  for ( usize i = 0; i < D; ++i ) {
    for ( usize j = i + 1; j < D; ++j ) {
      xp[i] = x[i] + h;
      xp[j] = x[j] + h;
      const F pp = f(xp);
      xp[j] = x[j] - h;
      const F pm = f(xp);
      xp[i] = x[i] - h;
      xp[j] = x[j] + h;
      const F mp = f(xp);
      xp[j] = x[j] - h;
      const F mm = f(xp);
      xp[i] = x[i];
      xp[j] = x[j];
      const F val = (pp - pm - mp + mm) / (F(4) * h2);
      H.data[i * D + j] = val;
      H.data[j * D + i] = val;
    }
  }
  return H;
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, usize Capacity>
  requires(D >= 1 && D <= 16 && Capacity >= 1 + 2 * D * D)
[[nodiscard]] inline mat<F, D, D>
hessian_batch(Fn f, const F (&x)[D], F h, derivative_batch_workspace<D, F, Capacity> &workspace) noexcept
{
  mat<F, D, D> result{};
  constexpr usize count = 1 + 2 * D * D;
  for ( usize d = 0; d < D; ++d ) workspace.coordinate_views[d] = workspace.coordinates[d];
  for ( usize sample = 0; sample < count; ++sample )
    for ( usize d = 0; d < D; ++d ) workspace.coordinates[d][sample] = x[d];

  usize sample = 1;
  for ( usize d = 0; d < D; ++d ) {
    workspace.coordinates[d][sample++] += h;
    workspace.coordinates[d][sample++] -= h;
  }
  for ( usize i = 0; i < D; ++i ) {
    for ( usize j = i + 1; j < D; ++j ) {
      workspace.coordinates[i][sample] += h;
      workspace.coordinates[j][sample++] += h;
      workspace.coordinates[i][sample] += h;
      workspace.coordinates[j][sample++] -= h;
      workspace.coordinates[i][sample] -= h;
      workspace.coordinates[j][sample++] += h;
      workspace.coordinates[i][sample] -= h;
      workspace.coordinates[j][sample++] -= h;
    }
  }
  f(workspace.coordinate_views, workspace.values, count);
  const F h2 = h * h;
  sample = 1;
  for ( usize d = 0; d < D; ++d ) {
    result.data[d * D + d] = (workspace.values[sample] - F(2) * workspace.values[0] + workspace.values[sample + 1]) / h2;
    sample += 2;
  }
  for ( usize i = 0; i < D; ++i ) {
    for ( usize j = i + 1; j < D; ++j ) {
      const F value
          = (workspace.values[sample] - workspace.values[sample + 1] - workspace.values[sample + 2] + workspace.values[sample + 3])
            / (F(4) * h2);
      result.data[i * D + j] = value;
      result.data[j * D + i] = value;
      sample += 4;
    }
  }
  return result;
}

template<ieee754_floating F>
inline void
diff(const F *y, F *dy, usize n, F dx) noexcept
{
  if ( y != dy ) {
    __integrate_arch::diff_uniform(y, dy, n, dx);
    return;
  }
  if ( n == 0 ) return;
  if ( n == 1 ) {
    dy[0] = F(0);
    return;
  }
  F previous = y[0];
  F current = y[1];
  dy[0] = (current - previous) / dx;
  for ( usize i = 1; i + 1 < n; ++i ) {
    const F next = y[i + 1];
    dy[i] = (next - previous) / (F(2) * dx);
    previous = current;
    current = next;
  }
  dy[n - 1] = (current - previous) / dx;
}

// non-uniform spacing form
template<ieee754_floating F>
inline void
diff(const F *xs, const F *ys, F *dy, usize n) noexcept
{
  if ( n == 0 ) return;
  if ( n == 1 ) {
    dy[0] = F(0);
    return;
  }
  F y_previous = ys[0];
  F y_current = ys[1];
  dy[0] = (y_current - y_previous) / (xs[1] - xs[0]);
  for ( usize i = 1; i + 1 < n; ++i ) {
    const F y_next = ys[i + 1];
    const F h_l = xs[i] - xs[i - 1];
    const F h_r = xs[i + 1] - xs[i];
    const F h_l2 = h_l * h_l;
    const F h_r2 = h_r * h_r;
    dy[i] = (-h_r2 * y_previous + (h_r2 - h_l2) * y_current + h_l2 * y_next) / (h_l * h_r * (h_l + h_r));
    y_previous = y_current;
    y_current = y_next;
  }
  dy[n - 1] = (y_current - y_previous) / (xs[n - 1] - xs[n - 2]);
}

template<ieee754_floating F>
inline void
gradient_irregular(const F *xs, const F *ys, F *gradient_out, usize n) noexcept
{
  diff<F>(xs, ys, gradient_out, n);
}

template<ieee754_floating F>
inline void
diff2(const F *xs, const F *ys, F *d2y, usize n) noexcept
{
  if ( n == 0 ) return;
  if ( n < 3 ) {
    for ( usize i = 0; i < n; ++i ) d2y[i] = F(0);
    return;
  }
  {
    const F h0 = xs[1] - xs[0];
    const F h1 = xs[2] - xs[1];
    d2y[0] = F(2) / (h0 + h1) * ((ys[2] - ys[1]) / h1 - (ys[1] - ys[0]) / h0);
  }
  for ( usize i = 1; i + 1 < n; ++i ) {
    const F h_l = xs[i] - xs[i - 1];
    const F h_r = xs[i + 1] - xs[i];
    d2y[i] = F(2) / (h_l + h_r) * ((ys[i + 1] - ys[i]) / h_r - (ys[i] - ys[i - 1]) / h_l);
  }
  {
    const F h_lm1 = xs[n - 2] - xs[n - 3];
    const F h_l = xs[n - 1] - xs[n - 2];
    d2y[n - 1] = F(2) / (h_lm1 + h_l) * ((ys[n - 1] - ys[n - 2]) / h_l - (ys[n - 2] - ys[n - 3]) / h_lm1);
  }
}

};      // namespace derive
};      // namespace integrate
};      // namespace math
};      // namespace micron
