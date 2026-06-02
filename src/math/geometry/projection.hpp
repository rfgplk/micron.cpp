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

//   right          = OpenGL/GLM default, eye looks down -Z (current behavior)
//   left           = D3D-style, eye looks down +Z
//   neg_one_to_one = OpenGL NDC, clip z in [-1, 1]  (current behavior)
//   zero_to_one    = Vulkan/D3D/Metal/WebGPU NDC, clip z in [0, 1]  ("ZO")
enum class handedness { right, left };
enum class clip_depth { neg_one_to_one, zero_to_one };

template<handedness Hd = handedness::right, clip_depth Cd = clip_depth::neg_one_to_one, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
perspective_projection(F fov_y, F aspect, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  const F f = F(1) / math::tan(fov_y * F(0.5));
  out.M.data[0 * 4 + 0] = f / aspect;
  out.M.data[1 * 4 + 1] = f;
  if constexpr ( Hd == handedness::right ) {
    out.M.data[3 * 4 + 2] = F(-1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = (far_z + near_z) / (near_z - far_z);
      out.M.data[2 * 4 + 3] = (F(2) * far_z * near_z) / (near_z - far_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (near_z - far_z);
      out.M.data[2 * 4 + 3] = (far_z * near_z) / (near_z - far_z);
    }
  } else {
    out.M.data[3 * 4 + 2] = F(1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = (far_z + near_z) / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(F(2) * far_z * near_z) / (far_z - near_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(far_z * near_z) / (far_z - near_z);
    }
  }
  // M[3, 3] stays zero
  return out;
}

template<handedness Hd = handedness::right, clip_depth Cd = clip_depth::neg_one_to_one, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
orthographic_projection(F left, F right, F bottom, F top, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  out.M.data[0 * 4 + 0] = F(2) / (right - left);
  out.M.data[1 * 4 + 1] = F(2) / (top - bottom);
  out.M.data[0 * 4 + 3] = -(right + left) / (right - left);
  out.M.data[1 * 4 + 3] = -(top + bottom) / (top - bottom);
  if constexpr ( Cd == clip_depth::neg_one_to_one ) {
    out.M.data[2 * 4 + 2] = (Hd == handedness::right ? F(-2) : F(2)) / (far_z - near_z);
    out.M.data[2 * 4 + 3] = -(far_z + near_z) / (far_z - near_z);
  } else {
    out.M.data[2 * 4 + 2] = (Hd == handedness::right ? F(-1) : F(1)) / (far_z - near_z);
    out.M.data[2 * 4 + 3] = -near_z / (far_z - near_z);
  }
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

template<handedness Hd = handedness::right, clip_depth Cd = clip_depth::neg_one_to_one, ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
frustum(F left, F right, F bottom, F top, F near_z, F far_z) noexcept
{
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  out.M.data[0 * 4 + 0] = F(2) * near_z / (right - left);
  out.M.data[1 * 4 + 1] = F(2) * near_z / (top - bottom);
  if constexpr ( Hd == handedness::right ) {
    out.M.data[0 * 4 + 2] = (right + left) / (right - left);
    out.M.data[1 * 4 + 2] = (top + bottom) / (top - bottom);
    out.M.data[3 * 4 + 2] = F(-1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = -(far_z + near_z) / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(F(2) * far_z * near_z) / (far_z - near_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (near_z - far_z);
      out.M.data[2 * 4 + 3] = (far_z * near_z) / (near_z - far_z);
    }
  } else {
    out.M.data[0 * 4 + 2] = -(right + left) / (right - left);
    out.M.data[1 * 4 + 2] = -(top + bottom) / (top - bottom);
    out.M.data[3 * 4 + 2] = F(1);
    if constexpr ( Cd == clip_depth::neg_one_to_one ) {
      out.M.data[2 * 4 + 2] = (far_z + near_z) / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(F(2) * far_z * near_z) / (far_z - near_z);
    } else {
      out.M.data[2 * 4 + 2] = far_z / (far_z - near_z);
      out.M.data[2 * 4 + 3] = -(far_z * near_z) / (far_z - near_z);
    }
  }
  return out;
}

template<handedness Hd = handedness::right, ieee754_floating F>
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
  // s (side): RH = f x up, LH = up x f
  vec<F, 3> s{};
  if constexpr ( Hd == handedness::right )
    s = linalg::ops::cross<F>(f, up);
  else
    s = linalg::ops::cross<F>(up, f);
  F sn = math::fsqrt(s.data[0] * s.data[0] + s.data[1] * s.data[1] + s.data[2] * s.data[2]);
  if ( sn > F(0) ) {
    F inv = F(1) / sn;
    for ( usize i = 0; i < 3; ++i ) s.data[i] *= inv;
  }
  // u (true up): RH = s x f, LH = f x s
  vec<F, 3> u{};
  if constexpr ( Hd == handedness::right )
    u = linalg::ops::cross<F>(s, f);
  else
    u = linalg::ops::cross<F>(f, s);

  transform<F, 3, transform_mode::isometry> out{};
  out.M = mat<F, 4, 4>::zero();
  // rotation part: rows are (s, u, -f) for RH, (s, u, +f) for LH
  const F fz = (Hd == handedness::right) ? F(-1) : F(1);
  out.M.data[0 * 4 + 0] = s.data[0];
  out.M.data[0 * 4 + 1] = s.data[1];
  out.M.data[0 * 4 + 2] = s.data[2];
  out.M.data[1 * 4 + 0] = u.data[0];
  out.M.data[1 * 4 + 1] = u.data[1];
  out.M.data[1 * 4 + 2] = u.data[2];
  out.M.data[2 * 4 + 0] = fz * f.data[0];
  out.M.data[2 * 4 + 1] = fz * f.data[1];
  out.M.data[2 * 4 + 2] = fz * f.data[2];
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// closed-form O(1) inverses of the projection matrices above
template<ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
inv_perspective(const transform<F, 3, transform_mode::projective> &p) noexcept
{
  const F A = p.M.data[0 * 4 + 0];
  const F B = p.M.data[1 * 4 + 1];
  const F P = p.M.data[0 * 4 + 2];
  const F Q = p.M.data[1 * 4 + 2];
  const F C = p.M.data[2 * 4 + 2];
  const F D = p.M.data[2 * 4 + 3];
  const F E = p.M.data[3 * 4 + 2];
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  out.M.data[0 * 4 + 0] = F(1) / A;
  out.M.data[0 * 4 + 3] = -P / (A * E);
  out.M.data[1 * 4 + 1] = F(1) / B;
  out.M.data[1 * 4 + 3] = -Q / (B * E);
  out.M.data[2 * 4 + 3] = F(1) / E;
  out.M.data[3 * 4 + 2] = F(1) / D;
  out.M.data[3 * 4 + 3] = -C / (D * E);
  return out;
}

// inverse of an orthographic matrix; affine-diagonal layout
template<ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::projective>
inv_orthographic(const transform<F, 3, transform_mode::projective> &o) noexcept
{
  const F A = o.M.data[0 * 4 + 0];
  const F B = o.M.data[1 * 4 + 1];
  const F C = o.M.data[2 * 4 + 2];
  const F Tx = o.M.data[0 * 4 + 3];
  const F Ty = o.M.data[1 * 4 + 3];
  const F Tz = o.M.data[2 * 4 + 3];
  transform<F, 3, transform_mode::projective> out{};
  out.M = mat<F, 4, 4>::zero();
  out.M.data[0 * 4 + 0] = F(1) / A;
  out.M.data[1 * 4 + 1] = F(1) / B;
  out.M.data[2 * 4 + 2] = F(1) / C;
  out.M.data[0 * 4 + 3] = -Tx / A;
  out.M.data[1 * 4 + 3] = -Ty / B;
  out.M.data[2 * 4 + 3] = -Tz / C;
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

};      // namespace geometry
};      // namespace math
};      // namespace micron
