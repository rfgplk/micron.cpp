//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 2D rotation groupl; SO2

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
#include "../../matrix/mat.hpp"
#include "../../mk.hpp"
#include "../../quants/vec.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{

template <ieee754_floating F> struct SO2 {
  F theta;

  [[nodiscard, gnu::always_inline]] static constexpr SO2
  identity() noexcept
  {
    return SO2{ F(0) };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SO2
  compose(const SO2 &a, const SO2 &b) noexcept
  {
    return SO2{ a.theta + b.theta };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SO2
  inverse(const SO2 &g) noexcept
  {
    return SO2{ -g.theta };
  }

  [[nodiscard, gnu::always_inline]] static constexpr SO2
  exp_map(F omega) noexcept
  {
    return SO2{ omega };
  }

  [[nodiscard, gnu::always_inline]] static constexpr F
  log_map(const SO2 &g) noexcept
  {
    return g.theta;
  }

  [[nodiscard, gnu::flatten]] static constexpr mat<F, 2, 2>
  to_matrix(const SO2 &g) noexcept
  {
    const F c = math::cos<F>(g.theta);
    const F s = math::sin<F>(g.theta);
    mat<F, 2, 2> r{};
    r.data[0] = c;
    r.data[1] = -s;
    r.data[2] = s;
    r.data[3] = c;
    return r;
  }

  [[nodiscard]] static constexpr SO2
  from_matrix(const mat<F, 2, 2> &R) noexcept
  {
    return SO2{ math::atan2<F>(R.data[2], R.data[0]) };
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, 2>
  rotate(const SO2 &g, const vec<F, 2> &v) noexcept
  {
    const F c = math::cos<F>(g.theta);
    const F s = math::sin<F>(g.theta);
    return vec<F, 2>{ c * v.data[0] - s * v.data[1], s * v.data[0] + c * v.data[1] };
  }

  [[nodiscard, gnu::always_inline]] static constexpr F
  adjoint(const SO2 &) noexcept
  {
    return F(1);
  }
};

};     // namespace lie

template <ieee754_floating F> struct traits<lie::SO2<F>> {
  using point_type = lie::SO2<F>;
  using tangent_type = F;
  using scalar_type = F;
  using category = lie_group_tag;
  static constexpr usize dim = 1;
  static constexpr usize ambient_dim = 1;
};

};     // namespace manifolds
};     // namespace math
};     // namespace micron
