//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"
#include "__vector_types_arm64.hpp"

#if !defined(__micron_arch_arm64) || !defined(__micron_arm_neon)
#error "__asm_blocks_arm64.hpp requires AArch64 + NEON"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// inline-asm leaf blocks for memcpy/memset/memcmp
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte copy (LDR Q / STR Q)
__inline_g void
__block_copy_16(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t;
  __asm__("ldr %q0, %1" : "=w"(t) : "Q"(*reinterpret_cast<const uint8x16_t *>(s)));
  __asm__("str %q1, %0" : "=Q"(*reinterpret_cast<uint8x16_t *>(d)) : "w"(t));
}

__inline_g void
__block_copy_16_a(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t;
  __asm__("ldr %q0, %1" : "=w"(t) : "Q"(*reinterpret_cast<const uint8x16_t *>(s)));
  __asm__("str %q1, %0" : "=Q"(*reinterpret_cast<uint8x16_t *>(d)) : "w"(t));
}

__inline_g void
__block_copy_16_nt(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t;
  __asm__("ldr %q0, %1" : "=w"(t) : "Q"(*reinterpret_cast<const uint8x16_t *>(s)));
  __asm__("str %q1, %0" : "=Q"(*reinterpret_cast<uint8x16_t *>(d)) : "w"(t));
}

__inline_g void
__block_move_16(u8 *d, const u8 *s) noexcept
{
  uint8x16_t t;
  __asm__("ldr %q0, %1" : "=w"(t) : "Q"(*reinterpret_cast<const uint8x16_t *>(s)));
  __asm__("str %q1, %0" : "=Q"(*reinterpret_cast<uint8x16_t *>(d)) : "w"(t));
}

