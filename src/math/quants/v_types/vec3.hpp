//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../__special/initializer_list"

#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../sqrt.hpp"
#include "../../trig.hpp"

#include "../../../type_traits.hpp"
#include "../../../types.hpp"

#include "../../../except.hpp"

namespace micron
{

template <typename T = float>
  requires micron::is_floating_point_v<T>
struct vector_3 {
  T x, y, z;

  ~vector_3() = default;
  constexpr vector_3() : x(T{}), y(T{}), z(T{}) {}
  constexpr vector_3(T a, T b, T c) : x(a), y(b), z(c) {}
  constexpr vector_3(const vector_3<T> &o) = default;
  constexpr vector_3(vector_3<T> &&o) = default;
  constexpr vector_3(const std::initializer_list<T> &o)
  {
    if ( o.size() != 3 )
      exc<except::library_error>("vector_3(): initializer_list size isn't equal to 3");
    auto it = o.begin();
    x = *it++;
    y = *it++;
    z = *it++;
  }

  constexpr vector_3<T> &operator=(const vector_3<T> &o) = default;
  constexpr vector_3<T> &operator=(vector_3<T> &&o) = default;

  constexpr vector_3<T>
  operator+(const vector_3<T> &o) const
  {
    return { x + o.x, y + o.y, z + o.z };
  }
  constexpr vector_3<T>
  operator-(const vector_3<T> &o) const
  {
    return { x - o.x, y - o.y, z - o.z };
  }
  constexpr vector_3<T> &
  operator+=(const vector_3<T> &o)
  {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
  }
  constexpr vector_3<T> &
  operator-=(const vector_3<T> &o)
  {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
  }

  constexpr vector_3<T>
  operator*(T s) const
  {
    return { x * s, y * s, z * s };
  }
  constexpr vector_3<T> &
  operator*=(T s)
  {
    x *= s;
    y *= s;
    z *= s;
    return *this;
  }
  friend constexpr vector_3<T>
  operator*(T s, const vector_3<T> &v)
  {
    return { v.x * s, v.y * s, v.z * s };
  }

  constexpr vector_3<T>
  operator/(T s) const
  {
    return { x / s, y / s, z / s };
  }
  constexpr vector_3<T> &
  operator/=(T s)
  {
    x /= s;
    y /= s;
    z /= s;
    return *this;
  }
  friend constexpr vector_3<T>
  operator/(T s, const vector_3<T> &v)
  {
    return { s / v.x, s / v.y, s / v.z };
  }

  constexpr vector_3<T>
  operator+(T s) const
  {
    return { x + s, y + s, z + s };
  }
  constexpr vector_3<T>
  operator-(T s) const
  {
    return { x - s, y - s, z - s };
  }
  friend constexpr vector_3<T>
  operator+(T s, const vector_3<T> &v)
  {
    return { s + v.x, s + v.y, s + v.z };
  }
  friend constexpr vector_3<T>
  operator-(T s, const vector_3<T> &v)
  {
    return { s - v.x, s - v.y, s - v.z };
  }

  constexpr vector_3<T>
  operator-() const
  {
    return { -x, -y, -z };
  }

  constexpr vector_3<T>
  product(const vector_3<T> &o) const
  {
    return { x * o.x, y * o.y, z * o.z };
  }
  constexpr vector_3<T>
  operator*(const vector_3<T> &o) const
  {
    return product(o);
  }

  constexpr T
  dot(const vector_3<T> &o) const
  {
    return x * o.x + y * o.y + z * o.z;
  }

  constexpr vector_3<T>
  cross(const vector_3<T> &o) const
  {
    return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
  }

  constexpr T
  squared_norm() const
  {
    return x * x + y * y + z * z;
  }

  constexpr T
  magnitude() const
  {
    return math::fsqrt(squared_norm());
  }

  constexpr T
  l1_norm() const
  {
    return math::fabs(x) + math::fabs(y) + math::fabs(z);
  }

  constexpr T
  linf_norm() const
  {
    return math::fabsmax(math::fabsmax(math::fabs(x), math::fabs(y)), math::fabs(z));
  }

