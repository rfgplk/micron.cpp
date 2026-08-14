//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "__arch.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cpuid
// cpp alternative to the external block

namespace micron
{

struct cpuid_regs {
  u32 eax;
  u32 ebx;
  u32 ecx;
  u32 edx;
};

#if defined(__micron_arch_x86_any)

[[gnu::always_inline]] inline cpuid_regs
__cpuid_read(u32 leaf, u32 sub = 0) noexcept
{
  cpuid_regs r{ 0, 0, 0, 0 };
#if defined(__micron_arch_x86) && (defined(__PIC__) || defined(__pic__))
  // NOTE: on 32-bit PIC ebx is the GOT pointer and cannot be an output operand
  __asm__ __volatile__("xchgl %%ebx, %1\n\t"
                       "cpuid\n\t"
                       "xchgl %%ebx, %1"
                       : "=a"(r.eax), "=&r"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
                       : "0"(leaf), "2"(sub));
#else
  __asm__ __volatile__("cpuid" : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx) : "0"(leaf), "2"(sub));
#endif
  return r;
}

inline u32
__cpuid_max_leaf(void) noexcept
{
  return __cpuid_read(0u).eax;
}

inline u32
__cpuid_max_ext_leaf(void) noexcept
{
  return __cpuid_read(0x8000'0000u).eax;
}

inline bool
__cpuid_has_leaf(u32 leaf) noexcept
{
  if ( leaf >= 0x8000'0000u ) return __cpuid_max_ext_leaf() >= leaf;
  return __cpuid_max_leaf() >= leaf;
}

#endif      // __micron_arch_x86_any

};      // namespace micron
