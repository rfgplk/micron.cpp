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
struct vector_2 {
  T x, y;

  ~vector_2() = default;

  constexpr vector_2() : x(T{}), y(T{}) {}

  constexpr vector_2(T a, T b) : x(a), y(b) {}

  constexpr vector_2(const vector_2<T> &o) = default;
  constexpr vector_2(vector_2<T> &&o) = default;

  constexpr vector_2(const std::initializer_list<T> &o)
  {
    if ( o.size() != 2 )
      exc<except::library_error>("vector_2(): initializer_list size isn't equal to 2");
    auto it = o.begin();
    x = *it++;
    y = *it++;
  }

  constexpr vector_2<T> &operator=(const vector_2<T> &o) = default;
  constexpr vector_2<T> &operator=(vector_2<T> &&o) = default;

  constexpr vector_2<T>
  operator+(const vector_2<T> &o) const
  {
    return { x + o.x, y + o.y };
  }

  constexpr vector_2<T>
  operator-(const vector_2<T> &o) const
  {
    return { x - o.x, y - o.y };
  }

  constexpr vector_2<T> &
  operator+=(const vector_2<T> &o)
  {
    x += o.x;
    y += o.y;
    return *this;
  }

  constexpr vector_2<T> &
  operator-=(const vector_2<T> &o)
  {
    x -= o.x;
    y -= o.y;
    return *this;
  }

  constexpr vector_2<T>
  operator*(T s) const
  {
    return { x * s, y * s };
  }

  constexpr vector_2<T> &
  operator*=(T s)
  {
    x *= s;
    y *= s;
    return *this;
  }

  friend constexpr vector_2<T>
  operator*(T s, const vector_2<T> &v)
  {
    return { v.x * s, v.y * s };
  }

  constexpr vector_2<T>
  operator/(T s) const
  {
    return { x / s, y / s };
  }

  constexpr vector_2<T> &
  operator/=(T s)
  {
    x /= s;
    y /= s;
    return *this;
  }

  friend constexpr vector_2<T>
  operator/(T s, const vector_2<T> &v)
  {
    return { s / v.x, s / v.y };
  }

  constexpr vector_2<T>
  operator+(T s) const
  {
    return { x + s, y + s };
  }

  constexpr vector_2<T>
  operator-(T s) const
  {
    return { x - s, y - s };
  }

  friend constexpr vector_2<T>
  operator+(T s, const vector_2<T> &v)
  {
    return { s + v.x, s + v.y };
  }

  friend constexpr vector_2<T>
  operator-(T s, const vector_2<T> &v)
  {
    return { s - v.x, s - v.y };
  }

  constexpr vector_2<T>
  operator-() const
  {
    return { -x, -y };
  }

  constexpr vector_2<T>
  product(const vector_2<T> &o) const
  {
    return { x * o.x, y * o.y };
  }

  constexpr vector_2<T>
  operator*(const vector_2<T> &o) const
  {
    return product(o);
  }

  constexpr T
  dot(const vector_2<T> &o) const
  {
    return x * o.x + y * o.y;
  }

  constexpr T
  cross(const vector_2<T> &o) const
  {
    return x * o.y - y * o.x;
  }

  constexpr T
  squared_norm() const
  {
    return x * x + y * y;
  }

  constexpr T
  magnitude() const
  {
    return math::fsqrt(squared_norm());
  }

  constexpr T
  l1_norm() const
  {
    return math::fabs(x) + math::fabs(y);
  }

  constexpr T
  linf_norm() const
  {
    return math::fabsmax(math::fabs(x), math::fabs(y));
  }

  constexpr T
  lp_norm(T p) const
  {
    return math::fpow(fpow(math::fabs(x), p) + math::fpow(math::fabs(y), p), T{ 1 } / p);
  }

  constexpr T
  distance(const vector_2<T> &v) const
  {
    return (*this - v).magnitude();
  }

  constexpr T
  squared_distance(const vector_2<T> &v) const
  {
    return (*this - v).squared_norm();
  }

