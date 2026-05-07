//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Hamilton algebra

#include "../../concepts.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../constants.hpp"
#include "../generic.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "../quants/vecs.hpp"
#include "../sqrt.hpp"

namespace micron
{
namespace math
{
namespace quaternions
{

// smallest |q|^2 whose reciprocal is still representable as a normal floating-point value
// anything at or below this threshold is treated as a degenerate quaternion
template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
__safe_min_n2() noexcept
{
  return ieee::from_bits<T>(typename ieee::traits<T>::uint_type(ieee::traits<T>::implicit_one));
}

template <ieee754_floating T> struct alignas(micron::math::vec_align_v<T, 4>) quaternion {
  using value_type = T;

  T x;
  T y;
  T z;
  T w;

  constexpr quaternion() noexcept : x(T(0)), y(T(0)), z(T(0)), w(T(0)) {}

  constexpr quaternion(T xx, T yy, T zz, T ww) noexcept : x(xx), y(yy), z(zz), w(ww) {}

  // identity rotation (w = 1, vec = 0)
  [[nodiscard, gnu::always_inline]] static constexpr quaternion
  identity() noexcept
  {
    return { T(0), T(0), T(0), T(1) };
  }

  // pure-vector quaternion (w = 0)
  [[nodiscard, gnu::always_inline]] static constexpr quaternion
  pure(T vx, T vy, T vz) noexcept
  {
    return { vx, vy, vz, T(0) };
  }

  [[nodiscard, gnu::always_inline]] static constexpr quaternion
  pure(const micron::vector_3<T> &v) noexcept
  {
    return { v.x, v.y, v.z, T(0) };
  }

  // scalar (real) quaternion (vec = 0)
  [[nodiscard, gnu::always_inline]] static constexpr quaternion
  scalar(T s) noexcept
  {
    return { T(0), T(0), T(0), s };
  }

  // a NaN-tagged quaternion used to surface unrecoverable inputs
  [[nodiscard, gnu::always_inline]] static constexpr quaternion
  nan() noexcept
  {
    const T n = ieee::qnan_v<T>();
    return { n, n, n, n };
  }

  [[nodiscard, gnu::always_inline]] constexpr T
  dot(const quaternion &o) const noexcept
  {
    return x * o.x + y * o.y + z * o.z + w * o.w;
  }

  [[nodiscard, gnu::always_inline]] constexpr T
  squared_norm() const noexcept
  {
    return x * x + y * y + z * z + w * w;
  }

  [[nodiscard, gnu::always_inline]] constexpr T
  magnitude() const noexcept
  {
    return math::fsqrt(squared_norm());
  }

  [[nodiscard]] constexpr bool
  all_finite() const noexcept
  {
    return ieee::is_finite<T>(x) && ieee::is_finite<T>(y) && ieee::is_finite<T>(z) && ieee::is_finite<T>(w);
  }

  [[nodiscard]] constexpr quaternion
  normalized() const noexcept
  {
    if ( !all_finite() ) return nan();
    const T n2 = squared_norm();
    if ( n2 <= __safe_min_n2<T>() ) return nan();
    const T inv = math::frsqrt(n2);
    return { x * inv, y * inv, z * inv, w * inv };
  }

  [[nodiscard]] constexpr bool
  is_normalized(T eps = math::default_eps<T>()) const noexcept
  {
    if ( !all_finite() ) return false;
    const T diff = squared_norm() - T(1);
    return math::fabs(diff) <= eps;
  }
};

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
identity() noexcept
{
  return quaternion<T>::identity();
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
multiply(const quaternion<T> &a, const quaternion<T> &b) noexcept
{
  const T ax = a.x, ay = a.y, az = a.z, aw = a.w;
  const T bx = b.x, by = b.y, bz = b.z, bw = b.w;
  return quaternion<T>{ aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx, aw * bz + ax * by - ay * bx + az * bw,
                        aw * bw - ax * bx - ay * by - az * bz };
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
compose(const quaternion<T> &a, const quaternion<T> &b) noexcept
{
  return multiply<T>(a, b);
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
conjugate(const quaternion<T> &q) noexcept
{
  return quaternion<T>{ -q.x, -q.y, -q.z, q.w };
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
inverse(const quaternion<T> &q) noexcept
{
  if ( !q.all_finite() ) return quaternion<T>::nan();
  const T n2 = q.squared_norm();
  if ( n2 <= __safe_min_n2<T>() ) return quaternion<T>::nan();
  const T inv = T(1) / n2;
  return quaternion<T>{ -q.x * inv, -q.y * inv, -q.z * inv, q.w * inv };
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
inverse_unit(const quaternion<T> &q) noexcept
{
  return conjugate<T>(q);
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
dot(const quaternion<T> &a, const quaternion<T> &b) noexcept
{
  return a.dot(b);
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
norm(const quaternion<T> &q) noexcept
{
  return q.magnitude();
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
norm_sq(const quaternion<T> &q) noexcept
{
  return q.squared_norm();
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr quaternion<T>
normalize(const quaternion<T> &q) noexcept
{
  return q.normalized();
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr bool
is_unit(const quaternion<T> &q, T tol = math::default_eps<T>()) noexcept
{
  return q.is_normalized(tol);
}

};     // namespace quaternions
};     // namespace math
};     // namespace micron
