//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// projections: perspective, orthographic, frustum, look_at
//
// standard right-handed graphics conventions:
//   -> eye looks down -Z
//   -> +X right, +Y up

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../linalg/ops.hpp"
#include "../mk.hpp"
#include "../quants/vec.hpp"
#include "../sqrt.hpp"
#include "../trig.hpp"
#include "transform.hpp"

namespace micron
{
namespace math
{
namespace geometry
{

template <ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
perspective_projection(F fov_y, F aspect, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  const F f = F(1) / math::tan(fov_y * F(0.5));
  out.M.data[0 * 4 + 0] = f / aspect;
  out.M.data[1 * 4 + 1] = f;
  out.M.data[2 * 4 + 2] = (far_z + near_z) / (near_z - far_z);
  out.M.data[2 * 4 + 3] = (F(2) * far_z * near_z) / (near_z - far_z);
  out.M.data[3 * 4 + 2] = F(-1);
  // M[3, 3] stays zero
  return out;
}

template <ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
orthographic_projection(F left, F right, F bottom, F top, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  out.M.data[0 * 4 + 0] = F(2) / (right - left);
  out.M.data[1 * 4 + 1] = F(2) / (top - bottom);
  out.M.data[2 * 4 + 2] = F(-2) / (far_z - near_z);
  out.M.data[0 * 4 + 3] = -(right + left) / (right - left);
  out.M.data[1 * 4 + 3] = -(top + bottom) / (top - bottom);
  out.M.data[2 * 4 + 3] = -(far_z + near_z) / (far_z - near_z);
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

template <ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
frustum(F left, F right, F bottom, F top, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  out.M.data[0 * 4 + 0] = F(2) * near_z / (right - left);
  out.M.data[1 * 4 + 1] = F(2) * near_z / (top - bottom);
  out.M.data[0 * 4 + 2] = (right + left) / (right - left);
  out.M.data[1 * 4 + 2] = (top + bottom) / (top - bottom);
  out.M.data[2 * 4 + 2] = -(far_z + near_z) / (far_z - near_z);
  out.M.data[2 * 4 + 3] = -(F(2) * far_z * near_z) / (far_z - near_z);
  out.M.data[3 * 4 + 2] = F(-1);
  return out;
}

template <ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::isometry>
look_at(const vec<F, 3> &eye, const vec<F, 3> &target, const vec<F, 3> &up) noexcept
{
  // forward = normalize(target - eye)
  vec<F, 3> f{};
  for ( usize i = 0; i < 3; ++i ) f.data[i] = target.data[i] - eye.data[i];
  F fn = math::fsqrt(f.data[0] * f.data[0] + f.data[1] * f.data[1] + f.data[2] * f.data[2]);
  if ( fn > F(0) ) {
    F inv = F(1) / fn;
    for ( usize i = 0; i < 3; ++i ) f.data[i] *= inv;
  }
  // s (side) = normalize(f x up)
  auto s = linalg::ops::cross<F>(f, up);
  F sn = math::fsqrt(s.data[0] * s.data[0] + s.data[1] * s.data[1] + s.data[2] * s.data[2]);
  if ( sn > F(0) ) {
    F inv = F(1) / sn;
    for ( usize i = 0; i < 3; ++i ) s.data[i] *= inv;
  }
  // u (true up) = s x f
  auto u = linalg::ops::cross<F>(s, f);

  transform<F, 3, transform_mode::isometry> out{};
  out.M = mat<F, 4, 4>::zero();
  // rotation part: rows are (s, u, -f)
  out.M.data[0 * 4 + 0] = s.data[0];
  out.M.data[0 * 4 + 1] = s.data[1];
  out.M.data[0 * 4 + 2] = s.data[2];
  out.M.data[1 * 4 + 0] = u.data[0];
  out.M.data[1 * 4 + 1] = u.data[1];
  out.M.data[1 * 4 + 2] = u.data[2];
  out.M.data[2 * 4 + 0] = -f.data[0];
  out.M.data[2 * 4 + 1] = -f.data[1];
  out.M.data[2 * 4 + 2] = -f.data[2];
  // translation: -R * eye
  F tx = F(0), ty = F(0), tz = F(0);
  for ( usize j = 0; j < 3; ++j ) {
    tx -= out.M.data[0 * 4 + j] * eye.data[j];
    ty -= out.M.data[1 * 4 + j] * eye.data[j];
    tz -= out.M.data[2 * 4 + j] * eye.data[j];
  }
  out.M.data[0 * 4 + 3] = tx;
  out.M.data[1 * 4 + 3] = ty;
  out.M.data[2 * 4 + 3] = tz;
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

};     // namespace geometry
};     // namespace math
};     // namespace micron
