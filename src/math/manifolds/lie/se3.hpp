//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 3D rigid-body group; SO(3) rotation + translation

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../linalg/ops.hpp"
#include "../../matrix/mat.hpp"
#include "../../mk.hpp"
#include "../../quants/vec.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"
#include "so3.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{

namespace __se3_impl
{

// Taylor coefficients of (1 - cos theta)/theta**2 as a polynomial in theta**2
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
lj_a_poly(F t2) noexcept
{
  F r = F(1) / F(3628800);
  r = r * t2 - F(1) / F(40320);
  r = r * t2 + F(1) / F(720);
  r = r * t2 - F(1) / F(24);
  r = r * t2 + F(0.5);
  return r;
}

// Taylor coefficients of (theta - sin theta)/theta**3 as a polynomial in theta**2
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
lj_b_poly(F t2) noexcept
{
  F r = F(1) / F(39916800);
  r = r * t2 - F(1) / F(362880);
  r = r * t2 + F(1) / F(5040);
  r = r * t2 - F(1) / F(120);
  r = r * t2 + F(1) / F(6);
  return r;
}

// Taylor of 1/theta**2
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
lj_inv_poly(F t2) noexcept
{
  F r = F(1) / F(1209600);
  r = r * t2 + F(1) / F(30240);
  r = r * t2 + F(1) / F(720);
  r = r * t2 + F(1) / F(12);
  return r;
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 3>
cross3(const vec<F, 3> &a, const vec<F, 3> &b) noexcept
{
  return vec<F, 3>{ a.data[1] * b.data[2] - a.data[2] * b.data[1], a.data[2] * b.data[0] - a.data[0] * b.data[2],
                    a.data[0] * b.data[1] - a.data[1] * b.data[0] };
}

// Branchless left-Jacobian, must faster
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 3>
left_jacobian_apply_with_ab(const vec<F, 3> &omega, const vec<F, 3> &v, F a, F b) noexcept
{
  const vec<F, 3> wxv = cross3<F>(omega, v);
  const vec<F, 3> wxwxv = cross3<F>(omega, wxv);
  return vec<F, 3>{ v.data[0] + a * wxv.data[0] + b * wxwxv.data[0], v.data[1] + a * wxv.data[1] + b * wxwxv.data[1],
                    v.data[2] + a * wxv.data[2] + b * wxwxv.data[2] };
}

// Left-Jacobian-inverse application
template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr vec<F, 3>
left_jacobian_inv_apply_with_c(const vec<F, 3> &omega, const vec<F, 3> &t, F c) noexcept
{
  const vec<F, 3> wxt = cross3<F>(omega, t);
  const vec<F, 3> wxwxt = cross3<F>(omega, wxt);
  return vec<F, 3>{ t.data[0] - F(0.5) * wxt.data[0] + c * wxwxt.data[0], t.data[1] - F(0.5) * wxt.data[1] + c * wxwxt.data[1],
                    t.data[2] - F(0.5) * wxt.data[2] + c * wxwxt.data[2] };
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline constexpr F
small_angle_threshold_sq() noexcept
{
  return F(1) / F(100);
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr vec<F, 3>
left_jacobian_apply(const vec<F, 3> &omega, const vec<F, 3> &v) noexcept
{
  const F wx = omega.data[0], wy = omega.data[1], wz = omega.data[2];
  const F theta_sq = wx * wx + wy * wy + wz * wz;
  F a, b;
  if ( theta_sq < small_angle_threshold_sq<F>() ) {
    a = lj_a_poly<F>(theta_sq);
    b = lj_b_poly<F>(theta_sq);
  } else {
    const F theta = math::fsqrt(theta_sq);
    const F half = theta * F(0.5);
    F s_half, c_half;
    math::sincos<F>(half, s_half, c_half);
    const F sin_theta = F(2) * s_half * c_half;
    const F cos_theta = c_half * c_half - s_half * s_half;
    a = (F(1) - cos_theta) / theta_sq;
    b = (theta - sin_theta) / (theta * theta_sq);
  }
  return left_jacobian_apply_with_ab<F>(omega, v, a, b);
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr vec<F, 3>
left_jacobian_inv_apply(const vec<F, 3> &omega, const vec<F, 3> &t) noexcept
{
  const F wx = omega.data[0], wy = omega.data[1], wz = omega.data[2];
  const F theta_sq = wx * wx + wy * wy + wz * wz;
  F coeff;
  if ( theta_sq < small_angle_threshold_sq<F>() ) {
    coeff = lj_inv_poly<F>(theta_sq);
  } else {
    const F theta = math::fsqrt(theta_sq);
    const F half = theta * F(0.5);
    F s_half, c_half;
    math::sincos<F>(half, s_half, c_half);
    coeff = (F(1) / theta_sq) - c_half / (F(2) * theta * s_half);
  }
  return left_jacobian_inv_apply_with_c<F>(omega, t, coeff);
}

};      // namespace __se3_impl

template<ieee754_floating F> struct SE3 {
  SO3<F> R;
  vec<F, 3> t;

  [[nodiscard, gnu::always_inline]] static constexpr SE3
  identity() noexcept
  {
    return SE3{ SO3<F>::identity(), vec<F, 3>{ F(0), F(0), F(0) } };
  }

  [[nodiscard, gnu::flatten]] static constexpr SE3
  compose(const SE3 &a, const SE3 &b) noexcept
  {
    return SE3{ SO3<F>::compose(a.R, b.R), a.t + SO3<F>::rotate(a.R, b.t) };
  }

  [[nodiscard, gnu::flatten]] static constexpr SE3
  inverse(const SE3 &g) noexcept
  {
    const auto Rinv = SO3<F>::inverse(g.R);
    const auto tinv = SO3<F>::rotate(Rinv, vec<F, 3>{ -g.t.data[0], -g.t.data[1], -g.t.data[2] });
    return SE3{ Rinv, tinv };
  }

  [[nodiscard, gnu::flatten]] static constexpr SE3
  exp_map(const vec<F, 6> &xi) noexcept
  {
    const vec<F, 3> v{ xi.data[0], xi.data[1], xi.data[2] };
    const F wx = xi.data[3], wy = xi.data[4], wz = xi.data[5];
    const vec<F, 3> w{ wx, wy, wz };
    const F theta_sq = wx * wx + wy * wy + wz * wz;

    // optimized
    F scalar;
    F c_half;
    F a, b;

    if ( theta_sq < __se3_impl::small_angle_threshold_sq<F>() ) {
      scalar = __so3_impl::sinc_half_over_theta<F>(theta_sq);
      c_half = __so3_impl::cos_half<F>(theta_sq);
      a = __se3_impl::lj_a_poly<F>(theta_sq);
      b = __se3_impl::lj_b_poly<F>(theta_sq);
    } else {
      const F theta = math::fsqrt(theta_sq);
      const F half = theta * F(0.5);
      F s_half;
      math::sincos<F>(half, s_half, c_half);
      scalar = s_half / theta;
      const F sin_theta = F(2) * s_half * c_half;
      const F cos_theta = c_half * c_half - s_half * s_half;
      a = (F(1) - cos_theta) / theta_sq;
      b = (theta - sin_theta) / (theta * theta_sq);
    }

    const quat<F> q{ wx * scalar, wy * scalar, wz * scalar, c_half };
    const vec<F, 3> t_out = __se3_impl::left_jacobian_apply_with_ab<F>(w, v, a, b);
    return SE3{ SO3<F>{ q }, t_out };
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, 6>
  log_map(const SE3 &g) noexcept
  {
    F qx = g.R.q.data[0], qy = g.R.q.data[1], qz = g.R.q.data[2], qw = g.R.q.data[3];
    if ( qw < F(0) ) {
      qx = -qx;
      qy = -qy;
      qz = -qz;
      qw = -qw;
    }
    const F xyz_sq = qx * qx + qy * qy + qz * qz;

    F wx, wy, wz;
    F theta_sq;
    if ( xyz_sq < __so3_impl::small_angle_threshold_sq<F>() ) {
      const F inv_qw = F(1) / qw;
      const F y2 = xyz_sq * (inv_qw * inv_qw);
      F p = F(-1) / F(11);
      p = p * y2 + F(1) / F(9);
      p = p * y2 - F(1) / F(7);
      p = p * y2 + F(1) / F(5);
      p = p * y2 - F(1) / F(3);
      p = p * y2 + F(1);
      const F k = F(2) * p * inv_qw;
      wx = qx * k;
      wy = qy * k;
      wz = qz * k;
      theta_sq = wx * wx + wy * wy + wz * wz;
    } else {
      const F xyz_norm = math::fsqrt(xyz_sq);
      const F angle = F(2) * math::atan2<F>(xyz_norm, qw);
      const F k = angle / xyz_norm;
      wx = qx * k;
      wy = qy * k;
      wz = qz * k;
      theta_sq = angle * angle;
    }
    const vec<F, 3> w{ wx, wy, wz };

    F coeff;
    if ( theta_sq < __se3_impl::small_angle_threshold_sq<F>() ) {
      coeff = __se3_impl::lj_inv_poly<F>(theta_sq);
    } else {
      const F xyz_norm = math::fsqrt(xyz_sq);
      const F theta = math::fsqrt(theta_sq);
      coeff = (F(1) / theta_sq) - qw / (F(2) * theta * xyz_norm);
    }
    const vec<F, 3> v_out = __se3_impl::left_jacobian_inv_apply_with_c<F>(w, g.t, coeff);

    return vec<F, 6>{ v_out.data[0], v_out.data[1], v_out.data[2], wx, wy, wz };
  }

  [[nodiscard, gnu::flatten]] static constexpr mat<F, 4, 4>
  to_matrix(const SE3 &g) noexcept
  {
    const auto R3 = SO3<F>::to_matrix(g.R);
    mat<F, 4, 4> M = mat<F, 4, 4>::zero();
    for ( usize r = 0; r < 3; ++r )
      for ( usize c = 0; c < 3; ++c ) M.data[r * 4 + c] = R3.data[r * 3 + c];
    M.data[0 * 4 + 3] = g.t.data[0];
    M.data[1 * 4 + 3] = g.t.data[1];
    M.data[2 * 4 + 3] = g.t.data[2];
    M.data[3 * 4 + 3] = F(1);
    return M;
  }

  [[nodiscard, gnu::flatten]] static constexpr mat<F, 6, 6>
  adjoint(const SE3 &g) noexcept
  {
    const auto R3 = SO3<F>::to_matrix(g.R);
    const F tx = g.t.data[0], ty = g.t.data[1], tz = g.t.data[2];
    const F s00 = F(0), s01 = -tz, s02 = ty;
    const F s10 = tz, s11 = F(0), s12 = -tx;
    const F s20 = -ty, s21 = tx, s22 = F(0);
    F tr[9];
    tr[0] = s00 * R3.data[0] + s01 * R3.data[3] + s02 * R3.data[6];
    tr[1] = s00 * R3.data[1] + s01 * R3.data[4] + s02 * R3.data[7];
    tr[2] = s00 * R3.data[2] + s01 * R3.data[5] + s02 * R3.data[8];
    tr[3] = s10 * R3.data[0] + s11 * R3.data[3] + s12 * R3.data[6];
    tr[4] = s10 * R3.data[1] + s11 * R3.data[4] + s12 * R3.data[7];
    tr[5] = s10 * R3.data[2] + s11 * R3.data[5] + s12 * R3.data[8];
    tr[6] = s20 * R3.data[0] + s21 * R3.data[3] + s22 * R3.data[6];
    tr[7] = s20 * R3.data[1] + s21 * R3.data[4] + s22 * R3.data[7];
    tr[8] = s20 * R3.data[2] + s21 * R3.data[5] + s22 * R3.data[8];
    mat<F, 6, 6> Ad = mat<F, 6, 6>::zero();
    for ( usize r = 0; r < 3; ++r )
      for ( usize c = 0; c < 3; ++c ) {
        Ad.data[r * 6 + c] = R3.data[r * 3 + c];
        Ad.data[r * 6 + (c + 3)] = tr[r * 3 + c];
        Ad.data[(r + 3) * 6 + (c + 3)] = R3.data[r * 3 + c];
      }
    return Ad;
  }
};

};      // namespace lie

template<ieee754_floating F> struct traits<lie::SE3<F>> {
  using point_type = lie::SE3<F>;
  using tangent_type = vec<F, 6>;
  using scalar_type = F;
  using category = lie_group_tag;
  static constexpr usize dim = 6;
  static constexpr usize ambient_dim = 16;
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