  constexpr T
  lp_norm(T p) const
  {
    return math::fpow(math::fpow(math::fabs(x), p) + math::fpow(math::fabs(y), p) + math::fpow(math::fabs(z), p), T{ 1 } / p);
  }

  constexpr T
  distance(const vector_3<T> &v) const
  {
    return (*this - v).magnitude();
  }

  constexpr T
  squared_distance(const vector_3<T> &v) const
  {
    return (*this - v).squared_norm();
  }

  constexpr vector_3<T>
  normalized() const
  {
    T inv = math::frsqrt(squared_norm());
    return { x * inv, y * inv, z * inv };
  }

  constexpr vector_3<T> &
  normalize()
  {
    T inv = math::frsqrt(squared_norm());
    x *= inv;
    y *= inv;
    z *= inv;
    return *this;
  }

  constexpr bool
  is_normalized(T eps = math::default_eps<T>()) const
  {
    T diff = squared_norm() - T{ 1 };
    return math::fabs(diff) <= eps;
  }

  constexpr T
  angle(const vector_3<T> &v) const
  {
    T c = cos_angle(v);
    c = math::fclamp(c, T{ -1 }, T{ 1 });
    return math::facos(c);
  }

  constexpr T
  cos_angle(const vector_3<T> &v) const
  {
    return dot(v) * math::frsqrt(squared_norm() * v.squared_norm());
  }

  constexpr T
  scalar_project(const vector_3<T> &v) const
  {
    return dot(v) / v.magnitude();
  }

  constexpr vector_3<T>
  project_onto(const vector_3<T> &v) const
  {
    return v * (dot(v) / v.squared_norm());
  }

  constexpr vector_3<T>
  reject_from(const vector_3<T> &v) const
  {
    return *this - project_onto(v);
  }

  constexpr vector_3<T>
  reflect(const vector_3<T> &normal) const
  {
    return *this - normal * (T{ 2 } * dot(normal));
  }

  constexpr vector_3<T>
  refract(const vector_3<T> &normal, T eta) const
  {
    T nd = dot(normal);
    T k = T{ 1 } - eta * eta * (T{ 1 } - nd * nd);
    if ( k < T{ 0 } )
      return {};
    return *this * eta - normal * (eta * nd + math::fsqrt(k));
  }

  constexpr vector_3<T>
  abs() const
  {
    return { math::fabs(x), math::fabs(y), math::fabs(z) };
  }

  constexpr vector_3<T>
  sign() const
  {
    return { static_cast<T>(isign(x)), static_cast<T>(isign(y)), static_cast<T>(isign(z)) };
  }

  constexpr vector_3<T>
  floor() const
  {
    return { math::ffloor(x), math::ffloor(y), math::ffloor(z) };
  }

  constexpr vector_3<T>
  ceil() const
  {
    return { math::fceil(x), math::fceil(y), math::fceil(z) };
  }

  constexpr vector_3<T>
  round() const
  {
    return { math::fround(x), math::fround(y), math::fround(z) };
  }

  constexpr vector_3<T>
  trunc() const
  {
    return { math::ftrunc(x), math::ftrunc(y), math::ftrunc(z) };
  }

  constexpr vector_3<T>
  frac() const
  {
    return { math::ffract(x), math::ffract(y), math::ffract(z) };
  }

  constexpr vector_3<T>
  clamp(T lo, T hi) const
  {
    return { math::fclamp(x, lo, hi), math::fclamp(y, lo, hi), math::fclamp(z, lo, hi) };
  }

  constexpr vector_3<T>
  clamp(const vector_3<T> &lo, const vector_3<T> &hi) const
  {
    return { math::fclamp(x, lo.x, hi.x), math::fclamp(y, lo.y, hi.y), math::fclamp(z, lo.z, hi.z) };
  }

  constexpr vector_3<T>
  saturate() const
  {
    return { math::fclamp(x, T{ 0 }, T{ 1 }), math::fclamp(y, T{ 0 }, T{ 1 }), math::fclamp(z, T{ 0 }, T{ 1 }) };
  }

