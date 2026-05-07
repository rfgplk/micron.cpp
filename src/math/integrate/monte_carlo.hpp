//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%
// Monte-Carlo quadrature
//
// -> monte_carlo(f, lo, hi, n, rng)
// -> quasi_monte_carlo(f, lo, hi, n)

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../rng/dist.hpp"
#include "../rng/engines.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

template <usize D, ieee754_floating F, typename Fn, rng::rng_concept Rng>
[[nodiscard]] inline F
monte_carlo(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &g) noexcept
{
  if ( n_samples == 0 ) return F(0);
  F volume = F(1);
  for ( usize d = 0; d < D; ++d ) volume *= (hi[d] - lo[d]);
  F sum = F(0);
  F x[D];
  for ( usize s = 0; s < n_samples; ++s ) {
    for ( usize d = 0; d < D; ++d ) {
      const F u = rng::dist::uniform_real<F>(g);
      x[d] = math::fma<F>(u, hi[d] - lo[d], lo[d]);
    }
    sum += f(x);
  }
  return volume * sum / F(n_samples);
}

namespace __impl_monte_carlo
{

inline constexpr u32 halton_primes[16] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
};

template <ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
halton(usize i, u32 base) noexcept
{
  F f = F(1);
  F r = F(0);
  while ( i > 0 ) {
    f = f / F(base);
    r += f * F(i % base);
    i /= base;
  }
  return r;
}

};     // namespace __impl_monte_carlo

template <usize D, ieee754_floating F, typename Fn>
  requires(D <= 16)
[[nodiscard]] inline F
quasi_monte_carlo(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples) noexcept
{
  if ( n_samples == 0 ) return F(0);
  F volume = F(1);
  for ( usize d = 0; d < D; ++d ) volume *= (hi[d] - lo[d]);
  F sum = F(0);
  F x[D];
  for ( usize s = 1; s <= n_samples; ++s ) {
    for ( usize d = 0; d < D; ++d ) {
      const F u = __impl_monte_carlo::halton<F>(s, __impl_monte_carlo::halton_primes[d]);
      x[d] = math::fma<F>(u, hi[d] - lo[d], lo[d]);
    }
    sum += f(x);
  }
  return volume * sum / F(n_samples);
}

};     // namespace integrate
};     // namespace math
};     // namespace micron
