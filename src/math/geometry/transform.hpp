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
#include "../linalg/types.hpp"
#include "../manifolds/lie/se3.hpp"
#include "../manifolds/lie/so3.hpp"
#include "../matrix/mat.hpp"
#include "../quants/quat.hpp"
#include "../quants/vec.hpp"
#include "scaling.hpp"
#include "translation.hpp"

namespace micron
{
namespace math
{
namespace geometry
{

enum class transform_mode {
  isometry,
  affine,
  projective,
};

template <ieee754_floating F, usize Dim, transform_mode Mode = transform_mode::affine>
  requires(Dim == 2 || Dim == 3)
struct transform {
  static constexpr usize H = Dim + 1;     // homogeneous size
  mat<F, H, H> M;

  [[nodiscard]] static constexpr transform
  identity() noexcept
  {
    transform r{};
    r.M = mat<F, H, H>::identity();
    return r;
  }

  [[nodiscard]] static constexpr transform
  from_translation(const translation<F, Dim> &t) noexcept
  {
    transform r{};
    r.M = mat<F, H, H>::identity();
    for ( usize i = 0; i < Dim; ++i ) r.M.data[i * H + Dim] = t.t.data[i];
    return r;
  }

  [[nodiscard]] static constexpr transform
  from_scaling(const scaling<F, Dim> &s) noexcept
  {
    transform r{};
    r.M = mat<F, H, H>::zero();
    for ( usize i = 0; i < Dim; ++i ) r.M.data[i * H + i] = s.s.data[i];
    r.M.data[Dim * H + Dim] = F(1);
    return r;
  }

  [[nodiscard]] static constexpr transform
  from_linear(const mat<F, Dim, Dim> &A) noexcept
  {
    transform r{};
    r.M = mat<F, H, H>::zero();
    for ( usize i = 0; i < Dim; ++i )
      for ( usize j = 0; j < Dim; ++j ) r.M.data[i * H + j] = A.data[i * Dim + j];
    r.M.data[Dim * H + Dim] = F(1);
    return r;
  }

  [[nodiscard]] static constexpr transform
  from_so3(const manifolds::lie::SO3<F> &R) noexcept
    requires(Dim == 3)
  {
    transform out{};
    out.M = mat<F, H, H>::zero();
    auto Rm = manifolds::lie::SO3<F>::to_matrix(R);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) out.M.data[i * H + j] = Rm.data[i * 3 + j];
    out.M.data[3 * H + 3] = F(1);
    return out;
  }

  [[nodiscard]] static constexpr transform
  from_se3(const manifolds::lie::SE3<F> &T) noexcept
    requires(Dim == 3)
  {
    transform out{};
    out.M = mat<F, H, H>::zero();
    auto Rm = manifolds::lie::SO3<F>::to_matrix(T.R);
    for ( usize i = 0; i < 3; ++i )
      for ( usize j = 0; j < 3; ++j ) out.M.data[i * H + j] = Rm.data[i * 3 + j];
    for ( usize i = 0; i < 3; ++i ) out.M.data[i * H + 3] = T.t.data[i];
    out.M.data[3 * H + 3] = F(1);
    return out;
  }

  [[nodiscard]] static constexpr transform
  from_quat(const quat<F> &q) noexcept
    requires(Dim == 3)
  {
    transform out{};
    out.M = mat<F, H, H>::zero();
    const F x = q.data[0];
    const F y = q.data[1];
    const F z = q.data[2];
    const F w = q.data[3];
    const F xx = x * x, yy = y * y, zz = z * z;
    const F xy = x * y, xz = x * z, yz = y * z;
    const F wx = w * x, wy = w * y, wz = w * z;
    out.M.data[0 * H + 0] = F(1) - F(2) * (yy + zz);
    out.M.data[0 * H + 1] = F(2) * (xy - wz);
    out.M.data[0 * H + 2] = F(2) * (xz + wy);
    out.M.data[1 * H + 0] = F(2) * (xy + wz);
    out.M.data[1 * H + 1] = F(1) - F(2) * (xx + zz);
    out.M.data[1 * H + 2] = F(2) * (yz - wx);
    out.M.data[2 * H + 0] = F(2) * (xz - wy);
    out.M.data[2 * H + 1] = F(2) * (yz + wx);
    out.M.data[2 * H + 2] = F(1) - F(2) * (xx + yy);
    out.M.data[3 * H + 3] = F(1);
    return out;
  }

  [[nodiscard, gnu::always_inline]] constexpr mat<F, Dim, Dim>
  linear_part() const noexcept
  {
    mat<F, Dim, Dim> r{};
    for ( usize i = 0; i < Dim; ++i )
      for ( usize j = 0; j < Dim; ++j ) r.data[i * Dim + j] = M.data[i * H + j];
    return r;
  }

