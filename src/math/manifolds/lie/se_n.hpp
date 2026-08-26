//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// N dimension rigid-body group

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
struct SEn {
  static constexpr usize H = N + 1;
  mat<F, H, H> T;

  [[nodiscard, gnu::always_inline]] static constexpr SEn
  identity() noexcept
  {
    return SEn{ mat<F, H, H>::identity() };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SEn
  from_matrix(const mat<F, H, H> &T) noexcept
  {
    return SEn{ T };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SEn
  compose(const SEn &a, const SEn &b) noexcept
  {
    return SEn{ linalg::ops::gemm(a.T, b.T) };
  }

  [[nodiscard]] static SEn
  inverse(const SEn &g) noexcept
  {
    mat<F, H, H> Tinv = mat<F, H, H>::zero();
    for ( usize r = 0; r < N; ++r )
      for ( usize c = 0; c < N; ++c ) Tinv.data[r * H + c] = g.T.data[c * H + r];
    for ( usize r = 0; r < N; ++r ) {
      F acc{};
      for ( usize c = 0; c < N; ++c ) acc -= g.T.data[c * H + r] * g.T.data[c * H + N];
      Tinv.data[r * H + N] = acc;
    }
    Tinv.data[N * H + N] = F(1);
    return SEn{ Tinv };
  }

  [[nodiscard, gnu::always_inline]] static SEn
  between(const SEn &a, const SEn &b) noexcept
  {
    return compose(inverse(a), b);
  }

  [[nodiscard]] static SEn
  exp_map(const mat<F, H, H> &Xi) noexcept
  {
    return SEn{ algebra_exp<F, H>(Xi) };
  }

  [[nodiscard]] static mat<F, H, H>
  log_map(const SEn &g) noexcept
  {
    return algebra_log<F, H>(g.T);
  }

  [[nodiscard, gnu::always_inline]] static constexpr mat<F, H, H>
  to_matrix(const SEn &g) noexcept
  {
    return g.T;
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, N>
  act(const SEn &g, const vec<F, N> &point) noexcept
  {
    vec<F, N> out{};
    for ( usize r = 0; r < N; ++r ) {
      F acc = g.T.data[r * H + N];
      for ( usize c = 0; c < N; ++c ) acc = math::fma<F>(g.T.data[r * H + c], point.data[c], acc);
      out.data[r] = acc;
    }
    return out;
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, N>
  act_vector(const SEn &g, const vec<F, N> &direction) noexcept
  {
    vec<F, N> out{};
    for ( usize r = 0; r < N; ++r ) {
      F acc = F(0);
      for ( usize c = 0; c < N; ++c ) acc = math::fma<F>(g.T.data[r * H + c], direction.data[c], acc);
      out.data[r] = acc;
    }
    return out;
  }

  [[nodiscard, gnu::always_inline]] static vec<F, N>
  inverse_act(const SEn &g, const vec<F, N> &point) noexcept
  {
    return act(inverse(g), point);
  }

  [[nodiscard]] static SEn
  interpolate(const SEn &a, const SEn &b, F t) noexcept
  {
    return compose(a, exp_map(log_map(between(a, b)) * t));
  }
};

};      // namespace lie

template<ieee754_floating F, usize N> struct traits<lie::SEn<F, N>> {
  using point_type = lie::SEn<F, N>;
  using tangent_type = mat<F, N + 1, N + 1>;
  using scalar_type = F;
  using category = lie_group_tag;
  static constexpr usize dim = N + N * (N - 1) / 2;
  static constexpr usize ambient_dim = (N + 1) * (N + 1);
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
