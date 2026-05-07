//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Euler angle conversions
//
// yaw, pitch, roll [Z, Y, X]
//   R(yaw, pitch, roll) = Rz(yaw) x Ry(pitch) x Rx(roll)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../mk.hpp"
#include "../quants/vecs.hpp"
#include "algebra.hpp"

namespace micron
{
namespace math
{
namespace quaternions
{

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
from_euler_zyx(T yaw, T pitch, T roll) noexcept
{
  const T cy = math::cos<T>(yaw * T(0.5));
  const T sy = math::sin<T>(yaw * T(0.5));
  const T cp = math::cos<T>(pitch * T(0.5));
  const T sp = math::sin<T>(pitch * T(0.5));
  const T cr = math::cos<T>(roll * T(0.5));
  const T sr = math::sin<T>(roll * T(0.5));
  return quaternion<T>{ cy * cp * sr - sy * sp * cr, sy * cp * sr + cy * sp * cr, sy * cp * cr - cy * sp * sr,
                        cy * cp * cr + sy * sp * sr };
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr micron::vector_3<T>
to_euler_zyx(const quaternion<T> &q) noexcept
{
  const T x = q.x, y = q.y, z = q.z, w = q.w;
  // pitch (y-axis): asin(2(wy - zx))
  T sin_p = T(2) * (w * y - z * x);
  if ( sin_p > T(1) ) sin_p = T(1);
  if ( sin_p < T(-1) ) sin_p = T(-1);
  const T pitch = math::asin<T>(sin_p);
  T yaw, roll;
  if ( math::fabs(sin_p) >= T(0.9999999) ) {
    yaw = math::atan2<T>(T(2) * (x * y + w * z), T(1) - T(2) * (y * y + z * z));
    roll = T(0);
  } else {
    yaw = math::atan2<T>(T(2) * (w * z + x * y), T(1) - T(2) * (y * y + z * z));
    roll = math::atan2<T>(T(2) * (w * x + y * z), T(1) - T(2) * (x * x + y * y));
  }
  return micron::vector_3<T>{ yaw, pitch, roll };
}

};     // namespace quaternions
};     // namespace math
};     // namespace micron
