//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%
// N-dimension rotation group

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../linalg/matfunc.hpp"
#include "../../linalg/ops.hpp"
#include "../../matrix/mat.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"
#include "algebra.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{

template<ieee754_floating F, usize N>
  requires(N >= 2)
struct SOn {
  mat<F, N, N> R;

  [[nodiscard, gnu::always_inline]] static constexpr SOn
  identity() noexcept
  {
    return SOn{ mat<F, N, N>::identity() };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SOn
  compose(const SOn &a, const SOn &b) noexcept
  {
    return SOn{ linalg::ops::gemm(a.R, b.R) };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SOn
  inverse(const SOn &g) noexcept
  {
    return SOn{ linalg::ops::transpose(g.R) };
  }

  [[nodiscard]] static SOn
  exp_map(const mat<F, N, N> &X) noexcept
  {
    return SOn{ algebra_exp<F, N>(X) };
  }

  [[nodiscard]] static mat<F, N, N>
  log_map(const SOn &g) noexcept
  {
    return algebra_log<F, N>(g.R);
  }

  [[nodiscard, gnu::always_inline]] static constexpr mat<F, N, N>
  to_matrix(const SOn &g) noexcept
  {
    return g.R;
  }

  [[nodiscard, gnu::always_inline]] static constexpr SOn
  from_matrix(const mat<F, N, N> &R) noexcept
  {
    return SOn{ R };
  }

  [[nodiscard, gnu::always_inline]] static constexpr vec<F, N>
  rotate(const SOn &g, const vec<F, N> &v) noexcept
  {
    return linalg::ops::gemv(g.R, v);
  }
};

};      // namespace lie

template<ieee754_floating F, usize N> struct traits<lie::SOn<F, N>> {
  using point_type = lie::SOn<F, N>;
  using tangent_type = mat<F, N, N>;
  using scalar_type = F;
  using category = lie_group_tag;
  static constexpr usize dim = N * (N - 1) / 2;
  static constexpr usize ambient_dim = N * N;
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