  constexpr vector_2<T>
  normalized() const
  {
    T inv = math::frsqrt(squared_norm());
    return { x * inv, y * inv };
  }

  constexpr vector_2<T> &
  normalize()
  {
    T inv = math::frsqrt(squared_norm());
    x *= inv;
    y *= inv;
    return *this;
  }

  constexpr bool
  is_normalized(T eps = math::default_eps<T>()) const
  {
    T diff = squared_norm() - T{ 1 };
    return math::fabs(diff) <= eps;
  }

  constexpr T
  angle(const vector_2<T> &v) const
  {
    T c = cos_angle(v);

    c = math::fclamp(c, T{ -1 }, T{ 1 });
    return math::facos(c);
  }

  constexpr T
  cos_angle(const vector_2<T> &v) const
  {
    return dot(v) * math::frsqrt(squared_norm() * v.squared_norm());
  }

  constexpr T
  scalar_project(const vector_2<T> &v) const
  {
    return dot(v) / v.magnitude();
  }

  constexpr vector_2<T>
  project_onto(const vector_2<T> &v) const
  {
    return v * (dot(v) / v.squared_norm());
  }

  constexpr vector_2<T>
  reject_from(const vector_2<T> &v) const
  {
    return *this - project_onto(v);
  }

  constexpr vector_2<T>
  reflect(const vector_2<T> &normal) const
  {
    return *this - normal * (T{ 2 } * dot(normal));
  }

  constexpr vector_2<T>
  refract(const vector_2<T> &normal, T eta) const
  {
    T nd = dot(normal);
    T k = T{ 1 } - eta * eta * (T{ 1 } - nd * nd);
    if ( k < T{ 0 } )
      return {};
    return *this * eta - normal * (eta * nd + math::fsqrt(k));
  }

  constexpr vector_2<T>
  abs() const
  {
    return { math::fabs(x), math::fabs(y) };
  }

  constexpr vector_2<T>
  sign() const
  {
    return { static_cast<T>(isign(x)), static_cast<T>(isign(y)) };
  }

  constexpr vector_2<T>
  floor() const
  {
    return { math::ffloor(x), math::ffloor(y) };
  }

  constexpr vector_2<T>
  ceil() const
  {
    return { math::fceil(x), math::fceil(y) };
  }

  constexpr vector_2<T>
  round() const
  {
    return { math::fround(x), math::fround(y) };
  }

  constexpr vector_2<T>
  trunc() const
  {
    return { math::ftrunc(x), math::ftrunc(y) };
  }

  constexpr vector_2<T>
  frac() const
  {
    return { math::ffract(x), math::ffract(y) };
  }

  constexpr vector_2<T>
  clamp(T lo, T hi) const
  {
    return { math::fclamp(x, lo, hi), math::fclamp(y, lo, hi) };
  }

  constexpr vector_2<T>
  clamp(const vector_2<T> &lo, const vector_2<T> &hi) const
  {
    return { math::fclamp(x, lo.x, hi.x), math::fclamp(y, lo.y, hi.y) };
  }

  constexpr vector_2<T>
  saturate() const
  {
    return { math::fclamp(x, T{ 0 }, T{ 1 }), math::fclamp(y, T{ 0 }, T{ 1 }) };
  }

  constexpr vector_2<T>
  sqrt() const
  {
    return { math::fsqrt(x), math::fsqrt(y) };
  }

  constexpr vector_2<T>
  rsqrt() const
  {
    return { math::frsqrt(x), math::frsqrt(y) };
  }

  constexpr vector_2<T>
  exp() const
  {
    return { math::fexp(x), math::fexp(y) };
  }

  constexpr vector_2<T>
  exp2() const
  {
    return { math::fexp2(x), math::fexp2(y) };
  }

  constexpr vector_2<T>
  log() const
  {
    return { math::flog(x), math::flog(y) };
  }

  constexpr vector_2<T>
  log2() const
  {
    return { math::flog2(x), math::flog2(y) };
  }

  constexpr vector_2<T>
  log10() const
  {
    return { math::flog10(x), math::flog10(y) };
  }

