//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../external/x86.h"

namespace micron
{

namespace simd
{

struct __simd_flags {
  char fma;
  char fma3;
  char fma4;
  char sse;
  char sse2;
  char sse3;
  char ssse3;
  char sse4_1;
  char sse4_2;
  char avx;
  char avx2;
};

__simd_flags
__get_runtime_features(void)
{
  __simd_flags flags;
  processor_t cpu;
  info(&cpu);
  flags.fma = cpu.features.fma;
  flags.fma3 = cpu.features.fma3;
  flags.fma4 = cpu.features.fma4;
  flags.sse = cpu.features.sse;
  flags.sse2 = cpu.features.sse2;
  flags.sse3 = cpu.features.sse3;
  flags.ssse3 = cpu.features.ssse3;
  flags.sse4_1 = cpu.features.sse4_1;
  flags.sse4_2 = cpu.features.sse4_2;
  flags.avx = cpu.features.avx;
  flags.avx2 = cpu.features.avx2;
  return flags;
}

inline bool
__has_fma(const __simd_flags &flags)
{
  return flags.fma;
}
inline bool
__has_sse(const __simd_flags &flags)
{
  return flags.sse3 and flags.sse2 and flags.sse4_2 and flags.sse4_2;
}
inline bool
__has_avx128(const __simd_flags &flags)
{
  return flags.avx;
}
inline bool
__has_avx256(const __simd_flags &flags)
{
  return flags.avx2;
}

#ifdef SIMD_RUNTIME_CHECKING
__attribute__((constructor)) void
__runtime_check(void)
{
  static __simd_flags flags = __get_runtime_flags;
}
#endif

};
};
