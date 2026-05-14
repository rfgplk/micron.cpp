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

template <ieee754_floating F>
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
    for ( usize j = 0; j < 3; ++j ) Vt.data[i * 3 + j] = sv.V.data[j * 3 + i];     // V^T

  // det(U) * det(V) sign
  F detU = linalg::ops::det3<F>(sv.U);
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

};     // namespace geometry
};     // namespace math
};     // namespace micron
