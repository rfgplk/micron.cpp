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

template <ieee754_floating F, rng_concept Rng>
[[gnu::flatten]] inline void
fill_uniform(F *__restrict__ out, usize N, Rng &g) noexcept
{
  F *end = out + N;
  while ( out != end ) *out++ = dist::uniform_real<F>(g);
}

template <ieee754_floating F, rng_concept Rng>
[[gnu::flatten]] inline void
fill_uniform(F *__restrict__ out, usize N, Rng &g, F lo, F hi) noexcept
{
  F *end = out + N;
  F r = hi - lo;
  while ( out != end ) *out++ = lo + r * dist::uniform_real<F>(g);
}

template <ieee754_floating F, rng_concept Rng>
[[gnu::flatten]] inline void
fill_normal(F *__restrict__ out, usize N, Rng &g, F mu = F(0), F sigma = F(1)) noexcept
{
  F *end = out + N;
  while ( out != end ) *out++ = dist::normal_ziggurat<F>(g, mu, sigma);
}

template <rng_concept Rng>
[[gnu::flatten]] inline void
fill_bytes(u8 *__restrict__ out, usize N, Rng &g) noexcept
{
  while ( N >= 8 ) {
    u64 r = g.next();
    __builtin_memcpy(out, &r, 8);
    out += 8;
    N -= 8;
  }
  if ( N != 0 ) {
    u64 r = g.next();
    __builtin_memcpy(out, &r, N);
  }
}

template <typename T, rng_concept Rng>
  requires(micron::is_integral_v<T>)
[[gnu::flatten]] inline void
fill_uniform_int(T *__restrict__ out, usize N, Rng &g, T lo, T hi) noexcept
{
  T *end = out + N;
  while ( out != end ) *out++ = dist::uniform_int<T>(g, lo, hi);
}

};     // namespace fill
};     // namespace rng
};     // namespace math
};     // namespace micron
