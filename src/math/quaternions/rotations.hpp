//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../blas/identities.hpp"
#include "../constants.hpp"
#include "../generic.hpp"
#include "../ieee.hpp"
#include "../matrix/mat.hpp"
#include "../mk.hpp"
#include "../quants/quat.hpp"
#include "../quants/vec.hpp"
#include "../quants/vecs.hpp"
#include "../sqrt.hpp"
#include "../trig.hpp"
#include "algebra.hpp"

namespace micron
{
namespace math
{
namespace quaternions
{

template<ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
from_axis_angle(T ax, T ay, T az, T angle) noexcept
{
  const T aax = math::fabs(ax);
  const T aay = math::fabs(ay);
  const T aaz = math::fabs(az);
  const T m = math::fmax(aax, math::fmax(aay, aaz));
  if ( m == T(0) ) return identity<T>();
  const T inv_m = T(1) / m;
  const T sx = ax * inv_m;
  const T sy = ay * inv_m;
  const T sz = az * inv_m;
  const T inv_n = math::frsqrt(sx * sx + sy * sy + sz * sz) * inv_m;
  const T half = angle * T(0.5);
  const T s = math::sin<T>(half);
  const T c = math::cos<T>(half);
  const T k = s * inv_n;
  return quaternion<T>{ ax * k, ay * k, az * k, c };
}

template<ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
from_axis_angle(const micron::vector_3<T> &axis, T angle) noexcept
{
  return from_axis_angle<T>(axis.x, axis.y, axis.z, angle);
}

template<ieee754_floating T> struct axis_angle_t {
  micron::vector_3<T> axis;
  T angle;
};

// NOTE: uses atan2(|v|, w) instead of acos(w) for numerical stability (slower)
// returns angle in [0, 2pi] with the recovered axis chosen so that from_axis_angle(axis, angle) reconstructs the input quaternion
template<ieee754_floating T>
[[nodiscard]] inline constexpr axis_angle_t<T>
to_axis_angle(const quaternion<T> &q) noexcept
{
  const T ax = math::fabs(q.x);
  const T ay = math::fabs(q.y);
  const T az = math::fabs(q.z);
  const T aw = math::fabs(q.w);
  const T m = math::fmax(math::fmax(ax, ay), math::fmax(az, aw));
  if ( m == T(0) ) {
    return axis_angle_t<T>{ micron::vector_3<T>{ T(1), T(0), T(0) }, T(0) };
  }
  const T inv_m = T(1) / m;
  const T sx = q.x * inv_m;
  const T sy = q.y * inv_m;
  const T sz = q.z * inv_m;
  const T sw = q.w * inv_m;
  const T inv_n = math::frsqrt(sx * sx + sy * sy + sz * sz + sw * sw) * inv_m;
  const T nx = q.x * inv_n;
  const T ny = q.y * inv_n;
  const T nz = q.z * inv_n;
  const T nw = q.w * inv_n;
  const T v2 = nx * nx + ny * ny + nz * nz;
  const T v_norm = math::fsqrt(v2);
  if ( v_norm < math::default_eps<T>() ) {
    return axis_angle_t<T>{ micron::vector_3<T>{ T(1), T(0), T(0) }, T(0) };
  }
  const T half_angle = math::atan2<T>(v_norm, nw);
  const T inv_v = T(1) / v_norm;
  return axis_angle_t<T>{ micron::vector_3<T>{ nx * inv_v, ny * inv_v, nz * inv_v }, T(2) * half_angle };
}

// NOTE: faster acos(w)-based decomposition
template<ieee754_floating T>
[[nodiscard]] inline constexpr axis_angle_t<T>
to_axis_angle_fast(const quaternion<T> &q) noexcept
{
  T w_clamped = q.w;
  if ( w_clamped > T(1) ) w_clamped = T(1);
  if ( w_clamped < T(-1) ) w_clamped = T(-1);
  const T half_angle = math::acos<T>(w_clamped);
  const T s = math::sin<T>(half_angle);
  if ( math::fabs(s) < math::default_eps<T>() ) {
    return axis_angle_t<T>{ micron::vector_3<T>{ T(1), T(0), T(0) }, T(0) };
  }
  const T inv_s = T(1) / s;
  return axis_angle_t<T>{ micron::vector_3<T>{ q.x * inv_s, q.y * inv_s, q.z * inv_s }, T(2) * half_angle };
}

// Shepperd's method
template<ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
from_matrix(const mat<T, 3, 3> &R) noexcept
{
  const T *__restrict__ d = R.data;
  const T m00 = d[0], m01 = d[1], m02 = d[2];
  const T m10 = d[3], m11 = d[4], m12 = d[5];
  const T m20 = d[6], m21 = d[7], m22 = d[8];
  const T tr = m00 + m11 + m22;
  const T eps = math::default_eps<T>();
  T x, y, z, w;
  if ( tr > T(0) ) {
    const T r = tr + T(1);
    const T s = math::fsqrt(r > T(0) ? r : T(0)) * T(2);
    const T inv_s = (s > eps) ? T(1) / s : T(0);
    w = T(0.25) * s;
    x = (m21 - m12) * inv_s;
    y = (m02 - m20) * inv_s;
    z = (m10 - m01) * inv_s;
  } else if ( m00 > m11 && m00 > m22 ) {
    const T r = T(1) + m00 - m11 - m22;
    const T s = math::fsqrt(r > T(0) ? r : T(0)) * T(2);
    const T inv_s = (s > eps) ? T(1) / s : T(0);
    x = T(0.25) * s;
    y = (m01 + m10) * inv_s;
    z = (m02 + m20) * inv_s;
    w = (m21 - m12) * inv_s;
  } else if ( m11 > m22 ) {
    const T r = T(1) + m11 - m00 - m22;
    const T s = math::fsqrt(r > T(0) ? r : T(0)) * T(2);
    const T inv_s = (s > eps) ? T(1) / s : T(0);
    y = T(0.25) * s;
    x = (m01 + m10) * inv_s;
    z = (m12 + m21) * inv_s;
    w = (m02 - m20) * inv_s;
  } else {
    const T r = T(1) + m22 - m00 - m11;
    const T s = math::fsqrt(r > T(0) ? r : T(0)) * T(2);
    const T inv_s = (s > eps) ? T(1) / s : T(0);
    z = T(0.25) * s;
    x = (m02 + m20) * inv_s;
    y = (m12 + m21) * inv_s;
    w = (m10 - m01) * inv_s;
  }
  const T n2 = x * x + y * y + z * z + w * w;
  if ( n2 == T(0) ) return identity<T>();
  const T inv_n = math::frsqrt(n2);
  return quaternion<T>{ x * inv_n, y * inv_n, z * inv_n, w * inv_n };
}

template<ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr mat<T, 3, 3>
to_matrix(const quaternion<T> &q) noexcept
{
  quat<T> qq{};
  qq.data[0] = q.x;
  qq.data[1] = q.y;
  qq.data[2] = q.z;
  qq.data[3] = q.w;
  return blas::identities::from_quat<T>(qq);
}

// v' = (2w^2/|q|^2 - 1) v + (2 (u . v)/|q|^2) u + (2 w/|q|^2) (u x v)
template<ieee754_floating T>
[[nodiscard]] inline constexpr micron::vector_3<T>
rotate(const quaternion<T> &q, const micron::vector_3<T> &v) noexcept
{
  const T ux = q.x, uy = q.y, uz = q.z, w = q.w;
  const T n2 = ux * ux + uy * uy + uz * uz + w * w;
  if ( n2 == T(0) ) return v;
  const T inv_n2 = T(1) / n2;
  const T two_inv_n2 = T(2) * inv_n2;
  const T a = w * w * two_inv_n2 - T(1);
  const T b = (ux * v.x + uy * v.y + uz * v.z) * two_inv_n2;
  const T c = w * two_inv_n2;
  const T cx = uy * v.z - uz * v.y;
  const T cy = uz * v.x - ux * v.z;
  const T cz = ux * v.y - uy * v.x;
  return micron::vector_3<T>{ a * v.x + b * ux + c * cx, a * v.y + b * uy + c * cy, a * v.z + b * uz + c * cz };
}

template<ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
x_quat(T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  return quaternion<T>{ s, T(0), T(0), c };
}

template<ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
y_quat(T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  return quaternion<T>{ T(0), s, T(0), c };
}

template<ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
z_quat(T angle) noexcept
{
  T s, c;
  math::sincos<T>(angle * T(0.5), s, c);
  return quaternion<T>{ T(0), T(0), s, c };
}

};      // namespace quaternions
};      // namespace math
};      // namespace micron