  [[nodiscard, gnu::always_inline]] constexpr vec<F, Dim>
  translation_part() const noexcept
  {
    vec<F, Dim> r{};
    for ( usize i = 0; i < Dim; ++i ) r.data[i] = M.data[i * H + Dim];
    return r;
  }

  [[nodiscard]] constexpr vec<F, Dim>
  apply(const vec<F, Dim> &p) const noexcept
  {
    if constexpr ( Mode == transform_mode::projective ) {
      F w = F(0);
      for ( usize j = 0; j < Dim; ++j ) w += M.data[Dim * H + j] * p.data[j];
      w += M.data[Dim * H + Dim];
      vec<F, Dim> r{};
      const F inv_w = (w != F(0)) ? F(1) / w : F(1);
      for ( usize i = 0; i < Dim; ++i ) {
        F s = M.data[i * H + Dim];
        for ( usize j = 0; j < Dim; ++j ) s += M.data[i * H + j] * p.data[j];
        r.data[i] = s * inv_w;
      }
      return r;
    } else {
      vec<F, Dim> r{};
      for ( usize i = 0; i < Dim; ++i ) {
        F s = M.data[i * H + Dim];
        for ( usize j = 0; j < Dim; ++j ) s += M.data[i * H + j] * p.data[j];
        r.data[i] = s;
      }
      return r;
    }
  }

  [[nodiscard]] constexpr vec<F, H>
  apply_homogeneous(const vec<F, H> &p) const noexcept
  {
    vec<F, H> r{};
    for ( usize i = 0; i < H; ++i ) {
      F s = F(0);
      for ( usize j = 0; j < H; ++j ) s += M.data[i * H + j] * p.data[j];
      r.data[i] = s;
    }
    return r;
  }

  [[nodiscard]] constexpr transform
  inverse() const noexcept
  {
    transform out{};
    if constexpr ( Mode == transform_mode::isometry ) {
      out.M = mat<F, H, H>::zero();
      for ( usize i = 0; i < Dim; ++i )
        for ( usize j = 0; j < Dim; ++j ) out.M.data[i * H + j] = M.data[j * H + i];     // R^T
      for ( usize i = 0; i < Dim; ++i ) {
        F s = F(0);
        for ( usize j = 0; j < Dim; ++j ) s -= out.M.data[i * H + j] * M.data[j * H + Dim];
        out.M.data[i * H + Dim] = s;
      }
      out.M.data[Dim * H + Dim] = F(1);
      return out;
    } else if constexpr ( Mode == transform_mode::affine ) {
      auto A = linear_part();
      auto t = translation_part();
      mat<F, Dim, Dim> Ainv{};
      if constexpr ( Dim == 2 ) {
        Ainv = linalg::ops::inv2<F>(A);
      } else {
        Ainv = linalg::ops::inv3<F>(A);
      }
      out.M = mat<F, H, H>::zero();
      for ( usize i = 0; i < Dim; ++i )
        for ( usize j = 0; j < Dim; ++j ) out.M.data[i * H + j] = Ainv.data[i * Dim + j];
      for ( usize i = 0; i < Dim; ++i ) {
        F s = F(0);
        for ( usize j = 0; j < Dim; ++j ) s -= Ainv.data[i * Dim + j] * t.data[j];
        out.M.data[i * H + Dim] = s;
      }
      out.M.data[Dim * H + Dim] = F(1);
      return out;
    } else {
      auto inv_r = linalg::decomp::inv<F, H>(M);
      out.M = inv_r.X;
      return out;
    }
  }
};

template <ieee754_floating F, usize Dim, transform_mode A, transform_mode B>
[[nodiscard]] inline constexpr transform<F, Dim,
                                         (A == transform_mode::projective || B == transform_mode::projective)
                                             ? transform_mode::projective
                                             : (A == transform_mode::affine || B == transform_mode::affine ? transform_mode::affine
                                                                                                           : transform_mode::isometry)>
operator*(const transform<F, Dim, A> &a, const transform<F, Dim, B> &b) noexcept
{
  constexpr transform_mode RM
      = (A == transform_mode::projective || B == transform_mode::projective)
            ? transform_mode::projective
            : (A == transform_mode::affine || B == transform_mode::affine ? transform_mode::affine : transform_mode::isometry);
  transform<F, Dim, RM> r{};
  constexpr usize H = Dim + 1;
  for ( usize i = 0; i < H; ++i ) {
    for ( usize j = 0; j < H; ++j ) {
      F s = F(0);
      for ( usize k = 0; k < H; ++k ) s += a.M.data[i * H + k] * b.M.data[k * H + j];
      r.M.data[i * H + j] = s;
    }
  }
  return r;
}

};     // namespace geometry
};     // namespace math
};     // namespace micron