__inline_g void
__block_move_pair_16(u8 *d, const u8 *s) noexcept
{
  uint8x16_t t0, t1;
  __asm__("ldp %q0, %q1, [%2]" : "=w"(t0), "=w"(t1) : "r"(s), "m"(*reinterpret_cast<const u8(*)[32]>(s)));
  __asm__("stp %q2, %q3, [%1]" : "=m"(*reinterpret_cast<u8(*)[32]>(d)) : "r"(d), "w"(t0), "w"(t1));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 32-byte pair copy (LDP Q,Q / STP Q,Q)
__inline_g void
__block_copy_pair_16(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t0, t1;
  __asm__("ldp %q0, %q1, [%2]" : "=w"(t0), "=w"(t1) : "r"(s), "m"(*reinterpret_cast<const u8(*)[32]>(s)));
  __asm__("stp %q2, %q3, [%1]" : "=m"(*reinterpret_cast<u8(*)[32]>(d)) : "r"(d), "w"(t0), "w"(t1));
}

__inline_g void
__block_copy_pair_16_nt(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t0, t1;
  __asm__("ldp %q0, %q1, [%2]" : "=w"(t0), "=w"(t1) : "r"(s), "m"(*reinterpret_cast<const u8(*)[32]>(s)));
  __asm__("stnp %q2, %q3, [%1]" : "=m"(*reinterpret_cast<u8(*)[32]>(d)) : "r"(d), "w"(t0), "w"(t1));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte broadcast + set (DUP / STR Q)
__inline_g uint8x16_t
__broadcast_byte_16(u8 b) noexcept
{
  uint8x16_t v;
  __asm__("dup %0.16b, %w1" : "=w"(v) : "r"(static_cast<u32>(b)));
  return v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte word broadcast (DUP .2D)
__inline_g uint8x16_t
__broadcast_word_16(u64 w) noexcept
{
  uint8x16_t v;
  __asm__("dup %0.2d, %1" : "=w"(v) : "r"(w));
  return v;
}

__inline_g void
__block_set_16(u8 *__restrict d, uint8x16_t v) noexcept
{
  __asm__("str %q1, %0" : "=Q"(*reinterpret_cast<uint8x16_t *>(d)) : "w"(v));
}

__inline_g void
__block_set_16_a(u8 *__restrict d, uint8x16_t v) noexcept
{
  __asm__("str %q1, %0" : "=Q"(*reinterpret_cast<uint8x16_t *>(d)) : "w"(v));
}

__inline_g void
__block_set_pair_16_nt(u8 *__restrict d, uint8x16_t v) noexcept
{
  __asm__("stnp %q2, %q2, [%1]" : "=m"(*reinterpret_cast<u8(*)[32]>(d)) : "r"(d), "w"(v));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 32/64-byte set (STP Q,Q)
__inline_g void
__block_set_pair_16(u8 *__restrict d, uint8x16_t v) noexcept
{
  __asm__("stp %q2, %q2, [%1]" : "=m"(*reinterpret_cast<u8(*)[32]>(d)) : "r"(d), "w"(v));
}

__inline_g void
__block_set_quad_16(u8 *__restrict d, uint8x16_t v) noexcept
{
  __asm__("stp %q2, %q2, [%1]\n\t"
          "stp %q2, %q2, [%1, #32]"
          : "=m"(*reinterpret_cast<u8(*)[64]>(d))
          : "r"(d), "w"(v));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 64-byte copy (2x LDP / 2x STP)
__inline_g void
__block_copy_64(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t0, t1, t2, t3;
  __asm__("ldp %q0, %q1, [%4]\n\t"
          "ldp %q2, %q3, [%4, #32]"
          : "=w"(t0), "=w"(t1), "=w"(t2), "=w"(t3)
          : "r"(s), "m"(*reinterpret_cast<const u8(*)[64]>(s)));
  __asm__("stp %q2, %q3, [%1]\n\t"
          "stp %q4, %q5, [%1, #32]"
          : "=m"(*reinterpret_cast<u8(*)[64]>(d))
          : "r"(d), "w"(t0), "w"(t1), "w"(t2), "w"(t3));
}

__inline_g void
__block_move_64(u8 *d, const u8 *s) noexcept
{
  uint8x16_t t0, t1, t2, t3;
  __asm__("ldp %q0, %q1, [%4]\n\t"
          "ldp %q2, %q3, [%4, #32]"
          : "=w"(t0), "=w"(t1), "=w"(t2), "=w"(t3)
          : "r"(s), "m"(*reinterpret_cast<const u8(*)[64]>(s)));
  __asm__("stp %q2, %q3, [%1]\n\t"
          "stp %q4, %q5, [%1, #32]"
          : "=m"(*reinterpret_cast<u8(*)[64]>(d))
          : "r"(d), "w"(t0), "w"(t1), "w"(t2), "w"(t3));
}

__inline_g void
__block_copy_64_nt(u8 *__restrict d, const u8 *__restrict s) noexcept
{
  uint8x16_t t0, t1, t2, t3;
  __asm__("ldp %q0, %q1, [%4]\n\t"
          "ldp %q2, %q3, [%4, #32]"
          : "=w"(t0), "=w"(t1), "=w"(t2), "=w"(t3)
          : "r"(s), "m"(*reinterpret_cast<const u8(*)[64]>(s)));
  __asm__("stnp %q2, %q3, [%1]\n\t"
          "stnp %q4, %q5, [%1, #32]"
          : "=m"(*reinterpret_cast<u8(*)[64]>(d))
          : "r"(d), "w"(t0), "w"(t1), "w"(t2), "w"(t3));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// DC ZVA zero-fill (large zero memset)
__inline_g u64
__read_dczid(void) noexcept
{
  u64 v;
  __asm__("mrs %0, dczid_el0" : "=r"(v));
  return v;
}

// zeroes the 64-byte block at p (p must be 64-aligned); the write is not
// expressible as an "=m" operand, hence volatile + memory clobber
__inline_g void
__dc_zva_64(u8 *p) noexcept
{
  __asm__ __volatile__("dc zva, %0" : : "r"(p) : "memory");
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte memcmp (CMEQ.16B + UMINV.B)
__inline_g bool
__block_all_eq_16(const u8 *a, const u8 *b) noexcept
{
  uint8x16_t va, vb, ceq;
  __asm__("ldr %q0, %1" : "=w"(va) : "Q"(*reinterpret_cast<const uint8x16_t *>(a)));
  __asm__("ldr %q0, %1" : "=w"(vb) : "Q"(*reinterpret_cast<const uint8x16_t *>(b)));
  __asm__("cmeq %0.16b, %1.16b, %2.16b" : "=w"(ceq) : "w"(va), "w"(vb));
  uint8x16_t minv;
  __asm__("uminv %b0, %1.16b" : "=w"(minv) : "w"(ceq));
  return reinterpret_cast<const u8 *>(&minv)[0] == 0xFFu;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 16-byte byte-search mask (CMEQ + SHRN narrowed to 64-bit nibble mask)
// Returns a 64-bit value with 4 bits per source byte.
// Consumer uses __builtin_ctzll(m) >> 2 / (63 - __builtin_clzll(m)) >> 2 for byte index.
__inline_g u64
__block_eq_mask_16(const u8 *p, u8 c) noexcept
{
  uint8x16_t v, bc, ceq;
  __asm__("dup %0.16b, %w1" : "=w"(bc) : "r"(static_cast<u32>(c)));
  __asm__("ldr %q0, %1" : "=w"(v) : "Q"(*reinterpret_cast<const uint8x16_t *>(p)));
  __asm__("cmeq %0.16b, %1.16b, %2.16b" : "=w"(ceq) : "w"(v), "w"(bc));
  uint8x16_t narrowed;
  __asm__("shrn %0.8b, %1.8h, #4" : "=w"(narrowed) : "w"(ceq));
  u64 m;
  __asm__("umov %0, %1.d[0]" : "=r"(m) : "w"(narrowed));
  return m;
}

#undef __inline_g

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron
