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
struct vector_4 {
  T x, y, z, w;

  ~vector_4() = default;

  constexpr vector_4() : x(T{}), y(T{}), z(T{}), w(T{}) {}

  constexpr vector_4(T a, T b, T c, T d) : x(a), y(b), z(c), w(d) {}

  constexpr vector_4(const vector_4<T> &o) = default;
  constexpr vector_4(vector_4<T> &&o) = default;

  constexpr vector_4(const std::initializer_list<T> &o)
  {
    if ( o.size() != 4 )
      exc<except::library_error>("vector_4(): initializer_list size isn't equal to 4");
    auto it = o.begin();
    x = *it++;
    y = *it++;
    z = *it++;
    w = *it++;
  }

  constexpr vector_4<T> &operator=(const vector_4<T> &o) = default;
  constexpr vector_4<T> &operator=(vector_4<T> &&o) = default;

  constexpr vector_4<T>
  operator+(const vector_4<T> &o) const
  {
    return { x + o.x, y + o.y, z + o.z, w + o.w };
  }

  constexpr vector_4<T>
  operator-(const vector_4<T> &o) const
  {
    return { x - o.x, y - o.y, z - o.z, w - o.w };
  }

  constexpr vector_4<T> &
  operator+=(const vector_4<T> &o)
  {
    x += o.x;
    y += o.y;
    z += o.z;
    w += o.w;
    return *this;
  }

  constexpr vector_4<T> &
  operator-=(const vector_4<T> &o)
  {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    w -= o.w;
    return *this;
  }

  constexpr vector_4<T>
  operator*(T s) const
  {
    return { x * s, y * s, z * s, w * s };
  }

  constexpr vector_4<T> &
  operator*=(T s)
  {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
  }

  friend constexpr vector_4<T>
  operator*(T s, const vector_4<T> &v)
  {
    return { v.x * s, v.y * s, v.z * s, v.w * s };
  }

  constexpr vector_4<T>
  operator/(T s) const
  {
    return { x / s, y / s, z / s, w / s };
  }

  constexpr vector_4<T> &
  operator/=(T s)
  {
    x /= s;
    y /= s;
    z /= s;
    w /= s;
    return *this;
  }

  friend constexpr vector_4<T>
  operator/(T s, const vector_4<T> &v)
  {
    return { s / v.x, s / v.y, s / v.z, s / v.w };
  }

  constexpr vector_4<T>
  operator+(T s) const
  {
    return { x + s, y + s, z + s, w + s };
  }

  constexpr vector_4<T>
  operator-(T s) const
  {
    return { x - s, y - s, z - s, w - s };
  }

  friend constexpr vector_4<T>
  operator+(T s, const vector_4<T> &v)
  {
    return { s + v.x, s + v.y, s + v.z, s + v.w };
  }

  friend constexpr vector_4<T>
  operator-(T s, const vector_4<T> &v)
  {
    return { s - v.x, s - v.y, s - v.z, s - v.w };
  }

  constexpr vector_4<T>
  operator-() const
  {
    return { -x, -y, -z, -w };
  }

  constexpr vector_4<T>
  product(const vector_4<T> &o) const
  {
    return { x * o.x, y * o.y, z * o.z, w * o.w };
  }

  constexpr vector_4<T>
  operator*(const vector_4<T> &o) const
  {
    return product(o);
  }

  constexpr T
  dot(const vector_4<T> &o) const
  {
    return x * o.x + y * o.y + z * o.z + w * o.w;
  }

  constexpr T
  squared_norm() const
  {
    return x * x + y * y + z * z + w * w;
  }

  constexpr T
  magnitude() const
  {
    return math::fsqrt(squared_norm());
  }

  constexpr T
  l1_norm() const
  {
    return math::fabs(x) + math::fabs(y) + math::fabs(z) + math::fabs(w);
  }

  constexpr T
  linf_norm() const
  {
    return math::fabsmax(math::fabsmax(math::fabs(x), math::fabs(y)), math::fabsmax(math::fabs(z), math::fabs(w)));
  }

  constexpr T
  lp_norm(T p) const
  {
    return math::fpow(math::fpow(math::fabs(x), p) + math::fpow(math::fabs(y), p) + math::fpow(math::fabs(z), p)
                          + math::fpow(math::fabs(w), p),
                      T{ 1 } / p);
  }

