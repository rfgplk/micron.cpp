//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// quaternion time-evolution
//
// -> derivative(q, omega)
// -> integrate(q, omega, dt)
// -> integrate_small_angle()
// -> integrate_pade()
// -> angular_velocity(q0, q1, dt)
// -> angular_velocity_pade()

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../constants.hpp"
#include "../generic.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "../quants/vecs.hpp"
#include "../sqrt.hpp"
#include "../trig.hpp"
#include "algebra.hpp"
#include "rotations.hpp"

namespace micron
{
namespace math
{
namespace quaternions
{

namespace __impl_kinematics
{

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
small_half_phi_sq() noexcept
{
  return T(1e-4);
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr T
log_pade_cw_floor() noexcept
{
  return T(0.5);
}

template <ieee754_floating T>
[[nodiscard, gnu::always_inline]] inline constexpr micron::vector_3<T>
nan_vec() noexcept
{
  const T n = ieee::qnan_v<T>();
  return micron::vector_3<T>{ n, n, n };
}

};     // namespace __impl_kinematics

// q_dot = (1/2) * q (X) (omega, 0)
// NOTE: omega is the angular velocity in the BODY frame
// For a world-frame angular velocity omega_w, use (1/2) * (omega_w, 0) (X) q
template <ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
derivative(const quaternion<T> &q, const micron::vector_3<T> &omega) noexcept
{
  const quaternion<T> w_q{ omega.x, omega.y, omega.z, T(0) };
  const auto p = multiply<T>(q, w_q);
  return quaternion<T>{ p.x * T(0.5), p.y * T(0.5), p.z * T(0.5), p.w * T(0.5) };
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
integrate(const quaternion<T> &q, const micron::vector_3<T> &omega, T dt) noexcept
{
  const T n2 = omega.x * omega.x + omega.y * omega.y + omega.z * omega.z;
  const T half_phi2 = n2 * dt * dt * T(0.25);

  T sx, sy, sz, c;
  if ( half_phi2 < __impl_kinematics::small_half_phi_sq<T>() ) {
    const T hp4 = half_phi2 * half_phi2;
    const T axis_scale = T(0.5) * dt * (T(1) - half_phi2 * (T(1) / T(6)) + hp4 * (T(1) / T(120)));
    c = T(1) - half_phi2 * T(0.5) + hp4 * (T(1) / T(24));
    sx = omega.x * axis_scale;
    sy = omega.y * axis_scale;
    sz = omega.z * axis_scale;
  } else {
    const T n = math::fsqrt(n2);
    const T half_phi = T(0.5) * n * dt;
    const T s = math::sin<T>(half_phi);
    c = math::cos<T>(half_phi);
    const T axis_scale = s / n;
    sx = omega.x * axis_scale;
    sy = omega.y * axis_scale;
    sz = omega.z * axis_scale;
  }
  const quaternion<T> dq{ sx, sy, sz, c };
  return multiply<T>(q, dq).normalized();
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
integrate_small_angle(const quaternion<T> &q, const micron::vector_3<T> &omega, T dt) noexcept
{
  const T n2 = omega.x * omega.x + omega.y * omega.y + omega.z * omega.z;
  const T phi2 = n2 * dt * dt;
  const T phi4 = phi2 * phi2;
  const T s = T(0.5) * dt * (T(1) - phi2 * (T(1) / T(24)) + phi4 * (T(1) / T(1920)));
  const T c = T(1) - phi2 * T(0.125) + phi4 * (T(1) / T(384));
  const quaternion<T> dq{ omega.x * s, omega.y * s, omega.z * s, c };
  return multiply<T>(q, dq).normalized();
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr quaternion<T>
integrate_pade(const quaternion<T> &q, const micron::vector_3<T> &omega, T dt) noexcept
{
  const T h = T(0.5) * dt;
  const T vx = omega.x * h, vy = omega.y * h, vz = omega.z * h;
  const T v2 = vx * vx + vy * vy + vz * vz;
  const T w0 = T(1) - v2 * (T(1) / T(12));
  const T a = w0 * w0;
  const T b = v2 * T(0.25);
  const T denom = a + b;
  if ( denom <= __safe_min_n2<T>() ) return quaternion<T>::nan();
  const T inv_d = T(1) / denom;
  const T num_w = (a - b) * inv_d;
  const T num_v = w0 * inv_d;
  const quaternion<T> dq{ vx * num_v, vy * num_v, vz * num_v, num_w };
  return multiply<T>(q, dq).normalized();
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr micron::vector_3<T>
angular_velocity(const quaternion<T> &q0, const quaternion<T> &q1, T dt) noexcept
{
  if ( !ieee::is_finite<T>(dt) || math::fabs(dt) <= math::default_eps<T>() ) {
    return __impl_kinematics::nan_vec<T>();
  }
  const auto dq = multiply<T>(inverse_unit<T>(q0), q1);
  const auto aa = to_axis_angle<T>(dq);
  const T s = aa.angle / dt;
  return micron::vector_3<T>{ aa.axis.x * s, aa.axis.y * s, aa.axis.z * s };
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr micron::vector_3<T>
log_map_pade(const quaternion<T> &q) noexcept
{
  const T sgn = (q.w >= T(0)) ? T(1) : T(-1);
  const T cx = q.x * sgn, cy = q.y * sgn, cz = q.z * sgn, cw = q.w * sgn;
  if ( cw < __impl_kinematics::log_pade_cw_floor<T>() ) {
    const auto aa = to_axis_angle<T>(q);
    return micron::vector_3<T>{ aa.axis.x * aa.angle, aa.axis.y * aa.angle, aa.axis.z * aa.angle };
  }
  const T s2 = cx * cx + cy * cy + cz * cz;
  const T cw2 = cw * cw;
  const T num = T(15) * cw2 + T(4) * s2;
  const T den = (T(15) * cw2 + T(9) * s2) * cw;
  const T scale = T(2) * num / den;
  return micron::vector_3<T>{ cx * scale, cy * scale, cz * scale };
}

template <ieee754_floating T>
[[nodiscard]] inline constexpr micron::vector_3<T>
angular_velocity_pade(const quaternion<T> &q0, const quaternion<T> &q1, T dt) noexcept
{
  if ( !ieee::is_finite<T>(dt) || math::fabs(dt) <= math::default_eps<T>() ) {
    return __impl_kinematics::nan_vec<T>();
  }
  const auto v = log_map_pade<T>(multiply<T>(inverse_unit<T>(q0), q1));
  const T inv_dt = T(1) / dt;
  return micron::vector_3<T>{ v.x * inv_dt, v.y * inv_dt, v.z * inv_dt };
}

};     // namespace quaternions
};     // namespace math
};     // namespace micron