  constexpr vector_3<T>
  sqrt() const
  {
    return { math::fsqrt(x), math::fsqrt(y), math::fsqrt(z) };
  }

  constexpr vector_3<T>
  rsqrt() const
  {
    return { math::frsqrt(x), math::frsqrt(y), math::frsqrt(z) };
  }

  constexpr vector_3<T>
  exp() const
  {
    return { math::fexp(x), math::fexp(y), math::fexp(z) };
  }

  constexpr vector_3<T>
  exp2() const
  {
    return { math::fexp2(x), math::fexp2(y), math::fexp2(z) };
  }

  constexpr vector_3<T>
  log() const
  {
    return { math::flog(x), math::flog(y), math::flog(z) };
  }

  constexpr vector_3<T>
  log2() const
  {
    return { math::flog2(x), math::flog2(y), math::flog2(z) };
  }

  constexpr vector_3<T>
  log10() const
  {
    return { math::flog10(x), math::flog10(y), math::flog10(z) };
  }

  constexpr vector_3<T>
  sin() const
  {
    return { math::fsin(x), math::fsin(y), math::fsin(z) };
  }

  constexpr vector_3<T>
  cos() const
  {
    return { math::fcos(x), math::fcos(y), math::fcos(z) };
  }

  constexpr vector_3<T>
  tan() const
  {
    return { math::ftan(x), math::ftan(y), math::ftan(z) };
  }

  constexpr vector_3<T>
  asin() const
  {
    return { math::fasin(x), math::fasin(y), math::fasin(z) };
  }

  constexpr vector_3<T>
  acos() const
  {
    return { math::facos(x), math::facos(y), math::facos(z) };
  }

  constexpr vector_3<T>
  atan() const
  {
    return { math::fatan(x), math::fatan(y), math::fatan(z) };
  }

  constexpr vector_3<T>
  sinh() const
  {
    return { math::fsinh(x), math::fsinh(y), math::fsinh(z) };
  }

  constexpr vector_3<T>
  cosh() const
  {
    return { math::fcosh(x), math::fcosh(y), math::fcosh(z) };
  }

  constexpr vector_3<T>
  tanh() const
  {
    return { math::ftanh(x), math::ftanh(y), math::ftanh(z) };
  }

  constexpr vector_3<T>
  erf() const
  {
    return { math::ferf(x), math::ferf(y), math::ferf(z) };
  }

  constexpr vector_3<T>
  erfc() const
  {
    return { math::ferfc(x), math::ferfc(y), math::ferfc(z) };
  }

  constexpr vector_3<T>
  gamma() const
  {
    return { math::fgamma(x), math::fgamma(y), math::fgamma(z) };
  }

  constexpr vector_3<T>
  min(const vector_3<T> &v) const
  {
    return { math::fmin(x, v.x), math::fmin(y, v.y), math::fmin(z, v.z) };
  }

  constexpr vector_3<T>
  max(const vector_3<T> &v) const
  {
    return { math::fmax(x, v.x), math::fmax(y, v.y), math::fmax(z, v.z) };
  }

  constexpr vector_3<T>
  fmin(const vector_3<T> &v) const
  {
    return min(v);
  }

  constexpr vector_3<T>
  fmax(const vector_3<T> &v) const
  {
    return max(v);
  }

  constexpr vector_3<T>
  pow(const vector_3<T> &v) const
  {
    return { math::fpow(x, v.x), math::fpow(y, v.y), math::fpow(z, v.z) };
  }

  constexpr vector_3<T>
  pow(T s) const
  {
    return { math::fpow(x, s), math::fpow(y, s), math::fpow(z, s) };
  }
  friend constexpr vector_3<T>
  pow(T s, const vector_3<T> &v)
  {
    return { math::fpow(s, v.x), math::fpow(s, v.y), math::fpow(s, v.z) };
  }

  constexpr vector_3<T>
  atan2(const vector_3<T> &v) const
  {
    return { math::fatan2(x, v.x), math::fatan2(y, v.y), math::fatan2(z, v.z) };
  }

