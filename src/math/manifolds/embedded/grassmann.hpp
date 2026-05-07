//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%
// grassmannian

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

namespace __grassmann_impl
{

template <ieee754_floating F, usize N, usize P>
[[nodiscard, gnu::flatten]] inline mat<F, P, P>
xt_y(const mat<F, N, P> &X, const mat<F, N, P> &Y) noexcept
{
  mat<F, P, P> M{};
  for ( usize i = 0; i < P; ++i )
    for ( usize j = 0; j < P; ++j ) {
      F acc = F(0);
      for ( usize k = 0; k < N; ++k ) acc = math::fma<F>(X.data[k * P + i], Y.data[k * P + j], acc);
      M.data[i * P + j] = acc;
    }
  return M;
}

};     // namespace __grassmann_impl

template <ieee754_floating F, usize N, usize P>
  requires(N >= P && P >= 1)
struct grassmann {
  using value_type = F;
  static constexpr usize rows = N;
  static constexpr usize cols = P;

  [[nodiscard, gnu::flatten]] static mat<F, N, P>
  project_to_tangent(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
  {
    auto A = __grassmann_impl::xt_y<F, N, P>(X, V);
    auto XA = linalg::ops::gemm(X, A);
    mat<F, N, P> R{};
    for ( usize i = 0; i < N * P; ++i ) R.data[i] = V.data[i] - XA.data[i];
    return R;
  }

  [[nodiscard]] static mat<F, N, P>
  exp_map(const mat<F, N, P> &X, const mat<F, N, P> &V) noexcept
  {
    auto sv = linalg::decomp::svd<F, N, P>(V);
    F cs[P], sn[P];
    for ( usize i = 0; i < P; ++i ) {
      cs[i] = math::cos<F>(sv.S.data[i]);
      sn[i] = math::sin<F>(sv.S.data[i]);
    }
    auto XVv = linalg::ops::gemm(X, sv.V);
    mat<F, N, P> XVcos{};
    mat<F, N, P> Usin{};
    for ( usize i = 0; i < N; ++i ) {
      for ( usize k = 0; k < P; ++k ) {
        XVcos.data[i * P + k] = XVv.data[i * P + k] * cs[k];
        Usin.data[i * P + k] = sv.U.data[i * N + k] * sn[k];
      }
    }
    mat<F, N, P> R{};
    for ( usize r = 0; r < N; ++r ) {
      for ( usize c = 0; c < P; ++c ) {
        F acc = F(0);
        for ( usize k = 0; k < P; ++k ) {
          const F a = XVcos.data[r * P + k] + Usin.data[r * P + k];
          acc = math::fma<F>(a, sv.V.data[c * P + k], acc);
        }
        R.data[r * P + c] = acc;
      }
    }
    return R;
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
    return exp_map(X, V);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  inverse_retract(const mat<F, N, P> &X, const mat<F, N, P> &Y) noexcept
  {
    return log_map(X, Y);
  }

  [[nodiscard]] static vec<F, P>
  principal_angles(const mat<F, N, P> &X, const mat<F, N, P> &Y) noexcept
  {
    auto M = __grassmann_impl::xt_y<F, N, P>(X, Y);
    auto sv = linalg::decomp::svd<F, P, P>(M);
    vec<F, P> angles{};
    for ( usize i = 0; i < P; ++i ) {
      F s = sv.S.data[i];
      if ( s > F(1) ) s = F(1);
      if ( s < F(-1) ) s = F(-1);
      angles.data[i] = math::acos<F>(s);
    }
    return angles;
  }

  [[nodiscard, gnu::flatten]] static F
  distance(const mat<F, N, P> &X, const mat<F, N, P> &Y) noexcept
  {
    auto a = principal_angles(X, Y);
    F s = F(0);
    for ( usize i = 0; i < P; ++i ) s = math::fma<F>(a.data[i], a.data[i], s);
    return math::fsqrt(s);
  }

  [[nodiscard, gnu::flatten]] static F
  inner(const mat<F, N, P> &, const mat<F, N, P> &U, const mat<F, N, P> &V) noexcept
  {
    F s = F(0);
    for ( usize i = 0; i < N * P; ++i ) s = math::fma<F>(U.data[i], V.data[i], s);
    return s;
  }

  [[nodiscard, gnu::always_inline]] static F
  norm(const mat<F, N, P> &p, const mat<F, N, P> &v) noexcept
  {
    return math::fsqrt(inner(p, v, v));
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

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  parallel_transport(const mat<F, N, P> &, const mat<F, N, P> &Y, const mat<F, N, P> &V) noexcept
  {
    return project_to_tangent(Y, V);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, P>
  vector_transport(const mat<F, N, P> &, const mat<F, N, P> &Y, const mat<F, N, P> &V) noexcept
  {
    return project_to_tangent(Y, V);
  }
};

template <ieee754_floating F, usize N, usize P> struct traits<grassmann<F, N, P>> {
  using point_type = mat<F, N, P>;
  using tangent_type = mat<F, N, P>;
  using scalar_type = F;
  using category = grassmann_tag;
  static constexpr usize dim = P * (N - P);
  static constexpr usize ambient_dim = N * P;
};

};     // namespace manifolds
};     // namespace math
};     // namespace micron
