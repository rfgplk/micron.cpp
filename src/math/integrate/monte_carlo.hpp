//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// allocation free Monte Carlo and randomized quasi-Monte Carlo quadrature

#include "../../concepts.hpp"
#include "../../types.hpp"
#include "../bits/impl.hpp"
#include "../ieee.hpp"
#include "../mk.hpp"
#include "../rng/dist.hpp"
#include "../rng/engines.hpp"
#include "bits/kernels.hpp"
#include "concepts.hpp"

namespace micron
{
namespace math
{
namespace integrate
{

enum class monte_carlo_status : u32 { ok = 0, no_samples = 1, non_finite = 2, invalid_input = 3 };

template<ieee754_floating F> struct monte_carlo_result {
  F estimate{ 0 };
  F variance{ 0 };
  F standard_error{ 0 };
  usize evaluations{ 0 };
  monte_carlo_status status{ monte_carlo_status::ok };
};

template<usize D, ieee754_floating F, usize BatchSize = 64> struct monte_carlo_batch_workspace {
  static_assert(D >= 1 && D <= 16, "Monte Carlo dimensions must be in [1, 16]");
  static_assert(BatchSize > 0, "Monte Carlo batch size must be non-zero");
  alignas(64) F coordinates[D][BatchSize]{};
  alignas(64) F values[BatchSize]{};
  const F *coordinate_views[D]{};

  static constexpr usize batch_size = BatchSize;
};

namespace __impl_monte_carlo
{

inline constexpr u32 halton_primes[16] = {
  2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
};

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
halton(usize i, u32 base) noexcept
{
  F factor = F(1);
  F value = F(0);
  while ( i > 0 ) {
    factor /= F(base);
    value += factor * F(i % base);
    i /= base;
  }
  return value;
}

[[nodiscard, gnu::always_inline]] inline constexpr u64
mix64(u64 value) noexcept
{
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

template<ieee754_floating F>
[[nodiscard, gnu::always_inline]] inline F
unit_from_u64(u64 value) noexcept
{
  if constexpr ( sizeof(F) == 8 )
    return F(static_cast<double>(value >> 11) * (1.0 / 9007199254740992.0));
  else
    return F(static_cast<float>(value >> 40) * (1.0f / 16777216.0f));
}

[[nodiscard]] inline constexpr u32
gcd(u32 a, u32 b) noexcept
{
  while ( b != 0 ) {
    const u32 r = a % b;
    a = b;
    b = r;
  }
  return a;
}

template<ieee754_floating F>
[[nodiscard]] inline F
scrambled_halton_coordinate(usize index, u32 base, u64 scramble) noexcept
{
  const u32 multiplier = u32(scramble % (base - 1)) + 1;
  const u32 shift = u32((scramble >> 32) % base);
  F factor = F(1);
  F value = F(0);
  while ( index > 0 ) {
    factor /= F(base);
    const u32 digit = u32(index % base);
    value += factor * F((multiplier * digit + shift) % base);
    index /= base;
  }
  return value + F(shift) * factor / F(base - 1);
}

struct sobol_parameter {
  u8 degree;
  u32 polynomial;
  u8 initial[6];
};

inline constexpr sobol_parameter sobol_parameters[15] = {
  { 1, 0, { 1, 0, 0, 0, 0, 0 } },  { 2, 1, { 1, 3, 0, 0, 0, 0 } },    { 3, 1, { 1, 3, 1, 0, 0, 0 } },    { 3, 2, { 1, 1, 1, 0, 0, 0 } },
  { 4, 1, { 1, 3, 5, 13, 0, 0 } }, { 4, 4, { 1, 1, 5, 5, 0, 0 } },    { 5, 2, { 1, 3, 3, 9, 7, 0 } },    { 5, 4, { 1, 1, 5, 11, 27, 0 } },
  { 5, 7, { 1, 1, 7, 13, 3, 0 } }, { 5, 11, { 1, 1, 5, 1, 15, 0 } },  { 5, 13, { 1, 1, 1, 3, 29, 0 } },  { 5, 14, { 1, 3, 5, 5, 21, 0 } },
  { 6, 1, { 1, 3, 3, 9, 9, 7 } },  { 6, 13, { 1, 1, 1, 15, 11, 5 } }, { 6, 16, { 1, 3, 7, 13, 3, 25 } },
};

template<usize D>
inline void
sobol_directions(u64 (&directions)[D][64]) noexcept
{
  for ( usize bit = 0; bit < 64; ++bit ) directions[0][bit] = u64(1) << (63 - bit);
  for ( usize dimension = 1; dimension < D; ++dimension ) {
    const sobol_parameter &parameter = sobol_parameters[dimension - 1];
    const usize degree = parameter.degree;
    for ( usize bit = 0; bit < degree; ++bit ) directions[dimension][bit] = u64(parameter.initial[bit]) << (63 - bit);
    for ( usize bit = degree; bit < 64; ++bit ) {
      u64 value = directions[dimension][bit - degree] ^ (directions[dimension][bit - degree] >> degree);
      for ( usize k = 1; k < degree; ++k ) {
        if ( (parameter.polynomial >> (degree - 1 - k)) & 1U ) value ^= directions[dimension][bit - k];
      }
      directions[dimension][bit] = value;
    }
  }
}

template<usize D, ieee754_floating F>
[[nodiscard]] inline F
volume(const F (&lo)[D], const F (&hi)[D]) noexcept
{
  F result = F(1);
  for ( usize d = 0; d < D; ++d ) result *= hi[d] - lo[d];
  return result;
}

template<usize D, ieee754_floating F>
[[nodiscard]] inline bool
valid_bounds(const F (&lo)[D], const F (&hi)[D]) noexcept
{
  for ( usize d = 0; d < D; ++d )
    if ( !ieee::is_finite<F>(lo[d]) || !ieee::is_finite<F>(hi[d]) ) return false;
  return true;
}

template<ieee754_floating F>
inline void
welford_add(F x, usize n, F &mean, F &m2) noexcept
{
  const F delta = x - mean;
  mean += delta / F(n);
  m2 += delta * (x - mean);
}

template<ieee754_floating F>
inline void
finish(monte_carlo_result<F> &result, F mean, F m2, usize count, F volume) noexcept
{
  result.estimate = volume * mean;
  if ( count > 1 ) result.variance = volume * volume * m2 / F(count - 1);
  result.standard_error = count > 0 ? mk::pow_ns::sqrt<F>(result.variance / F(count)) : F(0);
}

};      // namespace __impl_monte_carlo

template<usize D, ieee754_floating F, typename Fn, rng::rng_concept Rng>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
monte_carlo_detailed(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &generator) noexcept
{
  monte_carlo_result<F> result{};
  if ( n_samples == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  F mean = F(0), m2 = F(0), point[D]{};
  for ( usize sample = 0; sample < n_samples; ++sample ) {
    for ( usize d = 0; d < D; ++d ) {
      const F unit = rng::dist::uniform_real<F>(generator);
      point[d] = math::fma<F>(unit, hi[d] - lo[d], lo[d]);
    }
    const F value = f(point);
    ++result.evaluations;
    if ( !ieee::is_finite<F>(value) ) {
      result.status = monte_carlo_status::non_finite;
      return result;
    }
    __impl_monte_carlo::welford_add(value, sample + 1, mean, m2);
  }
  __impl_monte_carlo::finish(result, mean, m2, n_samples, __impl_monte_carlo::volume(lo, hi));
  return result;
}

template<usize D, ieee754_floating F, typename Fn, rng::rng_concept Rng>
[[nodiscard]] inline monte_carlo_result<F>
monte_carlo_stats(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &generator) noexcept
{
  return monte_carlo_detailed<D, F>(f, lo, hi, n_samples, generator);
}

template<usize D, ieee754_floating F, typename Fn, rng::rng_concept Rng>
[[nodiscard]] inline F
monte_carlo(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &generator) noexcept
{
  return monte_carlo_detailed<D, F>(f, lo, hi, n_samples, generator).estimate;
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, rng::rng_concept Rng, usize BatchSize>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
monte_carlo_batch_detailed(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &generator,
                           monte_carlo_batch_workspace<D, F, BatchSize> &workspace) noexcept
{
  monte_carlo_result<F> result{};
  if ( n_samples == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  for ( usize d = 0; d < D; ++d ) workspace.coordinate_views[d] = workspace.coordinates[d];
  F mean = F(0), m2 = F(0);
  usize completed = 0;
  while ( completed < n_samples ) {
    const usize remaining = n_samples - completed;
    const usize count = remaining < BatchSize ? remaining : BatchSize;
    for ( usize sample = 0; sample < count; ++sample )
      for ( usize d = 0; d < D; ++d ) workspace.coordinates[d][sample] = rng::dist::uniform_real<F>(generator);
    for ( usize d = 0; d < D; ++d )
      __integrate_arch::affine_transform(workspace.coordinates[d], workspace.coordinates[d], count, hi[d] - lo[d], lo[d]);
    f(workspace.coordinate_views, workspace.values, count);
    for ( usize sample = 0; sample < count; ++sample ) {
      const F value = workspace.values[sample];
      ++result.evaluations;
      if ( !ieee::is_finite<F>(value) ) {
        result.status = monte_carlo_status::non_finite;
        return result;
      }
      __impl_monte_carlo::welford_add(value, completed + sample + 1, mean, m2);
    }
    completed += count;
  }
  __impl_monte_carlo::finish(result, mean, m2, n_samples, __impl_monte_carlo::volume(lo, hi));
  return result;
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, rng::rng_concept Rng, usize BatchSize>
[[nodiscard]] inline monte_carlo_result<F>
monte_carlo_batch(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &generator,
                  monte_carlo_batch_workspace<D, F, BatchSize> &workspace) noexcept
{
  return monte_carlo_batch_detailed<D, F>(f, lo, hi, n_samples, generator, workspace);
}

template<usize D, ieee754_floating F, typename Fn, rng::rng_concept Rng>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
antithetic_monte_carlo(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &generator) noexcept
{
  monte_carlo_result<F> result{};
  if ( n_samples == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  F mean = F(0), m2 = F(0), point[D]{}, antithetic[D]{};
  usize observations = 0;
  while ( result.evaluations < n_samples ) {
    for ( usize d = 0; d < D; ++d ) {
      const F unit = rng::dist::uniform_real<F>(generator);
      point[d] = math::fma<F>(unit, hi[d] - lo[d], lo[d]);
      antithetic[d] = math::fma<F>(F(1) - unit, hi[d] - lo[d], lo[d]);
    }
    const F first = f(point);
    ++result.evaluations;
    F observation = first;
    if ( result.evaluations < n_samples ) {
      const F second = f(antithetic);
      ++result.evaluations;
      if ( !ieee::is_finite<F>(second) ) {
        result.status = monte_carlo_status::non_finite;
        return result;
      }
      observation = F(0.5) * (first + second);
    }
    if ( !ieee::is_finite<F>(observation) ) {
      result.status = monte_carlo_status::non_finite;
      return result;
    }
    __impl_monte_carlo::welford_add(observation, ++observations, mean, m2);
  }
  __impl_monte_carlo::finish(result, mean, m2, observations, __impl_monte_carlo::volume(lo, hi));
  return result;
}

template<usize D, ieee754_floating F, typename Fn, rng::rng_concept Rng>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
stratified_monte_carlo(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples, Rng &generator) noexcept
{
  monte_carlo_result<F> result{};
  if ( n_samples == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  u32 multiplier[D]{}, shift[D]{};
  for ( usize d = 0; d < D; ++d ) {
    u32 candidate = u32(generator.next() % n_samples);
    if ( candidate == 0 ) candidate = 1;
    while ( __impl_monte_carlo::gcd(candidate, u32(n_samples)) != 1 ) {
      ++candidate;
      if ( candidate >= n_samples ) candidate = 1;
    }
    multiplier[d] = candidate;
    shift[d] = u32(generator.next() % n_samples);
  }
  F mean = F(0), m2 = F(0), point[D]{};
  for ( usize sample = 0; sample < n_samples; ++sample ) {
    for ( usize d = 0; d < D; ++d ) {
      const usize stratum = (usize(multiplier[d]) * sample + shift[d]) % n_samples;
      const F unit = (F(stratum) + rng::dist::uniform_real<F>(generator)) / F(n_samples);
      point[d] = math::fma<F>(unit, hi[d] - lo[d], lo[d]);
    }
    const F value = f(point);
    ++result.evaluations;
    if ( !ieee::is_finite<F>(value) ) {
      result.status = monte_carlo_status::non_finite;
      return result;
    }
    __impl_monte_carlo::welford_add(value, sample + 1, mean, m2);
  }
  __impl_monte_carlo::finish(result, mean, m2, n_samples, __impl_monte_carlo::volume(lo, hi));
  return result;
}

template<usize D, ieee754_floating F, typename Fn>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline F
quasi_monte_carlo(Fn f, const F (&lo)[D], const F (&hi)[D], usize n_samples) noexcept
{
  if ( n_samples == 0 || !__impl_monte_carlo::valid_bounds(lo, hi) ) return F(0);
  F sum = F(0), point[D]{};
  for ( usize sample = 1; sample <= n_samples; ++sample ) {
    for ( usize d = 0; d < D; ++d ) {
      const F unit = __impl_monte_carlo::halton<F>(sample, __impl_monte_carlo::halton_primes[d]);
      point[d] = math::fma<F>(unit, hi[d] - lo[d], lo[d]);
    }
    sum += f(point);
  }
  return __impl_monte_carlo::volume(lo, hi) * sum / F(n_samples);
}

template<usize D, ieee754_floating F, typename Fn>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
scrambled_halton(Fn f, const F (&lo)[D], const F (&hi)[D], usize samples_per_replica, usize replicas = 8,
                 u64 seed = 0x8b8b8b8b243f6a88ULL) noexcept
{
  monte_carlo_result<F> result{};
  if ( samples_per_replica == 0 || replicas == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  const F volume = __impl_monte_carlo::volume(lo, hi);
  F replica_mean = F(0), replica_m2 = F(0), point[D]{};
  for ( usize replica = 0; replica < replicas; ++replica ) {
    F sum = F(0);
    for ( usize sample = 1; sample <= samples_per_replica; ++sample ) {
      for ( usize d = 0; d < D; ++d ) {
        const u64 scramble = __impl_monte_carlo::mix64(seed ^ (u64(replica + 1) << 32) ^ u64(d + 1));
        const F unit = __impl_monte_carlo::scrambled_halton_coordinate<F>(sample, __impl_monte_carlo::halton_primes[d], scramble);
        point[d] = math::fma<F>(unit, hi[d] - lo[d], lo[d]);
      }
      const F value = f(point);
      ++result.evaluations;
      if ( !ieee::is_finite<F>(value) ) {
        result.status = monte_carlo_status::non_finite;
        return result;
      }
      sum += value;
    }
    const F estimate = volume * sum / F(samples_per_replica);
    __impl_monte_carlo::welford_add(estimate, replica + 1, replica_mean, replica_m2);
  }
  result.estimate = replica_mean;
  if ( replicas > 1 ) result.variance = replica_m2 / F(replicas - 1);
  result.standard_error = mk::pow_ns::sqrt<F>(result.variance / F(replicas));
  return result;
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, usize BatchSize>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
scrambled_halton_batch(Fn f, const F (&lo)[D], const F (&hi)[D], usize samples_per_replica, usize replicas, u64 seed,
                       monte_carlo_batch_workspace<D, F, BatchSize> &workspace) noexcept
{
  monte_carlo_result<F> result{};
  if ( samples_per_replica == 0 || replicas == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  for ( usize d = 0; d < D; ++d ) workspace.coordinate_views[d] = workspace.coordinates[d];
  const F volume = __impl_monte_carlo::volume(lo, hi);
  F replica_mean = F(0), replica_m2 = F(0);
  for ( usize replica = 0; replica < replicas; ++replica ) {
    F sum = F(0);
    usize completed = 0;
    while ( completed < samples_per_replica ) {
      const usize remaining = samples_per_replica - completed;
      const usize count = remaining < BatchSize ? remaining : BatchSize;
      for ( usize sample = 0; sample < count; ++sample ) {
        const usize index = completed + sample + 1;
        for ( usize d = 0; d < D; ++d ) {
          const u64 scramble = __impl_monte_carlo::mix64(seed ^ (u64(replica + 1) << 32) ^ u64(d + 1));
          workspace.coordinates[d][sample]
              = __impl_monte_carlo::scrambled_halton_coordinate<F>(index, __impl_monte_carlo::halton_primes[d], scramble);
        }
      }
      for ( usize d = 0; d < D; ++d )
        __integrate_arch::affine_transform(workspace.coordinates[d], workspace.coordinates[d], count, hi[d] - lo[d], lo[d]);
      f(workspace.coordinate_views, workspace.values, count);
      for ( usize sample = 0; sample < count; ++sample ) {
        if ( !ieee::is_finite<F>(workspace.values[sample]) ) {
          result.status = monte_carlo_status::non_finite;
          return result;
        }
        sum += workspace.values[sample];
      }
      result.evaluations += count;
      completed += count;
    }
    const F estimate = volume * sum / F(samples_per_replica);
    __impl_monte_carlo::welford_add(estimate, replica + 1, replica_mean, replica_m2);
  }
  result.estimate = replica_mean;
  if ( replicas > 1 ) result.variance = replica_m2 / F(replicas - 1);
  result.standard_error = mk::pow_ns::sqrt<F>(result.variance / F(replicas));
  return result;
}

template<usize D, ieee754_floating F, typename Fn>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
scrambled_sobol(Fn f, const F (&lo)[D], const F (&hi)[D], usize samples_per_replica, usize replicas = 8,
                u64 seed = 0xd1b54a32d192ed03ULL) noexcept
{
  monte_carlo_result<F> result{};
  if ( samples_per_replica == 0 || replicas == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  u64 directions[D][64]{};
  __impl_monte_carlo::sobol_directions<D>(directions);
  const F volume = __impl_monte_carlo::volume(lo, hi);
  F replica_mean = F(0), replica_m2 = F(0), point[D]{};
  for ( usize replica = 0; replica < replicas; ++replica ) {
    u64 state[D]{};
    F sum = F(0);
    for ( usize sample = 0; sample < samples_per_replica; ++sample ) {
      if ( sample != 0 ) {
        const usize bit = usize(__builtin_ctzll(u64(sample)));
        for ( usize d = 0; d < D; ++d ) state[d] ^= directions[d][bit];
      }
      for ( usize d = 0; d < D; ++d ) {
        const u64 shift = __impl_monte_carlo::mix64(seed ^ (u64(replica + 1) << 32) ^ u64(d + 1));
        const F unit = __impl_monte_carlo::unit_from_u64<F>(state[d] ^ shift);
        point[d] = math::fma<F>(unit, hi[d] - lo[d], lo[d]);
      }
      const F value = f(point);
      ++result.evaluations;
      if ( !ieee::is_finite<F>(value) ) {
        result.status = monte_carlo_status::non_finite;
        return result;
      }
      sum += value;
    }
    const F estimate = volume * sum / F(samples_per_replica);
    __impl_monte_carlo::welford_add(estimate, replica + 1, replica_mean, replica_m2);
  }
  result.estimate = replica_mean;
  if ( replicas > 1 ) result.variance = replica_m2 / F(replicas - 1);
  result.standard_error = mk::pow_ns::sqrt<F>(result.variance / F(replicas));
  return result;
}

template<usize D, ieee754_floating F, callable_real_batch_d<F> Fn, usize BatchSize>
  requires(D >= 1 && D <= 16)
[[nodiscard]] inline monte_carlo_result<F>
scrambled_sobol_batch(Fn f, const F (&lo)[D], const F (&hi)[D], usize samples_per_replica, usize replicas, u64 seed,
                      monte_carlo_batch_workspace<D, F, BatchSize> &workspace) noexcept
{
  monte_carlo_result<F> result{};
  if ( samples_per_replica == 0 || replicas == 0 ) {
    result.status = monte_carlo_status::no_samples;
    return result;
  }
  if ( !__impl_monte_carlo::valid_bounds(lo, hi) ) {
    result.status = monte_carlo_status::invalid_input;
    return result;
  }
  for ( usize d = 0; d < D; ++d ) workspace.coordinate_views[d] = workspace.coordinates[d];
  u64 directions[D][64]{};
  __impl_monte_carlo::sobol_directions<D>(directions);
  const F volume = __impl_monte_carlo::volume(lo, hi);
  F replica_mean = F(0), replica_m2 = F(0);
  for ( usize replica = 0; replica < replicas; ++replica ) {
    u64 state[D]{};
    F sum = F(0);
    usize completed = 0;
    while ( completed < samples_per_replica ) {
      const usize remaining = samples_per_replica - completed;
      const usize count = remaining < BatchSize ? remaining : BatchSize;
      for ( usize sample = 0; sample < count; ++sample ) {
        const usize index = completed + sample;
        if ( index != 0 ) {
          const usize bit = usize(__builtin_ctzll(u64(index)));
          for ( usize d = 0; d < D; ++d ) state[d] ^= directions[d][bit];
        }
        for ( usize d = 0; d < D; ++d ) {
          const u64 shift = __impl_monte_carlo::mix64(seed ^ (u64(replica + 1) << 32) ^ u64(d + 1));
          workspace.coordinates[d][sample] = __impl_monte_carlo::unit_from_u64<F>(state[d] ^ shift);
        }
      }
      for ( usize d = 0; d < D; ++d )
        __integrate_arch::affine_transform(workspace.coordinates[d], workspace.coordinates[d], count, hi[d] - lo[d], lo[d]);
      f(workspace.coordinate_views, workspace.values, count);
      for ( usize sample = 0; sample < count; ++sample ) {
        if ( !ieee::is_finite<F>(workspace.values[sample]) ) {
          result.status = monte_carlo_status::non_finite;
          return result;
        }
        sum += workspace.values[sample];
      }
      result.evaluations += count;
      completed += count;
    }
    const F estimate = volume * sum / F(samples_per_replica);
    __impl_monte_carlo::welford_add(estimate, replica + 1, replica_mean, replica_m2);
  }
  result.estimate = replica_mean;
  if ( replicas > 1 ) result.variance = replica_m2 / F(replicas - 1);
  result.standard_error = mk::pow_ns::sqrt<F>(result.variance / F(replicas));
  return result;
}

};      // namespace integrate
};      // namespace math
};      // namespace micron
