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

#include "vec3.hpp"
#include "vec4.hpp"
#include "vec8.hpp"

namespace micron
{

template <typename T = float>
  requires micron::is_floating_point_v<T>
struct vector_16 {

  T x, y, z, w, a, b, c, d, e, f, g, h, i, j, k, l;

  ~vector_16() = default;

  constexpr vector_16()
      : x(T{}), y(T{}), z(T{}), w(T{}), a(T{}), b(T{}), c(T{}), d(T{}), e(T{}), f(T{}), g(T{}), h(T{}), i(T{}), j(T{}), k(T{}), l(T{})
  {
  }

  constexpr vector_16(T x_, T y_, T z_, T w_, T a_, T b_, T c_, T d_, T e_, T f_, T g_, T h_, T i_, T j_, T k_, T l_)
      : x(x_), y(y_), z(z_), w(w_), a(a_), b(b_), c(c_), d(d_), e(e_), f(f_), g(g_), h(h_), i(i_), j(j_), k(k_), l(l_)
  {
  }

  constexpr vector_16(const vector_16<T> &o) = default;
  constexpr vector_16(vector_16<T> &&o) = default;

  constexpr vector_16(const std::initializer_list<T> &o)
  {
    if ( o.size() != 16 )
      exc<except::library_error>("vector_16(): initializer_list size isn't equal to 16");
    auto it = o.begin();
    x = *it++;
    y = *it++;
    z = *it++;
    w = *it++;
    a = *it++;
    b = *it++;
    c = *it++;
    d = *it++;
    e = *it++;
    f = *it++;
    g = *it++;
    h = *it++;
    i = *it++;
    j = *it++;
    k = *it++;
    l = *it++;
  }

  constexpr vector_16<T> &operator=(const vector_16<T> &o) = default;
  constexpr vector_16<T> &operator=(vector_16<T> &&o) = default;

  constexpr T
  s0() const
  {
    return x;
  }

  constexpr T
  s1() const
  {
    return y;
  }

  constexpr T
  s2() const
  {
    return z;
  }

  constexpr T
  s3() const
  {
    return w;
  }

  constexpr T
  s4() const
  {
    return a;
  }

  constexpr T
  s5() const
  {
    return b;
  }

  constexpr T
  s6() const
  {
    return c;
  }

  constexpr T
  s7() const
  {
    return d;
  }

  constexpr T
  s8() const
  {
    return e;
  }

  constexpr T
  s9() const
  {
    return f;
  }

  constexpr T
  sa() const
  {
    return g;
  }

  constexpr T
  sb() const
  {
    return h;
  }

  constexpr T
  sc() const
  {
    return i;
  }

  constexpr T
  sd() const
  {
    return j;
  }

  constexpr T
  se() const
  {
    return k;
  }

  constexpr T
  sf() const
  {
    return l;
  }

#define __v16_binop(op)                                                                                                                    \
  constexpr vector_16<T> operator op(const vector_16<T> &o) const                                                                          \
  {                                                                                                                                        \
    return { x op o.x, y op o.y, z op o.z, w op o.w, a op o.a, b op o.b, c op o.c, d op o.d,                                               \
             e op o.e, f op o.f, g op o.g, h op o.h, i op o.i, j op o.j, k op o.k, l op o.l };                                             \
  }
  __v16_binop(+) __v16_binop(-) __v16_binop(*)
#undef __v16_binop

#define __v16_selfop(op)                                                                                                                   \
  constexpr vector_16<T> &operator op(const vector_16<T> &o)                                                                               \
  {                                                                                                                                        \
    x op o.x;                                                                                                                              \
    y op o.y;                                                                                                                              \
    z op o.z;                                                                                                                              \
    w op o.w;                                                                                                                              \
    a op o.a;                                                                                                                              \
    b op o.b;                                                                                                                              \
    c op o.c;                                                                                                                              \
    d op o.d;                                                                                                                              \
    e op o.e;                                                                                                                              \
    f op o.f;                                                                                                                              \
    g op o.g;                                                                                                                              \
    h op o.h;                                                                                                                              \
    i op o.i;                                                                                                                              \
    j op o.j;                                                                                                                              \
    k op o.k;                                                                                                                              \
    l op o.l;                                                                                                                              \
    return *this;                                                                                                                          \
  }
      __v16_selfop(+=) __v16_selfop(-=)
#undef __v16_selfop

