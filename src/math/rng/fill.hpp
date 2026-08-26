//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "dist.hpp"
#include "engines.hpp"
#include "ziggurat.hpp"

// TODO: consider moving to algorithms/ and expanding

namespace micron
{
namespace math
{
namespace rng
{
namespace fill
{

template<ieee754_floating F, rng_concept Rng>
[[gnu::flatten]] inline void
fill_uniform(F *__restrict__ out, usize N, Rng &g) noexcept
{
  while ( N >= 4 ) {
    out[0] = dist::uniform_real<F>(g);
    out[1] = dist::uniform_real<F>(g);
    out[2] = dist::uniform_real<F>(g);
    out[3] = dist::uniform_real<F>(g);
    out += 4;
    N -= 4;
  }
  while ( N-- != 0 ) *out++ = dist::uniform_real<F>(g);
}

template<ieee754_floating F, rng_concept Rng>
[[gnu::flatten]] inline void
fill_uniform(F *__restrict__ out, usize N, Rng &g, F lo, F hi) noexcept
{
  const F r = hi - lo;
  while ( N >= 4 ) {
    out[0] = lo + r * dist::uniform_real<F>(g);
    out[1] = lo + r * dist::uniform_real<F>(g);
    out[2] = lo + r * dist::uniform_real<F>(g);
    out[3] = lo + r * dist::uniform_real<F>(g);
    out += 4;
    N -= 4;
  }
  while ( N-- != 0 ) *out++ = lo + r * dist::uniform_real<F>(g);
}

template<ieee754_floating F, rng_concept Rng>
[[gnu::flatten]] inline void
fill_normal(F *__restrict__ out, usize N, Rng &g, F mu = F(0), F sigma = F(1)) noexcept
{
  while ( N >= 4 ) {
    out[0] = dist::normal_ziggurat<F>(g, mu, sigma);
    out[1] = dist::normal_ziggurat<F>(g, mu, sigma);
    out[2] = dist::normal_ziggurat<F>(g, mu, sigma);
    out[3] = dist::normal_ziggurat<F>(g, mu, sigma);
    out += 4;
    N -= 4;
  }
  while ( N-- != 0 ) *out++ = dist::normal_ziggurat<F>(g, mu, sigma);
}

template<rng_concept Rng>
[[gnu::flatten]] inline void
fill_bytes(u8 *__restrict__ out, usize N, Rng &g) noexcept
{
  while ( N >= 8 ) {
    const u64 r = rng::__impl::next64(g);
    __builtin_memcpy(out, &r, 8);
    out += 8;
    N -= 8;
  }
  if ( N != 0 ) {
    const u64 r = rng::__impl::next64(g);
    __builtin_memcpy(out, &r, N);
  }
}

template<typename T, rng_concept Rng>
  requires(micron::is_integral_v<T>)
[[gnu::flatten]] inline void
fill_uniform_int(T *__restrict__ out, usize N, Rng &g, T lo, T hi) noexcept
{
  T *end = out + N;
  while ( out != end ) *out++ = dist::uniform_int<T>(g, lo, hi);
}

};      // namespace fill
};      // namespace rng
};      // namespace math
};      // namespace micron
