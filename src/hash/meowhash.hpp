//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"

#if defined(__micron_arch_x86_any)

#include "../memory/cmemory.hpp"
#include "../simd/aliases.hpp"
#include "../simd/simd.hpp"
#include "../tuple.hpp"
#include "../types.hpp"

// ============================================================================
//  This file embeds a port of the Meow hash into micron.
//  Upstream notice (zlib) reproduced verbatim below.
//
//  Meow - A Fast Non-cryptographic Hash
//  (C) Copyright 2018-2019 by Molly Rocket, Inc. (https://mollyrocket.com)
//  See https://mollyrocket.com/meowhash for details.
//
//  zlib License
//
//  (C) Copyright 2018-2019 Molly Rocket, Inc.
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Authors: JACOB CHRISTIAN MUNCH-ANDERSEN (https://twitter.com/nohatcoder) and
//  CASEY MURATORI (https://caseymuratori.com) created the final Meow hash. JEFF
//  ROBERTS contributed the end-of-buffer residual handling this port keeps;
//  MARTINS MOZEIKO ported Meow to ARM and ANSI-C.
//
//  NOTE: Altered source. Adapted from meow_hash_x64_aesni.h to micron's native intrinsics
// ============================================================================

namespace micron
{
namespace hashes
{

alignas(16) inline constexpr u8 __meow_shift_adjust[32]
    = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
alignas(16) inline constexpr u8 __meow_mask_len[32]
    = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
alignas(16) inline constexpr u8 __meow_default_seed[128]
    = { 0x32, 0x43, 0xF6, 0xA8, 0x88, 0x5A, 0x30, 0x8D, 0x31, 0x31, 0x98, 0xA2, 0xE0, 0x37, 0x07, 0x34, 0x4A, 0x40, 0x93, 0x82, 0x22, 0x99,
        0xF3, 0x1D, 0x00, 0x82, 0xEF, 0xA9, 0x8E, 0xC4, 0xE6, 0xC8, 0x94, 0x52, 0x82, 0x1E, 0x63, 0x8D, 0x01, 0x37, 0x7B, 0xE5, 0x46, 0x6C,
        0xF3, 0x4E, 0x90, 0xC6, 0xCC, 0x0A, 0xC2, 0x9B, 0x7C, 0x97, 0xC5, 0x0D, 0xD3, 0xF8, 0x4D, 0x5B, 0x5B, 0x54, 0x70, 0x91, 0x79, 0x21,
        0x6D, 0x5D, 0x98, 0x97, 0x9F, 0xB1, 0xBD, 0x13, 0x10, 0xBA, 0x69, 0x8D, 0xFB, 0x5A, 0xC2, 0xFF, 0xD7, 0x2D, 0xBD, 0x01, 0xAD, 0xFB,
        0x7B, 0x8E, 0x1A, 0xFE, 0xD6, 0xA2, 0x67, 0xE9, 0x6B, 0xA7, 0xC9, 0x04, 0x5F, 0x12, 0xC7, 0xF9, 0x92, 0x4A, 0x19, 0x94, 0x7B, 0x39,
        0x16, 0xCF, 0x70, 0x80, 0x1F, 0x2E, 0x28, 0x58, 0xEF, 0xC1, 0x66, 0x36, 0x92, 0x0D, 0x87, 0x15, 0x74, 0xE6 };

#define __meow_reorder_barrier /* no-op on GCC */
#define prefetcht0(A) _mm_prefetch((const char *)(A), 3)
#define movdqu(A, B) A = _mm_loadu_si128((const __m128i_u *)(B))
#define movdqu_mem(A, B) _mm_storeu_si128((__m128i_u *)(A), B)
#define movq(A, B) A = _mm_set_epi64x(0, (long long)(B));
#define aesdec(A, B) A = _mm_aesdec_si128(A, B)
#define pshufb(A, B) A = _mm_shuffle_epi8(A, B)
#define pxor(A, B) A = _mm_xor_si128(A, B)
#define paddq(A, B) A = _mm_add_epi64(A, B)
#define pand(A, B) A = _mm_and_si128(A, B)
#define palignr(A, B, i) A = _mm_alignr_epi8(A, B, i)
#define pxor_clear(A, B) A = _mm_setzero_si128();

#define MEOW_MIX_REG(r1, r2, r3, r4, r5, i1, i2, i3, i4)                                                                                   \
  aesdec(r1, r2);                                                                                                                          \
  __meow_reorder_barrier;                                                                                                                  \
  paddq(r3, i1);                                                                                                                           \
  pxor(r2, i2);                                                                                                                            \
  aesdec(r2, r4);                                                                                                                          \
  __meow_reorder_barrier;                                                                                                                  \
  paddq(r5, i3);                                                                                                                           \
  pxor(r4, i4);

#define MEOW_MIX(r1, r2, r3, r4, r5, ptr)                                                                                                  \
  MEOW_MIX_REG(r1, r2, r3, r4, r5, _mm_loadu_si128((const __m128i_u *)((ptr) + 15)), _mm_loadu_si128((const __m128i_u *)((ptr) + 0)),      \
               _mm_loadu_si128((const __m128i_u *)((ptr) + 1)), _mm_loadu_si128((const __m128i_u *)((ptr) + 16)))

#define MEOW_SHUFFLE(r1, r2, r3, r4, r5, r6)                                                                                               \
  aesdec(r1, r4);                                                                                                                          \
  paddq(r2, r5);                                                                                                                           \
  pxor(r4, r6);                                                                                                                            \
  aesdec(r4, r2);                                                                                                                          \
  paddq(r5, r6);                                                                                                                           \
  pxor(r2, r3)

#define MEOW_PAGESIZE 4096
#define MEOW_PREFETCH 4096
#define MEOW_PREFETCH_LIMIT 0x3ff

[[gnu::target("aes,sse4.1")]] static inline __m128i
__meow_hash(const void *Seed128Init, usize Len, const void *SourceInit) noexcept
{
  __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7;            // hash accumulation lanes
  __m128i xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;      // residual / length ingests

  const u8 *rax = (const u8 *)SourceInit;
  const u8 *rcx = (const u8 *)Seed128Init;

  // seed the eight hash registers
  movdqu(xmm0, rcx + 0x00);
  movdqu(xmm1, rcx + 0x10);
  movdqu(xmm2, rcx + 0x20);
  movdqu(xmm3, rcx + 0x30);
  movdqu(xmm4, rcx + 0x40);
  movdqu(xmm5, rcx + 0x50);
  movdqu(xmm6, rcx + 0x60);
  movdqu(xmm7, rcx + 0x70);

  // hash all full 256-byte blocks
  usize BlockCount = (Len >> 8);
  if ( BlockCount > MEOW_PREFETCH_LIMIT ) {
    while ( BlockCount-- ) {
      prefetcht0(rax + MEOW_PREFETCH + 0x00);
      prefetcht0(rax + MEOW_PREFETCH + 0x40);
      prefetcht0(rax + MEOW_PREFETCH + 0x80);
      prefetcht0(rax + MEOW_PREFETCH + 0xc0);

      MEOW_MIX(xmm0, xmm4, xmm6, xmm1, xmm2, rax + 0x00);
      MEOW_MIX(xmm1, xmm5, xmm7, xmm2, xmm3, rax + 0x20);
      MEOW_MIX(xmm2, xmm6, xmm0, xmm3, xmm4, rax + 0x40);
      MEOW_MIX(xmm3, xmm7, xmm1, xmm4, xmm5, rax + 0x60);
      MEOW_MIX(xmm4, xmm0, xmm2, xmm5, xmm6, rax + 0x80);
      MEOW_MIX(xmm5, xmm1, xmm3, xmm6, xmm7, rax + 0xa0);
      MEOW_MIX(xmm6, xmm2, xmm4, xmm7, xmm0, rax + 0xc0);
      MEOW_MIX(xmm7, xmm3, xmm5, xmm0, xmm1, rax + 0xe0);

      rax += 0x100;
    }
  } else {
    while ( BlockCount-- ) {
      MEOW_MIX(xmm0, xmm4, xmm6, xmm1, xmm2, rax + 0x00);
      MEOW_MIX(xmm1, xmm5, xmm7, xmm2, xmm3, rax + 0x20);
      MEOW_MIX(xmm2, xmm6, xmm0, xmm3, xmm4, rax + 0x40);
      MEOW_MIX(xmm3, xmm7, xmm1, xmm4, xmm5, rax + 0x60);
      MEOW_MIX(xmm4, xmm0, xmm2, xmm5, xmm6, rax + 0x80);
      MEOW_MIX(xmm5, xmm1, xmm3, xmm6, xmm7, rax + 0xa0);
      MEOW_MIX(xmm6, xmm2, xmm4, xmm7, xmm0, rax + 0xc0);
      MEOW_MIX(xmm7, xmm3, xmm5, xmm0, xmm1, rax + 0xe0);

      rax += 0x100;
    }
  }

  // load any less-than-32-byte residual
  pxor_clear(xmm9, xmm9);
  pxor_clear(xmm11, xmm11);

  // the part that is _not_ 16-byte aligned
  const u8 *Last = (const u8 *)SourceInit + (Len & ~(usize)0xf);
  unsigned Len8 = (Len & 0xf);
  if ( Len8 ) {
    movdqu(xmm8, &__meow_mask_len[0x10 - Len8]);

    const u8 *LastOk = (const u8 *)((((usize)(((const u8 *)SourceInit) + Len - 1)) | (MEOW_PAGESIZE - 1)) - 16);
    int Align = (Last > LastOk) ? ((int)(usize)Last) & 0xf : 0;
    movdqu(xmm10, &__meow_shift_adjust[Align]);
    movdqu(xmm9, Last - Align);
    pshufb(xmm9, xmm10);

    // and off the extra bytes (jeffr)
    pand(xmm9, xmm8);
  }

  // the part that _is_ 16-byte aligned
  if ( Len & 0x10 ) {
    xmm11 = xmm9;
    movdqu(xmm9, Last - 0x10);
  }

  // construct the residual and length ingests
  xmm8 = xmm9;
  xmm10 = xmm9;
  palignr(xmm8, xmm11, 15);
  palignr(xmm10, xmm11, 1);

  // room for a 128-bit + 64-bit nonce here; left zeroed by design
  pxor_clear(xmm12, xmm12);
  pxor_clear(xmm13, xmm13);
  pxor_clear(xmm14, xmm14);
  movq(xmm15, Len);
  palignr(xmm12, xmm15, 15);
  palignr(xmm14, xmm15, 1);

  // always mix the less-than-32-byte residual, even if empty, to keep the pattern
  MEOW_MIX_REG(xmm0, xmm4, xmm6, xmm1, xmm2, xmm8, xmm9, xmm10, xmm11);

  // append the length, to avoid problems with the 32-byte padding
  MEOW_MIX_REG(xmm1, xmm5, xmm7, xmm2, xmm3, xmm12, xmm13, xmm14, xmm15);

  // hash all full 32-byte blocks
  unsigned LaneCount = (Len >> 5) & 0x7;
  if ( LaneCount == 0 ) goto MixDown;
  MEOW_MIX(xmm2, xmm6, xmm0, xmm3, xmm4, rax + 0x00);
  --LaneCount;
  if ( LaneCount == 0 ) goto MixDown;
  MEOW_MIX(xmm3, xmm7, xmm1, xmm4, xmm5, rax + 0x20);
  --LaneCount;
  if ( LaneCount == 0 ) goto MixDown;
  MEOW_MIX(xmm4, xmm0, xmm2, xmm5, xmm6, rax + 0x40);
  --LaneCount;
  if ( LaneCount == 0 ) goto MixDown;
  MEOW_MIX(xmm5, xmm1, xmm3, xmm6, xmm7, rax + 0x60);
  --LaneCount;
  if ( LaneCount == 0 ) goto MixDown;
  MEOW_MIX(xmm6, xmm2, xmm4, xmm7, xmm0, rax + 0x80);
  --LaneCount;
  if ( LaneCount == 0 ) goto MixDown;
  MEOW_MIX(xmm7, xmm3, xmm5, xmm0, xmm1, rax + 0xa0);
  --LaneCount;
  if ( LaneCount == 0 ) goto MixDown;
  MEOW_MIX(xmm0, xmm4, xmm6, xmm1, xmm2, rax + 0xc0);
  --LaneCount;

  // mix the eight lanes down to one 128-bit hash
MixDown:

  MEOW_SHUFFLE(xmm0, xmm1, xmm2, xmm4, xmm5, xmm6);
  MEOW_SHUFFLE(xmm1, xmm2, xmm3, xmm5, xmm6, xmm7);
  MEOW_SHUFFLE(xmm2, xmm3, xmm4, xmm6, xmm7, xmm0);
  MEOW_SHUFFLE(xmm3, xmm4, xmm5, xmm7, xmm0, xmm1);
  MEOW_SHUFFLE(xmm4, xmm5, xmm6, xmm0, xmm1, xmm2);
  MEOW_SHUFFLE(xmm5, xmm6, xmm7, xmm1, xmm2, xmm3);
  MEOW_SHUFFLE(xmm6, xmm7, xmm0, xmm2, xmm3, xmm4);
  MEOW_SHUFFLE(xmm7, xmm0, xmm1, xmm3, xmm4, xmm5);
  MEOW_SHUFFLE(xmm0, xmm1, xmm2, xmm4, xmm5, xmm6);
  MEOW_SHUFFLE(xmm1, xmm2, xmm3, xmm5, xmm6, xmm7);
  MEOW_SHUFFLE(xmm2, xmm3, xmm4, xmm6, xmm7, xmm0);
  MEOW_SHUFFLE(xmm3, xmm4, xmm5, xmm7, xmm0, xmm1);

  paddq(xmm0, xmm2);
  paddq(xmm1, xmm3);
  paddq(xmm4, xmm6);
  paddq(xmm5, xmm7);
  pxor(xmm0, xmm1);
  pxor(xmm4, xmm5);
  paddq(xmm0, xmm4);

  return xmm0;
}

#undef __meow_reorder_barrier
#undef prefetcht0
#undef movdqu
#undef movdqu_mem
#undef movq
#undef aesdec
#undef pshufb
#undef pxor
#undef paddq
#undef pand
#undef palignr
#undef pxor_clear
#undef MEOW_MIX_REG
#undef MEOW_MIX
#undef MEOW_SHUFFLE
#undef MEOW_PAGESIZE
#undef MEOW_PREFETCH
#undef MEOW_PREFETCH_LIMIT

inline void
__meow_make_seed(u8 seed128[128], u64 seed) noexcept
{
  micron::cbytecpy<128>(seed128, __meow_default_seed);
  u64 w0;
  micron::cbytecpy<sizeof(u64)>(&w0, seed128 + 0);
  w0 ^= seed;
  micron::cbytecpy<sizeof(u64)>(seed128 + 0, &w0);
  u64 w1;
  micron::cbytecpy<sizeof(u64)>(&w1, seed128 + 8);
  w1 ^= seed * 0x9E3779B97F4A7C15ull;
  micron::cbytecpy<sizeof(u64)>(seed128 + 8, &w1);
}

template<u64 Seed = 0>
[[gnu::target("aes,sse4.1")]] inline micron::pair<u64, u64>
meowhash128(const byte *data, usize sz) noexcept
{
  alignas(16) u8 seed128[128];
  __meow_make_seed(seed128, Seed);
  __m128i h = __meow_hash(seed128, sz, data);
  return { (u64)_mm_extract_epi64(h, 0), (u64)_mm_extract_epi64(h, 1) };
}

[[gnu::target("aes,sse4.1")]] inline micron::pair<u64, u64>
meowhash128(const byte *data, u64 seed, usize sz) noexcept
{
  alignas(16) u8 seed128[128];
  __meow_make_seed(seed128, seed);
  __m128i h = __meow_hash(seed128, sz, data);
  return { (u64)_mm_extract_epi64(h, 0), (u64)_mm_extract_epi64(h, 1) };
}

template<u64 Seed = 0>
[[gnu::target("aes,sse4.1")]] inline u64
meowhash64(const byte *data, usize sz) noexcept
{
  auto h = meowhash128<Seed>(data, sz);
  return h.a ^ h.b;
}

[[gnu::target("aes,sse4.1")]] inline u64
meowhash64(const byte *data, u64 seed, usize sz) noexcept
{
  auto h = meowhash128(data, seed, sz);
  return h.a ^ h.b;
}

};      // namespace hashes

};      // namespace micron

#endif
