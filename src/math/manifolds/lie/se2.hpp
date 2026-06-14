//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 2D rigid-body group; SO2 rotation + translation

#include "../../../concepts.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../matrix/mat.hpp"
#include "../../mk.hpp"
#include "../../quants/vec.hpp"
#include "../tags.hpp"
#include "../tangent.hpp"
#include "so2.hpp"

namespace micron
{
namespace math
{
namespace manifolds
{
namespace lie
{

template<ieee754_floating F> struct SE2 {
  SO2<F> R;
  vec<F, 2> t;

  [[nodiscard, gnu::always_inline]] static constexpr SE2
  identity() noexcept
  {
    return SE2{ SO2<F>::identity(), vec<F, 2>{ F(0), F(0) } };
  }

  [[nodiscard, gnu::flatten]] static constexpr SE2
  compose(const SE2 &a, const SE2 &b) noexcept
  {
    const auto Rb = SO2<F>::rotate(a.R, b.t);
    return SE2{ SO2<F>::compose(a.R, b.R), a.t + Rb };
  }

  [[nodiscard, gnu::flatten]] static constexpr SE2
  inverse(const SE2 &g) noexcept
  {
    const auto Rinv = SO2<F>::inverse(g.R);
    const auto tinv = SO2<F>::rotate(Rinv, vec<F, 2>{ -g.t.data[0], -g.t.data[1] });
    return SE2{ Rinv, tinv };
  }

  [[nodiscard, gnu::flatten]] static constexpr SE2
  exp_map(const vec<F, 3> &xi) noexcept
  {
    const F vx = xi.data[0], vy = xi.data[1], omega = xi.data[2];
    F a, b;      // V = [[a, -b], [b, a]]
    if ( omega * omega < F(0.01) ) {
      // Taylor
      a = F(1) - omega * omega / F(6);
      b = omega * F(0.5) - omega * omega * omega / F(24);
    } else {
      const F s = math::sin<F>(omega);
      const F c = math::cos<F>(omega);
      a = s / omega;
      b = (F(1) - c) / omega;
    }
    return SE2{ SO2<F>{ omega }, vec<F, 2>{ a * vx - b * vy, b * vx + a * vy } };
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, 3>
  log_map(const SE2 &g) noexcept
  {
    const F two_pi = F(2) * math::constant_pi<F>;
    const F ratio = g.R.theta / two_pi;
    const F k = static_cast<F>(static_cast<i64>(ratio + (ratio >= F(0) ? F(0.5) : F(-0.5))));
    const F omega = g.R.theta - two_pi * k;
    F a, b;      // V = [[a, -b], [b, a]]
    // threshold matched to SO3/SE3 (|omega| < 0.1)
    if ( omega * omega < F(0.01) ) {
      a = F(1) - omega * omega / F(6);
      b = omega * F(0.5) - omega * omega * omega / F(24);
    } else {
      const F s = math::sin<F>(omega);
      const F c = math::cos<F>(omega);
      a = s / omega;
      b = (F(1) - c) / omega;
    }
    const F det = a * a + b * b;
    const F inv = F(1) / det;
    const F vx = (a * g.t.data[0] + b * g.t.data[1]) * inv;
    const F vy = (-b * g.t.data[0] + a * g.t.data[1]) * inv;
    return vec<F, 3>{ vx, vy, omega };
  }

  [[nodiscard, gnu::flatten]] static constexpr mat<F, 3, 3>
  to_matrix(const SE2 &g) noexcept
  {
    const auto R2 = SO2<F>::to_matrix(g.R);
    mat<F, 3, 3> M{};
    M.data[0] = R2.data[0];
    M.data[1] = R2.data[1];
    M.data[2] = g.t.data[0];
    M.data[3] = R2.data[2];
    M.data[4] = R2.data[3];
    M.data[5] = g.t.data[1];
    M.data[6] = F(0);
    M.data[7] = F(0);
    M.data[8] = F(1);
    return M;
  }
};

};      // namespace lie

template<ieee754_floating F> struct traits<lie::SE2<F>> {
  using point_type = lie::SE2<F>;
  using tangent_type = vec<F, 3>;
  using scalar_type = F;
  using category = lie_group_tag;
  static constexpr usize dim = 3;
  static constexpr usize ambient_dim = 9;
};

};      // namespace manifolds
};      // namespace math
};      // namespace micron
