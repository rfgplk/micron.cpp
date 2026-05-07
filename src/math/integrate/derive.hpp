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

template <ieee754_floating F>
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

template <ieee754_floating F>
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

template <ieee754_floating F, callable_real<F> Fn>
[[nodiscard, gnu::always_inline]] inline constexpr F
forward(Fn f, F x, F h) noexcept
{
  return (f(x + h) - f(x)) / h;
}

template <ieee754_floating F, callable_real<F> Fn>
[[nodiscard, gnu::always_inline]] inline constexpr F
backward(Fn f, F x, F h) noexcept
{
  return (f(x) - f(x - h)) / h;
}

template <ieee754_floating F, callable_real<F> Fn>
[[nodiscard, gnu::always_inline]] inline constexpr F
central(Fn f, F x, F h) noexcept
{
  return (f(x + h) - f(x - h)) / (F(2) * h);
}

// f'(x) ≈ (-f(x+2h) + 8 f(x+h) - 8 f(x-h) + f(x-2h)) / (12 h)
template <ieee754_floating F, callable_real<F> Fn>
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

template <ieee754_floating F, callable_real<F> Fn>
[[nodiscard]] inline F
adaptive(Fn f, F x, F h0 = F(0), F tol = F(0), usize max_levels = 12) noexcept
{
  if ( h0 == F(0) ) h0 = h_central<F>(x);
  if ( tol == F(0) ) tol = math::default_eps<F>() * F(100);
  auto step = [&](F h) noexcept -> F { return central<F>(f, x, h); };
  return richardson::extrapolate<F>(step, h0, F(2), 2, max_levels, tol);
}

template <usize Order, ieee754_floating F, callable_real<F> Fn>
  requires(Order >= 1 and Order <= 4)
[[nodiscard]] inline F
nth(Fn f, F x, F h) noexcept
{
  if constexpr ( Order == 1 ) {
    return central<F>(f, x, h);
  } else if constexpr ( Order == 2 ) {
    // f''(x) ≈ (f(x+h) - 2 f(x) + f(x-h)) / h**2
    return (f(x + h) - F(2) * f(x) + f(x - h)) / (h * h);
  } else if constexpr ( Order == 3 ) {
    // (-f(x-2h) + 2 f(x-h) - 2 f(x+h) + f(x+2h)) / (2 h**3)
    const F h2 = F(2) * h;
    return (-f(x - h2) + F(2) * f(x - h) - F(2) * f(x + h) + f(x + h2)) / (F(2) * h * h * h);
  } else {     // Order == 4
    // (f(x-2h) - 4 f(x-h) + 6 f(x) - 4 f(x+h) + f(x+2h)) / h**4
    const F h2 = F(2) * h;
    return (f(x - h2) - F(4) * f(x - h) + F(6) * f(x) - F(4) * f(x + h) + f(x + h2)) / (h * h * h * h);
  }
}

template <usize D, ieee754_floating F, callable_real_d<F, D> Fn>
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

template <usize D, usize R, ieee754_floating F, typename Fn>
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

// (f(x+he_i+he_j) - f(x+he_i-he_j) - f(x-he_i+he_j) + f(x-he_i-he_j)) / (4 h**2)
template <usize D, ieee754_floating F, callable_real_d<F, D> Fn>
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

template <ieee754_floating F>
inline void
diff(const F *y, F *dy, usize n, F dx) noexcept
{
  if ( n == 0 ) return;
  if ( n == 1 ) {
    dy[0] = F(0);
    return;
  }
  // forward at left edge
  dy[0] = (y[1] - y[0]) / dx;
  // central for interior
  for ( usize i = 1; i + 1 < n; ++i ) dy[i] = (y[i + 1] - y[i - 1]) / (F(2) * dx);
  // backward at right edge
  dy[n - 1] = (y[n - 1] - y[n - 2]) / dx;
}

// non-uniform spacing form
template <ieee754_floating F>
inline void
diff(const F *xs, const F *ys, F *dy, usize n) noexcept
{
  if ( n == 0 ) return;
  if ( n == 1 ) {
    dy[0] = F(0);
    return;
  }
  dy[0] = (ys[1] - ys[0]) / (xs[1] - xs[0]);
  for ( usize i = 1; i + 1 < n; ++i ) {
    const F h_l = xs[i] - xs[i - 1];
    const F h_r = xs[i + 1] - xs[i];
    const F h_l2 = h_l * h_l;
    const F h_r2 = h_r * h_r;
    dy[i] = (-h_r2 * ys[i - 1] + (h_r2 - h_l2) * ys[i] + h_l2 * ys[i + 1]) / (h_l * h_r * (h_l + h_r));
  }
  dy[n - 1] = (ys[n - 1] - ys[n - 2]) / (xs[n - 1] - xs[n - 2]);
}

template <ieee754_floating F>
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

};     // namespace derive
};     // namespace integrate
};     // namespace math
};     // namespace micron
