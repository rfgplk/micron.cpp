//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"
#include "../__asm/rdrand.hpp"
#include "engines.hpp"

namespace micron
{
namespace math
{
namespace rng
{
namespace hardware
{

[[nodiscard, gnu::always_inline]] inline bool
rdrand64(u64 &out) noexcept
{
  return __asm_op::rdrand64(out);
}

[[nodiscard, gnu::always_inline]] inline bool
rdseed64(u64 &out) noexcept
{
  return __asm_op::rdseed64(out);
}

[[nodiscard]] inline xoshiro256ss
seed_from_hw() noexcept
{
  u64 a = 0, b = 0, c = 0, d = 0;
  const bool got = rdrand64(a) && rdrand64(b) && rdrand64(c) && rdrand64(d);
  if ( !got ) {
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
    splitmix64 sm{ __asm_op::rdtsc64() ^ 0xa5a5a5a5a5a5a5a5ULL };
#else
    splitmix64 sm{ 0xc0ffee0123456789ULL };
#endif
    a = sm.next();
    b = sm.next();
    c = sm.next();
    d = sm.next();
  }
  return xoshiro256ss{ a, b, c, d };
}

};     // namespace hardware
};     // namespace rng
};     // namespace math
};     // namespace micron
