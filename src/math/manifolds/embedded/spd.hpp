//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// symmetric positive definite cone

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../linalg/decomp_ext.hpp"
#include "../../linalg/matfunc.hpp"
#include "../../linalg/ops.hpp"
#include "../../matrix/mat.hpp"
#include "../../mk.hpp"
#include "../metrics.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{

namespace __spd_impl
{

template<ieee754_floating F, usize N, typename Apply>
[[nodiscard]] inline mat<F, N, N>
sym_func(const mat<F, N, N> &P, Apply apply) noexcept
{
  auto er = linalg::decomp::eigen_sym<F, N>(P);
  vec<F, N> fl{};
  for ( usize i = 0; i < N; ++i ) fl.data[i] = apply(er.values.data[i]);
  mat<F, N, N> M{};
  for ( usize r = 0; r < N; ++r ) {
    for ( usize c = 0; c < N; ++c ) {
      F acc = F(0);
      for ( usize k = 0; k < N; ++k ) acc += er.vectors.data[r * N + k] * fl.data[k] * er.vectors.data[c * N + k];
      M.data[r * N + c] = acc;
    }
  }
  return M;
}

template<ieee754_floating F, usize N>
[[nodiscard]] inline mat<F, N, N>
symmetrize(const mat<F, N, N> &A) noexcept
{
  mat<F, N, N> S{};
  for ( usize r = 0; r < N; ++r )
    for ( usize c = 0; c < N; ++c ) S.data[r * N + c] = F(0.5) * (A.data[r * N + c] + A.data[c * N + r]);
  return S;
}

template<ieee754_floating F, usize N>
[[nodiscard, gnu::flatten]] inline F
frobenius_norm(const mat<F, N, N> &A) noexcept
{
  F s = F(0);
  for ( usize i = 0; i < N * N; ++i ) s = math::fma<F>(A.data[i], A.data[i], s);
  return math::fsqrt(s);
}

};      // namespace __spd_impl

template<ieee754_floating F, usize N>
  requires(N >= 1)
struct spd {
  using value_type = F;
  static constexpr usize dim_n = N;

  [[nodiscard]] static mat<F, N, N>
  sqrt_pd(const mat<F, N, N> &P) noexcept
  {
    return __spd_impl::sym_func<F, N>(P, [](F x) noexcept { return math::fsqrt(x); });
  }

  [[nodiscard]] static mat<F, N, N>
  inv_sqrt_pd(const mat<F, N, N> &P) noexcept
  {
    const F eps = math::default_eps<F>();
    return __spd_impl::sym_func<F, N>(P, [eps](F x) noexcept { return F(1) / math::fsqrt(x > eps ? x : eps); });
  }

  [[nodiscard]] static mat<F, N, N>
  inverse_pd(const mat<F, N, N> &P) noexcept
  {
    const F eps = math::default_eps<F>();
    return __spd_impl::sym_func<F, N>(P, [eps](F x) noexcept { return F(1) / (x > eps ? x : eps); });
  }

  [[nodiscard]] static mat<F, N, N>
  project_to_manifold(const mat<F, N, N> &A) noexcept
  {
    auto S = __spd_impl::symmetrize<F, N>(A);
    const F eps = math::default_eps<F>();
    return __spd_impl::sym_func<F, N>(S, [eps](F x) noexcept { return x > eps ? x : eps; });
  }

  [[nodiscard, gnu::always_inline]] static constexpr mat<F, N, N>
  project_to_tangent(const mat<F, N, N> &, const mat<F, N, N> &V) noexcept
  {
    return __spd_impl::symmetrize<F, N>(V);
  }

  template<metric_tag Metric = affine_invariant_metric>
  [[nodiscard]] static mat<F, N, N>
  exp_map(const mat<F, N, N> &P, const mat<F, N, N> &V) noexcept
  {
    if constexpr ( micron::is_same_v<Metric, log_euclidean_metric> ) {
      auto LP = linalg::matfunc::logm<F, N>(P).X;
      auto S = __spd_impl::symmetrize<F, N>(V);
      mat<F, N, N> sum{};
      for ( usize i = 0; i < N * N; ++i ) sum.data[i] = LP.data[i] + S.data[i];
      return linalg::matfunc::expm<F, N>(sum).X;
    } else {
      auto P_half = sqrt_pd(P);
      auto P_neg_half = inv_sqrt_pd(P);
      auto Vs = __spd_impl::symmetrize<F, N>(V);
      auto inner = linalg::ops::gemm(linalg::ops::gemm(P_neg_half, Vs), P_neg_half);
      auto E = linalg::matfunc::expm<F, N>(inner).X;
      return linalg::ops::gemm(linalg::ops::gemm(P_half, E), P_half);
    }
  }

