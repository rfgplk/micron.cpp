//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// carry-less-multiply CRC64 folding
//
// WARNING: crc_simd depends on crc.hpps tables, it will not compile without them

#include "../types.hpp"

#if defined(__micron_arch_x86_any) && defined(__micron_x86_pclmul)
#define __micron_crc_clmul 1
#include "../simd/aliases.hpp"
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_pmull) && defined(__micron_arm_neon)
#define __micron_crc_clmul 1
#include "../simd/aliases.hpp"
#endif

namespace micron
{
namespace crc
{
namespace __simd
{

// x^n mod P, where P = x^64 + poly
consteval u64
__xpow_mod(usize n, u64 poly) noexcept
{
  u64 r = 1;      // x^0
  for ( usize i = 0; i < n; ++i ) r = (r << 1) ^ ((r >> 63) ? poly : 0ull);
  return r;
}

template<u64 Poly, usize N> struct __fold_k {
  static constexpr u64 lo = __xpow_mod(N, Poly);
  static constexpr u64 hi = __xpow_mod(N + 64, Poly);
};

#if defined(__micron_crc_clmul)

[[gnu::always_inline]] static inline u64
__norm_byte(u64 crc, u8 b, const u64 *lut) noexcept
{
  return (crc << 8) ^ lut[((crc >> 56) ^ b) & 0xFFu];
}

#if defined(__micron_arch_x86_any)

namespace __sse = micron::simd::sse;
namespace __clm = micron::simd::aes;

using __v128 = __m128i;

[[gnu::always_inline, gnu::target("ssse3")]] static inline __v128
__bswap128(__v128 v) noexcept
{
  const __v128 rev = __sse::setr_i8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
  return __sse::shuffle_v_i8(v, rev);
}

[[gnu::always_inline, gnu::target("ssse3")]] static inline __v128
__ld_be(const u8 *p) noexcept
{
  return __bswap128(__sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p)));
}

template<u64 Poly, usize N>
[[gnu::always_inline, gnu::target("pclmul,sse4.1")]] static inline __v128
__kconst() noexcept
{
  // { lo, hi } in memory order -> low qword = lo, high qword = hi
  const u64 k[2] = { __fold_k<Poly, N>::lo, __fold_k<Poly, N>::hi };
  return __sse::loadu_i128(reinterpret_cast<const __m128i_u *>(k));
}

// acc * x^N mod P, kept as 128 bits (congruent, not reduced)
[[gnu::always_inline, gnu::target("pclmul,sse4.1")]] static inline __v128
__fold(__v128 acc, __v128 k) noexcept
{
  const __v128 h = __clm::clmul_64<0x11>(acc, k);      // acc_hi (x) k_hi
  const __v128 l = __clm::clmul_64<0x00>(acc, k);      // acc_lo (x) k_lo
  return __sse::xor_i128(h, l);
}

template<u64 Poly>
[[gnu::target("pclmul,sse4.1,ssse3")]] static inline u64
__crc64_norm_clmul(u64 init, const u8 *buf, usize len, const u64 *lut) noexcept
{
  usize nc = len / 16;      // whole 16-byte chunks
  const u8 *p = buf;

  u8 head[16];
  for ( usize i = 0; i < 16; ++i ) head[i] = p[i];
  for ( usize i = 0; i < 8; ++i ) head[i] ^= static_cast<u8>(init >> (56 - 8 * i));

  const __v128 k1 = __kconst<Poly, 128>();      // advance 1 chunk
  __v128 acc;

  if ( nc >= 8 ) {
    const __v128 k4 = __kconst<Poly, 512>();      // advance 4 chunks
    __v128 a0 = __ld_be(head);                    // chunk 0 (carries init)
    __v128 a1 = __ld_be(p + 16);
    __v128 a2 = __ld_be(p + 32);
    __v128 a3 = __ld_be(p + 48);
    p += 64;
    nc -= 4;

    while ( nc >= 4 ) {
      a0 = __sse::xor_i128(__fold(a0, k4), __ld_be(p + 0));
      a1 = __sse::xor_i128(__fold(a1, k4), __ld_be(p + 16));
      a2 = __sse::xor_i128(__fold(a2, k4), __ld_be(p + 32));
      a3 = __sse::xor_i128(__fold(a3, k4), __ld_be(p + 48));
      p += 64;
      nc -= 4;
    }

    // recombine: acc0 is 3 chunks behind acc3, acc1 two, acc2 one.
    const __v128 k2 = __kconst<Poly, 256>();
    const __v128 k3 = __kconst<Poly, 384>();
    acc = __sse::xor_i128(__sse::xor_i128(__fold(a0, k3), __fold(a1, k2)), __sse::xor_i128(__fold(a2, k1), a3));
  } else {
    acc = __ld_be(head);
    p += 16;
    --nc;
  }

  while ( nc > 0 ) {
    acc = __sse::xor_i128(__fold(acc, k1), __ld_be(p));
    p += 16;
    --nc;
  }

  u8 tmp[16];
  __sse::storeu_i128(reinterpret_cast<__m128i_u *>(tmp), __bswap128(acc));

  u64 crc = 0;
  for ( usize i = 0; i < 16; ++i ) crc = __norm_byte(crc, tmp[i], lut);

  // the < 16-byte remainder just continues the scalar recurrence
  for ( usize i = 0; i < (len % 16); ++i ) crc = __norm_byte(crc, p[i], lut);
  return crc;
}

#else      // ARM (AArch64 pmull/pmull2, AArch32 vmull.p64) — same identity, same constants

namespace __nc = micron::simd::neon_crypto;

using __v128 = uint8x16_t;

[[gnu::always_inline]] static inline __v128
__ld_be(const u8 *p) noexcept
{
  return __nc::rev_bytes_q(__nc::load_u8q(p));
}

template<u64 Poly, usize N>
[[gnu::always_inline]] static inline __v128
__kconst() noexcept
{
  const u64 k[2] = { __fold_k<Poly, N>::lo, __fold_k<Poly, N>::hi };
  return __nc::load_u8q(reinterpret_cast<const u8 *>(k));
}

[[gnu::always_inline]] static inline __v128
__fold(__v128 acc, __v128 k) noexcept
{
  // identical identity to x86: acc_hi (x) k_hi  ^  acc_lo (x) k_lo
  const __v128 h = __nc::clmul_hi(acc, k);
  const __v128 l = __nc::clmul_lo(acc, k);
  return __nc::xor_q(h, l);
}

template<u64 Poly>
static inline u64
__crc64_norm_clmul(u64 init, const u8 *buf, usize len, const u64 *lut) noexcept
{
  usize nc_ = len / 16;
  const u8 *p = buf;

  u8 head[16];
  for ( usize i = 0; i < 16; ++i ) head[i] = p[i];
  for ( usize i = 0; i < 8; ++i ) head[i] ^= static_cast<u8>(init >> (56 - 8 * i));

  const __v128 k1 = __kconst<Poly, 128>();
  __v128 acc;

  if ( nc_ >= 8 ) {
    const __v128 k4 = __kconst<Poly, 512>();
    __v128 a0 = __ld_be(head);
    __v128 a1 = __ld_be(p + 16);
    __v128 a2 = __ld_be(p + 32);
    __v128 a3 = __ld_be(p + 48);
    p += 64;
    nc_ -= 4;

    while ( nc_ >= 4 ) {
      a0 = __nc::xor_q(__fold(a0, k4), __ld_be(p + 0));
      a1 = __nc::xor_q(__fold(a1, k4), __ld_be(p + 16));
      a2 = __nc::xor_q(__fold(a2, k4), __ld_be(p + 32));
      a3 = __nc::xor_q(__fold(a3, k4), __ld_be(p + 48));
      p += 64;
      nc_ -= 4;
    }

    const __v128 k2 = __kconst<Poly, 256>();
    const __v128 k3 = __kconst<Poly, 384>();
    acc = __nc::xor_q(__nc::xor_q(__fold(a0, k3), __fold(a1, k2)), __nc::xor_q(__fold(a2, k1), a3));
  } else {
    acc = __ld_be(head);
    p += 16;
    --nc_;
  }

  while ( nc_ > 0 ) {
    acc = __nc::xor_q(__fold(acc, k1), __ld_be(p));
    p += 16;
    --nc_;
  }

  u8 tmp[16];
  __nc::store_u8q(tmp, __nc::rev_bytes_q(acc));

  u64 crc = 0;
  for ( usize i = 0; i < 16; ++i ) crc = __norm_byte(crc, tmp[i], lut);
  for ( usize i = 0; i < (len % 16); ++i ) crc = __norm_byte(crc, p[i], lut);
  return crc;
}

#endif      // arch

inline constexpr usize __clmul_min = 64;

#endif      // __micron_crc_clmul

};      // namespace __simd
};      // namespace crc
};      // namespace micron