  constexpr T
  distance(const vector_4<T> &v) const
  {
    return (*this - v).magnitude();
  }

  constexpr T
  squared_distance(const vector_4<T> &v) const
  {
    return (*this - v).squared_norm();
  }

  constexpr vector_4<T>
  normalized() const
  {
    T inv = math::frsqrt(squared_norm());
    return { x * inv, y * inv, z * inv, w * inv };
  }

  constexpr vector_4<T> &
  normalize()
  {
    T inv = math::frsqrt(squared_norm());
    x *= inv;
    y *= inv;
    z *= inv;
    w *= inv;
    return *this;
  }

  constexpr bool
  is_normalized(T eps = math::default_eps<T>()) const
  {
    T diff = squared_norm() - T{ 1 };
    return math::fabs(diff) <= eps;
  }

  constexpr T
  angle(const vector_4<T> &v) const
  {
    T c = cos_angle(v);
    c = math::fclamp(c, T{ -1 }, T{ 1 });
    return math::facos(c);
  }

  constexpr T
  cos_angle(const vector_4<T> &v) const
  {
    return dot(v) * math::frsqrt(squared_norm() * v.squared_norm());
  }

  constexpr T
  scalar_project(const vector_4<T> &v) const
  {
    return dot(v) / v.magnitude();
  }

  constexpr vector_4<T>
  project_onto(const vector_4<T> &v) const
  {
    return v * (dot(v) / v.squared_norm());
  }

  constexpr vector_4<T>
  reject_from(const vector_4<T> &v) const
  {
    return *this - project_onto(v);
  }

  constexpr vector_4<T>
  reflect(const vector_4<T> &normal) const
  {
    return *this - normal * (T{ 2 } * dot(normal));
  }

  constexpr vector_4<T>
  refract(const vector_4<T> &normal, T eta) const
  {
    T nd = dot(normal);
    T k = T{ 1 } - eta * eta * (T{ 1 } - nd * nd);
    if ( k < T{ 0 } )
      return {};
    return *this * eta - normal * (eta * nd + math::fsqrt(k));
  }

  constexpr vector_4<T>
  abs() const
  {
    return { math::fabs(x), math::fabs(y), math::fabs(z), math::fabs(w) };
  }

  constexpr vector_4<T>
  sign() const
  {
    return { static_cast<T>(isign(x)), static_cast<T>(isign(y)), static_cast<T>(isign(z)), static_cast<T>(isign(w)) };
  }

  constexpr vector_4<T>
  floor() const
  {
    return { math::ffloor(x), math::ffloor(y), math::ffloor(z), math::ffloor(w) };
  }

  constexpr vector_4<T>
  ceil() const
  {
    return { math::fceil(x), math::fceil(y), math::fceil(z), math::fceil(w) };
  }

  constexpr vector_4<T>
  round() const
  {
    return { math::fround(x), math::fround(y), math::fround(z), math::fround(w) };
  }

  constexpr vector_4<T>
  trunc() const
  {
    return { math::ftrunc(x), math::ftrunc(y), math::ftrunc(z), math::ftrunc(w) };
  }

  constexpr vector_4<T>
  frac() const
  {
    return { math::ffract(x), math::ffract(y), math::ffract(z), math::ffract(w) };
  }

  constexpr vector_4<T>
  clamp(T lo, T hi) const
  {
    return { math::fclamp(x, lo, hi), math::fclamp(y, lo, hi), math::fclamp(z, lo, hi), math::fclamp(w, lo, hi) };
  }

  constexpr vector_4<T>
  clamp(const vector_4<T> &lo, const vector_4<T> &hi) const
  {
    return { math::fclamp(x, lo.x, hi.x), math::fclamp(y, lo.y, hi.y), math::fclamp(z, lo.z, hi.z), math::fclamp(w, lo.w, hi.w) };
  }

  constexpr vector_4<T>
  saturate() const
  {
    return { math::fclamp(x, T{ 0 }, T{ 1 }), math::fclamp(y, T{ 0 }, T{ 1 }), math::fclamp(z, T{ 0 }, T{ 1 }),
             math::fclamp(w, T{ 0 }, T{ 1 }) };
  }

