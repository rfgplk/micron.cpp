//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../simd/simd.hpp"
#include "../types.hpp"

#include "charreach.hpp"

#if defined(__micron_arch_x86_any)
#include "../simd/aliases/avx2.hpp"
#include "../simd/aliases/sse.hpp"
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm64)
#include "../simd/aliases/neon.hpp"
#endif

namespace micron
{
namespace rgx
{

struct truffle_masks {
  u8 lo_clear[16] = {};      // for bytes with bit 7 == 0
  u8 lo_set[16] = {};        // for bytes with bit 7 == 1
};

inline truffle_masks
truffle_build(const charreach &cls) noexcept
{
  truffle_masks t;
  for ( unsigned c = 0; c < 256; ++c ) {
    if ( !cls.test((u8)c) ) continue;
    u8 *tbl = (c & 0x80) ? t.lo_set : t.lo_clear;
    tbl[c & 0x0f] |= (u8)(1u << ((c >> 4) & 7));
  }
  return t;
}

inline usize
truffle_find_first(const char *p, usize n, const truffle_masks &t) noexcept
{
  alignas(16) u8 pow2[16] = { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };
  usize i = 0;

#if defined(__micron_x86_avx2)
  {
    namespace avx2 = micron::simd::avx2;
    namespace sse = micron::simd::sse;
    __m256i loc = avx2::broadcast_i128_to_i256(sse::loadu_i128(reinterpret_cast<const __m128i_u *>(t.lo_clear)));
    __m256i los = avx2::broadcast_i128_to_i256(sse::loadu_i128(reinterpret_cast<const __m128i_u *>(t.lo_set)));
    __m256i pw = avx2::broadcast_i128_to_i256(sse::loadu_i128(reinterpret_cast<const __m128i_u *>(pow2)));
    __m256i m07 = avx2::set1_i8(0x07);
    __m256i hib = avx2::set1_i8((char)0x80);
    __m256i zero = avx2::zero_i256();
    for ( ; i + 32 <= n; i += 32 ) {
      __m256i v = avx2::loadu_i256(reinterpret_cast<const __m256i *>(p + i));
      __m256i s1 = avx2::shuffle_v_i8_256(loc, v);
      __m256i s2 = avx2::shuffle_v_i8_256(los, avx2::xor_i256(v, hib));
      __m256i clo = avx2::or_i256(s1, s2);
      __m256i hi3 = avx2::and_i256(avx2::shr_i16(v, 4), m07);
      __m256i bv = avx2::shuffle_v_i8_256(pw, hi3);
      __m256i res = avx2::and_i256(clo, bv);
      unsigned z = (unsigned)avx2::movemask_i8(avx2::cmpeq_i8(res, zero));
      unsigned inclass = ~z;      // 0-bit in z == in class
      if ( inclass ) return i + (usize)__builtin_ctz(inclass);
    }
  }
#elif defined(__micron_arm_neon) && defined(__micron_arch_arm64)
  {
    namespace neon = micron::simd::neon;
    uint8x16_t loc = neon::load_u8(t.lo_clear), los = neon::load_u8(t.lo_set), pw = neon::load_u8(pow2);
    uint8x16_t m8f = neon::dup_u8(0x8f), m07 = neon::dup_u8(0x07), hib = neon::dup_u8(0x80), zero = neon::dup_u8(0);
    for ( ; i + 16 <= n; i += 16 ) {
      uint8x16_t v = neon::load_u8(reinterpret_cast<const u8 *>(p + i));
      uint8x16_t s1 = neon::tbl1_u8(loc, neon::and_u8(v, m8f));
      uint8x16_t s2 = neon::tbl1_u8(los, neon::and_u8(neon::eor_u8(v, hib), m8f));
      uint8x16_t clo = neon::orr_u8(s1, s2);
      uint8x16_t hi3 = neon::and_u8(neon::shr_imm_u8<4>(v), m07);
      uint8x16_t bv = neon::tbl1_u8(pw, hi3);
      uint8x16_t res = neon::and_u8(clo, bv);
      u32 z = neon::movemask_u8(neon::ceq_u8(res, zero));      // bit set where byte NOT in class
      u32 inclass = (~z) & 0xffffu;
      if ( inclass ) return i + (usize)__builtin_ctz(inclass);
    }
  }
#endif

  for ( ; i < n; ++i ) {
    u8 c = (u8)p[i];
    u8 tbl = (c & 0x80) ? t.lo_set[c & 0x0f] : t.lo_clear[c & 0x0f];
    if ( (tbl >> ((c >> 4) & 7)) & 1 ) return i;
  }
  return n;
}

};      // namespace rgx
};      // namespace micron
