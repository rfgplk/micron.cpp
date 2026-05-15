//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cubic smoothing spline (Reinsch); greedy adaptive-knot interpolation

#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../linalg/banded.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "bits/impl.hpp"
#include "cubic_1d.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace splines
{

namespace __impl_smoothing
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reinsch_workspace
//
//  h         h_i = xs[i+1] - xs[i]
//  r_diag    diagonal of R
//  r_d1      super diagonal of R
//  m_diag    diagonal of Q W^{-1} Q^T
//  m_d1      offset-1 of Q W^{-1} Q^T
//  m_d2      offset-2 of Q W^{-1} Q^T
//  rhs0      Q y
//  c_diag    diagonal of A = R + \ M (per \)
//  c_d1      offset-1 of A
//  c_d2      offset-2 of A
//  gamma     interior second derivatives γ
//  z_diag    Z = A^{-1} diagonal
//  z_d1      Z offset-1
//  z_d2      Z offset-2
//  y_hat     smoothed values
//  q_t_g     (Q^T y) projected back to n-vector

template<ieee754_floating F> struct reinsch_workspace {
  vector<F> h;
  vector<F> r_diag, r_d1;
  vector<F> m_diag, m_d1, m_d2;
  vector<F> rhs0;
  vector<F> c_diag, c_d1, c_d2;
  vector<F> gamma;
  vector<F> z_diag, z_d1, z_d2;
  vector<F> y_hat;
  vector<F> q_t_g;
};

template<ieee754_floating F>
inline void
reinsch_precompute(reinsch_workspace<F> &ws, const F *__restrict__ xs, const F *__restrict__ ys, const F *w, bool uniform_w,
                   usize n) noexcept
{
  ws.h.reserve(n - 1);
  ws.h.set_size(0);
  for ( usize i = 0; i + 1 < n; ++i ) ws.h.emplace_back(xs[i + 1] - xs[i]);

  const usize m = n - 2;
  auto fill_zero = [&](vector<F> &v, usize sz) {
    v.reserve(sz);
    v.set_size(0);
    for ( usize i = 0; i < sz; ++i ) v.emplace_back(F(0));
  };
  fill_zero(ws.r_diag, m);
  fill_zero(ws.r_d1, m == 0 ? 0 : m - 1);
  fill_zero(ws.m_diag, m);
  fill_zero(ws.m_d1, m == 0 ? 0 : m - 1);
  fill_zero(ws.m_d2, m < 2 ? 0 : m - 2);
  fill_zero(ws.rhs0, m);
  fill_zero(ws.c_diag, m);
  fill_zero(ws.c_d1, m == 0 ? 0 : m - 1);
  fill_zero(ws.c_d2, m < 2 ? 0 : m - 2);
  fill_zero(ws.gamma, m);
  fill_zero(ws.z_diag, m);
  fill_zero(ws.z_d1, m == 0 ? 0 : m - 1);
  fill_zero(ws.z_d2, m < 2 ? 0 : m - 2);
  fill_zero(ws.y_hat, n);
  fill_zero(ws.q_t_g, n);

  const F third = F(1) / F(3);
  const F sixth = F(1) / F(6);
  auto invw = [&](usize k) -> F { return uniform_w ? F(1) : (F(1) / w[k]); };

  const F *__restrict__ h = ws.h.data();
  for ( usize i = 0; i + 2 < n; ++i ) {
    const F hi = h[i];
    const F hi1 = h[i + 1];
    const F qi = F(1) / hi;
    const F qi1 = F(1) / hi1;
    const F qm = qi + qi1;

    ws.r_diag[i] = (hi + hi1) * third;
    ws.m_diag[i] = qi * qi * invw(i) + qm * qm * invw(i + 1) + qi1 * qi1 * invw(i + 2);

    if ( i + 3 < n ) {
      ws.r_d1[i] = hi1 * sixth;
      const F qj = qi1;
      const F qjm = qi1 + F(1) / h[i + 2];
      ws.m_d1[i] = -qm * qj * invw(i + 1) + qj * (-qjm) * invw(i + 2);
    }
    if ( i + 4 < n ) {
      ws.m_d2[i] = qi1 * (F(1) / h[i + 2]) * invw(i + 2);
    }
    ws.rhs0[i] = qi * ys[i] - qm * ys[i + 1] + qi1 * ys[i + 2];
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reinsch_solve_one_lambda

template<ieee754_floating F>
[[nodiscard]] inline bool
reinsch_solve_one_lambda(reinsch_workspace<F> &ws, const F *__restrict__ xs, const F *__restrict__ ys, const F *w, bool uniform_w, usize n,
                         F lambda, bool need_trace, F *out_rss, F *out_tr) noexcept
{
  const usize m = n - 2;

  for ( usize i = 0; i < m; ++i ) ws.c_diag[i] = ws.r_diag[i] + lambda * ws.m_diag[i];
  if ( m >= 2 ) {
    for ( usize i = 0; i + 1 < m; ++i ) ws.c_d1[i] = ws.r_d1[i] + lambda * ws.m_d1[i];
  }
  if ( m >= 3 ) {
    for ( usize i = 0; i + 2 < m; ++i ) ws.c_d2[i] = lambda * ws.m_d2[i];
  }

  if ( !linalg::pent_spd_factor<F>(ws.c_d2.data(), ws.c_d1.data(), ws.c_diag.data(), m) ) return false;

  for ( usize i = 0; i < m; ++i ) ws.gamma[i] = ws.rhs0[i];
  linalg::pent_spd_solve_factored<F>(ws.c_d2.data(), ws.c_d1.data(), ws.c_diag.data(), ws.gamma.data(), m);

  const F *__restrict__ h = ws.h.data();
  for ( usize k = 0; k < n; ++k ) {
    F qty = F(0);
    if ( k >= 2 && k - 2 < m ) qty += ws.gamma[k - 2] * (F(1) / h[k - 1]);
    if ( k >= 1 && k - 1 < m ) qty += ws.gamma[k - 1] * (-(F(1) / h[k - 1] + F(1) / h[k]));
    if ( k < m ) qty += ws.gamma[k] * (F(1) / h[k]);
    ws.q_t_g[k] = qty;
    const F invw = uniform_w ? F(1) : (F(1) / w[k]);
    ws.y_hat[k] = ys[k] - lambda * invw * qty;
  }

  F ssr = F(0);
  for ( usize k = 0; k < n; ++k ) {
    const F r = ys[k] - ws.y_hat[k];
    ssr += r * r;
  }
  if ( out_rss ) *out_rss = ssr;

  if ( need_trace ) {
    linalg::pent_spd_takahashi<F>(ws.c_d2.data(), ws.c_d1.data(), ws.c_diag.data(), m, ws.z_diag.data(), ws.z_d1.data(), ws.z_d2.data());
    F t = F(0);
    for ( usize i = 0; i < m; ++i ) t += ws.z_diag[i] * ws.m_diag[i];
    if ( m >= 2 ) {
      for ( usize i = 0; i + 1 < m; ++i ) t += F(2) * ws.z_d1[i] * ws.m_d1[i];
    }
    if ( m >= 3 ) {
      for ( usize i = 0; i + 2 < m; ++i ) t += F(2) * ws.z_d2[i] * ws.m_d2[i];
    }
    if ( out_tr ) *out_tr = t;
  }
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// gcv_score
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
gcv_score(F rss, F lambda, F tr_inv_m, usize n) noexcept
{
  const F denom_term = lambda * tr_inv_m;
  if ( !(denom_term > F(0)) ) return F(1e30);
  return F(n) * rss / (denom_term * denom_term);
}

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
build_smoothing_spline_from_y_hat(const F *xs, const F *y_hat, usize n, build_info<F> *info) noexcept
{
  raw_slice<const F> xs_s(xs, n);
  raw_slice<const F> ys_s(y_hat, n);
  return make_cubic<F>(xs_s, ys_s, bc_kind::natural, F(0), F(0), info);
}

template<ieee754_floating F>
[[nodiscard]] inline F
gcv_search(reinsch_workspace<F> &ws, const F *xs, const F *ys, const F *w, bool uniform_w, usize n, build_info<F> *info) noexcept
{
  const F phi = (math::fsqrt(F(5)) - F(1)) * F(0.5);
  F a = F(-12);
  F b = F(12);
  F c = b - phi * (b - a);
  F d = a + phi * (b - a);

  auto eval_at = [&](F log_lambda) -> F {
    const F lambda = math::mk::exp_ns::exp10<F>(log_lambda);
    F rss = F(0), tr = F(0);
    if ( !reinsch_solve_one_lambda<F>(ws, xs, ys, w, uniform_w, n, lambda, true, &rss, &tr) ) return F(1e30);
    return gcv_score<F>(rss, lambda, tr, n);
  };

  F fc = eval_at(c);
  F fd = eval_at(d);
  usize iter = 0;
  for ( iter = 0; iter < 60; ++iter ) {
    if ( b - a < F(1e-3) ) break;
    if ( fc < fd ) {
      b = d;
      d = c;
      fd = fc;
      c = b - phi * (b - a);
      fc = eval_at(c);
    } else {
      a = c;
      c = d;
      fc = fd;
      d = a + phi * (b - a);
      fd = eval_at(d);
    }
  }
  if ( info ) info->n_iterations = iter + 2;      // +2 for the initial fc, fd
  const F log_lambda_opt = F(0.5) * (a + b);
  return math::mk::exp_ns::exp10<F>(log_lambda_opt);
}

};      // namespace __impl_smoothing

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_smoothing(raw_slice<const F> xs, raw_slice<const F> ys, raw_slice<const F> w, F lambda, build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> s{};
  s.bc = bc_kind::natural;

  if ( xs.size() != ys.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return s;
  }
  if ( xs.size() < 4 ) {
    if ( info ) info->status = build_status::too_few_points;
    return s;
  }
  if ( w.size() != 0 && w.size() != xs.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return s;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(xs.ptr, xs.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return s;
  }
  const usize n = xs.size();
  const bool uniform_w = (w.size() == 0);

  if ( lambda == F(0) ) {
    return make_cubic<F>(xs, ys, bc_kind::natural, F(0), F(0), info);
  }

  __impl_smoothing::reinsch_workspace<F> ws;
  __impl_smoothing::reinsch_precompute<F>(ws, xs.ptr, ys.ptr, w.ptr, uniform_w, n);

  F lam = lambda;
  if ( lambda < F(0) ) lam = __impl_smoothing::gcv_search<F>(ws, xs.ptr, ys.ptr, w.ptr, uniform_w, n, info);

  F rss = F(0);
  if ( !__impl_smoothing::reinsch_solve_one_lambda<F>(ws, xs.ptr, ys.ptr, w.ptr, uniform_w, n, lam, false, &rss, nullptr) ) {
    if ( info ) info->status = build_status::singular_system;
    return s;
  }

  build_info<F> bi{};
  auto out = __impl_smoothing::build_smoothing_spline_from_y_hat<F>(xs.ptr, ws.y_hat.data(), n, &bi);
  if ( info ) {
    info->status = bi.status;
    info->residual = (lambda < F(0)) ? lam : rss;
  }
  return out;
}

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_adaptive_knots(raw_slice<const F> xs, raw_slice<const F> ys, F max_abs_err, usize max_knots, build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> result{};
  result.bc = bc_kind::natural;

  if ( xs.size() != ys.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return result;
  }
  if ( xs.size() < 4 ) {
    if ( info ) info->status = build_status::too_few_points;
    return result;
  }
  if ( !(max_abs_err > F(0)) || max_knots < 4 ) {
    if ( info ) info->status = build_status::invalid_argument;
    return result;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(xs.ptr, xs.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return result;
  }

  const usize n = xs.size();
  if ( max_knots > n ) max_knots = n;

  vector<usize> active(0);
  active.reserve(max_knots);
  active.emplace_back(0);
  active.emplace_back(n / 3);
  active.emplace_back((2 * n) / 3);
  active.emplace_back(n - 1);
  vector<usize> uniq(0);
  uniq.reserve(active.size());
  for ( usize i = 0; i < active.size(); ++i ) {
    if ( uniq.size() == 0 || uniq[uniq.size() - 1] != active[i] ) uniq.emplace_back(active[i]);
  }
  active = uniq;
  if ( active.size() < 4 ) {
    usize k = 1;
    while ( active.size() < 4 && k + 1 < n ) {
      bool present = false;
      for ( usize j = 0; j < active.size(); ++j )
        if ( active[j] == k ) {
          present = true;
          break;
        }
      if ( !present ) {
        usize j = 0;
        while ( j < active.size() && active[j] < k ) ++j;
        active.emplace_back(F(0));
        for ( usize jj = active.size() - 1; jj > j; --jj ) active[jj] = active[jj - 1];
        active[j] = k;
      }
      ++k;
    }
  }

  vector<F> kx(0), ky(0);
  kx.reserve(max_knots);
  ky.reserve(max_knots);

  usize iter = 0;
  while ( true ) {
    ++iter;
    kx.set_size(0);
    ky.set_size(0);
    for ( usize i = 0; i < active.size(); ++i ) {
      kx.emplace_back(xs[active[i]]);
      ky.emplace_back(ys[active[i]]);
    }
    raw_slice<const F> kxs(kx.data(), kx.size());
    raw_slice<const F> kys(ky.data(), ky.size());
    result = make_cubic<F>(kxs, kys, bc_kind::natural, F(0), F(0), nullptr);

    F worst = F(0);
    usize worst_i = 0;
    bool found = false;
    for ( usize i = 0; i < n; ++i ) {
      bool present = false;
      for ( usize j = 0; j < active.size(); ++j )
        if ( active[j] == i ) {
          present = true;
          break;
        }
      if ( present ) continue;
      const F y_pred = evaluate<F>(result, xs[i]);
      const F err = (ys[i] >= y_pred) ? (ys[i] - y_pred) : (y_pred - ys[i]);
      if ( err > worst ) {
        worst = err;
        worst_i = i;
        found = true;
      }
    }

    if ( !found || worst <= max_abs_err || active.size() >= max_knots ) {
      if ( info ) {
        info->status = (worst <= max_abs_err) ? build_status::ok : build_status::max_iter;
        info->n_iterations = iter;
        info->residual = worst;
      }
      return result;
    }

    usize j = 0;
    while ( j < active.size() && active[j] < worst_i ) ++j;
    active.emplace_back(0);
    for ( usize jj = active.size() - 1; jj > j; --jj ) active[jj] = active[jj - 1];
    active[j] = worst_i;
  }
}

};      // namespace splines
};      // namespace math
};      // namespace micron
