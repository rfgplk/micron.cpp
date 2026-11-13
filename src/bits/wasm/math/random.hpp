// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../../../math/rng/engines.hpp"
#include "../host.hpp"

namespace micron::math::rng::hardware
{
[[nodiscard]] inline bool
rdrand64(u64 &out) noexcept
{
  return __micron_wasm_host_random(&out, sizeof(out)) == 0;
}

[[nodiscard]] inline bool
rdseed64(u64 &out) noexcept
{
  return rdrand64(out);
}

[[nodiscard]] inline xoshiro256ss
seed_from_hw() noexcept
{
  u64 seed[4];
  if ( __micron_wasm_host_random(seed, sizeof(seed)) != 0 || (seed[0] | seed[1] | seed[2] | seed[3]) == 0 ) micron::wasm::panic();
  return xoshiro256ss{ seed[0], seed[1], seed[2], seed[3] };
}
}      // namespace micron::math::rng::hardware
