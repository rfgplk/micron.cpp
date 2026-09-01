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
#include "../mk.hpp"
#include "../sqrt.hpp"
#include "engines.hpp"
#include "ziggurat.hpp"

namespace micron
{
namespace math
{
namespace rng
{
namespace dist
{

template<ieee754_floating F, rng_concept Rng> [[nodiscard]] inline F uniform_open_real(Rng &g) noexcept;

namespace __impl
{

struct __poisson_cache {
  f64 lambda;
  f64 cutoff;
  f64 log_lambda;
  f64 b;
  f64 a;
  f64 inv_alpha;
  f64 vr;
  bool valid;
  bool small;

  constexpr explicit __poisson_cache(f64 rate) noexcept
      : lambda(rate), cutoff(0.0), log_lambda(0.0), b(0.0), a(0.0), inv_alpha(0.0), vr(0.0),
        valid(rate > 0.0 && math::ieee::is_finite(rate)), small(rate < 10.0)
  {
    if ( !valid ) return;
    if ( small ) {
      cutoff = math::fexp(-rate);
      return;
    }
    const f64 root = math::fsqrt(rate);
    log_lambda = math::flog(rate);
    b = 0.931 + 2.53 * root;
    a = -0.059 + 0.02483 * b;
    inv_alpha = 1.1239 + 1.1328 / (b - 3.4);
    vr = 0.9277 - 3.6224 / (b - 2.0);
  }
};

template<typename I, rng_concept Rng>
[[nodiscard]] inline I
__poisson_cached(Rng &g, const __poisson_cache &p) noexcept
{
  if ( !p.valid ) return I(0);
  if ( p.small ) {
    I k = 0;
    f64 product = 1.0;
    do {
      ++k;
      product *= uniform_open_real<f64>(g);
    } while ( product > p.cutoff );
    return I(k - 1);
  }

  for ( ;; ) {
    const f64 u = uniform_open_real<f64>(g) - 0.5;
    const f64 v = uniform_open_real<f64>(g);
    const f64 us = 0.5 - (u < 0.0 ? -u : u);
    const i64 k = i64(math::ffloor((2.0 * p.a / us + p.b) * u + p.lambda + 0.43));
    if ( k < 0 ) continue;
    if ( us >= 0.07 && v <= p.vr ) return I(k);
    if ( us < 0.013 && v > us ) continue;
    const f64 lhs = math::flog(v) + math::flog(p.inv_alpha) - math::flog(p.a / (us * us) + p.b);
    const f64 rhs = -p.lambda + f64(k) * p.log_lambda - math::mk::special::lgamma<f64>(f64(k + 1));
    if ( lhs <= rhs ) return I(k);
  }
}

};      // namespace __impl

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// uniform_real
template<ieee754_floating F = f64, rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline F
uniform_real(Rng &g) noexcept
{
  if constexpr ( sizeof(F) == 8 ) {
    const u64 r = rng::__impl::next64(g);
    return F(static_cast<double>(r >> 11) * 0x1.0p-53);
  } else {
    const u32 r = rng::__impl::next32(g);
    return F(static_cast<float>(r >> 8) * 0x1.0p-24f);
  }
}

template<ieee754_floating F, rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline F
uniform_real(Rng &g, F lo, F hi) noexcept
{
  return lo + (hi - lo) * uniform_real<F>(g);
}

// (0, 1), for inverse transforms which cannot accept either endpoint.  Odd
// mantissas avoid a retry branch and remain exactly representable.
template<ieee754_floating F = f64, rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline F
uniform_open_real(Rng &g) noexcept
{
  if constexpr ( sizeof(F) == 8 ) {
    const u64 odd = ((rng::__impl::next64(g) >> 12) << 1) | 1ULL;
    return F(odd * 0x1.0p-53);
  } else {
    const u32 odd = ((rng::__impl::next32(g) >> 9) << 1) | 1u;
    return F(odd * 0x1.0p-24f);
  }
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// uniform_int via Lemire
template<typename U, rng_concept Rng>
  requires(micron::is_unsigned_v<U> && micron::is_integral_v<U>)
[[nodiscard, gnu::always_inline]] inline U
uniform_uint_below(Rng &g, U range) noexcept
{
  if ( range == 0 ) return 0;
  if constexpr ( sizeof(U) <= 4 ) {
    u32 x = rng::__impl::next32(g);
    u64 m = u64(x) * u64(range);
    u32 l = u32(m);
    if ( l < range ) {
      u32 t = u32(-range) % range;
      while ( l < t ) {
        x = rng::__impl::next32(g);
        m = u64(x) * u64(range);
        l = u32(m);
      }
    }
    return U(m >> 32);
  } else {
    u64 x = rng::__impl::next64(g);
    u128 m = u128(x) * u128(range);
    u64 l = u64(m);
    if ( l < range ) {
      u64 t = u64(0 - range) % range;
      while ( l < t ) {
        x = rng::__impl::next64(g);
        m = u128(x) * u128(range);
        l = u64(m);
      }
    }
    return U(u64(m >> 64));
  }
}

template<typename T, rng_concept Rng>
  requires(micron::is_integral_v<T>)
[[nodiscard, gnu::always_inline]] inline T
uniform_int(Rng &g, T lo, T hi) noexcept
{
  if ( hi <= lo ) return lo;
  using U = micron::make_unsigned_t<T>;
  const U span = U(hi) - U(lo);
  if ( span == U(~U(0)) ) {
    if constexpr ( sizeof(U) <= 4 )
      return T(U(rng::__impl::next32(g)));
    else
      return T(U(rng::__impl::next64(g)));
  }
  const U range = span + U(1);
  return T(U(lo) + uniform_uint_below<U>(g, range));
}

// %%%%%%%%%%%%%%%%%%%%%%%
// bernoulli
template<rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline bool
bernoulli(Rng &g, f64 p = 0.5) noexcept
{
  if ( p <= 0.0 ) return false;
  if ( p >= 1.0 ) return true;
  if ( p == 0.5 ) return (rng::__impl::next32(g) >> 31) != 0;
  return uniform_real<f64>(g) < p;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// normal via 256-layer Ziggurat

template<ieee754_floating F = f64, rng_concept Rng>
[[nodiscard]] inline F
normal(Rng &g, F mu = F(0), F sigma = F(1)) noexcept
{
  return normal_ziggurat<F>(g, mu, sigma);
}

// %%%%%%%%%%%%%%%%%%%%%
// exp_dist
template<ieee754_floating F = f64, rng_concept Rng>
[[nodiscard, gnu::always_inline]] inline F
exp_dist(Rng &g, F lambda = F(1)) noexcept
{
  // domain guard
  if ( lambda <= F(0) ) return math::ieee::inf_v<F>(0);
  return -math::flog(uniform_open_real<F>(g)) / lambda;
}

// %%%%%%%%%%%%%%%
// poisson
template<typename I = i64, rng_concept Rng>
[[nodiscard]] inline I
poisson(Rng &g, f64 lambda) noexcept
{
  const __impl::__poisson_cache cache(lambda);
  return __impl::__poisson_cached<I>(g, cache);
}

};      // namespace dist
};      // namespace rng
};      // namespace math
};      // namespace micron
