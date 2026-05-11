//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Euler angle conversions and axis-tagged rotations
//
//  -> 12 Tait-Bryan / proper-Euler sequences (intrinsic / body-frame)
//  -> enum-tagged rotate(): one sincos + 8 mul + 4 add per call
//  -> direct Euler <-> rotation-matrix (3x3 and 4x4) routes

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../constants.hpp"
#include "../matrix/mat.hpp"
#include "../mk.hpp"
#include "../quants/vecs.hpp"
#include "algebra.hpp"

namespace micron
{
namespace math
{
namespace quaternions
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Public tag types
enum class rotations : u8 {
  // Tait-Bryan body-axis labels (aerospace)
  roll = 0,      // X-axis
  pitch = 1,     // Y-axis
  yaw = 2,       // Z-axis
  // Proper-Euler symbol aliases (same physical axes)
  alpha = roll,
  beta = pitch,
  gamma = yaw,
};

enum class frame : u8 {
  body = 0,      // intrinsic:  q' = q x dq
  world = 1,     // extrinsic:  q' = dq x q
};

enum class euler_order : u8 {
  // Tait-Bryan (3 distinct axes)
  XYZ = 0,
  XZY = 1,
  YXZ = 2,
  YZX = 3,
  ZXY = 4,
  ZYX = 5,
  // Proper-Euler (axis repeated)
  XYX = 6,
  XZX = 7,
  YXY = 8,
  YZY = 9,
  ZXZ = 10,
  ZYZ = 11,
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Internal helpers

namespace __impl_euler
{

template <rotations R>
inline constexpr u8 axis_of_v = ((R == rotations::roll) || (R == rotations::alpha))   ? u8(0)
                                : ((R == rotations::pitch) || (R == rotations::beta)) ? u8(1)
                                                                                      : u8(2);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Axis-aligned Hamilton multiply
//
// Hamilton (a x b):
//   x = aw*bx + ax*bw + ay*bz - az*by
//   y = aw*by - ax*bz + ay*bw + az*bx
//   z = aw*bz + ax*by - ay*bx + az*bw
//   w = aw*bw - ax*bx - ay*by - az*bz

// body-frame
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
mul_x_body(const quaternion<T> &q, T s, T c) noexcept
{
  return quaternion<T>{ q.w * s + q.x * c, q.y * c + q.z * s, q.z * c - q.y * s, q.w * c - q.x * s };
}

// body-frame
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
mul_y_body(const quaternion<T> &q, T s, T c) noexcept
{
  return quaternion<T>{ q.x * c - q.z * s, q.w * s + q.y * c, q.x * s + q.z * c, q.w * c - q.y * s };
}

// body-frame
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
mul_z_body(const quaternion<T> &q, T s, T c) noexcept
{
  return quaternion<T>{ q.x * c + q.y * s, q.y * c - q.x * s, q.w * s + q.z * c, q.w * c - q.z * s };
}

// world-frame
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
mul_x_world(const quaternion<T> &q, T s, T c) noexcept
{
  return quaternion<T>{ c * q.x + s * q.w, c * q.y - s * q.z, c * q.z + s * q.y, c * q.w - s * q.x };
}

// world-frame
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
mul_y_world(const quaternion<T> &q, T s, T c) noexcept
{
  return quaternion<T>{ c * q.x + s * q.z, c * q.y + s * q.w, c * q.z - s * q.x, c * q.w - s * q.y };
}

// world-frame
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
mul_z_world(const quaternion<T> &q, T s, T c) noexcept
{
  return quaternion<T>{ c * q.x - s * q.y, c * q.y + s * q.x, c * q.z + s * q.w, c * q.w - s * q.z };
}

// fast unit normalization of an already-near-unit quaternion (used by rotate_safe)
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
unit_renorm(const quaternion<T> &q) noexcept
{
  const T n2 = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
  const T inv = math::frsqrt(n2);
  return quaternion<T>{ q.x * inv, q.y * inv, q.z * inv, q.w * inv };
}

};     // namespace __impl_euler

// %%%%%%%%%%$$$$$$$$$$%%%%%%%%%%%%%%%%%%%%%%%%%%
// single-axis quaternion builders

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
quat_x(T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  return quaternion<T>{ s, T(0), T(0), c };
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
quat_y(T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  return quaternion<T>{ T(0), s, T(0), c };
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
quat_z(T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  return quaternion<T>{ T(0), T(0), s, c };
}

template <rotations Axis, ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
axis_quat(T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  if constexpr ( __impl_euler::axis_of_v<Axis> == u8(0) )
    return quaternion<T>{ s, T(0), T(0), c };
  else if constexpr ( __impl_euler::axis_of_v<Axis> == u8(1) )
    return quaternion<T>{ T(0), s, T(0), c };
  else
    return quaternion<T>{ T(0), T(0), s, c };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rotate()

template <rotations Axis, frame F = frame::body, ieee754_floating T>
[[gnu::always_inline]] inline constexpr void
rotate(quaternion<T> &q, T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  if constexpr ( F == frame::body ) {
    if constexpr ( __impl_euler::axis_of_v<Axis> == u8(0) )
      q = __impl_euler::mul_x_body<T>(q, s, c);
    else if constexpr ( __impl_euler::axis_of_v<Axis> == u8(1) )
      q = __impl_euler::mul_y_body<T>(q, s, c);
    else
      q = __impl_euler::mul_z_body<T>(q, s, c);
  } else {
    if constexpr ( __impl_euler::axis_of_v<Axis> == u8(0) )
      q = __impl_euler::mul_x_world<T>(q, s, c);
    else if constexpr ( __impl_euler::axis_of_v<Axis> == u8(1) )
      q = __impl_euler::mul_y_world<T>(q, s, c);
    else
      q = __impl_euler::mul_z_world<T>(q, s, c);
  }
}

template <rotations Axis, frame F = frame::body, ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
rotated(const quaternion<T> &q, T angle) noexcept
{
  quaternion<T> r = q;
  rotate<Axis, F, T>(r, angle);
  return r;
}

template <rotations Axis, frame F = frame::body, ieee754_floating T>
[[gnu::always_inline]] inline constexpr void
rotate_safe(quaternion<T> &q, T angle) noexcept
{
  rotate<Axis, F, T>(q, angle);
  q = __impl_euler::unit_renorm<T>(q);
}

template <rotations Axis, frame F = frame::body, ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
rotated_safe(const quaternion<T> &q, T angle) noexcept
{
  quaternion<T> r = q;
  rotate_safe<Axis, F, T>(r, angle);
  return r;
}

template <ieee754_floating T>
inline constexpr void
rotate(quaternion<T> &q, rotations axis, T angle, frame f = frame::body) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  const u8 ax = ((axis == rotations::roll) || (axis == rotations::alpha))   ? u8(0)
                : ((axis == rotations::pitch) || (axis == rotations::beta)) ? u8(1)
                                                                            : u8(2);
  if ( f == frame::body ) {
    if ( ax == u8(0) )
      q = __impl_euler::mul_x_body<T>(q, s, c);
    else if ( ax == u8(1) )
      q = __impl_euler::mul_y_body<T>(q, s, c);
    else
      q = __impl_euler::mul_z_body<T>(q, s, c);
  } else {
    if ( ax == u8(0) )
      q = __impl_euler::mul_x_world<T>(q, s, c);
    else if ( ax == u8(1) )
      q = __impl_euler::mul_y_world<T>(q, s, c);
    else
      q = __impl_euler::mul_z_world<T>(q, s, c);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// single-axis vector rotation
template <rotations Axis, ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr micron::vector_3<T>
rotate_axis(const micron::vector_3<T> &v, T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle, s, c);
  if constexpr ( __impl_euler::axis_of_v<Axis> == u8(0) ) {
    return micron::vector_3<T>{ v.x, c * v.y - s * v.z, s * v.y + c * v.z };
  } else if constexpr ( __impl_euler::axis_of_v<Axis> == u8(1) ) {
    return micron::vector_3<T>{ c * v.x + s * v.z, v.y, -s * v.x + c * v.z };
  } else {
    return micron::vector_3<T>{ c * v.x - s * v.y, s * v.x + c * v.y, v.z };
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%#%%%%%%%%%%%%
// euler to quaternion (all 12 sequences, closed form)
template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
from_euler(T a, T b, T c) noexcept
{
  T s1, c1, s2, c2, s3, c3;
  math::sincos<T>(a * T(0.5), s1, c1);
  math::sincos<T>(b * T(0.5), s2, c2);
  math::sincos<T>(c * T(0.5), s3, c3);

  // Tait-Bryan
  if constexpr ( Ord == euler_order::XYZ ) {
    return quaternion<T>{ s1 * c2 * c3 + c1 * s2 * s3, c1 * s2 * c3 - s1 * c2 * s3, c1 * c2 * s3 + s1 * s2 * c3,
                          c1 * c2 * c3 - s1 * s2 * s3 };
  } else if constexpr ( Ord == euler_order::XZY ) {
    return quaternion<T>{ s1 * c2 * c3 - c1 * s2 * s3, c1 * c2 * s3 - s1 * s2 * c3, c1 * s2 * c3 + s1 * c2 * s3,
                          c1 * c2 * c3 + s1 * s2 * s3 };
  } else if constexpr ( Ord == euler_order::YXZ ) {
    return quaternion<T>{ c1 * s2 * c3 + s1 * c2 * s3, s1 * c2 * c3 - c1 * s2 * s3, c1 * c2 * s3 - s1 * s2 * c3,
                          c1 * c2 * c3 + s1 * s2 * s3 };
  } else if constexpr ( Ord == euler_order::YZX ) {
    return quaternion<T>{ c1 * c2 * s3 + s1 * s2 * c3, s1 * c2 * c3 + c1 * s2 * s3, c1 * s2 * c3 - s1 * c2 * s3,
                          c1 * c2 * c3 - s1 * s2 * s3 };
  } else if constexpr ( Ord == euler_order::ZXY ) {
    return quaternion<T>{ c1 * s2 * c3 - s1 * c2 * s3, c1 * c2 * s3 + s1 * s2 * c3, s1 * c2 * c3 + c1 * s2 * s3,
                          c1 * c2 * c3 - s1 * s2 * s3 };
  } else if constexpr ( Ord == euler_order::ZYX ) {
    return quaternion<T>{ c1 * c2 * s3 - s1 * s2 * c3, c1 * s2 * c3 + s1 * c2 * s3, s1 * c2 * c3 - c1 * s2 * s3,
                          c1 * c2 * c3 + s1 * s2 * s3 };
    // true Euler
  } else if constexpr ( Ord == euler_order::XYX ) {
    return quaternion<T>{ c1 * c2 * s3 + s1 * c2 * c3, c1 * s2 * c3 + s1 * s2 * s3, s1 * s2 * c3 - c1 * s2 * s3,
                          c1 * c2 * c3 - s1 * c2 * s3 };
  } else if constexpr ( Ord == euler_order::XZX ) {
    return quaternion<T>{ c1 * c2 * s3 + s1 * c2 * c3, c1 * s2 * s3 - s1 * s2 * c3, s1 * s2 * s3 + c1 * s2 * c3,
                          c1 * c2 * c3 - s1 * c2 * s3 };
  } else if constexpr ( Ord == euler_order::YXY ) {
    return quaternion<T>{ c1 * s2 * c3 + s1 * s2 * s3, c1 * c2 * s3 + s1 * c2 * c3, c1 * s2 * s3 - s1 * s2 * c3,
                          c1 * c2 * c3 - s1 * c2 * s3 };
  } else if constexpr ( Ord == euler_order::YZY ) {
    return quaternion<T>{ s1 * s2 * c3 - c1 * s2 * s3, c1 * c2 * s3 + s1 * c2 * c3, s1 * s2 * s3 + c1 * s2 * c3,
                          c1 * c2 * c3 - s1 * c2 * s3 };
  } else if constexpr ( Ord == euler_order::ZXZ ) {
    return quaternion<T>{ c1 * s2 * c3 + s1 * s2 * s3, s1 * s2 * c3 - c1 * s2 * s3, c1 * c2 * s3 + s1 * c2 * c3,
                          c1 * c2 * c3 - s1 * c2 * s3 };
  } else {     // ZYZ
    return quaternion<T>{ c1 * s2 * s3 - s1 * s2 * c3, c1 * s2 * c3 + s1 * s2 * s3, c1 * c2 * s3 + s1 * c2 * c3,
                          c1 * c2 * c3 - s1 * c2 * s3 };
  }
}

template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
from_euler(const micron::vector_3<T> &abc) noexcept
{
  return from_euler<Ord, T>(abc.x, abc.y, abc.z);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// euler to rotation matrix 3x3 (all 12 sequences, closed form)

template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr mat<T, 3, 3>
from_euler_matrix(T a, T b, T c) noexcept
{
  T sa, ca, sb, cb, sg, cg;
  math::sincos<T>(a, sa, ca);
  math::sincos<T>(b, sb, cb);
  math::sincos<T>(c, sg, cg);
  mat<T, 3, 3> R{};
  T *__restrict__ d = R.data;

  if constexpr ( Ord == euler_order::XYZ ) {
    d[0] = cb * cg;
    d[1] = -cb * sg;
    d[2] = sb;
    d[3] = sa * sb * cg + ca * sg;
    d[4] = ca * cg - sa * sb * sg;
    d[5] = -sa * cb;
    d[6] = sa * sg - ca * sb * cg;
    d[7] = ca * sb * sg + sa * cg;
    d[8] = ca * cb;
  } else if constexpr ( Ord == euler_order::XZY ) {
    d[0] = cb * cg;
    d[1] = -sb;
    d[2] = cb * sg;
    d[3] = ca * sb * cg + sa * sg;
    d[4] = ca * cb;
    d[5] = ca * sb * sg - sa * cg;
    d[6] = sa * sb * cg - ca * sg;
    d[7] = sa * cb;
    d[8] = sa * sb * sg + ca * cg;
  } else if constexpr ( Ord == euler_order::YXZ ) {
    d[0] = ca * cg + sa * sb * sg;
    d[1] = sa * sb * cg - ca * sg;
    d[2] = sa * cb;
    d[3] = cb * sg;
    d[4] = cb * cg;
    d[5] = -sb;
    d[6] = ca * sb * sg - sa * cg;
    d[7] = sa * sg + ca * sb * cg;
    d[8] = ca * cb;
  } else if constexpr ( Ord == euler_order::YZX ) {
    d[0] = ca * cb;
    d[1] = sa * sg - ca * sb * cg;
    d[2] = ca * sb * sg + sa * cg;
    d[3] = sb;
    d[4] = cb * cg;
    d[5] = -cb * sg;
    d[6] = -sa * cb;
    d[7] = sa * sb * cg + ca * sg;
    d[8] = ca * cg - sa * sb * sg;
  } else if constexpr ( Ord == euler_order::ZXY ) {
    d[0] = ca * cg - sa * sb * sg;
    d[1] = -sa * cb;
    d[2] = ca * sg + sa * sb * cg;
    d[3] = sa * cg + ca * sb * sg;
    d[4] = ca * cb;
    d[5] = sa * sg - ca * sb * cg;
    d[6] = -cb * sg;
    d[7] = sb;
    d[8] = cb * cg;
  } else if constexpr ( Ord == euler_order::ZYX ) {
    d[0] = ca * cb;
    d[1] = ca * sb * sg - sa * cg;
    d[2] = ca * sb * cg + sa * sg;
    d[3] = sa * cb;
    d[4] = sa * sb * sg + ca * cg;
    d[5] = sa * sb * cg - ca * sg;
    d[6] = -sb;
    d[7] = cb * sg;
    d[8] = cb * cg;
  } else if constexpr ( Ord == euler_order::XYX ) {
    d[0] = cb;
    d[1] = sb * sg;
    d[2] = sb * cg;
    d[3] = sa * sb;
    d[4] = ca * cg - sa * cb * sg;
    d[5] = -ca * sg - sa * cb * cg;
    d[6] = -ca * sb;
    d[7] = sa * cg + ca * cb * sg;
    d[8] = ca * cb * cg - sa * sg;
  } else if constexpr ( Ord == euler_order::XZX ) {
    d[0] = cb;
    d[1] = -sb * cg;
    d[2] = sb * sg;
    d[3] = ca * sb;
    d[4] = ca * cb * cg - sa * sg;
    d[5] = -ca * cb * sg - sa * cg;
    d[6] = sa * sb;
    d[7] = sa * cb * cg + ca * sg;
    d[8] = ca * cg - sa * cb * sg;
  } else if constexpr ( Ord == euler_order::YXY ) {
    d[0] = ca * cg - sa * cb * sg;
    d[1] = sa * sb;
    d[2] = ca * sg + sa * cb * cg;
    d[3] = sb * sg;
    d[4] = cb;
    d[5] = -sb * cg;
    d[6] = -sa * cg - ca * cb * sg;
    d[7] = ca * sb;
    d[8] = ca * cb * cg - sa * sg;
  } else if constexpr ( Ord == euler_order::YZY ) {
    d[0] = ca * cb * cg - sa * sg;
    d[1] = -ca * sb;
    d[2] = ca * cb * sg + sa * cg;
    d[3] = sb * cg;
    d[4] = cb;
    d[5] = sb * sg;
    d[6] = -sa * cb * cg - ca * sg;
    d[7] = sa * sb;
    d[8] = ca * cg - sa * cb * sg;
  } else if constexpr ( Ord == euler_order::ZXZ ) {
    d[0] = ca * cg - sa * cb * sg;
    d[1] = -ca * sg - sa * cb * cg;
    d[2] = sa * sb;
    d[3] = sa * cg + ca * cb * sg;
    d[4] = ca * cb * cg - sa * sg;
    d[5] = -ca * sb;
    d[6] = sb * sg;
    d[7] = sb * cg;
    d[8] = cb;
  } else {     // ZYZ
    d[0] = ca * cb * cg - sa * sg;
    d[1] = -ca * cb * sg - sa * cg;
    d[2] = ca * sb;
    d[3] = sa * cb * cg + ca * sg;
    d[4] = ca * cg - sa * cb * sg;
    d[5] = sa * sb;
    d[6] = -sb * cg;
    d[7] = sb * sg;
    d[8] = cb;
  }
  return R;
}

template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr mat<T, 3, 3>
from_euler_matrix(const micron::vector_3<T> &abc) noexcept
{
  return from_euler_matrix<Ord, T>(abc.x, abc.y, abc.z);
}

template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr mat<T, 4, 4>
from_euler_matrix4(T a, T b, T c) noexcept
{
  const mat<T, 3, 3> R = from_euler_matrix<Ord, T>(a, b, c);
  mat<T, 4, 4> M{};
  T *__restrict__ d = M.data;
  const T *__restrict__ r = R.data;
  d[0] = r[0];
  d[1] = r[1];
  d[2] = r[2];
  d[3] = T(0);
  d[4] = r[3];
  d[5] = r[4];
  d[6] = r[5];
  d[7] = T(0);
  d[8] = r[6];
  d[9] = r[7];
  d[10] = r[8];
  d[11] = T(0);
  d[12] = T(0);
  d[13] = T(0);
  d[14] = T(0);
  d[15] = T(1);
  return M;
}

template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr mat<T, 4, 4>
from_euler_matrix4(const micron::vector_3<T> &abc) noexcept
{
  return from_euler_matrix4<Ord, T>(abc.x, abc.y, abc.z);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rotation matrix to Euler (all 12 sequences)

namespace __impl_euler
{

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
clamp_unit(T x) noexcept
{
  if ( x > T(1) ) return T(1);
  if ( x < T(-1) ) return T(-1);
  return x;
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
gimbal_floor() noexcept
{
  return T(1) - math::default_eps<T>();
}

};     // namespace __impl_euler

template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr micron::vector_3<T>
to_euler_matrix(const mat<T, 3, 3> &R) noexcept
{
  const T *__restrict__ d = R.data;
  const T R00 = d[0], R01 = d[1], R02 = d[2];
  const T R10 = d[3], R11 = d[4], R12 = d[5];
  const T R20 = d[6], R21 = d[7], R22 = d[8];
  const T glim = __impl_euler::gimbal_floor<T>();
  T a, b, c;

  // NOTE: gimbal-lock formulas are unified across both beta branches per sequence
  // each chosen atan2 pair gives sin/cos of the merged angle with the
  // correct sign in both gimbal poles, so no further sign branching needed
  if constexpr ( Ord == euler_order::XYZ ) {
    const T sb = __impl_euler::clamp_unit<T>(R02);
    b = math::asin<T>(sb);
    if ( math::fabs(sb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(R21, R11);
    } else {
      a = math::atan2<T>(-R12, R22);
      c = math::atan2<T>(-R01, R00);
    }
  } else if constexpr ( Ord == euler_order::XZY ) {
    const T sb = __impl_euler::clamp_unit<T>(-R01);
    b = math::asin<T>(sb);
    if ( math::fabs(sb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(-R12, R22);
    } else {
      a = math::atan2<T>(R21, R11);
      c = math::atan2<T>(R02, R00);
    }
  } else if constexpr ( Ord == euler_order::YXZ ) {
    const T sb = __impl_euler::clamp_unit<T>(-R12);
    b = math::asin<T>(sb);
    if ( math::fabs(sb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(-R20, R00);
    } else {
      a = math::atan2<T>(R02, R22);
      c = math::atan2<T>(R10, R11);
    }
  } else if constexpr ( Ord == euler_order::YZX ) {
    const T sb = __impl_euler::clamp_unit<T>(R10);
    b = math::asin<T>(sb);
    if ( math::fabs(sb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(R02, R22);
    } else {
      a = math::atan2<T>(-R20, R00);
      c = math::atan2<T>(-R12, R11);
    }
  } else if constexpr ( Ord == euler_order::ZXY ) {
    const T sb = __impl_euler::clamp_unit<T>(R21);
    b = math::asin<T>(sb);
    if ( math::fabs(sb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(R10, R00);
    } else {
      a = math::atan2<T>(-R01, R11);
      c = math::atan2<T>(-R20, R22);
    }
  } else if constexpr ( Ord == euler_order::ZYX ) {
    const T sb = __impl_euler::clamp_unit<T>(-R20);
    b = math::asin<T>(sb);
    if ( math::fabs(sb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(-R01, R11);
    } else {
      a = math::atan2<T>(R10, R00);
      c = math::atan2<T>(R21, R22);
    }
  } else if constexpr ( Ord == euler_order::XYX ) {
    const T cb = __impl_euler::clamp_unit<T>(R00);
    b = math::acos<T>(cb);
    if ( math::fabs(cb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(R21, R11);
    } else {
      a = math::atan2<T>(R10, -R20);
      c = math::atan2<T>(R01, R02);
    }
  } else if constexpr ( Ord == euler_order::XZX ) {
    const T cb = __impl_euler::clamp_unit<T>(R00);
    b = math::acos<T>(cb);
    if ( math::fabs(cb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(-R12, R22);
    } else {
      a = math::atan2<T>(R20, R10);
      c = math::atan2<T>(R02, -R01);
    }
  } else if constexpr ( Ord == euler_order::YXY ) {
    const T cb = __impl_euler::clamp_unit<T>(R11);
    b = math::acos<T>(cb);
    if ( math::fabs(cb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(-R20, R00);
    } else {
      a = math::atan2<T>(R01, R21);
      c = math::atan2<T>(R10, -R12);
    }
  } else if constexpr ( Ord == euler_order::YZY ) {
    const T cb = __impl_euler::clamp_unit<T>(R11);
    b = math::acos<T>(cb);
    if ( math::fabs(cb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(R02, R22);
    } else {
      a = math::atan2<T>(R21, -R01);
      c = math::atan2<T>(R12, R10);
    }
  } else if constexpr ( Ord == euler_order::ZXZ ) {
    const T cb = __impl_euler::clamp_unit<T>(R22);
    b = math::acos<T>(cb);
    if ( math::fabs(cb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(R10, R00);
    } else {
      a = math::atan2<T>(R02, -R12);
      c = math::atan2<T>(R20, R21);
    }
  } else {     // ZYZ
    const T cb = __impl_euler::clamp_unit<T>(R22);
    b = math::acos<T>(cb);
    if ( math::fabs(cb) >= glim ) {
      c = T(0);
      a = math::atan2<T>(-R01, R11);
    } else {
      a = math::atan2<T>(R12, R02);
      c = math::atan2<T>(R21, -R20);
    }
  }
  return micron::vector_3<T>{ a, b, c };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// quaternion to euler (all 12 sequences)

template <euler_order Ord, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr micron::vector_3<T>
to_euler(const quaternion<T> &q) noexcept
{
  const T x = q.x, y = q.y, z = q.z, w = q.w;
  const T xx = x * x, yy = y * y, zz = z * z;
  const T xy = x * y, xz = x * z, yz = y * z;
  const T wx = w * x, wy = w * y, wz = w * z;
  // matrix elements (lazy-eval per sequence)
  const T R00 = T(1) - T(2) * (yy + zz);
  const T R01 = T(2) * (xy - wz);
  const T R02 = T(2) * (xz + wy);
  const T R10 = T(2) * (xy + wz);
  const T R11 = T(1) - T(2) * (xx + zz);
  const T R12 = T(2) * (yz - wx);
  const T R20 = T(2) * (xz - wy);
  const T R21 = T(2) * (yz + wx);
  const T R22 = T(1) - T(2) * (xx + yy);
  mat<T, 3, 3> R{};
  R.data[0] = R00;
  R.data[1] = R01;
  R.data[2] = R02;
  R.data[3] = R10;
  R.data[4] = R11;
  R.data[5] = R12;
  R.data[6] = R20;
  R.data[7] = R21;
  R.data[8] = R22;
  return to_euler_matrix<Ord, T>(R);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// orientate3 / orientate4 / orientate
// GLM style

template <euler_order Ord = euler_order::YXZ, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr mat<T, 3, 3>
orientate3(const micron::vector_3<T> &angles) noexcept
{
  return from_euler_matrix<Ord, T>(angles.x, angles.y, angles.z);
}

template <euler_order Ord = euler_order::YXZ, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr mat<T, 4, 4>
orientate4(const micron::vector_3<T> &angles) noexcept
{
  return from_euler_matrix4<Ord, T>(angles.x, angles.y, angles.z);
}

template <euler_order Ord = euler_order::YXZ, ieee754_floating T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
orientate(const micron::vector_3<T> &angles) noexcept
{
  return from_euler<Ord, T>(angles.x, angles.y, angles.z);
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
from_euler_zyx(T yaw, T pitch, T roll) noexcept
{
  return from_euler<euler_order::ZYX, T>(yaw, pitch, roll);
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr micron::vector_3<T>
to_euler_zyx(const quaternion<T> &q) noexcept
{
  return to_euler<euler_order::ZYX, T>(q);
}

};     // namespace quaternions
};     // namespace math
};     // namespace micron
