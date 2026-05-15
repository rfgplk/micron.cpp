//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 3D rigid-body group; SO3 rotation + translation

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

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr vec<F, 3>
left_jacobian_apply(const vec<F, 3> &omega, const vec<F, 3> &v) noexcept
{
  const F wx = omega.data[0], wy = omega.data[1], wz = omega.data[2];
  const F theta_sq = wx * wx + wy * wy + wz * wz;
  const vec<F, 3> wxv{ wy * v.data[2] - wz * v.data[1], wz * v.data[0] - wx * v.data[2], wx * v.data[1] - wy * v.data[0] };
  const vec<F, 3> wxwxv{ wy * wxv.data[2] - wz * wxv.data[1], wz * wxv.data[0] - wx * wxv.data[2], wx * wxv.data[1] - wy * wxv.data[0] };
  F a, b;
  if ( theta_sq < math::default_eps<F>() ) {
    a = F(0.5) - theta_sq / F(24);
    b = F(1) / F(6) - theta_sq / F(120);
  } else {
    const F theta = math::fsqrt(theta_sq);
    const F s = math::sin<F>(theta);
    const F c = math::cos<F>(theta);
    a = (F(1) - c) / theta_sq;
    b = (theta - s) / (theta_sq * theta);
  }
  return vec<F, 3>{ v.data[0] + a * wxv.data[0] + b * wxwxv.data[0], v.data[1] + a * wxv.data[1] + b * wxwxv.data[1],
                    v.data[2] + a * wxv.data[2] + b * wxwxv.data[2] };
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr vec<F, 3>
left_jacobian_inv_apply(const vec<F, 3> &omega, const vec<F, 3> &t) noexcept
{
  const F wx = omega.data[0], wy = omega.data[1], wz = omega.data[2];
  const F theta_sq = wx * wx + wy * wy + wz * wz;
  const vec<F, 3> wxt{ wy * t.data[2] - wz * t.data[1], wz * t.data[0] - wx * t.data[2], wx * t.data[1] - wy * t.data[0] };
  const vec<F, 3> wxwxt{ wy * wxt.data[2] - wz * wxt.data[1], wz * wxt.data[0] - wx * wxt.data[2], wx * wxt.data[1] - wy * wxt.data[0] };
  F coeff;
  if ( theta_sq < math::default_eps<F>() ) {
    coeff = F(1) / F(12) + theta_sq / F(720);
  } else {
    const F theta = math::fsqrt(theta_sq);
    const F half = theta * F(0.5);
    const F s_half = math::sin<F>(half);
    const F c_half = math::cos<F>(half);
    coeff = (F(1) / theta_sq) - c_half / (F(2) * theta * s_half);
  }
  return vec<F, 3>{ t.data[0] - F(0.5) * wxt.data[0] + coeff * wxwxt.data[0], t.data[1] - F(0.5) * wxt.data[1] + coeff * wxwxt.data[1],
                    t.data[2] - F(0.5) * wxt.data[2] + coeff * wxwxt.data[2] };
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
    const vec<F, 3> w{ xi.data[3], xi.data[4], xi.data[5] };
    return SE3{ SO3<F>::exp_map(w), __se3_impl::left_jacobian_apply<F>(w, v) };
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, 6>
  log_map(const SE3 &g) noexcept
  {
    const auto w = SO3<F>::log_map(g.R);
    const auto v = __se3_impl::left_jacobian_inv_apply<F>(w, g.t);
    return vec<F, 6>{ v.data[0], v.data[1], v.data[2], w.data[0], w.data[1], w.data[2] };
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