  template<metric_tag Metric = affine_invariant_metric>
  [[nodiscard]] static mat<F, N, N>
  log_map(const mat<F, N, N> &P, const mat<F, N, N> &Q) noexcept
  {
    if constexpr ( micron::is_same_v<Metric, log_euclidean_metric> ) {
      auto LP = linalg::matfunc::logm<F, N>(P).X;
      auto LQ = linalg::matfunc::logm<F, N>(Q).X;
      mat<F, N, N> r{};
      for ( usize i = 0; i < N * N; ++i ) r.data[i] = LQ.data[i] - LP.data[i];
      return r;
    } else {
      auto P_half = sqrt_pd(P);
      auto P_neg_half = inv_sqrt_pd(P);
      auto inner = linalg::ops::gemm(linalg::ops::gemm(P_neg_half, Q), P_neg_half);
      auto L = linalg::matfunc::logm<F, N>(inner).X;
      return linalg::ops::gemm(linalg::ops::gemm(P_half, L), P_half);
    }
  }

  template<metric_tag Metric = affine_invariant_metric>
  [[nodiscard]] static F
  distance(const mat<F, N, N> &P, const mat<F, N, N> &Q) noexcept
  {
    if constexpr ( micron::is_same_v<Metric, log_euclidean_metric> ) {
      auto LP = linalg::matfunc::logm<F, N>(P).X;
      auto LQ = linalg::matfunc::logm<F, N>(Q).X;
      mat<F, N, N> diff{};
      for ( usize i = 0; i < N * N; ++i ) diff.data[i] = LP.data[i] - LQ.data[i];
      return __spd_impl::frobenius_norm<F, N>(diff);
    } else {
      auto P_neg_half = inv_sqrt_pd(P);
      auto inner = linalg::ops::gemm(linalg::ops::gemm(P_neg_half, Q), P_neg_half);
      auto L = linalg::matfunc::logm<F, N>(inner).X;
      return __spd_impl::frobenius_norm<F, N>(L);
    }
  }

  [[nodiscard]] static mat<F, N, N>
  retract(const mat<F, N, N> &P, const mat<F, N, N> &V) noexcept
  {
    auto Vs = __spd_impl::symmetrize<F, N>(V);
    auto Pinv = inverse_pd(P);
    auto VP_invV = linalg::ops::gemm(linalg::ops::gemm(Vs, Pinv), Vs);
    mat<F, N, N> R{};
    for ( usize i = 0; i < N * N; ++i ) R.data[i] = P.data[i] + Vs.data[i] + F(0.5) * VP_invV.data[i];
    return project_to_manifold(R);
  }

  [[nodiscard]] static mat<F, N, N>
  inverse_retract(const mat<F, N, N> &P, const mat<F, N, N> &Q) noexcept
  {
    return log_map<affine_invariant_metric>(P, Q);
  }

  template<metric_tag Metric = affine_invariant_metric>
  [[nodiscard]] static F
  inner(const mat<F, N, N> &P, const mat<F, N, N> &U, const mat<F, N, N> &V) noexcept
  {
    if constexpr ( micron::is_same_v<Metric, log_euclidean_metric> ) {
      F s = F(0);
      for ( usize i = 0; i < N * N; ++i ) s = math::fma<F>(U.data[i], V.data[i], s);
      return s;
    } else {
      auto Pinv = inverse_pd(P);
      auto A = linalg::ops::gemm(Pinv, U);
      auto B = linalg::ops::gemm(Pinv, V);
      F s = F(0);
      for ( usize i = 0; i < N; ++i )
        for ( usize j = 0; j < N; ++j ) s = math::fma<F>(A.data[i * N + j], B.data[j * N + i], s);
      return s;
    }
  }

  template<metric_tag Metric = affine_invariant_metric>
  [[nodiscard]] static F
  norm(const mat<F, N, N> &P, const mat<F, N, N> &V) noexcept
  {
    return math::fsqrt(inner<Metric>(P, V, V));
  }

  [[nodiscard]] static mat<F, N, N>
  parallel_transport(const mat<F, N, N> &P, const mat<F, N, N> &Q, const mat<F, N, N> &V) noexcept
  {
    auto Pinv = inverse_pd(P);
    auto QP = linalg::ops::gemm(Q, Pinv);
    auto E = linalg::matfunc::sqrtm<F, N>(QP).X;
    auto Et = linalg::ops::transpose(E);
    return linalg::ops::gemm(linalg::ops::gemm(E, V), Et);
  }

  [[nodiscard, gnu::always_inline]] static mat<F, N, N>
  vector_transport(const mat<F, N, N> &P, const mat<F, N, N> &Q, const mat<F, N, N> &V) noexcept
  {
    return parallel_transport(P, Q, V);
  }
};

template<ieee754_floating F, usize N> struct traits<spd<F, N>> {
  using point_type = mat<F, N, N>;
  using tangent_type = mat<F, N, N>;
  using scalar_type = F;
  using category = spd_tag;
  static constexpr usize dim = N * (N + 1) / 2;
  static constexpr usize ambient_dim = N * N;
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