  constexpr vector_2<T>
  sin() const
  {
    return { math::fsin(x), math::fsin(y) };
  }

  constexpr vector_2<T>
  cos() const
  {
    return { math::fcos(x), math::fcos(y) };
  }

  constexpr vector_2<T>
  tan() const
  {
    return { math::ftan(x), math::ftan(y) };
  }

  constexpr vector_2<T>
  asin() const
  {
    return { math::fasin(x), math::fasin(y) };
  }

  constexpr vector_2<T>
  acos() const
  {
    return { math::facos(x), math::facos(y) };
  }

  constexpr vector_2<T>
  atan() const
  {
    return { math::fatan(x), math::fatan(y) };
  }

  constexpr vector_2<T>
  sinh() const
  {
    return { math::fsinh(x), math::fsinh(y) };
  }

  constexpr vector_2<T>
  cosh() const
  {
    return { math::fcosh(x), math::fcosh(y) };
  }

  constexpr vector_2<T>
  tanh() const
  {
    return { math::ftanh(x), math::ftanh(y) };
  }

  constexpr vector_2<T>
  erf() const
  {
    return { math::ferf(x), math::ferf(y) };
  }

  constexpr vector_2<T>
  erfc() const
  {
    return { math::ferfc(x), math::ferfc(y) };
  }

  constexpr vector_2<T>
  gamma() const
  {
    return { math::fgamma(x), math::fgamma(y) };
  }

  constexpr vector_2<T>
  min(const vector_2<T> &v) const
  {
    return { math::fmin(x, v.x), math::fmin(y, v.y) };
  }

  constexpr vector_2<T>
  max(const vector_2<T> &v) const
  {
    return { math::fmax(x, v.x), math::fmax(y, v.y) };
  }

  constexpr vector_2<T>
  fmin(const vector_2<T> &v) const
  {
    return min(v);
  }

  constexpr vector_2<T>
  fmax(const vector_2<T> &v) const
  {
    return max(v);
  }

  constexpr vector_2<T>
  pow(const vector_2<T> &v) const
  {
    return { math::fpow(x, v.x), math::fpow(y, v.y) };
  }

  constexpr vector_2<T>
  pow(T s) const
  {
    return { math::fpow(x, s), math::fpow(y, s) };
  }

  friend constexpr vector_2<T>
  pow(T s, const vector_2<T> &v)
  {
    return { math::fpow(s, v.x), math::fpow(s, v.y) };
  }

  constexpr vector_2<T>
  atan2(const vector_2<T> &v) const
  {
    return { math::fatan2(x, v.x), math::fatan2(y, v.y) };
  }

  constexpr vector_2<T>
  atan2(T s) const
  {
    return { math::fatan2(x, s), math::fatan2(y, s) };
  }

  constexpr T
  sum() const
  {
    return x + y;
  }

  constexpr T
  mean() const
  {
    return (x + y) * T{ 0.5 };
  }

  constexpr T
  prod() const
  {
    return x * y;
  }

  constexpr T
  min_component() const
  {
    return math::fmin(x, y);
  }

  constexpr T
  max_component() const
  {
    return math::fmax(x, y);
  }

  constexpr int
  argmin() const
  {
    return x <= y ? 0 : 1;
  }

  constexpr int
  argmax() const
  {
    return x >= y ? 0 : 1;
  }

  constexpr bool
  all() const
  {
    return x != T{} && y != T{};
  }

  constexpr bool
  any() const
  {
    return x != T{} || y != T{};
  }

  constexpr bool
  operator==(const vector_2<T> &o) const
  {
    return x == o.x && y == o.y;
  }

  constexpr bool
  operator!=(const vector_2<T> &o) const
  {
    return !(*this == o);
  }

  constexpr bool
  almost_equal(const vector_2<T> &o, T eps = math::default_eps<T>()) const
  {
    return math::fabs(x - o.x) <= eps && math::fabs(y - o.y) <= eps;
  }

  constexpr vector_2<T>
  operator<(const vector_2<T> &o) const
  {
    return { static_cast<T>(x < o.x), static_cast<T>(y < o.y) };
  }

