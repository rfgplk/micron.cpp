//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// 256-layer ziggurat tables generated at compile time via Newton iteration

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits.hpp"
#include "../bits/exp.hpp"
#include "../bits/log.hpp"
#include "../generic.hpp"
#include "../log.hpp"
#include "../sqrt.hpp"
#include "engines.hpp"

namespace micron
{
namespace math
{
namespace rng
{
namespace dist
{

namespace mkbits
{

struct ziggurat_tables {
  f64 x[257];
  f64 y[256];
  u64 k[256];
  f64 w[256];

  static constexpr f64 R = 3.6541528853610088;
  static constexpr f64 V = 0.00492867323399;     // area per layer
};

[[nodiscard]] consteval ziggurat_tables
build_ziggurat() noexcept
{
  ziggurat_tables t{};
  auto phi = [](f64 x) constexpr {
    f64 r = 1.0;
    f64 term = 1.0;
    f64 a = -x * x * 0.5;
    for ( int n = 1; n <= 40; ++n ) {
      term *= a / f64(n);
      r += term;
      if ( term > -1e-300 && term < 1e-300 ) break;
    }
    return r;
  };

  t.x[0] = ziggurat_tables::R;
  t.x[256] = 0.0;
  f64 f = phi(ziggurat_tables::R);
  t.y[0] = f;
  for ( int i = 1; i < 256; ++i ) {
    f64 yi = t.y[i - 1] + ziggurat_tables::V / t.x[i - 1];
    if ( yi <= 0.0 ) yi = 1e-300;
    if ( yi >= 1.0 ) yi = 1.0 - 1e-15;
    f64 xi = t.x[i - 1] * 0.95;
    for ( int it = 0; it < 50; ++it ) {
      f64 phi_x = phi(xi);
      f64 d = phi_x - yi;
      f64 dp = -xi * phi_x;
      if ( dp == 0.0 ) break;
      f64 dx = d / dp;
      xi -= dx;
      if ( dx < 1e-15 && dx > -1e-15 ) break;
      if ( xi < 0.0 ) xi = 1e-12;
    }
    t.x[i] = xi;
    t.y[i] = phi(xi);
  }
  for ( int i = 0; i < 255; ++i ) {
    t.k[i] = u64((t.x[i + 1] / t.x[i]) * f64(u64(1) << 53));
    t.w[i] = t.x[i] / f64(u64(1) << 53);
  }
  t.k[255] = 0;
  t.w[255] = t.x[255] / f64(u64(1) << 53);
  return t;
}

inline constinit const ziggurat_tables ziggurat = build_ziggurat();

};     // namespace mkbits

template <ieee754_floating F = f64, rng_concept Rng>
[[nodiscard]] inline F
normal_ziggurat(Rng &g, F mu = F(0), F sigma = F(1)) noexcept
{
  const f64 *__restrict__ wt = mkbits::ziggurat.w;
  const f64 *__restrict__ yt = mkbits::ziggurat.y;
  const u64 *__restrict__ kt = mkbits::ziggurat.k;

  for ( ;; ) {
    u64 u = g.next();
    i64 j = i64(u >> 11);     // 53-bit signed magnitude
    u64 sign_u = u & 1ULL;
    if ( sign_u ) j = -j;
    u64 i = (u >> 1) & 0xFFULL;
    f64 x = f64(j) * wt[i];
    if ( u64(j < 0 ? -j : j) < kt[i] ) [[likely]]
      return F(mu + sigma * F(x));
    if ( i == 0 ) {
      f64 r = mkbits::ziggurat_tables::R;
      f64 xt_;
      f64 yt_;
      do {
        f64 u1 = (g.next() >> 11) * (1.0 / 9007199254740992.0);
        f64 u2 = (g.next() >> 11) * (1.0 / 9007199254740992.0);
        if ( u1 <= 0.0 ) u1 = 1e-300;
        if ( u2 <= 0.0 ) u2 = 1e-300;
        xt_ = -math::mkbits::log_ns::log_f64(u1) / r;
        yt_ = -math::mkbits::log_ns::log_f64(u2);
      } while ( yt_ + yt_ < xt_ * xt_ );
      f64 v = (j < 0) ? -(r + xt_) : (r + xt_);
      return F(mu + sigma * F(v));
    }
    // wedge rejection
    f64 u1 = (g.next() >> 11) * (1.0 / 9007199254740992.0);
    f64 fy = yt[i] + (yt[i - 1] - yt[i]) * u1;
    f64 ax = x < 0 ? -x : x;
    f64 phi_x = math::mkbits::exp_ns::exp_f64(-ax * ax * 0.5);
    if ( fy < phi_x ) return F(mu + sigma * F(x));
  }
}

};     // namespace dist
};     // namespace rng
};     // namespace math
};     // namespace micron
