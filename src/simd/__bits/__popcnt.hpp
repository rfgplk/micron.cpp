//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

#if !defined(__micron_arch_x86_any)
#error "__popcnt.hpp included on a non-x86 build"
#endif

// freestanding POPCNT [popcntintrin.h]

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

// POPCNT only exists after SSE4.2); SWAR workarounds
__inline_g int
_mm_popcnt_u32(unsigned int v) noexcept
{
#if defined(__micron_x86_popcnt)
  return __builtin_popcount(v);
#else
  v = v - ((v >> 1) & 0x55555555u);
  v = (v & 0x33333333u) + ((v >> 2) & 0x33333333u);
  v = (v + (v >> 4)) & 0x0f0f0f0fu;
  return (int)((v * 0x01010101u) >> 24);
#endif
}

__inline_g int
_mm_popcnt_u64(unsigned long long v) noexcept
{
#if defined(__micron_x86_popcnt)
  return __builtin_popcountll(v);
#elif defined(__micron_arch_width_64)
  v = v - ((v >> 1) & 0x5555555555555555ull);
  v = (v & 0x3333333333333333ull) + ((v >> 2) & 0x3333333333333333ull);
  v = (v + (v >> 4)) & 0x0f0f0f0f0f0f0f0full;
  return (int)((v * 0x0101010101010101ull) >> 56);
#else
  return _mm_popcnt_u32((unsigned)v) + _mm_popcnt_u32((unsigned)(v >> 32));
#endif
}

__inline_g int
_popcnt32(int v) noexcept
{
  return _mm_popcnt_u32((unsigned)v);
}

__inline_g int
_popcnt64(long long v) noexcept
{
  return _mm_popcnt_u64((unsigned long long)v);
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_mm_popcnt_u32);
__inject_i(_mm_popcnt_u64);
__inject_i(_popcnt32);
__inject_i(_popcnt64);
#undef __inject_i
#endif
