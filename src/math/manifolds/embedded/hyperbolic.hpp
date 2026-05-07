//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// hyperbolic<F, N>

#include "../../../concepts.hpp"
#include "../../../except.hpp"
#include "../../../types.hpp"
#include "../../constants.hpp"
#include "../../generic.hpp"
#include "../../linalg/ops.hpp"
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

template <ieee754_floating F, usize N>
  requires(N >= 1 && N <= 15)
struct hyperbolic {
  using value_type = F;
  static constexpr usize ambient = N + 1;

  [[nodiscard, gnu::flatten]] static constexpr F
  minkowski(const vec<F, ambient> &x, const vec<F, ambient> &y) noexcept
  {
    F s = -x.data[0] * y.data[0];
    for ( usize i = 1; i < ambient; ++i ) s = math::fma<F>(x.data[i], y.data[i], s);
    return s;
  }

  [[nodiscard, gnu::flatten]] static constexpr vec<F, ambient>
  project_to_tangent(const vec<F, ambient> &p, const vec<F, ambient> &v) noexcept
  {
    const F ip = minkowski(p, v);
    vec<F, ambient> r{};
    for ( usize i = 0; i < ambient; ++i ) r.data[i] = v.data[i] + ip * p.data[i];
    return r;
  }

  [[nodiscard]] static vec<F, ambient>
  project_to_manifold(const vec<F, ambient> &x)
  {
    const F nn = -minkowski(x, x);
    if ( nn <= F(0) || x.data[0] <= F(0) ) exc<except::domain_error>("hyperbolic::project_to_manifold: not a future-timelike vector");
    const F k = F(1) / math::fsqrt(nn);
    vec<F, ambient> r{};
    for ( usize i = 0; i < ambient; ++i ) r.data[i] = x.data[i] * k;
    return r;
  }

  [[nodiscard, gnu::flatten]] static vec<F, ambient>
  exp_map(const vec<F, ambient> &p, const vec<F, ambient> &v) noexcept
  {
    F vv = minkowski(v, v);
    if ( vv < F(0) ) vv = F(0);
    const F theta = math::fsqrt(vv);
    if ( theta < math::default_eps<F>() ) {
      vec<F, ambient> r{};
      for ( usize i = 0; i < ambient; ++i ) r.data[i] = p.data[i] + v.data[i];
      return r;
    }
    const F ch = math::cosh<F>(theta);
    const F sh = math::sinh<F>(theta);
    const F k = sh / theta;
    vec<F, ambient> r{};
    for ( usize i = 0; i < ambient; ++i ) r.data[i] = ch * p.data[i] + k * v.data[i];
    return r;
  }

  [[nodiscard, gnu::flatten]] static vec<F, ambient>
  log_map(const vec<F, ambient> &p, const vec<F, ambient> &q)
  {
    F mpq = minkowski(p, q);
    if ( mpq > F(-1) ) mpq = F(-1);
    const F theta = math::acosh<F>(-mpq);
    if ( theta < math::default_eps<F>() ) {
      vec<F, ambient> diff{};
      for ( usize i = 0; i < ambient; ++i ) diff.data[i] = q.data[i] - p.data[i];
      return project_to_tangent(p, diff);
    }
    const F sh = math::sinh<F>(theta);
    const F k = theta / sh;
    vec<F, ambient> r{};
    for ( usize i = 0; i < ambient; ++i ) r.data[i] = (q.data[i] + mpq * p.data[i]) * k;
    return r;
  }

  [[nodiscard, gnu::always_inline]] static F
  distance(const vec<F, ambient> &p, const vec<F, ambient> &q) noexcept
  {
    F mpq = minkowski(p, q);
    if ( mpq > F(-1) ) mpq = F(-1);
    return math::acosh<F>(-mpq);
  }

  [[nodiscard, gnu::always_inline]] static constexpr F
  inner(const vec<F, ambient> &, const vec<F, ambient> &u, const vec<F, ambient> &v) noexcept
  {
    return minkowski(u, v);
  }

  [[nodiscard, gnu::always_inline]] static F
  norm(const vec<F, ambient> &p, const vec<F, ambient> &v) noexcept
  {
    F vv = inner(p, v, v);
    return math::fsqrt(vv > F(0) ? vv : F(0));
  }

  [[nodiscard]] static vec<F, ambient>
  retract(const vec<F, ambient> &p, const vec<F, ambient> &v)
  {
    vec<F, ambient> s{};
    for ( usize i = 0; i < ambient; ++i ) s.data[i] = p.data[i] + v.data[i];
    return project_to_manifold(s);
  }

  [[nodiscard, gnu::always_inline]] static vec<F, ambient>
  inverse_retract(const vec<F, ambient> &p, const vec<F, ambient> &q)
  {
    return log_map(p, q);
  }

  [[nodiscard, gnu::flatten]] static vec<F, ambient>
  parallel_transport(const vec<F, ambient> &p, const vec<F, ambient> &q, const vec<F, ambient> &v) noexcept
  {
    const F denom = F(1) - minkowski(p, q);
    if ( math::fabs(denom) < math::default_eps<F>() ) return project_to_tangent(q, v);
    const F qv = minkowski(q, v);
    const F k = qv / denom;
    vec<F, ambient> r{};
    for ( usize i = 0; i < ambient; ++i ) r.data[i] = v.data[i] - k * (p.data[i] + q.data[i]);
    return r;
  }

  [[nodiscard, gnu::always_inline]] static vec<F, ambient>
  vector_transport(const vec<F, ambient> &, const vec<F, ambient> &q, const vec<F, ambient> &v) noexcept
  {
    return project_to_tangent(q, v);
  }
};

template <ieee754_floating F, usize N> struct traits<hyperbolic<F, N>> {
  using point_type = vec<F, N + 1>;
  using tangent_type = vec<F, N + 1>;
  using scalar_type = F;
  using category = hyperbolic_tag;
  static constexpr usize dim = N;
  static constexpr usize ambient_dim = N + 1;
};

};     // namespace manifolds
};     // namespace math
};     // namespace micron
