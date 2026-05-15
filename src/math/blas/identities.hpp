//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../matrix/mat.hpp"
#include "../matrix/views.hpp"
#include "../mk.hpp"
#include "../quants/quat.hpp"
#include "../quants/vec.hpp"
#include "../quants/views.hpp"
#include "bits/impl.hpp"
#include "concepts.hpp"
#include "level2.hpp"

namespace micron
{
namespace math
{
namespace blas
{
namespace identities
{

using micron::math::matrix::col_view;
using micron::math::matrix::row_view;
using micron::math::quants::vec_view;

//   R = [ 1-2(y²+z²)   2(xy-wz)    2(xz+wy)  ]
//       [ 2(xy+wz)    1-2(x²+z²)   2(yz-wx)  ]
//       [ 2(xz-wy)    2(yz+wx)    1-2(x²+y²) ]
template<ieee754_floating F, typename V>
[[gnu::flatten]] inline constexpr void
from_quat(const quat<F> &q, V out) noexcept
{
  const F x = q.data[0], y = q.data[1], z = q.data[2], w = q.data[3];
  const F xx = x * x, yy = y * y, zz = z * z;
  const F xy = x * y, xz = x * z, yz = y * z;
  const F wx = w * x, wy = w * y, wz = w * z;
  const auto wr = [&](usize r, usize c, F v) constexpr noexcept {
    if constexpr ( row_view_like<V> )
      bits::mat_at(out.data, r, c, ssize_t(out.ld), 1) = v;
    else
      bits::mat_at(out.data, r, c, 1, ssize_t(out.ld)) = v;
  };
  wr(0, 0, F(1) - F(2) * (yy + zz));
  wr(0, 1, F(2) * (xy - wz));
  wr(0, 2, F(2) * (xz + wy));
  wr(1, 0, F(2) * (xy + wz));
  wr(1, 1, F(1) - F(2) * (xx + zz));
  wr(1, 2, F(2) * (yz - wx));
  wr(2, 0, F(2) * (xz - wy));
  wr(2, 1, F(2) * (yz + wx));
  wr(2, 2, F(1) - F(2) * (xx + yy));
}

// fixed-size return form
template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr mat<F, 3, 3>
from_quat(const quat<F> &q) noexcept
{
  mat<F, 3, 3> m{};
  row_view<F> v{ m.data, 3, 3, 3 };
  from_quat<F>(q, v);
  return m;
}

//   [v]_× = [  0  -z   y ]
//           [  z   0  -x ]
//           [ -y   x   0 ]
template<ieee754_floating F, typename V>
[[gnu::flatten]] inline constexpr void
skew(F vx, F vy, F vz, V out) noexcept
{
  const auto wr = [&](usize r, usize c, F v) constexpr noexcept {
    if constexpr ( row_view_like<V> )
      bits::mat_at(out.data, r, c, ssize_t(out.ld), 1) = v;
    else
      bits::mat_at(out.data, r, c, 1, ssize_t(out.ld)) = v;
  };
  wr(0, 0, F(0));
  wr(0, 1, -vz);
  wr(0, 2, vy);
  wr(1, 0, vz);
  wr(1, 1, F(0));
  wr(1, 2, -vx);
  wr(2, 0, -vy);
  wr(2, 1, vx);
  wr(2, 2, F(0));
}

template<ieee754_floating F, typename V>
[[gnu::flatten]] inline constexpr void
skew(const vec<F, 3> &v, V out) noexcept
{
  skew<F>(v.data[0], v.data[1], v.data[2], out);
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr mat<F, 3, 3>
skew(const vec<F, 3> &v) noexcept
{
  mat<F, 3, 3> m{};
  row_view<F> rv{ m.data, 3, 3, 3 };
  skew<F>(v, rv);
  return m;
}

template<blas_scalar T, typename V>
  requires(mat_view_like<V>)
[[gnu::flatten]] inline constexpr void
outer(T alpha, const vec_view<T> &x, const vec_view<T> &y, V A) noexcept
{
  const ssize_t rs = row_view_like<V> ? ssize_t(A.ld) : ssize_t(1);
  const ssize_t cs = row_view_like<V> ? ssize_t(1) : ssize_t(A.ld);
  for ( usize i = 0; i < A.rows; ++i )
    for ( usize j = 0; j < A.cols; ++j ) bits::mat_at(A.data, i, j, rs, cs) = T(0);
  level2::ger<T>(alpha, x, y, A);
}

template<ieee754_floating F, typename V>
  requires(mat_view_like<V>)
[[gnu::flatten]] inline constexpr void
givens(usize i, usize j, F c, F s, V out) noexcept
{
  const ssize_t rs = row_view_like<V> ? ssize_t(out.ld) : ssize_t(1);
  const ssize_t cs = row_view_like<V> ? ssize_t(1) : ssize_t(out.ld);
  for ( usize r = 0; r < out.rows; ++r )
    for ( usize cc = 0; cc < out.cols; ++cc ) bits::mat_at(out.data, r, cc, rs, cs) = (r == cc) ? F(1) : F(0);
  bits::mat_at(out.data, i, i, rs, cs) = c;
  bits::mat_at(out.data, j, j, rs, cs) = c;
  bits::mat_at(out.data, i, j, rs, cs) = s;
  bits::mat_at(out.data, j, i, rs, cs) = -s;
}

template<blas_scalar T, typename V>
  requires(mat_view_like<V>)
[[gnu::flatten]] inline constexpr void
householder(const vec_view<T> &v, V H_out, T beta) noexcept
{
  const ssize_t rs = row_view_like<V> ? ssize_t(H_out.ld) : ssize_t(1);
  const ssize_t cs = row_view_like<V> ? ssize_t(1) : ssize_t(H_out.ld);
  for ( usize r = 0; r < H_out.rows; ++r ) {
    const T mb_vr = -beta * v.data[ssize_t(r) * v.inc];
    for ( usize c = 0; c < H_out.cols; ++c ) {
      const T vc = v.data[ssize_t(c) * v.inc];
      const T diag = (r == c) ? T(1) : T(0);
      bits::mat_at(H_out.data, r, c, rs, cs) = bits::fma_acc<T>(mb_vr, vc, diag);
    }
  }
}

template<blas_real T, typename V>
  requires(mat_view_like<V>)
[[gnu::flatten]] inline constexpr void
householder(const vec_view<T> &v, V H_out) noexcept
{
  T vtv = T(0);
  for ( usize i = 0; i < v.n; ++i ) {
    const T vi = v.data[ssize_t(i) * v.inc];
    vtv = math::fma<T>(vi, vi, vtv);
  }
  const T beta = (vtv > T(0)) ? T(2) / vtv : T(0);
  householder<T>(v, H_out, beta);
}

template<ieee754_floating F, typename V>
  requires(mat_view_like<V>)
[[gnu::flatten]] inline constexpr void
from_axis_angle(F kx, F ky, F kz, F theta, V out) noexcept
{
  const F c = mk::trig::cos<F>(theta);
  const F s = mk::trig::sin<F>(theta);
  const F omc = F(1) - c;
  const auto wr = [&](usize r, usize cc, F v) constexpr noexcept {
    if constexpr ( row_view_like<V> )
      bits::mat_at(out.data, r, cc, ssize_t(out.ld), 1) = v;
    else
      bits::mat_at(out.data, r, cc, 1, ssize_t(out.ld)) = v;
  };
  wr(0, 0, c + kx * kx * omc);
  wr(0, 1, kx * ky * omc - kz * s);
  wr(0, 2, kx * kz * omc + ky * s);
  wr(1, 0, ky * kx * omc + kz * s);
  wr(1, 1, c + ky * ky * omc);
  wr(1, 2, ky * kz * omc - kx * s);
  wr(2, 0, kz * kx * omc - ky * s);
  wr(2, 1, kz * ky * omc + kx * s);
  wr(2, 2, c + kz * kz * omc);
}

template<ieee754_floating F, typename V>
  requires(mat_view_like<V>)
[[gnu::flatten]] inline constexpr void
from_axis_angle(const vec<F, 3> &axis, F theta, V out) noexcept
{
  from_axis_angle<F>(axis.data[0], axis.data[1], axis.data[2], theta, out);
}

template<ieee754_floating F>
[[nodiscard, gnu::flatten]] inline constexpr mat<F, 3, 3>
from_axis_angle(const vec<F, 3> &axis, F theta) noexcept
{
  mat<F, 3, 3> m{};
  row_view<F> rv{ m.data, 3, 3, 3 };
  from_axis_angle<F>(axis, theta, rv);
  return m;
}

};      // namespace identities
};      // namespace blas
};      // namespace math
};      // namespace micron
