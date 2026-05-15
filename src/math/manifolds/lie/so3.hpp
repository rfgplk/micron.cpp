//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 3D rotation group; SO3

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../blas/identities.hpp"
#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../linalg/ops.hpp"
#include "../../matrix/mat.hpp"
#include "../../mk.hpp"
#include "../../quants/quat.hpp"
#include "../../quants/vec.hpp"
#include "../../quants/vecs.hpp"
#include "../../quaternions/rotations.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{

template<ieee754_floating F> struct SO3 {
  quat<F> q;

  [[nodiscard, gnu::always_inline]] static constexpr SO3
  identity() noexcept
  {
    return SO3{ quat<F>{ F(0), F(0), F(0), F(1) } };
  }

  [[nodiscard, gnu::flatten]] static constexpr SO3
  from_matrix(const mat<F, 3, 3> &R) noexcept
  {
    auto qv4 = quaternions::from_matrix<F>(R);      // vector_4<F>
    return SO3{ quat<F>{ qv4.x, qv4.y, qv4.z, qv4.w } };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SO3
  from_quat(const quat<F> &qq) noexcept
  {
    return SO3{ qq };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SO3
  compose(const SO3 &a, const SO3 &b) noexcept
  {
    return SO3{ linalg::ops::mul<F>(a.q, b.q) };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SO3
  inverse(const SO3 &g) noexcept
  {
    return SO3{ linalg::ops::conjugate<F>(g.q) };
  }

  [[nodiscard, gnu::flatten]] static constexpr SO3
  exp_map(const vec<F, 3> &omega) noexcept
  {
    const F wx = omega.data[0], wy = omega.data[1], wz = omega.data[2];
    const F theta_sq = wx * wx + wy * wy + wz * wz;
    if ( theta_sq < math::default_eps<F>() ) {
      const F half_co = F(0.5) - theta_sq * F(1) / F(48);
      const F w = F(1) - theta_sq * F(0.125);
      return SO3{ quat<F>{ wx * half_co, wy * half_co, wz * half_co, w } };
    }
    const F theta = math::fsqrt(theta_sq);
    const F half = theta * F(0.5);
    const F s = math::sin<F>(half);
    const F c = math::cos<F>(half);
    const F k = s / theta;
    return SO3{ quat<F>{ wx * k, wy * k, wz * k, c } };
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, 3>
  log_map(const SO3 &g) noexcept
  {
    const F qx = g.q.data[0], qy = g.q.data[1], qz = g.q.data[2], qw = g.q.data[3];
    const F xyz_sq = qx * qx + qy * qy + qz * qz;
    if ( xyz_sq < math::default_eps<F>() ) {
      const F two_inv_w = F(2) / qw;
      return vec<F, 3>{ qx * two_inv_w, qy * two_inv_w, qz * two_inv_w };
    }
    const F xyz_norm = math::fsqrt(xyz_sq);
    const F angle = F(2) * math::atan2<F>(xyz_norm, qw);
    const F k = angle / xyz_norm;
    return vec<F, 3>{ qx * k, qy * k, qz * k };
  }

  [[nodiscard, gnu::always_inline]] static constexpr mat<F, 3, 3>
  to_matrix(const SO3 &g) noexcept
  {
    return blas::identities::from_quat<F>(g.q);
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, 3>
  rotate(const SO3 &g, const vec<F, 3> &v) noexcept
  {
    return linalg::ops::rotate<F>(g.q, v);
  }

  [[nodiscard, gnu::always_inline]] static constexpr mat<F, 3, 3>
  adjoint(const SO3 &g) noexcept
  {
    return to_matrix(g);
  }
};

};      // namespace lie

template<ieee754_floating F> struct traits<lie::SO3<F>> {
  using point_type = lie::SO3<F>;
  using tangent_type = vec<F, 3>;
  using scalar_type = F;
  using category = lie_group_tag;
  static constexpr usize dim = 3;
  static constexpr usize ambient_dim = 9;
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
