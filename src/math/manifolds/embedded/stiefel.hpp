//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// stiefel orthonormal P-frames

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../linalg/decomp_ext.hpp"
#include "../../linalg/ops.hpp"
#include "../../matrix/mat.hpp"
#include "../../mk.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

namespace __stiefel_impl
{

template<ieee754_floating F, usize N, usize P>
[[nodiscard, gnu::flatten]] inline mat<F, P, P>
xt_v(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
{
  mat<F, P, P> M{};
  for ( usize i = 0; i < P; ++i )
    for ( usize j = 0; j < P; ++j ) {
      F acc = F(0);
      for ( usize k = 0; k < N; ++k ) acc = math::fma<F>(X.data[k * P + i], V.data[k * P + j], acc);
      M.data[i * P + j] = acc;
    }
  return M;
}

template<ieee754_floating F, usize N, usize P>
[[nodiscard]] inline mat<F, N, P>
mgs(const mat<F, N, P> &A) noexcept
{
  mat<F, N, P> Q = A;
  for ( usize j = 0; j < P; ++j ) {
    F s = F(0);
    for ( usize i = 0; i < N; ++i ) s = math::fma<F>(Q.data[i * P + j], Q.data[i * P + j], s);
    s = math::fsqrt(s);
    if ( s > math::default_eps<F>() ) {
      const F inv = F(1) / s;
      for ( usize i = 0; i < N; ++i ) Q.data[i * P + j] *= inv;
    }
    for ( usize k = j + 1; k < P; ++k ) {
      F proj = F(0);
      for ( usize i = 0; i < N; ++i ) proj = math::fma<F>(Q.data[i * P + j], Q.data[i * P + k], proj);
      for ( usize i = 0; i < N; ++i ) Q.data[i * P + k] -= proj * Q.data[i * P + j];
    }
  }
  return Q;
}

};      // namespace __stiefel_impl

// NOTE: P >= 2 is required
template<ieee754_floating F, usize N, usize P>
  requires(N >= P && P >= 2)
struct stiefel {
  using value_type = F;
  static constexpr usize rows = N;
  static constexpr usize cols = P;

  [[nodiscard, gnu::flatten]] static mat<F, N, P>
  project_to_tangent(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
  {
    auto A = __stiefel_impl::xt_v<F, N, P>(X, V);
    // S = (A + Aᵀ) / 2
    mat<F, P, P> S{};
    for ( usize i = 0; i < P; ++i )
      for ( usize j = 0; j < P; ++j ) S.data[i * P + j] = F(0.5) * (A.data[i * P + j] + A.data[j * P + i]);
    auto XS = linalg::ops::gemm(X, S);
    mat<F, N, P> R{};
    for ( usize i = 0; i < N * P; ++i ) R.data[i] = V.data[i] - XS.data[i];
    return R;
  }

  [[nodiscard]] static mat<F, N, P>
  retract_qr(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
  {
    mat<F, N, P> A{};
    for ( usize i = 0; i < N * P; ++i ) A.data[i] = X.data[i] + V.data[i];
    return __stiefel_impl::mgs<F, N, P>(A);
  }

  [[nodiscard]] static mat<F, N, P>
  retract_polar(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
  {
    mat<F, N, P> A{};
    for ( usize i = 0; i < N * P; ++i ) A.data[i] = X.data[i] + V.data[i];
    auto s = linalg::decomp::svd<F, N, P>(A);
    // polar factor Q = U[:, :P] · Vᵀ ; U is N×N, V is P×P.
    mat<F, N, P> Q{};
    for ( usize r = 0; r < N; ++r ) {
      for ( usize c = 0; c < P; ++c ) {
        F acc = F(0);
        for ( usize k = 0; k < P; ++k ) acc = math::fma<F>(s.U.data[r * N + k], s.V.data[c * P + k], acc);
        Q.data[r * P + c] = acc;
      }
    }
    return Q;
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  exp_map(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
  {
    return retract_polar(X, V);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  log_map(const mat<F, N, P> &X, const mat<F, N, P> &Y) noexcept
  {
    mat<F, N, P> diff{};
    for ( usize i = 0; i < N * P; ++i ) diff.data[i] = Y.data[i] - X.data[i];
    return project_to_tangent(X, diff);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  retract(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
  {
    return retract_polar(X, V);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  inverse_retract(const mat<F, N, P> &X, const mat<F, N, P> &Y) noexcept
  {
    return log_map(X, Y);
  }

  [[nodiscard]] static mat<F, N, P>
  project_to_manifold(const mat<F, N, P> &A) noexcept
  {
    auto s = linalg::decomp::svd<F, N, P>(A);
    mat<F, N, P> Q{};
    for ( usize r = 0; r < N; ++r ) {
      for ( usize c = 0; c < P; ++c ) {
        F acc = F(0);
        for ( usize k = 0; k < P; ++k ) acc = math::fma<F>(s.U.data[r * N + k], s.V.data[c * P + k], acc);
        Q.data[r * P + c] = acc;
      }
    }
    return Q;
  }

  [[nodiscard, gnu::flatten]] static F
  inner(const mat<F, N, P> &, const mat<F, N, P> &U, const mat<F, N, P> &V) noexcept
  {
    F s = F(0);
    for ( usize i = 0; i < N * P; ++i ) s = math::fma<F>(U.data[i], V.data[i], s);
    return s;
  }

  [[nodiscard, gnu::flatten]] static F
  inner_canonical(const mat<F, N, P> &X, const mat<F, N, P> &U, const mat<F, N, P> &V) noexcept
  {
    // tr(Uᵀ V) − ½ tr((Uᵀ X)(Xᵀ V))
    F t1 = F(0);
    for ( usize i = 0; i < N * P; ++i ) t1 = math::fma<F>(U.data[i], V.data[i], t1);
    auto UtX = __stiefel_impl::xt_v<F, N, P>(U, X);      // U^T X : P×P
    auto XtV = __stiefel_impl::xt_v<F, N, P>(X, V);      // X^T V : P×P
    F t2 = F(0);
    for ( usize i = 0; i < P; ++i )
      for ( usize j = 0; j < P; ++j ) t2 = math::fma<F>(UtX.data[i * P + j], XtV.data[j * P + i], t2);
    return t1 - F(0.5) * t2;
  }

  [[nodiscard, gnu::always_inline]] static F
  norm(const mat<F, N, P> &p, const mat<F, N, P> &v) noexcept
  {
    return math::fsqrt(inner(p, v, v));
  }

  [[nodiscard]] static F
  distance(const mat<F, N, P> &X, const mat<F, N, P> &Y) noexcept
  {
    F s = F(0);
    for ( usize i = 0; i < N * P; ++i ) {
      const F d = X.data[i] - Y.data[i];
      s = math::fma<F>(d, d, s);
    }
    return math::fsqrt(s);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  vector_transport(const mat<F, N, P> &, const mat<F, N, P> &Y, const mat<F, N, P> &V) noexcept
  {
    return project_to_tangent(Y, V);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  parallel_transport(const mat<F, N, P> &X, const mat<F, N, P> &Y, const mat<F, N, P> &V) noexcept
  {
    return vector_transport(X, Y, V);
  }
};

template<ieee754_floating F, usize N, usize P> struct traits<stiefel<F, N, P>> {
  using point_type = mat<F, N, P>;
  using tangent_type = mat<F, N, P>;
  using scalar_type = F;
  using category = stiefel_tag;
  static constexpr usize dim = N * P - P * (P + 1) / 2;
  static constexpr usize ambient_dim = N * P;
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