          constexpr vector_16<T> product(const vector_16<T> &o) const
  {
    return *this * o;
  }

  constexpr vector_16<T>
  operator*(T s) const
  {
    return { x * s, y * s, z * s, w * s, a * s, b * s, c * s, d * s, e * s, f * s, g * s, h * s, i * s, j * s, k * s, l * s };
  }

  constexpr vector_16<T> &
  operator*=(T s)
  {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    a *= s;
    b *= s;
    c *= s;
    d *= s;
    e *= s;
    f *= s;
    g *= s;
    h *= s;
    i *= s;
    j *= s;
    k *= s;
    l *= s;
    return *this;
  }

  friend constexpr vector_16<T>
  operator*(T s, const vector_16<T> &v)
  {
    return { v.x * s, v.y * s, v.z * s, v.w * s, v.a * s, v.b * s, v.c * s, v.d * s,
             v.e * s, v.f * s, v.g * s, v.h * s, v.i * s, v.j * s, v.k * s, v.l * s };
  }

  constexpr vector_16<T>
  operator/(T s) const
  {
    return { x / s, y / s, z / s, w / s, a / s, b / s, c / s, d / s, e / s, f / s, g / s, h / s, i / s, j / s, k / s, l / s };
  }

  constexpr vector_16<T> &
  operator/=(T s)
  {
    x /= s;
    y /= s;
    z /= s;
    w /= s;
    a /= s;
    b /= s;
    c /= s;
    d /= s;
    e /= s;
    f /= s;
    g /= s;
    h /= s;
    i /= s;
    j /= s;
    k /= s;
    l /= s;
    return *this;
  }

  friend constexpr vector_16<T>
  operator/(T s, const vector_16<T> &v)
  {
    return { s / v.x, s / v.y, s / v.z, s / v.w, s / v.a, s / v.b, s / v.c, s / v.d,
             s / v.e, s / v.f, s / v.g, s / v.h, s / v.i, s / v.j, s / v.k, s / v.l };
  }

  constexpr vector_16<T>
  operator+(T s) const
  {
    return { x + s, y + s, z + s, w + s, a + s, b + s, c + s, d + s, e + s, f + s, g + s, h + s, i + s, j + s, k + s, l + s };
  }

  constexpr vector_16<T>
  operator-(T s) const
  {
    return { x - s, y - s, z - s, w - s, a - s, b - s, c - s, d - s, e - s, f - s, g - s, h - s, i - s, j - s, k - s, l - s };
  }

  friend constexpr vector_16<T>
  operator+(T s, const vector_16<T> &v)
  {
    return { s + v.x, s + v.y, s + v.z, s + v.w, s + v.a, s + v.b, s + v.c, s + v.d,
             s + v.e, s + v.f, s + v.g, s + v.h, s + v.i, s + v.j, s + v.k, s + v.l };
  }

  friend constexpr vector_16<T>
  operator-(T s, const vector_16<T> &v)
  {
    return { s - v.x, s - v.y, s - v.z, s - v.w, s - v.a, s - v.b, s - v.c, s - v.d,
             s - v.e, s - v.f, s - v.g, s - v.h, s - v.i, s - v.j, s - v.k, s - v.l };
  }

  constexpr vector_16<T>
  operator-() const
  {
    return { -x, -y, -z, -w, -a, -b, -c, -d, -e, -f, -g, -h, -i, -j, -k, -l };
  }

  constexpr T
  dot(const vector_16<T> &o) const
  {
    return x * o.x + y * o.y + z * o.z + w * o.w + a * o.a + b * o.b + c * o.c + d * o.d + e * o.e + f * o.f + g * o.g + h * o.h + i * o.i
           + j * o.j + k * o.k + l * o.l;
  }

  constexpr T
  squared_norm() const
  {
    return x * x + y * y + z * z + w * w + a * a + b * b + c * c + d * d + e * e + f * f + g * g + h * h + i * i + j * j + k * k + l * l;
  }