  constexpr vector_4<T>
  sqrt() const
  {
    return { math::fsqrt(x), math::fsqrt(y), math::fsqrt(z), math::fsqrt(w) };
  }

  constexpr vector_4<T>
  rsqrt() const
  {
    return { math::frsqrt(x), math::frsqrt(y), math::frsqrt(z), math::frsqrt(w) };
  }

  constexpr vector_4<T>
  exp() const
  {
    return { math::fexp(x), math::fexp(y), math::fexp(z), math::fexp(w) };
  }

  constexpr vector_4<T>
  exp2() const
  {
    return { math::fexp2(x), math::fexp2(y), math::fexp2(z), math::fexp2(w) };
  }

  constexpr vector_4<T>
  log() const
  {
    return { math::flog(x), math::flog(y), math::flog(z), math::flog(w) };
  }

  constexpr vector_4<T>
  log2() const
  {
    return { math::flog2(x), math::flog2(y), math::flog2(z), math::flog2(w) };
  }

  constexpr vector_4<T>
  log10() const
  {
    return { math::flog10(x), math::flog10(y), math::flog10(z), math::flog10(w) };
  }

  constexpr vector_4<T>
  sin() const
  {
    return { math::fsin(x), math::fsin(y), math::fsin(z), math::fsin(w) };
  }

  constexpr vector_4<T>
  cos() const
  {
    return { math::fcos(x), math::fcos(y), math::fcos(z), math::fcos(w) };
  }

  constexpr vector_4<T>
  tan() const
  {
    return { math::ftan(x), math::ftan(y), math::ftan(z), math::ftan(w) };
  }

  constexpr vector_4<T>
  asin() const
  {
    return { math::fasin(x), math::fasin(y), math::fasin(z), math::fasin(w) };
  }

  constexpr vector_4<T>
  acos() const
  {
    return { math::facos(x), math::facos(y), math::facos(z), math::facos(w) };
  }

  constexpr vector_4<T>
  atan() const
  {
    return { math::fatan(x), math::fatan(y), math::fatan(z), math::fatan(w) };
  }

  constexpr vector_4<T>
  sinh() const
  {
    return { math::fsinh(x), math::fsinh(y), math::fsinh(z), math::fsinh(w) };
  }

  constexpr vector_4<T>
  cosh() const
  {
    return { math::fcosh(x), math::fcosh(y), math::fcosh(z), math::fcosh(w) };
  }

  constexpr vector_4<T>
  tanh() const
  {
    return { math::ftanh(x), math::ftanh(y), math::ftanh(z), math::ftanh(w) };
  }

  constexpr vector_4<T>
  erf() const
  {
    return { math::ferf(x), math::ferf(y), math::ferf(z), math::ferf(w) };
  }

  constexpr vector_4<T>
  erfc() const
  {
    return { math::ferfc(x), math::ferfc(y), math::ferfc(z), math::ferfc(w) };
  }

  constexpr vector_4<T>
  gamma() const
  {
    return { math::fgamma(x), math::fgamma(y), math::fgamma(z), math::fgamma(w) };
  }

  constexpr vector_4<T>
  min(const vector_4<T> &v) const
  {
    return { math::fmin(x, v.x), math::fmin(y, v.y), math::fmin(z, v.z), math::fmin(w, v.w) };
  }

  constexpr vector_4<T>
  max(const vector_4<T> &v) const
  {
    return { math::fmax(x, v.x), math::fmax(y, v.y), math::fmax(z, v.z), math::fmax(w, v.w) };
  }

  constexpr vector_4<T>
  fmin(const vector_4<T> &v) const
  {
    return min(v);
  }

  constexpr vector_4<T>
  fmax(const vector_4<T> &v) const
  {
    return max(v);
  }

  constexpr vector_4<T>
  pow(const vector_4<T> &v) const
  {
    return { math::fpow(x, v.x), math::fpow(y, v.y), math::fpow(z, v.z), math::fpow(w, v.w) };
  }

  constexpr vector_4<T>
  pow(T s) const
  {
    return { math::fpow(x, s), math::fpow(y, s), math::fpow(z, s), math::fpow(w, s) };
  }

