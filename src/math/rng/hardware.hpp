//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../linux/sys/time.hpp"
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
  const bool got = rdrand64(a) && rdrand64(b) && rdrand64(c) && rdrand64(d) && ((a | b | c | d) != 0);
  if ( !got ) {
    micron::timespec_t __ts{};
    (void)micron::clock_gettime(micron::clock_monotonic, __ts);
    u64 mix = (static_cast<u64>(__ts.tv_sec) * 1'000'000'000ull) ^ static_cast<u64>(__ts.tv_nsec);
    micron::timespec_t __rt{};
    (void)micron::clock_gettime(micron::clock_realtime, __rt);
    mix ^= (static_cast<u64>(__rt.tv_sec) << 20) ^ static_cast<u64>(__rt.tv_nsec);
    mix ^= static_cast<u64>(reinterpret_cast<uintptr_t>(&__ts));
    if constexpr ( __asm_op::rdtsc64_available ) mix ^= __asm_op::rdtsc64();
    splitmix64 sm{ mix ^ 0xa5a5a5a5a5a5a5a5ULL };
    a = sm.next();
    b = sm.next();
    c = sm.next();
    d = sm.next();
  }
  return xoshiro256ss{ a, b, c, d };
}

};      // namespace hardware
};      // namespace rng
};      // namespace math
};      // namespace micron