  constexpr T
  magnitude() const
  {
    return math::fsqrt(squared_norm());
  }

  constexpr T
  l1_norm() const
  {
    return math::fabs(x) + math::fabs(y) + math::fabs(z) + math::fabs(w) + math::fabs(a) + math::fabs(b) + math::fabs(c) + math::fabs(d)
           + math::fabs(e) + math::fabs(f) + math::fabs(g) + math::fabs(h) + math::fabs(i) + math::fabs(j) + math::fabs(k) + math::fabs(l);
  }

  constexpr T
  linf_norm() const
  {
    T p0 = math::fabsmax(math::fabsmax(math::fabs(x), math::fabs(y)), math::fabsmax(math::fabs(z), math::fabs(w)));
    T p1 = math::fabsmax(math::fabsmax(math::fabs(a), math::fabs(b)), math::fabsmax(math::fabs(c), math::fabs(d)));
    T p2 = math::fabsmax(math::fabsmax(math::fabs(e), math::fabs(f)), math::fabsmax(math::fabs(g), math::fabs(h)));
    T p3 = math::fabsmax(math::fabsmax(math::fabs(i), math::fabs(j)), math::fabsmax(math::fabs(k), math::fabs(l)));
    return math::fabsmax(math::fabsmax(p0, p1), math::fabsmax(p2, p3));
  }

  constexpr T
  lp_norm(T p) const
  {
    return math::fpow(
        math::fpow(math::fabs(x), p) + math::fpow(math::fabs(y), p) + math::fpow(math::fabs(z), p) + math::fpow(math::fabs(w), p)
            + math::fpow(math::fabs(a), p) + math::fpow(math::fabs(b), p) + math::fpow(math::fabs(c), p) + math::fpow(math::fabs(d), p)
            + math::fpow(math::fabs(e), p) + math::fpow(math::fabs(f), p) + math::fpow(math::fabs(g), p) + math::fpow(math::fabs(h), p)
            + math::fpow(math::fabs(i), p) + math::fpow(math::fabs(j), p) + math::fpow(math::fabs(k), p) + math::fpow(math::fabs(l), p),
        T{ 1 } / p);
  }

  constexpr T
  distance(const vector_16<T> &v) const
  {
    return (*this - v).magnitude();
  }

  constexpr T
  squared_distance(const vector_16<T> &v) const
  {
    return (*this - v).squared_norm();
  }

  constexpr vector_16<T>
  normalized() const
  {
    T inv = math::frsqrt(squared_norm());
    return { x * inv, y * inv, z * inv, w * inv, a * inv, b * inv, c * inv, d * inv,
             e * inv, f * inv, g * inv, h * inv, i * inv, j * inv, k * inv, l * inv };
  }

  constexpr vector_16<T> &
  normalize()
  {
    T inv = math::frsqrt(squared_norm());
    x *= inv;
    y *= inv;
    z *= inv;
    w *= inv;
    a *= inv;
    b *= inv;
    c *= inv;
    d *= inv;
    e *= inv;
    f *= inv;
    g *= inv;
    h *= inv;
    i *= inv;
    j *= inv;
    k *= inv;
    l *= inv;
    return *this;
  }

  constexpr bool
  is_normalized(T eps = math::default_eps<T>()) const
  {
    return math::fabs(squared_norm() - T{ 1 }) <= eps;
  }

  constexpr T
  cos_angle(const vector_16<T> &v) const
  {
    return dot(v) * math::frsqrt(squared_norm() * v.squared_norm());
  }

  constexpr T
  angle(const vector_16<T> &v) const
  {
    return math::facos(math::fclamp(cos_angle(v), T{ -1 }, T{ 1 }));
  }

  constexpr T
  scalar_project(const vector_16<T> &v) const
  {
    return dot(v) / v.magnitude();
  }

  constexpr vector_16<T>
  project_onto(const vector_16<T> &v) const
  {
    return v * (dot(v) / v.squared_norm());
  }

  constexpr vector_16<T>
  reject_from(const vector_16<T> &v) const
  {
    return *this - project_onto(v);
  }

  constexpr vector_16<T>
  reflect(const vector_16<T> &normal) const
  {
    return *this - normal * (T{ 2 } * dot(normal));
  }

  constexpr vector_16<T>
  refract(const vector_16<T> &normal, T eta) const
  {
    T nd = dot(normal);
    T kk = T{ 1 } - eta * eta * (T{ 1 } - nd * nd);
    if ( kk < T{ 0 } )
      return {};
    return *this * eta - normal * (eta * nd + math::fsqrt(kk));
  }

#define __v16_unop(name, fn)                                                                                                               \
  constexpr vector_16<T> name() const                                                                                                      \
  {                                                                                                                                        \
    return { fn(x), fn(y), fn(z), fn(w), fn(a), fn(b), fn(c), fn(d), fn(e), fn(f), fn(g), fn(h), fn(i), fn(j), fn(k), fn(l) };             \
  }
  __v16_unop(abs, math::fabs) __v16_unop(floor, math::ffloor) __v16_unop(ceil, math::fceil) __v16_unop(round, math::fround)
      __v16_unop(trunc, math::ftrunc) __v16_unop(frac, math::ffract) __v16_unop(sqrt, math::fsqrt) __v16_unop(rsqrt, math::frsqrt)
          __v16_unop(exp, math::fexp) __v16_unop(exp2, math::fexp2) __v16_unop(log, math::flog) __v16_unop(log2, math::flog2)
              __v16_unop(log10, math::flog10) __v16_unop(sin, math::fsin) __v16_unop(cos, math::fcos) __v16_unop(tan, math::ftan)
                  __v16_unop(asin, math::fasin) __v16_unop(acos, math::facos) __v16_unop(atan, math::fatan) __v16_unop(sinh, math::fsinh)
                      __v16_unop(cosh, math::fcosh) __v16_unop(tanh, math::ftanh) __v16_unop(erf, math::ferf) __v16_unop(erfc, math::ferfc)
                          __v16_unop(gamma, math::fgamma)
#undef __v16_unop

                              constexpr vector_16<T> sign() const
  {
    return { static_cast<T>(isign(x)), static_cast<T>(isign(y)), static_cast<T>(isign(z)), static_cast<T>(isign(w)),
             static_cast<T>(isign(a)), static_cast<T>(isign(b)), static_cast<T>(isign(c)), static_cast<T>(isign(d)),
             static_cast<T>(isign(e)), static_cast<T>(isign(f)), static_cast<T>(isign(g)), static_cast<T>(isign(h)),
             static_cast<T>(isign(i)), static_cast<T>(isign(j)), static_cast<T>(isign(k)), static_cast<T>(isign(l)) };
  }

  constexpr vector_16<T>
  saturate() const
  {
    return { math::fclamp(x, T{ 0 }, T{ 1 }), math::fclamp(y, T{ 0 }, T{ 1 }), math::fclamp(z, T{ 0 }, T{ 1 }),
             math::fclamp(w, T{ 0 }, T{ 1 }), math::fclamp(a, T{ 0 }, T{ 1 }), math::fclamp(b, T{ 0 }, T{ 1 }),
             math::fclamp(c, T{ 0 }, T{ 1 }), math::fclamp(d, T{ 0 }, T{ 1 }), math::fclamp(e, T{ 0 }, T{ 1 }),
             math::fclamp(f, T{ 0 }, T{ 1 }), math::fclamp(g, T{ 0 }, T{ 1 }), math::fclamp(h, T{ 0 }, T{ 1 }),
             math::fclamp(i, T{ 0 }, T{ 1 }), math::fclamp(j, T{ 0 }, T{ 1 }), math::fclamp(k, T{ 0 }, T{ 1 }),
             math::fclamp(l, T{ 0 }, T{ 1 }) };
  }

  constexpr vector_16<T>
  clamp(T lo, T hi) const
  {
    return { math::fclamp(x, lo, hi), math::fclamp(y, lo, hi), math::fclamp(z, lo, hi), math::fclamp(w, lo, hi),
             math::fclamp(a, lo, hi), math::fclamp(b, lo, hi), math::fclamp(c, lo, hi), math::fclamp(d, lo, hi),
             math::fclamp(e, lo, hi), math::fclamp(f, lo, hi), math::fclamp(g, lo, hi), math::fclamp(h, lo, hi),
             math::fclamp(i, lo, hi), math::fclamp(j, lo, hi), math::fclamp(k, lo, hi), math::fclamp(l, lo, hi) };
  }