  friend constexpr vector_4<T>
  pow(T s, const vector_4<T> &v)
  {
    return { math::fpow(s, v.x), math::fpow(s, v.y), math::fpow(s, v.z), math::fpow(s, v.w) };
  }

  constexpr vector_4<T>
  atan2(const vector_4<T> &v) const
  {
    return { math::fatan2(x, v.x), math::fatan2(y, v.y), math::fatan2(z, v.z), math::fatan2(w, v.w) };
  }

  constexpr vector_4<T>
  atan2(T s) const
  {
    return { math::fatan2(x, s), math::fatan2(y, s), math::fatan2(z, s), math::fatan2(w, s) };
  }

  constexpr T
  sum() const
  {
    return x + y + z + w;
  }

  constexpr T
  mean() const
  {
    return (x + y + z + w) / T{ 4 };
  }

  constexpr T
  prod() const
  {
    return x * y * z * w;
  }

  constexpr T
  min_component() const
  {
    return math::fmin(math::fmin(x, y), math::fmin(z, w));
  }

  constexpr T
  max_component() const
  {
    return math::fmax(math::fmax(x, y), math::fmax(z, w));
  }

  constexpr int
  argmin() const
  {
    int idx = 0;
    T val = x;
    if ( y < val ) {
      val = y;
      idx = 1;
    }
    if ( z < val ) {
      val = z;
      idx = 2;
    }
    if ( w < val ) {
      idx = 3;
    }
    return idx;
  }

  constexpr int
  argmax() const
  {
    int idx = 0;
    T val = x;
    if ( y > val ) {
      val = y;
      idx = 1;
    }
    if ( z > val ) {
      val = z;
      idx = 2;
    }
    if ( w > val ) {
      idx = 3;
    }
    return idx;
  }

  constexpr bool
  all() const
  {
    return x != T{} && y != T{} && z != T{} && w != T{};
  }

  constexpr bool
  any() const
  {
    return x != T{} || y != T{} || z != T{} || w != T{};
  }

  constexpr bool
  operator==(const vector_4<T> &o) const
  {
    return x == o.x && y == o.y && z == o.z && w == o.w;
  }

  constexpr bool
  operator!=(const vector_4<T> &o) const
  {
    return !(*this == o);
  }

  constexpr bool
  almost_equal(const vector_4<T> &o, T eps = math::default_eps<T>()) const
  {
    return math::fabs(x - o.x) <= eps && math::fabs(y - o.y) <= eps && math::fabs(z - o.z) <= eps && math::fabs(w - o.w) <= eps;
  }

  constexpr vector_4<T>
  operator<(const vector_4<T> &o) const
  {
    return { static_cast<T>(x < o.x), static_cast<T>(y < o.y), static_cast<T>(z < o.z), static_cast<T>(w < o.w) };
  }

  constexpr vector_4<T>
  operator<=(const vector_4<T> &o) const
  {
    return { static_cast<T>(x <= o.x), static_cast<T>(y <= o.y), static_cast<T>(z <= o.z), static_cast<T>(w <= o.w) };
  }

  constexpr vector_4<T>
  operator>(const vector_4<T> &o) const
  {
    return { static_cast<T>(x > o.x), static_cast<T>(y > o.y), static_cast<T>(z > o.z), static_cast<T>(w > o.w) };
  }

  constexpr vector_4<T>
  operator>=(const vector_4<T> &o) const
  {
    return { static_cast<T>(x >= o.x), static_cast<T>(y >= o.y), static_cast<T>(z >= o.z), static_cast<T>(w >= o.w) };
  }

  constexpr vector_4<T>
  operator<(T s) const
  {
    return { static_cast<T>(x < s), static_cast<T>(y < s), static_cast<T>(z < s), static_cast<T>(w < s) };
  }

  constexpr vector_4<T>
  operator<=(T s) const
  {
    return { static_cast<T>(x <= s), static_cast<T>(y <= s), static_cast<T>(z <= s), static_cast<T>(w <= s) };
  }

  constexpr vector_4<T>
  operator>(T s) const
  {
    return { static_cast<T>(x > s), static_cast<T>(y > s), static_cast<T>(z > s), static_cast<T>(w > s) };
  }

