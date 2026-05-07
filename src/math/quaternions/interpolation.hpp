//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "algebra.hpp"

namespace micron
{
namespace math
{
namespace quaternions
{

namespace __impl_quaternions_interpolation
{

template <typename T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
flip_to_short_arc(const quaternion<T> &a, const quaternion<T> &b) noexcept
{
  return (a.dot(b) < T(0)) ? quaternion<T>{ -b.x, -b.y, -b.z, -b.w } : b;
}

};     // namespace __impl_quaternions_interpolation

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
lerp(const quaternion<T> &q0, const quaternion<T> &q1, micron::__type_identity_t<T> t) noexcept
{
  const auto q1f = __impl_quaternions_interpolation::flip_to_short_arc<T>(q0, q1);
  const T u = T(1) - t;
  return quaternion<T>{ q0.x * u + q1f.x * t, q0.y * u + q1f.y * t, q0.z * u + q1f.z * t, q0.w * u + q1f.w * t };
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
nlerp(const quaternion<T> &q0, const quaternion<T> &q1, micron::__type_identity_t<T> t) noexcept
{
  return lerp<T>(q0, q1, t).normalized();
}

template <typename T>
[[nodiscard, gnu::flatten]] inline constexpr quaternion<T>
slerp(const quaternion<T> &q0, const quaternion<T> &q1, micron::__type_identity_t<T> t) noexcept
{
  auto q1f = __impl_quaternions_interpolation::flip_to_short_arc<T>(q0, q1);
  T cos_theta = q0.dot(q1f);
  if ( cos_theta > T(1) ) cos_theta = T(1);
  if ( cos_theta < T(-1) ) cos_theta = T(-1);
  const T near_one = T(1) - math::default_eps<T>() * T(8);
  if ( cos_theta > near_one ) {
    return nlerp<T>(q0, q1f, t);
  }
  const T theta = math::acos<T>(cos_theta);
  const T inv_sin = T(1) / math::sin<T>(theta);
  const T s0 = math::sin<T>((T(1) - t) * theta) * inv_sin;
  const T s1 = math::sin<T>(t * theta) * inv_sin;
  return quaternion<T>{ q0.x * s0 + q1f.x * s1, q0.y * s0 + q1f.y * s1, q0.z * s0 + q1f.z * s1, q0.w * s0 + q1f.w * s1 };
}

};     // namespace quaternions
};     // namespace math
};     // namespace micron