  constexpr vector_16<T>
  clamp(const vector_16<T> &lo, const vector_16<T> &hi) const
  {
    return { math::fclamp(x, lo.x, hi.x), math::fclamp(y, lo.y, hi.y), math::fclamp(z, lo.z, hi.z), math::fclamp(w, lo.w, hi.w),
             math::fclamp(a, lo.a, hi.a), math::fclamp(b, lo.b, hi.b), math::fclamp(c, lo.c, hi.c), math::fclamp(d, lo.d, hi.d),
             math::fclamp(e, lo.e, hi.e), math::fclamp(f, lo.f, hi.f), math::fclamp(g, lo.g, hi.g), math::fclamp(h, lo.h, hi.h),
             math::fclamp(i, lo.i, hi.i), math::fclamp(j, lo.j, hi.j), math::fclamp(k, lo.k, hi.k), math::fclamp(l, lo.l, hi.l) };
  }

#define __v16_binop2(name, fn)                                                                                                             \
  constexpr vector_16<T> name(const vector_16<T> &v) const                                                                                 \
  {                                                                                                                                        \
    return { fn(x, v.x), fn(y, v.y), fn(z, v.z), fn(w, v.w), fn(a, v.a), fn(b, v.b), fn(c, v.c), fn(d, v.d),                               \
             fn(e, v.e), fn(f, v.f), fn(g, v.g), fn(h, v.h), fn(i, v.i), fn(j, v.j), fn(k, v.k), fn(l, v.l) };                             \
  }
  __v16_binop2(min, math::fmin) __v16_binop2(max, math::fmax) __v16_binop2(fmin, math::fmin) __v16_binop2(fmax, math::fmax)
      __v16_binop2(pow, math::fpow) __v16_binop2(atan2, math::fatan2)
#undef __v16_binop2

          constexpr vector_16<T> pow(T s) const
  {
    return { math::fpow(x, s), math::fpow(y, s), math::fpow(z, s), math::fpow(w, s), math::fpow(a, s), math::fpow(b, s),
             math::fpow(c, s), math::fpow(d, s), math::fpow(e, s), math::fpow(f, s), math::fpow(g, s), math::fpow(h, s),
             math::fpow(i, s), math::fpow(j, s), math::fpow(k, s), math::fpow(l, s) };
  }

  friend constexpr vector_16<T>
  pow(T s, const vector_16<T> &v)
  {
    return { math::fpow(s, v.x), math::fpow(s, v.y), math::fpow(s, v.z), math::fpow(s, v.w), math::fpow(s, v.a), math::fpow(s, v.b),
             math::fpow(s, v.c), math::fpow(s, v.d), math::fpow(s, v.e), math::fpow(s, v.f), math::fpow(s, v.g), math::fpow(s, v.h),
             math::fpow(s, v.i), math::fpow(s, v.j), math::fpow(s, v.k), math::fpow(s, v.l) };
  }

  constexpr vector_16<T>
  atan2(T s) const
  {
    return { math::fatan2(x, s), math::fatan2(y, s), math::fatan2(z, s), math::fatan2(w, s), math::fatan2(a, s), math::fatan2(b, s),
             math::fatan2(c, s), math::fatan2(d, s), math::fatan2(e, s), math::fatan2(f, s), math::fatan2(g, s), math::fatan2(h, s),
             math::fatan2(i, s), math::fatan2(j, s), math::fatan2(k, s), math::fatan2(l, s) };
  }

  constexpr T
  sum() const
  {
    return x + y + z + w + a + b + c + d + e + f + g + h + i + j + k + l;
  }

  constexpr T
  mean() const
  {
    return sum() / T{ 16 };
  }

  constexpr T
  prod() const
  {
    return x * y * z * w * a * b * c * d * e * f * g * h * i * j * k * l;
  }

  constexpr T
  min_component() const
  {
    T p0 = math::fmin(math::fmin(x, y), math::fmin(z, w));
    T p1 = math::fmin(math::fmin(a, b), math::fmin(c, d));
    T p2 = math::fmin(math::fmin(e, f), math::fmin(g, h));
    T p3 = math::fmin(math::fmin(i, j), math::fmin(k, l));
    return math::fmin(math::fmin(p0, p1), math::fmin(p2, p3));
  }

  constexpr T
  max_component() const
  {
    T p0 = math::fmax(math::fmax(x, y), math::fmax(z, w));
    T p1 = math::fmax(math::fmax(a, b), math::fmax(c, d));
    T p2 = math::fmax(math::fmax(e, f), math::fmax(g, h));
    T p3 = math::fmax(math::fmax(i, j), math::fmax(k, l));
    return math::fmax(math::fmax(p0, p1), math::fmax(p2, p3));
  }

  constexpr int
  argmin() const
  {
    const T vs[16] = { x, y, z, w, a, b, c, d, e, f, g, h, i, j, k, l };
    int idx = 0;
    for ( int n = 1; n < 16; ++n )
      if ( vs[n] < vs[idx] )
        idx = n;
    return idx;
  }

  constexpr int
  argmax() const
  {
    const T vs[16] = { x, y, z, w, a, b, c, d, e, f, g, h, i, j, k, l };
    int idx = 0;
    for ( int n = 1; n < 16; ++n )
      if ( vs[n] > vs[idx] )
        idx = n;
    return idx;
  }

  constexpr bool
  all() const
  {
    return x != T{} && y != T{} && z != T{} && w != T{} && a != T{} && b != T{} && c != T{} && d != T{} && e != T{} && f != T{} && g != T{}
           && h != T{} && i != T{} && j != T{} && k != T{} && l != T{};
  }

  constexpr bool
  any() const
  {
    return x != T{} || y != T{} || z != T{} || w != T{} || a != T{} || b != T{} || c != T{} || d != T{} || e != T{} || f != T{} || g != T{}
           || h != T{} || i != T{} || j != T{} || k != T{} || l != T{};
  }

  constexpr bool
  operator==(const vector_16<T> &o) const
  {
    return x == o.x && y == o.y && z == o.z && w == o.w && a == o.a && b == o.b && c == o.c && d == o.d && e == o.e && f == o.f && g == o.g
           && h == o.h && i == o.i && j == o.j && k == o.k && l == o.l;
  }

  constexpr bool
  operator!=(const vector_16<T> &o) const
  {
    return !(*this == o);
  }

  constexpr bool
  almost_equal(const vector_16<T> &o, T eps = math::default_eps<T>()) const
  {
    return math::fabs(x - o.x) <= eps && math::fabs(y - o.y) <= eps && math::fabs(z - o.z) <= eps && math::fabs(w - o.w) <= eps
           && math::fabs(a - o.a) <= eps && math::fabs(b - o.b) <= eps && math::fabs(c - o.c) <= eps && math::fabs(d - o.d) <= eps
           && math::fabs(e - o.e) <= eps && math::fabs(f - o.f) <= eps && math::fabs(g - o.g) <= eps && math::fabs(h - o.h) <= eps
           && math::fabs(i - o.i) <= eps && math::fabs(j - o.j) <= eps && math::fabs(k - o.k) <= eps && math::fabs(l - o.l) <= eps;
  }

#define __v16_cmpop(op)                                                                                                                    \
  constexpr vector_16<T> operator op(const vector_16<T> &o) const                                                                          \
  {                                                                                                                                        \
    return { static_cast<T>(x op o.x), static_cast<T>(y op o.y), static_cast<T>(z op o.z), static_cast<T>(w op o.w),                       \
             static_cast<T>(a op o.a), static_cast<T>(b op o.b), static_cast<T>(c op o.c), static_cast<T>(d op o.d),                       \
             static_cast<T>(e op o.e), static_cast<T>(f op o.f), static_cast<T>(g op o.g), static_cast<T>(h op o.h),                       \
             static_cast<T>(i op o.i), static_cast<T>(j op o.j), static_cast<T>(k op o.k), static_cast<T>(l op o.l) };                     \
  }                                                                                                                                        \
  constexpr vector_16<T> operator op(T s) const                                                                                            \
  {                                                                                                                                        \
    return { static_cast<T>(x op s), static_cast<T>(y op s), static_cast<T>(z op s), static_cast<T>(w op s),                               \
             static_cast<T>(a op s), static_cast<T>(b op s), static_cast<T>(c op s), static_cast<T>(d op s),                               \
             static_cast<T>(e op s), static_cast<T>(f op s), static_cast<T>(g op s), static_cast<T>(h op s),                               \
             static_cast<T>(i op s), static_cast<T>(j op s), static_cast<T>(k op s), static_cast<T>(l op s) };                             \
  }                                                                                                                                        \
  friend constexpr vector_16<T> operator op(T s, const vector_16<T> &v)                                                                    \
  {                                                                                                                                        \
    return { static_cast<T>(s op v.x), static_cast<T>(s op v.y), static_cast<T>(s op v.z), static_cast<T>(s op v.w),                       \
             static_cast<T>(s op v.a), static_cast<T>(s op v.b), static_cast<T>(s op v.c), static_cast<T>(s op v.d),                       \
             static_cast<T>(s op v.e), static_cast<T>(s op v.f), static_cast<T>(s op v.g), static_cast<T>(s op v.h),                       \
             static_cast<T>(s op v.i), static_cast<T>(s op v.j), static_cast<T>(s op v.k), static_cast<T>(s op v.l) };                     \
  }
  __v16_cmpop(<) __v16_cmpop(<=) __v16_cmpop(>) __v16_cmpop(>=)
#undef __v16_cmpop

