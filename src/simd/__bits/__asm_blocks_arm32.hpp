//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"
#include "__vector_types_arm32.hpp"

#if !defined(__micron_arch_arm32) || !defined(__micron_arm_neon)
#error "__asm_blocks_arm32.hpp requires ARMv7 + NEON"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// inline-asm leaf blocks for memcpy/memset/memcmp loops
namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"
#pragma GCC diagnostic ignored "-Wpedantic"

#define __inline_g [[gnu::always_inline, gnu::artificial]] static inline

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte copy (VLD1.8 / VST1.8)
__inline_g void
__block_copy_16(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t;
  __asm__("vld1.8 {%q0}, [%1]" : "=w"(t) : "r"(s), "m"(*reinterpret_cast<const u8(*)[16]>(s)));
  __asm__("vst1.8 {%q2}, [%1]" : "=m"(*reinterpret_cast<u8(*)[16]>(d)) : "r"(d), "w"(t));
}

__inline_g void
__block_copy_16_a(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t;
  __asm__("vld1.8 {%q0}, [%1 :128]" : "=w"(t) : "r"(s), "m"(*reinterpret_cast<const u8(*)[16]>(s)));
  __asm__("vst1.8 {%q2}, [%1 :128]" : "=m"(*reinterpret_cast<u8(*)[16]>(d)) : "r"(d), "w"(t));
}

__inline_g void
__block_copy_16_nt(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t;
  __asm__("vld1.8 {%q0}, [%1]" : "=w"(t) : "r"(s), "m"(*reinterpret_cast<const u8(*)[16]>(s)));
  __asm__("vst1.8 {%q2}, [%1]" : "=m"(*reinterpret_cast<u8(*)[16]>(d)) : "r"(d), "w"(t));
}

__inline_g void
__block_move_16(u8 *d, const u8 *s) noexcept
{
  uint8x16_t t;
  __asm__("vld1.8 {%q0}, [%1]" : "=w"(t) : "r"(s), "m"(*reinterpret_cast<const u8(*)[16]>(s)));
  __asm__("vst1.8 {%q2}, [%1]" : "=m"(*reinterpret_cast<u8(*)[16]>(d)) : "r"(d), "w"(t));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte broadcast + set (VDUP.8 / VST1.8)
__inline_g uint8x16_t
__broadcast_byte_16(u8 b) noexcept
{
  uint8x16_t v;
  __asm__("vdup.8 %q0, %1" : "=w"(v) : "r"(static_cast<u32>(b)));
  return v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte word broadcast: u64 -> 2x 64-bit lanes via vmov d, r, r + vmov q.d[1], q.d[0]
__inline_g uint8x16_t
__broadcast_word_16(u64 w) noexcept
{
  uint8x16_t v;
  __asm__("vmov %e0, %Q1, %R1\n\t"
          "vmov %f0, %e0"
          : "=w"(v)
          : "r"(w));
  return v;
}

__inline_g void
__block_set_16(u8 *__restrict d, uint8x16_t v) noexcept
{
  __asm__("vst1.8 {%q2}, [%1]" : "=m"(*reinterpret_cast<u8(*)[16]>(d)) : "r"(d), "w"(v));
}

__inline_g void
__block_set_16_a(u8 *__restrict d, uint8x16_t v) noexcept
{
  __asm__("vst1.8 {%q2}, [%1 :128]" : "=m"(*reinterpret_cast<u8(*)[16]>(d)) : "r"(d), "w"(v));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte memcmp (VCEQ.I8)
__inline_g uint8x16_t
__block_cmpeq_16(const u8 *a, const u8 *b) noexcept
{
  uint8x16_t va, vb, ceq;
  __asm__("vld1.8 {%q0}, [%1]" : "=w"(va) : "r"(a), "m"(*reinterpret_cast<const u8(*)[16]>(a)));
  __asm__("vld1.8 {%q0}, [%1]" : "=w"(vb) : "r"(b), "m"(*reinterpret_cast<const u8(*)[16]>(b)));
  __asm__("vceq.i8 %q0, %q1, %q2" : "=w"(ceq) : "w"(va), "w"(vb));
  return ceq;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte byte-search mask (VCEQ + VSHRN narrowed to 64-bit nibble mask)
__inline_g u64
__block_eq_mask_16(const u8 *p, u8 c) noexcept
{
  uint8x16_t v, bc, ceq;
  __asm__("vdup.8 %q0, %1" : "=w"(bc) : "r"(static_cast<u32>(c)));
  __asm__("vld1.8 {%q0}, [%1]" : "=w"(v) : "r"(p), "m"(*reinterpret_cast<const u8(*)[16]>(p)));
  __asm__("vceq.i8 %q0, %q1, %q2" : "=w"(ceq) : "w"(v), "w"(bc));
  uint8x8_t narrowed;
  __asm__("vshrn.i16 %P0, %q1, #4" : "=w"(narrowed) : "w"(ceq));
  u64 m;
  __asm__("vmov %Q0, %R0, %P1" : "=r"(m) : "w"(narrowed));
  return m;
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron
