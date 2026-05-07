//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"

namespace micron
{
namespace math
{
namespace __asm_op
{

[[nodiscard, gnu::always_inline]] inline bool
rdrand64(u64 &out) noexcept
{
#if defined(__micron_x86_rdrnd) && defined(__micron_arch_amd64)
  unsigned char ok = 0;
  for ( int retry = 0; retry < 10; ++retry ) {
    asm volatile("rdrand %0; setc %1" : "=r"(out), "=qm"(ok)::"cc");
    if ( ok ) return true;
  }
  return false;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_rng)
  u64 v = 0;
  for ( int retry = 0; retry < 10; ++retry ) {
    u32 nzcv;
    asm volatile("mrs %0, s3_3_c2_c4_0\n\tmrs %1, nzcv" : "=r"(v), "=r"(nzcv));
    if ( ((nzcv >> 30) & 1u) == 0u ) {
      out = v;
      return true;
    }
  }
  return false;
#else
  (void)out;
  return false;
#endif
}

[[nodiscard, gnu::always_inline]] inline bool
rdseed64(u64 &out) noexcept
{
#if defined(__micron_x86_rdseed) && defined(__micron_arch_amd64)
  unsigned char ok = 0;
  for ( int retry = 0; retry < 64; ++retry ) {
    asm volatile("rdseed %0; setc %1" : "=r"(out), "=qm"(ok)::"cc");
    if ( ok ) return true;
  }
  return false;
#elif defined(__micron_arch_arm64) && defined(__micron_arm_rng)
  u64 v = 0;
  for ( int retry = 0; retry < 64; ++retry ) {
    u32 nzcv;
    asm volatile("mrs %0, s3_3_c2_c4_1\n\tmrs %1, nzcv" : "=r"(v), "=r"(nzcv));
    if ( ((nzcv >> 30) & 1u) == 0u ) {
      out = v;
      return true;
    }
  }
  return false;
#else
  (void)out;
  return false;
#endif
}

// fallback method

[[nodiscard, gnu::always_inline]] inline u64
rdtsc64() noexcept
{
#if defined(__micron_arch_amd64)
  u32 lo = 0, hi = 0;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return (u64(hi) << 32) | u64(lo);
#elif defined(__micron_arch_arm64)
  u64 v;
  asm volatile("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#elif defined(__micron_arch_arm32)
  u32 lo, hi;
  asm volatile("mrrc p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));
  return (u64(hi) << 32) | u64(lo);
#else
  return 0;
#endif
}

};     // namespace __asm_op
};     // namespace math
};     // namespace micron