      constexpr vector_4<T> q0() const
  {
    return { x, y, z, w };
  }

  constexpr vector_4<T>
  q1() const
  {
    return { a, b, c, d };
  }

  constexpr vector_4<T>
  q2() const
  {
    return { e, f, g, h };
  }

  constexpr vector_4<T>
  q3() const
  {
    return { i, j, k, l };
  }

  constexpr vector_8<T>
  lo() const
  {
    return { x, y, z, w, a, b, c, d };
  }

  constexpr vector_8<T>
  hi() const
  {
    return { e, f, g, h, i, j, k, l };
  }

  constexpr vector_8<T>
  even() const
  {
    return { x, z, a, c, e, g, i, k };
  }

  constexpr vector_8<T>
  odd() const
  {
    return { y, w, b, d, f, h, j, l };
  }

  constexpr vector_16<T>
  rev() const
  {
    return { l, k, j, i, h, g, f, e, d, c, b, a, w, z, y, x };
  }

  constexpr vector_16<T>
  fma(const vector_16<T> &v, const vector_16<T> &u) const
  {
    return { math::ffma(x, v.x, u.x), math::ffma(y, v.y, u.y), math::ffma(z, v.z, u.z), math::ffma(w, v.w, u.w),
             math::ffma(a, v.a, u.a), math::ffma(b, v.b, u.b), math::ffma(c, v.c, u.c), math::ffma(d, v.d, u.d),
             math::ffma(e, v.e, u.e), math::ffma(f, v.f, u.f), math::ffma(g, v.g, u.g), math::ffma(h, v.h, u.h),
             math::ffma(i, v.i, u.i), math::ffma(j, v.j, u.j), math::ffma(k, v.k, u.k), math::ffma(l, v.l, u.l) };
  }

  constexpr vector_16<T>
  fma(T s, const vector_16<T> &u) const
  {
    return { math::ffma(x, s, u.x), math::ffma(y, s, u.y), math::ffma(z, s, u.z), math::ffma(w, s, u.w),
             math::ffma(a, s, u.a), math::ffma(b, s, u.b), math::ffma(c, s, u.c), math::ffma(d, s, u.d),
             math::ffma(e, s, u.e), math::ffma(f, s, u.f), math::ffma(g, s, u.g), math::ffma(h, s, u.h),
             math::ffma(i, s, u.i), math::ffma(j, s, u.j), math::ffma(k, s, u.k), math::ffma(l, s, u.l) };
  }

  constexpr vector_16<T>
  mul_add(const vector_16<T> &v, const vector_16<T> &u) const
  {
    return fma(v, u);
  }

  constexpr vector_16<T>
  mul_add(T s, const vector_16<T> &u) const
  {
    return fma(s, u);
  }
};

}     // namespace micron