  constexpr vector_4<T>
  operator>=(T s) const
  {
    return { static_cast<T>(x >= s), static_cast<T>(y >= s), static_cast<T>(z >= s), static_cast<T>(w >= s) };
  }

  friend constexpr vector_4<T>
  operator<(T s, const vector_4<T> &v)
  {
    return { static_cast<T>(s < v.x), static_cast<T>(s < v.y), static_cast<T>(s < v.z), static_cast<T>(s < v.w) };
  }

  friend constexpr vector_4<T>
  operator<=(T s, const vector_4<T> &v)
  {
    return { static_cast<T>(s <= v.x), static_cast<T>(s <= v.y), static_cast<T>(s <= v.z), static_cast<T>(s <= v.w) };
  }

  friend constexpr vector_4<T>
  operator>(T s, const vector_4<T> &v)
  {
    return { static_cast<T>(s > v.x), static_cast<T>(s > v.y), static_cast<T>(s > v.z), static_cast<T>(s > v.w) };
  }

  friend constexpr vector_4<T>
  operator>=(T s, const vector_4<T> &v)
  {
    return { static_cast<T>(s >= v.x), static_cast<T>(s >= v.y), static_cast<T>(s >= v.z), static_cast<T>(s >= v.w) };
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

  constexpr T
  ww() const
  {
    return w;
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
  xw() const
  {
    return { x, w };
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
  yw() const
  {
    return { y, w };
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
  zw() const
  {
    return { z, w };
  }

  constexpr vector_2<T>
  wx() const
  {
    return { w, x };
  }

  constexpr vector_2<T>
  wy() const
  {
    return { w, y };
  }

  constexpr vector_2<T>
  wz() const
  {
    return { w, z };
  }

  constexpr vector_3<T>
  xyz() const
  {
    return { x, y, z };
  }

  constexpr vector_3<T>
  xyw() const
  {
    return { x, y, w };
  }

  constexpr vector_3<T>
  xzw() const
  {
    return { x, z, w };
  }

  constexpr vector_3<T>
  yzw() const
  {
    return { y, z, w };
  }

  constexpr vector_3<T>
  zyx() const
  {
    return { z, y, x };
  }

  constexpr vector_4<T>
  xyzw() const
  {
    return { x, y, z, w };
  }

  constexpr vector_4<T>
  wzyx() const
  {
    return { w, z, y, x };
  }

  constexpr vector_4<T>
  xxxx() const
  {
    return { x, x, x, x };
  }

  constexpr vector_4<T>
  yyyy() const
  {
    return { y, y, y, y };
  }

  constexpr vector_4<T>
  zzzz() const
  {
    return { z, z, z, z };
  }

  constexpr vector_4<T>
  wwww() const
  {
    return { w, w, w, w };
  }

  constexpr vector_4<T>
  zwxy() const
  {
    return { z, w, x, y };
  }

  constexpr vector_4<T>
  yxwz() const
  {
    return { y, x, w, z };
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

  constexpr T
  a() const
  {
    return w;
  }

  constexpr vector_2<T>
  rg() const
  {
    return { x, y };
  }

  constexpr vector_3<T>
  rgb() const
  {
    return { x, y, z };
  }

  constexpr vector_4<T>
  rgba() const
  {
    return { x, y, z, w };
  }

  constexpr vector_4<T>
  bgra() const
  {
    return { z, y, x, w };
  }

  constexpr vector_4<T>
  abgr() const
  {
    return { w, z, y, x };
  }

  constexpr vector_4<T>
  fma(const vector_4<T> &v, const vector_4<T> &w_) const
  {
    return { math::ffma(x, v.x, w_.x), math::ffma(y, v.y, w_.y), math::ffma(z, v.z, w_.z), math::ffma(w, v.w, w_.w) };
  }

  constexpr vector_4<T>
  fma(T s, const vector_4<T> &w_) const
  {
    return { math::ffma(x, s, w_.x), math::ffma(y, s, w_.y), math::ffma(z, s, w_.z), math::ffma(w, s, w_.w) };
  }

  constexpr vector_4<T>
  mul_add(const vector_4<T> &v, const vector_4<T> &w_) const
  {
    return fma(v, w_);
  }

  constexpr vector_4<T>
  mul_add(T s, const vector_4<T> &w_) const
  {
    return fma(s, w_);
  }
};

}
