//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "__round_modes.hpp"

#if !defined(__micron_arch_x86_any)
#error "__prefetch_fence.hpp included on a non-x86 build"
#endif

#undef _mm_prefetch
#undef _mm_sfence
#undef _mm_lfence
#undef _mm_mfence
#undef _mm_clflush
#undef _mm_clflushopt
#undef _mm_clwb
#undef _mm_pause

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

__inline_g void
_mm_sfence() noexcept
{
  __builtin_ia32_sfence();
}

__inline_g void
_mm_lfence() noexcept
{
  __builtin_ia32_lfence();
}

__inline_g void
_mm_mfence() noexcept
{
  __builtin_ia32_mfence();
}

__inline_g void
_mm_clflush(const void *p) noexcept
{
  __builtin_ia32_clflush(p);
}

#if defined(__micron_x86_clflushopt) || defined(__CLFLUSHOPT__)
__inline_g void
_mm_clflushopt(const void *p) noexcept
{
  __builtin_ia32_clflushopt(p);
}
#endif
#if defined(__micron_x86_clwb) || defined(__CLWB__)
__inline_g void
_mm_clwb(const void *p) noexcept
{
  __builtin_ia32_clwb(p);
}
#endif

__inline_g void
_mm_pause() noexcept
{
  __asm__ __volatile__("pause" ::: "memory");
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#define _mm_prefetch(P, I) __builtin_ia32_prefetch((P), (((I) & 0xC) >> 2), ((I) & 0x3), (((I) & 0x10) >> 4))

#if defined(MICRON_SIMD_INJECT_INTRIN_SYMS)
#define __inject_i(name) using ::micron::simd::__bits::name
__inject_i(_mm_sfence);
__inject_i(_mm_lfence);
__inject_i(_mm_mfence);
__inject_i(_mm_clflush);
__inject_i(_mm_pause);
#if defined(__micron_x86_clflushopt) || defined(__CLFLUSHOPT__)
__inject_i(_mm_clflushopt);
#endif
#if defined(__micron_x86_clwb) || defined(__CLWB__)
__inject_i(_mm_clwb);
#endif
#undef __inject_i
#endif
