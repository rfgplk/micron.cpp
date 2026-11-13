// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"

namespace micron::__ml
{
using __v16 = simd::i128;
#define __MC_WASM_UNALIGNED(N)                                                                                                             \
  [[gnu::always_inline]] inline u##N __ldu##N(const byte *p) noexcept                                                                      \
  {                                                                                                                                        \
    u##N value;                                                                                                                            \
    __builtin_memcpy(&value, p, sizeof(value));                                                                                            \
    return value;                                                                                                                          \
  }                                                                                                                                        \
  [[gnu::always_inline]] inline void __stu##N(byte *p, u##N value) noexcept { __builtin_memcpy(p, &value, sizeof(value)); }
__MC_WASM_UNALIGNED(16)
__MC_WASM_UNALIGNED(32)
__MC_WASM_UNALIGNED(64)
#undef __MC_WASM_UNALIGNED

[[gnu::always_inline]] inline __v16
__ld16(const byte *p) noexcept
{
  __v16 value;
  __builtin_memcpy(&value, p, 16);
  return value;
}

[[gnu::always_inline]] inline void
__st16(byte *p, __v16 value) noexcept
{
  __builtin_memcpy(p, &value, 16);
}

[[gnu::always_inline]] inline void
__pin4(__v16 &, __v16 &, __v16 &, __v16 &) noexcept
{
  __asm__ volatile("" ::: "memory");
}
};      // namespace micron::__ml
