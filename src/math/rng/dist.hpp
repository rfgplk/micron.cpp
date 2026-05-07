//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rng distributions
//   uniform_real(rng) [0, 1)
//   uniform_real(rng, lo, hi)
//   uniform_int(rng, lo, hi)
//   bernoulli(rng, p)
//   normal(rng) N(0,1)
//   exp_dist(rng, lambda)
//   poisson(rng, lambda)

#include "../../concepts.hpp"
#include "../../types.hpp"
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// uniform_real
template <ieee754_floating F = f64, rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline F
uniform_real(Rng &g) noexcept
{
  // f64: take top 53 bits, multiply by 2^-53
  if constexpr ( sizeof(F) == 8 ) {
    u64 r = g.next();
    return F((r >> 11) * (1.0 / 9007199254740992.0));
  } else {
    u64 r = g.next();
    return F((r >> 40) * (1.0f / 16777216.0f));     // 24-bit f32 mantissa
  }
}

template <ieee754_floating F, rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline F
uniform_real(Rng &g, F lo, F hi) noexcept
{
  return lo + (hi - lo) * uniform_real<F>(g);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// uniform_int via Lemire
template <typename U, rng_concept Rng>
  requires(micron::is_unsigned_v<U> && micron::is_integral_v<U>)
[[nodiscard, gnu::always_inline]] inline U
uniform_uint_below(Rng &g, U range) noexcept
{
  if ( range == 0 ) return 0;
  if constexpr ( sizeof(U) == 4 ) {
    u32 x = u32(g.next());
    u64 m = u64(x) * u64(range);
    u32 l = u32(m);
    if ( l < range ) {
      u32 t = u32(-range) % range;
      while ( l < t ) {
        x = u32(g.next());
        m = u64(x) * u64(range);
        l = u32(m);
      }
    }
    return U(m >> 32);
  } else {
    u64 x = g.next();
    u128 m = u128(x) * u128(range);
    u64 l = u64(m);
    if ( l < range ) {
      u64 t = u64(0 - range) % range;
      while ( l < t ) {
        x = g.next();
        m = u128(x) * u128(range);
        l = u64(m);
      }
    }
    return U(u64(m >> 64));
  }
}

template <typename T, rng_concept Rng>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline T
uniform_int(Rng &g, T lo, T hi) noexcept
{
  if ( hi <= lo ) return lo;
  using U = micron::make_unsigned_t<T>;
  U range = U(hi - lo) + U(1);
  return T(lo + T(uniform_uint_below<U>(g, range)));
}

// %%%%%%%%%%%%%%%%%%%%%%%
// bernoulli
template <rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline bool
bernoulli(Rng &g, f64 p = 0.5) noexcept
{
  if ( p <= 0.0 ) return false;
  if ( p >= 1.0 ) return true;
  return uniform_real<f64>(g) < p;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// normal via the Marsaglia polar alg

template <ieee754_floating F = f64, rng_concept Rng>
[[nodiscard]] inline F
normal(Rng &g, F mu = F(0), F sigma = F(1)) noexcept
{
  for ( ;; ) {
    F u = uniform_real<F>(g) * F(2) - F(1);
    F v = uniform_real<F>(g) * F(2) - F(1);
    F s = u * u + v * v;
    if ( s < F(1) && s != F(0) ) {
      F m = math::fsqrt(F(F(-2) * mkbits::log_ns::log<F>(s) / s));
      return mu + sigma * (u * m);
    }
  }
}

// %%%%%%%%%%%%%%%%%%%%%
// exp_dist
template <ieee754_floating F = f64, rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline F
exp_dist(Rng &g, F lambda = F(1)) noexcept
{
  F u;
  do {
    u = uniform_real<F>(g);
  } while ( u <= F(0) );
  return -mkbits::log_ns::log<F>(u) / lambda;
}

// %%%%%%%%%%%%%%%
// poisson
template <typename I = i64, rng_concept Rng>
[[nodiscard]] inline I
poisson(Rng &g, f64 lambda) noexcept
{
  if ( lambda < 30.0 ) {
    f64 L = mkbits::exp_ns::exp<f64>(-lambda);
    I k = 0;
    f64 p = 1.0;
    do {
      ++k;
      p *= uniform_real<f64>(g);
    } while ( p > L );
    return I(k - 1);
  }
  f64 z = normal<f64>(g);
  f64 v = lambda + math::fsqrt(lambda) * z;
  if ( v < 0 ) v = 0;
  return I(v + 0.5);
}

};     // namespace dist
};     // namespace rng
};     // namespace math
};     // namespace micron