  constexpr vector_2<T>
  operator<=(const vector_2<T> &o) const
  {
    return { static_cast<T>(x <= o.x), static_cast<T>(y <= o.y) };
  }

  constexpr vector_2<T>
  operator>(const vector_2<T> &o) const
  {
    return { static_cast<T>(x > o.x), static_cast<T>(y > o.y) };
  }

  constexpr vector_2<T>
  operator>=(const vector_2<T> &o) const
  {
    return { static_cast<T>(x >= o.x), static_cast<T>(y >= o.y) };
  }

  constexpr vector_2<T>
  operator<(T s) const
  {
    return { static_cast<T>(x < s), static_cast<T>(y < s) };
  }

  constexpr vector_2<T>
  operator<=(T s) const
  {
    return { static_cast<T>(x <= s), static_cast<T>(y <= s) };
  }

  constexpr vector_2<T>
  operator>(T s) const
  {
    return { static_cast<T>(x > s), static_cast<T>(y > s) };
  }

  constexpr vector_2<T>
  operator>=(T s) const
  {
    return { static_cast<T>(x >= s), static_cast<T>(y >= s) };
  }

  friend constexpr vector_2<T>
  operator<(T s, const vector_2<T> &v)
  {
    return { static_cast<T>(s < v.x), static_cast<T>(s < v.y) };
  }

  friend constexpr vector_2<T>
  operator<=(T s, const vector_2<T> &v)
  {
    return { static_cast<T>(s <= v.x), static_cast<T>(s <= v.y) };
  }

  friend constexpr vector_2<T>
  operator>(T s, const vector_2<T> &v)
  {
    return { static_cast<T>(s > v.x), static_cast<T>(s > v.y) };
  }

  friend constexpr vector_2<T>
  operator>=(T s, const vector_2<T> &v)
  {
    return { static_cast<T>(s >= v.x), static_cast<T>(s >= v.y) };
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

  constexpr vector_2<T>
  xy() const
  {
    return { x, y };
  }

  constexpr vector_2<T>
  yx() const
  {
    return { y, x };
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

  constexpr vector_2<T>
  rg() const
  {
    return { x, y };
  }

  constexpr vector_2<T>
  gr() const
  {
    return { y, x };
  }

  constexpr vector_2<T>
  rr() const
  {
    return { x, x };
  }

  constexpr vector_2<T>
  gg() const
  {
    return { y, y };
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

  constexpr vector_2<T>
  uv() const
  {
    return { x, y };
  }

  constexpr vector_2<T>
  vu() const
  {
    return { y, x };
  }

  constexpr T
  s() const
  {
    return x;
  }

  constexpr T
  t() const
  {
    return y;
  }

  constexpr vector_2<T>
  st() const
  {
    return { x, y };
  }

  constexpr vector_2<T>
  ts() const
  {
    return { y, x };
  }

  constexpr vector_2<T>
  fma(const vector_2<T> &v, const vector_2<T> &w) const
  {
    return { math::ffma(x, v.x, w.x), math::ffma(y, v.y, w.y) };
  }

  constexpr vector_2<T>
  fma(T s, const vector_2<T> &w) const
  {
    return { math::ffma(x, s, w.x), math::ffma(y, s, w.y) };
  }

  constexpr vector_2<T>
  mul_add(const vector_2<T> &v, const vector_2<T> &w) const
  {
    return math::fma(v, w);
  }

  constexpr vector_2<T>
  mul_add(T s, const vector_2<T> &w) const
  {
    return math::fma(s, w);
  }

  constexpr vector_2<T>
  perp() const
  {
    return { -y, x };
  }

  constexpr vector_2<T>
  perp_cw() const
  {
    return { y, -x };
  }

  constexpr vector_2<T>
  rotated(T theta) const
  {
    T c = math::fcos(theta);
    T s_ = math::fsin(theta);
    return { x * c - y * s_, x * s_ + y * c };
  }

  constexpr T
  heading() const
  {
    return math::fatan2(y, x);
  }
};

}     // namespace micron