  constexpr vector_3<T>
  atan2(T s) const
  {
    return { math::fatan2(x, s), math::fatan2(y, s), math::fatan2(z, s) };
  }

  constexpr T
  sum() const
  {
    return x + y + z;
  }

  constexpr T
  mean() const
  {
    return (x + y + z) / T{ 3 };
  }

  constexpr T
  prod() const
  {
    return x * y * z;
  }

  constexpr T
  min_component() const
  {
    return math::fmin(math::fmin(x, y), z);
  }

  constexpr T
  max_component() const
  {
    return math::fmax(math::fmax(x, y), z);
  }

  constexpr int
  argmin() const
  {
    if ( x <= y && x <= z )
      return 0;
    if ( y <= z )
      return 1;
    return 2;
  }

  constexpr int
  argmax() const
  {
    if ( x >= y && x >= z )
      return 0;
    if ( y >= z )
      return 1;
    return 2;
  }

  constexpr bool
  all() const
  {
    return x != T{} && y != T{} && z != T{};
  }

  constexpr bool
  any() const
  {
    return x != T{} || y != T{} || z != T{};
  }

  constexpr bool
  operator==(const vector_3<T> &o) const
  {
    return x == o.x && y == o.y && z == o.z;
  }

  constexpr bool
  operator!=(const vector_3<T> &o) const
  {
    return !(*this == o);
  }

  constexpr bool
  almost_equal(const vector_3<T> &o, T eps = math::default_eps<T>()) const
  {
    return math::fabs(x - o.x) <= eps && math::fabs(y - o.y) <= eps && math::fabs(z - o.z) <= eps;
  }

  constexpr vector_3<T>
  operator<(const vector_3<T> &o) const
  {
    return { static_cast<T>(x < o.x), static_cast<T>(y < o.y), static_cast<T>(z < o.z) };
  }
  constexpr vector_3<T>
  operator<=(const vector_3<T> &o) const
  {
    return { static_cast<T>(x <= o.x), static_cast<T>(y <= o.y), static_cast<T>(z <= o.z) };
  }
  constexpr vector_3<T>
  operator>(const vector_3<T> &o) const
  {
    return { static_cast<T>(x > o.x), static_cast<T>(y > o.y), static_cast<T>(z > o.z) };
  }
  constexpr vector_3<T>
  operator>=(const vector_3<T> &o) const
  {
    return { static_cast<T>(x >= o.x), static_cast<T>(y >= o.y), static_cast<T>(z >= o.z) };
  }

  constexpr vector_3<T>
  operator<(T s) const
  {
    return { static_cast<T>(x < s), static_cast<T>(y < s), static_cast<T>(z < s) };
  }
  constexpr vector_3<T>
  operator<=(T s) const
  {
    return { static_cast<T>(x <= s), static_cast<T>(y <= s), static_cast<T>(z <= s) };
  }
  constexpr vector_3<T>
  operator>(T s) const
  {
    return { static_cast<T>(x > s), static_cast<T>(y > s), static_cast<T>(z > s) };
  }
  constexpr vector_3<T>
  operator>=(T s) const
  {
    return { static_cast<T>(x >= s), static_cast<T>(y >= s), static_cast<T>(z >= s) };
  }

  friend constexpr vector_3<T>
  operator<(T s, const vector_3<T> &v)
  {
    return { static_cast<T>(s < v.x), static_cast<T>(s < v.y), static_cast<T>(s < v.z) };
  }
  friend constexpr vector_3<T>
  operator<=(T s, const vector_3<T> &v)
  {
    return { static_cast<T>(s <= v.x), static_cast<T>(s <= v.y), static_cast<T>(s <= v.z) };
  }
  friend constexpr vector_3<T>
  operator>(T s, const vector_3<T> &v)
  {
    return { static_cast<T>(s > v.x), static_cast<T>(s > v.y), static_cast<T>(s > v.z) };
  }
  friend constexpr vector_3<T>
  operator>=(T s, const vector_3<T> &v)
  {
    return { static_cast<T>(s >= v.x), static_cast<T>(s >= v.y), static_cast<T>(s >= v.z) };
  }

