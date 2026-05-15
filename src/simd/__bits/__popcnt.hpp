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

__inline_g int
_mm_popcnt_u32(unsigned int v) noexcept
{
  return __builtin_popcount(v);
}

__inline_g int
_mm_popcnt_u64(unsigned long long v) noexcept
{
  return __builtin_popcountll(v);
}

__inline_g int
_popcnt32(int v) noexcept
{
  return __builtin_popcount((unsigned)v);
}

__inline_g int
_popcnt64(long long v) noexcept
{
  return __builtin_popcountll((unsigned long long)v);
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
