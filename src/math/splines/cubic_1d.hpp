//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// one dimensional cubic spline interpolation

#include "../../concepts.hpp"
#include "../../slice.hpp"
#include "../../types.hpp"
#include "../../vector/vector.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../linalg/banded.hpp"
#include "bits/impl.hpp"
#include "bits/kernels.hpp"
#include "concepts.hpp"
#include "tags.hpp"

namespace micron
{
namespace math
{
namespace splines
{

template<ieee754_floating F> struct cubic_spline_1d {
  vector<F> xs;                       // n knots, strictly increasing
  vector<poly_coeffs<F, 3>> seg;      // n-1 segments
  mutable usize last_hit{ 0 };
  bc_kind bc{ bc_kind::natural };
  extrap mode{ extrap::linear_continue };
};

namespace __impl_cubic_1d
{

template<ieee754_floating F>
[[nodiscard]] inline bool
solve_cubic_M(const F *xs, const F *ys, usize n, bc_kind bc, F left_slope, F right_slope, F *M_out) noexcept
{
  if ( n < 2 ) return false;

  if ( n == 2 ) {

    M_out[0] = F(0);
    M_out[1] = F(0);
    return true;
  }

  vector<F> a(n - 1, F(0));      // sub-diagonal
  vector<F> b(n, F(0));          // main diagonal
  vector<F> c(n - 1, F(0));      // super-diagonal
  vector<F> d(n, F(0));          // rhs / solution
  a.set_size(n - 1);
  b.set_size(n);
  c.set_size(n - 1);
  d.set_size(n);

  for ( usize i = 1; i + 1 < n; ++i ) {
    const F h0 = xs[i] - xs[i - 1];
    const F h1 = xs[i + 1] - xs[i];
    const F del0 = (ys[i] - ys[i - 1]) / h0;
    const F del1 = (ys[i + 1] - ys[i]) / h1;
    a[i - 1] = h0;
    b[i] = F(2) * (h0 + h1);
    c[i] = h1;
    d[i] = F(6) * (del1 - del0);
  }

  if ( bc == bc_kind::natural ) {

    b[0] = F(1);
    c[0] = F(0);
    d[0] = F(0);
    a[n - 2] = F(0);
    b[n - 1] = F(1);
    d[n - 1] = F(0);
  } else if ( bc == bc_kind::clamped ) {

    const F h0 = xs[1] - xs[0];
    const F del0 = (ys[1] - ys[0]) / h0;
    b[0] = F(2) * h0;
    c[0] = h0;
    d[0] = F(6) * (del0 - left_slope);

    const F hL = xs[n - 1] - xs[n - 2];
    const F delL = (ys[n - 1] - ys[n - 2]) / hL;
    a[n - 2] = hL;
    b[n - 1] = F(2) * hL;
    d[n - 1] = F(6) * (right_slope - delL);
  } else /* bc_kind::not_a_knot */ {
    if ( n < 4 ) {
      // n == 3 (n == 2 already returned above): the not-a-knot spline with a
      // single interior knot degenerates to the unique parabola through the 3
      // points, whose second derivative is constant
      const F h0 = xs[1] - xs[0];
      const F h1 = xs[2] - xs[1];
      const F del0 = (ys[1] - ys[0]) / h0;
      const F del1 = (ys[2] - ys[1]) / h1;
      const F second_dd = (del1 - del0) / (xs[2] - xs[0]);      // f[x0,x1,x2]
      const F Mconst = F(2) * second_dd;
      M_out[0] = Mconst;
      M_out[1] = Mconst;
      M_out[2] = Mconst;
      return true;
    }

    const usize m = n - 2;
    vector<F> ai(m == 0 ? 1 : m, F(0));      // sub
    vector<F> bi(m == 0 ? 1 : m, F(0));      // diag
    vector<F> ci(m == 0 ? 1 : m, F(0));      // super
    vector<F> di(m == 0 ? 1 : m, F(0));      // rhs / sol
    if ( m > 0 ) {
      ai.set_size(m);
      bi.set_size(m);
      ci.set_size(m);
      di.set_size(m);
    }

    {

      const F h0 = xs[1] - xs[0];
      const F h1 = xs[2] - xs[1];
      const F del0 = (ys[1] - ys[0]) / h0;
      const F del1 = (ys[2] - ys[1]) / h1;
      bi[0] = h0 + F(2) * h1;
      if ( m > 1 ) ci[0] = h1 - h0;
      di[0] = F(6) * h1 * (del1 - del0) / (h0 + h1);
    }

    for ( usize i = 2; i + 1 < n - 1; ++i ) {
      const usize r = i - 1;
      const F hp = xs[i] - xs[i - 1];
      const F hn = xs[i + 1] - xs[i];
      const F delp = (ys[i] - ys[i - 1]) / hp;
      const F deln = (ys[i + 1] - ys[i]) / hn;
      ai[r - 1] = hp;
      bi[r] = F(2) * (hp + hn);
      ci[r] = hn;
      di[r] = F(6) * (deln - delp);
    }

    if ( m >= 2 ) {

      const F hLm1 = xs[n - 2] - xs[n - 3];
      const F hL = xs[n - 1] - xs[n - 2];
      const F delLm1 = (ys[n - 2] - ys[n - 3]) / hLm1;
      const F delL = (ys[n - 1] - ys[n - 2]) / hL;
      ai[m - 2] = hLm1 - hL;
      bi[m - 1] = F(2) * hLm1 + hL;
      di[m - 1] = F(6) * hLm1 * (delL - delLm1) / (hLm1 + hL);
    }

    linalg::tridiag_solve<F>(ai.data(), bi.data(), ci.data(), di.data(), m);

    for ( usize r = 0; r < m; ++r ) M_out[r + 1] = di[r];
    {
      const F h0 = xs[1] - xs[0];
      const F h1 = xs[2] - xs[1];

      M_out[0] = (h0 + h1) / h1 * M_out[1] - h0 / h1 * M_out[2];
    }
    {
      const F hLm1 = xs[n - 2] - xs[n - 3];
      const F hL = xs[n - 1] - xs[n - 2];

      M_out[n - 1] = (hL + hLm1) / hLm1 * M_out[n - 2] - hL / hLm1 * M_out[n - 3];
    }
    return true;
  }

  linalg::tridiag_solve<F>(a.data(), b.data(), c.data(), d.data(), n);
  for ( usize i = 0; i < n; ++i ) M_out[i] = d[i];
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reusable factorization used by multi-coordinate curve builders

template<ieee754_floating F> struct cubic_system_factor {
  vector<F> storage;
  usize size{ 0 };
  bc_kind bc{ bc_kind::natural };
  bool quadratic_case{ false };

  [[nodiscard, gnu::always_inline]] F *
  lower() noexcept
  {
    return storage.data();
  }

  [[nodiscard, gnu::always_inline]] F *
  inverse_diagonal() noexcept
  {
    return storage.data() + size;
  }

  [[nodiscard, gnu::always_inline]] F *
  upper() noexcept
  {
    return storage.data() + 2 * size;
  }
};

template<ieee754_floating F>
[[nodiscard]] inline bool
factor_cubic_system(const F *xs, usize n, bc_kind bc, cubic_system_factor<F> &factor) noexcept
{
  if ( n < 2 ) return false;
  factor.bc = bc;
  factor.quadratic_case = bc == bc_kind::not_a_knot && n == 3;
  if ( n == 2 || factor.quadratic_case ) {
    factor.size = 0;
    return true;
  }
  factor.size = bc == bc_kind::not_a_knot ? n - 2 : n;
  const usize m = factor.size;
  factor.storage = vector<F>(3 * m, F(0));
  factor.storage.set_size(3 * m);
  F *lower = factor.lower();
  F *diagonal = factor.inverse_diagonal();
  F *upper = factor.upper();

  if ( bc == bc_kind::not_a_knot ) {
    const F h0 = xs[1] - xs[0];
    const F h1 = xs[2] - xs[1];
    diagonal[0] = h0 + F(2) * h1;
    if ( m > 1 ) upper[0] = h1 - h0;
    for ( usize i = 2; i + 1 < n - 1; ++i ) {
      const usize row = i - 1;
      const F hp = xs[i] - xs[i - 1];
      const F hn = xs[i + 1] - xs[i];
      lower[row] = hp;
      diagonal[row] = F(2) * (hp + hn);
      upper[row] = hn;
    }
    if ( m >= 2 ) {
      const F h0_last = xs[n - 2] - xs[n - 3];
      const F h1_last = xs[n - 1] - xs[n - 2];
      lower[m - 1] = h0_last - h1_last;
      diagonal[m - 1] = F(2) * h0_last + h1_last;
    }
  } else {
    for ( usize i = 1; i + 1 < n; ++i ) {
      const F h0 = xs[i] - xs[i - 1];
      const F h1 = xs[i + 1] - xs[i];
      lower[i] = h0;
      diagonal[i] = F(2) * (h0 + h1);
      upper[i] = h1;
    }
    if ( bc == bc_kind::natural ) {
      diagonal[0] = F(1);
      diagonal[n - 1] = F(1);
    } else {
      const F h0 = xs[1] - xs[0];
      const F h1 = xs[n - 1] - xs[n - 2];
      diagonal[0] = F(2) * h0;
      upper[0] = h0;
      lower[n - 1] = h1;
      diagonal[n - 1] = F(2) * h1;
    }
  }

  constexpr F tiny = default_eps<F>() * F(4);
  if ( !(math::fabs(diagonal[0]) > tiny) ) return false;
  diagonal[0] = F(1) / diagonal[0];
  if ( m > 1 ) upper[0] *= diagonal[0];
  for ( usize i = 1; i < m; ++i ) {
    const F pivot = diagonal[i] - lower[i] * upper[i - 1];
    if ( !(math::fabs(pivot) > tiny) ) return false;
    diagonal[i] = F(1) / pivot;
    if ( i + 1 < m ) upper[i] *= diagonal[i];
  }
  return true;
}

template<ieee754_floating F>
inline void
solve_cubic_factored(const cubic_system_factor<F> &factor, const F *xs, const F *ys, usize n, F left_slope, F right_slope,
                     F *moments) noexcept
{
  if ( n == 2 ) {
    moments[0] = moments[1] = F(0);
    return;
  }
  if ( factor.quadratic_case ) {
    const F h0 = xs[1] - xs[0];
    const F h1 = xs[2] - xs[1];
    const F delta0 = (ys[1] - ys[0]) / h0;
    const F delta1 = (ys[2] - ys[1]) / h1;
    const F value = F(2) * (delta1 - delta0) / (xs[2] - xs[0]);
    moments[0] = moments[1] = moments[2] = value;
    return;
  }

  const usize m = factor.size;
  F *rhs = moments;
  if ( factor.bc == bc_kind::not_a_knot ) {
    const F h0 = xs[1] - xs[0];
    const F h1 = xs[2] - xs[1];
    const F delta0 = (ys[1] - ys[0]) / h0;
    const F delta1 = (ys[2] - ys[1]) / h1;
    rhs[0] = F(6) * h1 * (delta1 - delta0) / (h0 + h1);
    for ( usize i = 2; i + 1 < n - 1; ++i ) {
      const F hp = xs[i] - xs[i - 1];
      const F hn = xs[i + 1] - xs[i];
      rhs[i - 1] = F(6) * ((ys[i + 1] - ys[i]) / hn - (ys[i] - ys[i - 1]) / hp);
    }
    if ( m >= 2 ) {
      const F hp = xs[n - 2] - xs[n - 3];
      const F hn = xs[n - 1] - xs[n - 2];
      rhs[m - 1] = F(6) * hp * ((ys[n - 1] - ys[n - 2]) / hn - (ys[n - 2] - ys[n - 3]) / hp) / (hp + hn);
    }
  } else {
    for ( usize i = 1; i + 1 < n; ++i ) {
      const F hp = xs[i] - xs[i - 1];
      const F hn = xs[i + 1] - xs[i];
      rhs[i] = F(6) * ((ys[i + 1] - ys[i]) / hn - (ys[i] - ys[i - 1]) / hp);
    }
    if ( factor.bc == bc_kind::natural ) {
      rhs[0] = rhs[n - 1] = F(0);
    } else {
      const F h0 = xs[1] - xs[0];
      const F hn = xs[n - 1] - xs[n - 2];
      rhs[0] = F(6) * ((ys[1] - ys[0]) / h0 - left_slope);
      rhs[n - 1] = F(6) * (right_slope - (ys[n - 1] - ys[n - 2]) / hn);
    }
  }

  const F *lower = factor.storage.data();
  const F *inverse = factor.storage.data() + m;
  const F *upper = factor.storage.data() + 2 * m;
  rhs[0] *= inverse[0];
  for ( usize i = 1; i < m; ++i ) rhs[i] = (rhs[i] - lower[i] * rhs[i - 1]) * inverse[i];
  for ( usize i = m - 1; i-- > 0; ) rhs[i] -= upper[i] * rhs[i + 1];

  if ( factor.bc == bc_kind::not_a_knot ) {
    for ( usize i = m; i-- > 0; ) moments[i + 1] = moments[i];
    const F h0 = xs[1] - xs[0];
    const F h1 = xs[2] - xs[1];
    moments[0] = (h0 + h1) * moments[1] / h1 - h0 * moments[2] / h1;
    const F hp = xs[n - 2] - xs[n - 3];
    const F hn = xs[n - 1] - xs[n - 2];
    moments[n - 1] = (hn + hp) * moments[n - 2] / hp - hn * moments[n - 3] / hp;
  }
}

};      // namespace __impl_cubic_1d

template<ieee754_floating F>
[[nodiscard]] inline cubic_spline_1d<F>
make_cubic(raw_slice<const F> xs, raw_slice<const F> ys, bc_kind bc = bc_kind::natural, F left_slope = F(0), F right_slope = F(0),
           build_info<F> *info = nullptr) noexcept
{
  cubic_spline_1d<F> s{};
  s.bc = bc;
  if ( xs.size() != ys.size() ) {
    if ( info ) info->status = build_status::size_mismatch;
    return s;
  }
  if ( xs.size() < 2 ) {
    if ( info ) info->status = build_status::too_few_points;
    return s;
  }
  if ( !__impl_splines_bits::strictly_increasing<F>(xs.ptr, xs.size()) ) {
    if ( info ) info->status = build_status::non_monotonic_x;
    return s;
  }
  const usize n = xs.size();

  // relative-spacing guard
  {
    const F span = xs[n - 1] - xs[0];
    const F min_h = F(1e-12) * span;
    for ( usize i = 0; i + 1 < n; ++i ) {
      if ( xs[i + 1] - xs[i] <= min_h ) {
        if ( info ) info->status = build_status::degenerate;
        return s;
      }
    }
  }

  vector<F> M(n, F(0));
  M.set_size(n);
  __impl_cubic_1d::cubic_system_factor<F> factor;
  if ( !__impl_cubic_1d::factor_cubic_system<F>(xs.ptr, n, bc, factor) ) {
    if ( info ) info->status = build_status::singular_system;
    return s;
  }
  __impl_cubic_1d::solve_cubic_factored<F>(factor, xs.ptr, ys.ptr, n, left_slope, right_slope, M.data());

  s.xs.reserve(n);
  s.seg.reserve(n - 1);
  for ( usize i = 0; i < n; ++i ) s.xs.emplace_back(xs[i]);
  poly_coeffs<F, 3> zero{};
  zero.data[0] = zero.data[1] = zero.data[2] = zero.data[3] = F(0);
  for ( usize i = 0; i + 1 < n; ++i ) s.seg.emplace_back(zero);
  __impl_splines_bits::build_cubic_segments<F>(xs.ptr, ys.ptr, M.data(), s.seg.data(), n);

  if ( info ) info->status = build_status::ok;
  return s;
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline F
evaluate(const cubic_spline_1d<F> &s, F x) noexcept
{
  const usize n = s.xs.size();
  const F *__restrict__ xs = s.xs.data();
  const auto *__restrict__ seg = s.seg.data();
  if ( n == 0 ) return F(0);
  if ( n == 1 ) return seg ? seg[0].data[0] : F(0);

  if ( x <= xs[0] ) {
    if ( s.mode == extrap::error_value ) return F(0);
    if ( s.mode == extrap::clamp_to_endpoints ) return seg[0].data[0];

    const F slope = seg[0].data[1];
    return math::fma<F>(slope, x - xs[0], seg[0].data[0]);
  }
  if ( x >= xs[n - 1] ) {
    if ( s.mode == extrap::error_value ) return F(0);
    if ( s.mode == extrap::clamp_to_endpoints ) {
      const auto &p = seg[n - 2];
      const F t = xs[n - 1] - xs[n - 2];
      return __impl_splines_bits::eval_cubic_local<F>(p, t);
    }
    const auto &p = seg[n - 2];
    const F t_end = xs[n - 1] - xs[n - 2];
    const F y_end = __impl_splines_bits::eval_cubic_local<F>(p, t_end);
    const F slope_end = __impl_splines_bits::eval_cubic_deriv1_local<F>(p, t_end);
    return math::fma<F>(slope_end, x - xs[n - 1], y_end);
  }

  const usize i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  return __impl_splines_bits::eval_cubic_local<F>(seg[i], x - xs[i]);
}

template<ieee754_floating F>
inline void
evaluate(const cubic_spline_1d<F> &s, const F *__restrict__ xq, F *__restrict__ out, usize n) noexcept
{
  if ( !__impl_splines_bits::is_sorted_nondecreasing<F>(xq, n) ) {
    for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }

  const usize ns = s.xs.size();
  if ( ns < 2 ) {
    for ( usize i = 0; i < n; ++i ) out[i] = evaluate<F>(s, xq[i]);
    return;
  }
  const F *__restrict__ xs = s.xs.data();
  const auto *__restrict__ seg = s.seg.data();

  usize query = 0;
  while ( query < n && xq[query] <= xs[0] ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }

  usize idx = 0;
  if ( query < n && xq[query] < xs[ns - 1] ) idx = __impl_splines_bits::locate_segment<F>(xs, ns, xq[query], idx);
  while ( query < n && xq[query] < xs[ns - 1] ) {
    while ( idx + 1 < ns - 1 && xq[query] > xs[idx + 1] ) ++idx;
    const usize begin = query;
    while ( query < n && xq[query] < xs[ns - 1] && xq[query] <= xs[idx + 1] ) ++query;
    __spline_arch::cubic_horner_batch(seg[idx], xs[idx], xq + begin, out + begin, query - begin);
  }
  while ( query < n ) {
    out[query] = evaluate<F>(s, xq[query]);
    ++query;
  }
  if ( n != 0 ) {
    if ( xq[n - 1] <= xs[0] )
      s.last_hit = 0;
    else if ( xq[n - 1] >= xs[ns - 1] )
      s.last_hit = ns - 2;
    else
      s.last_hit = idx;
  }
}

template<ieee754_floating F>
[[nodiscard]] inline F
derivative(const cubic_spline_1d<F> &s, F x, u32 order = 1) noexcept
{
  const usize n = s.xs.size();
  if ( n < 2 ) return F(0);
  const F *__restrict__ xs = s.xs.data();
  const auto *__restrict__ seg = s.seg.data();
  if ( x <= xs[0] ) {
    if ( order == 1 ) return seg[0].data[1];
    if ( order == 2 ) return F(2) * seg[0].data[2];
    return F(0);
  }
  if ( x >= xs[n - 1] ) {
    const auto &p = seg[n - 2];
    const F t = xs[n - 1] - xs[n - 2];
    if ( order == 1 ) return __impl_splines_bits::eval_cubic_deriv1_local<F>(p, t);
    if ( order == 2 ) return __impl_splines_bits::eval_cubic_deriv2_local<F>(p, t);
    return F(0);
  }
  const usize i = __impl_splines_bits::locate_segment<F>(xs, n, x, s.last_hit);
  const F t = x - xs[i];
  if ( order == 0 ) return __impl_splines_bits::eval_cubic_local<F>(seg[i], t);
  if ( order == 1 ) return __impl_splines_bits::eval_cubic_deriv1_local<F>(seg[i], t);
  if ( order == 2 ) return __impl_splines_bits::eval_cubic_deriv2_local<F>(seg[i], t);

  if ( order == 3 ) return F(6) * seg[i].data[3];
  return F(0);
}

template<ieee754_floating F>
[[nodiscard]] inline F
integral(const cubic_spline_1d<F> &s, F a, F b) noexcept
{
  if ( a == b ) return F(0);
  const F sign = (a < b) ? F(1) : F(-1);
  if ( a > b ) {
    const F t = a;
    a = b;
    b = t;
  }
  const usize n = s.xs.size();
  if ( n < 2 ) return F(0);
  const F *__restrict__ xs = s.xs.data();
  const auto *__restrict__ seg = s.seg.data();

  usize ia = 0, ib = 0;
  ia = __impl_splines_bits::locate_segment<F>(xs, n, a < xs[0] ? xs[0] : a, s.last_hit);
  ib = __impl_splines_bits::locate_segment<F>(xs, n, b > xs[n - 1] ? xs[n - 1] : b, s.last_hit);

  if ( a < xs[0] ) a = xs[0];
  if ( b > xs[n - 1] ) b = xs[n - 1];

  F sum = F(0);
  if ( ia == ib ) {
    const auto &p = seg[ia];
    const F lo = a - xs[ia];
    const F hi = b - xs[ia];
    sum = __impl_splines_bits::eval_cubic_antideriv_local<F>(p, hi) - __impl_splines_bits::eval_cubic_antideriv_local<F>(p, lo);
  } else {

    {
      const auto &p = seg[ia];
      const F lo = a - xs[ia];
      const F hi = xs[ia + 1] - xs[ia];
      sum += __impl_splines_bits::eval_cubic_antideriv_local<F>(p, hi) - __impl_splines_bits::eval_cubic_antideriv_local<F>(p, lo);
    }

    for ( usize i = ia + 1; i < ib; ++i ) {
      const auto &p = seg[i];
      const F hi = xs[i + 1] - xs[i];
      sum += __impl_splines_bits::eval_cubic_antideriv_local<F>(p, hi);
    }

    {
      const auto &p = seg[ib];
      const F hi = b - xs[ib];
      sum += __impl_splines_bits::eval_cubic_antideriv_local<F>(p, hi);
    }
  }
  return sign * sum;
}

};      // namespace splines
};      // namespace math
};      // namespace micron