  constexpr T
  xx() const
  {
    return x;
  }
  constexpr T
  yy() const
  {
    return y;
  }
  constexpr T
  zz() const
  {
    return z;
  }

  constexpr vector_2<T>
  xy() const
  {
    return { x, y };
  }
  constexpr vector_2<T>
  xz() const
  {
    return { x, z };
  }
  constexpr vector_2<T>
  yx() const
  {
    return { y, x };
  }
  constexpr vector_2<T>
  yz() const
  {
    return { y, z };
  }
  constexpr vector_2<T>
  zx() const
  {
    return { z, x };
  }
  constexpr vector_2<T>
  zy() const
  {
    return { z, y };
  }
  constexpr vector_2<T>
  xx_() const
  {
    return { x, x };
  }
  constexpr vector_2<T>
  yy_() const
  {
    return { y, y };
  }
  constexpr vector_2<T>
  zz_() const
  {
    return { z, z };
  }

  constexpr vector_3<T>
  xyz() const
  {
    return { x, y, z };
  }
  constexpr vector_3<T>
  xzy() const
  {
    return { x, z, y };
  }
  constexpr vector_3<T>
  yxz() const
  {
    return { y, x, z };
  }
  constexpr vector_3<T>
  yzx() const
  {
    return { y, z, x };
  }
  constexpr vector_3<T>
  zxy() const
  {
    return { z, x, y };
  }
  constexpr vector_3<T>
  zyx() const
  {
    return { z, y, x };
  }
  constexpr vector_3<T>
  xxx() const
  {
    return { x, x, x };
  }
  constexpr vector_3<T>
  yyy() const
  {
    return { y, y, y };
  }
  constexpr vector_3<T>
  zzz() const
  {
    return { z, z, z };
  }

  constexpr T
  r() const
  {
    return x;
  }
  constexpr T
  g() const
  {
    return y;
  }
  constexpr T
  b() const
  {
    return z;
  }
  constexpr vector_2<T>
  rg() const
  {
    return { x, y };
  }
  constexpr vector_2<T>
  rb() const
  {
    return { x, z };
  }
  constexpr vector_2<T>
  gb() const
  {
    return { y, z };
  }
  constexpr vector_3<T>
  rgb() const
  {
    return { x, y, z };
  }
  constexpr vector_3<T>
  bgr() const
  {
    return { z, y, x };
  }

  constexpr T
  u() const
  {
    return x;
  }
  constexpr T
  v() const
  {
    return y;
  }
  constexpr T
  w() const
  {
    return z;
  }
  constexpr vector_2<T>
  uv() const
  {
    return { x, y };
  }
  constexpr vector_3<T>
  uvw() const
  {
    return { x, y, z };
  }

  constexpr vector_3<T>
  fma(const vector_3<T> &v, const vector_3<T> &w) const
  {
    return { math::ffma(x, v.x, w.x), math::ffma(y, v.y, w.y), math::ffma(z, v.z, w.z) };
  }

  constexpr vector_3<T>
  fma(T s, const vector_3<T> &w) const
  {
    return { math::ffma(x, s, w.x), math::ffma(y, s, w.y), math::ffma(z, s, w.z) };
  }

  constexpr vector_3<T>
  mul_add(const vector_3<T> &v, const vector_3<T> &w) const
  {
    return fma(v, w);
  }

  constexpr vector_3<T>
  mul_add(T s, const vector_3<T> &w) const
  {
    return fma(s, w);
  }

  constexpr vector_3<T>
  perp(const vector_3<T> &v) const
  {
    return cross(v);
  }

  constexpr vector_3<T>
  rotated(const vector_3<T> &axis, T theta) const
  {
    T c = math::fcos(theta);
    T s_ = math::fsin(theta);
    return *this * c + axis.cross(*this) * s_ + axis * (axis.dot(*this) * (T{ 1 } - c));
  }
};

}
