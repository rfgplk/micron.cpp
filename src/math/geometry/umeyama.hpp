//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../ieee.hpp"
#include "../linalg/decomp.hpp"
#include "../linalg/ops.hpp"
#include "../matrix/dynmat.hpp"
#include "../matrix/mat.hpp"
#include "transform.hpp"

namespace micron
{
namespace math
{
namespace geometry
{

namespace __impl_umeyama
{

// svd3 forms u_j = H*v_j / sigma_j and simply skips the scaling when sigma_j == 0
template<ieee754_floating F>
inline void
complete_u(mat<F, 3, 3> &U) noexcept
{
  vec<F, 3> u0{ U.data[0], U.data[3], U.data[6] };
  vec<F, 3> u1{ U.data[1], U.data[4], U.data[7] };

  F n0 = math::fsqrt(u0.data[0] * u0.data[0] + u0.data[1] * u0.data[1] + u0.data[2] * u0.data[2]);
  if ( !(n0 > F(0.5)) ) {
    u0 = vec<F, 3>{ F(1), F(0), F(0) };
    n0 = F(1);
  }
  const F inv0 = F(1) / n0;
  for ( usize i = 0; i < 3; ++i ) u0.data[i] *= inv0;

  F d = u1.data[0] * u0.data[0] + u1.data[1] * u0.data[1] + u1.data[2] * u0.data[2];
  for ( usize i = 0; i < 3; ++i ) u1.data[i] -= d * u0.data[i];
  F n1 = math::fsqrt(u1.data[0] * u1.data[0] + u1.data[1] * u1.data[1] + u1.data[2] * u1.data[2]);
  if ( !(n1 > F(0.5)) ) {
    const F ax = math::fabs(u0.data[0]);
    const F ay = math::fabs(u0.data[1]);
    const F az = math::fabs(u0.data[2]);
    u1 = vec<F, 3>{ F(0), F(0), F(0) };
    if ( ax <= ay && ax <= az )
      u1.data[0] = F(1);
    else if ( ay <= az )
      u1.data[1] = F(1);
    else
      u1.data[2] = F(1);
    d = u1.data[0] * u0.data[0] + u1.data[1] * u0.data[1] + u1.data[2] * u0.data[2];
    for ( usize i = 0; i < 3; ++i ) u1.data[i] -= d * u0.data[i];
    n1 = math::fsqrt(u1.data[0] * u1.data[0] + u1.data[1] * u1.data[1] + u1.data[2] * u1.data[2]);
  }
  const F inv1 = F(1) / n1;
  for ( usize i = 0; i < 3; ++i ) u1.data[i] *= inv1;

  const vec<F, 3> u2 = linalg::ops::cross<F>(u0, u1);
  for ( usize i = 0; i < 3; ++i ) {
    U.data[i * 3 + 0] = u0.data[i];
    U.data[i * 3 + 1] = u1.data[i];
    U.data[i * 3 + 2] = u2.data[i];
  }
}

};      // namespace __impl_umeyama

template<ieee754_floating F>
[[nodiscard]] inline transform<F, 3, transform_mode::affine>
umeyama(const dynmat<F> &src, const dynmat<F> &dst, bool with_scaling = true) noexcept
{
  transform<F, 3, transform_mode::affine> out{};
  out.M = mat<F, 4, 4>::identity();
  const usize N = src.cols;
  if ( N == 0 || src.rows != 3 || dst.rows != 3 || dst.cols != N ) return out;

  // centroids
  vec<F, 3> cs{};
  vec<F, 3> cd{};
  for ( usize i = 0; i < 3; ++i ) {
    F s = F(0), d = F(0);
    for ( usize j = 0; j < N; ++j ) {
      s += src.at(i, j);
      d += dst.at(i, j);
    }
    cs.data[i] = s / F(N);
    cd.data[i] = d / F(N);
  }

  // cross-covariance H = (1/N) * sum_j (dst_j - cd) (src_j - cs)^T
  mat<F, 3, 3> H = mat<F, 3, 3>::zero();
  F var_src = F(0);
  for ( usize j = 0; j < N; ++j ) {
    F sx[3];
    F dx[3];
    for ( usize i = 0; i < 3; ++i ) {
      sx[i] = src.at(i, j) - cs.data[i];
      dx[i] = dst.at(i, j) - cd.data[i];
    }
    for ( usize i = 0; i < 3; ++i )
      for ( usize k = 0; k < 3; ++k ) H.data[i * 3 + k] += dx[i] * sx[k];
    for ( usize i = 0; i < 3; ++i ) var_src += sx[i] * sx[i];
  }
  for ( usize i = 0; i < 9; ++i ) H.data[i] /= F(N);
  var_src /= F(N);

  // SVD of H
  auto sv = linalg::decomp::svd3<F>(H);
  // build sign matrix S to ensure det(R) = +1
  mat<F, 3, 3> Vt{};
  for ( usize i = 0; i < 3; ++i )
    for ( usize j = 0; j < 3; ++j ) Vt.data[i * 3 + j] = sv.V.data[j * 3 + i];      // V^T

  // det(U) * det(V) sign
  F detU = linalg::ops::det3<F>(sv.U);
  if ( !(math::fabs(detU) > F(0.5)) ) {
    __impl_umeyama::complete_u<F>(sv.U);
    detU = linalg::ops::det3<F>(sv.U);
  }
  F detV = linalg::ops::det3<F>(sv.V);
  F sgn = (detU * detV >= F(0)) ? F(1) : F(-1);

  // R = U * diag(1, 1, sgn) * V^T
  mat<F, 3, 3> R = mat<F, 3, 3>::zero();
  for ( usize i = 0; i < 3; ++i )
    for ( usize j = 0; j < 3; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < 3; ++k ) {
        F sign_k = (k == 2) ? sgn : F(1);
        s += sv.U.data[i * 3 + k] * sign_k * Vt.data[k * 3 + j];
      }
      R.data[i * 3 + j] = s;
    }

  // scale
  F scale = F(1);
  if ( with_scaling ) {
    F trace_Ds = sv.S.data[0] + sv.S.data[1] + sgn * sv.S.data[2];
    if ( var_src > F(0) ) scale = trace_Ds / var_src;
  }

  // translation: cd - scale * R * cs
  vec<F, 3> t{};
  for ( usize i = 0; i < 3; ++i ) {
    F s = cd.data[i];
    for ( usize j = 0; j < 3; ++j ) s -= scale * R.data[i * 3 + j] * cs.data[j];
    t.data[i] = s;
  }

  // assemble into homogeneous 4x4
  out.M = mat<F, 4, 4>::zero();
  for ( usize i = 0; i < 3; ++i )
    for ( usize j = 0; j < 3; ++j ) out.M.data[i * 4 + j] = scale * R.data[i * 3 + j];
  for ( usize i = 0; i < 3; ++i ) out.M.data[i * 4 + 3] = t.data[i];
  out.M.data[3 * 4 + 3] = F(1);
  return out;
}

};      // namespace geometry
};      // namespace math
};      // namespace micron
