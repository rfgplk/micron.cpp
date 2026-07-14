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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// crc_simd code
//  .. __micron_crc_clmul   crc64 normal-form fold; pclmul
//  .. __micron_crc32_clmul reflected crc32 fold; hard requires SSE4.1
// NOTE: apparently every x86 part that has PCLMUL (>Westmere) also has SSE4.1, so this never actually excludes a real
// CPU
#if defined(__micron_arch_x86_any) && defined(__micron_x86_pclmul)
#define __micron_crc_clmul 1
#if defined(__micron_x86_sse4_1)
#define __micron_crc32_clmul 1
#endif
#include "../simd/aliases/aes.hpp"
#include "../simd/aliases/sse.hpp"
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_pmull) && defined(__micron_arm_neon)
#define __micron_crc_clmul 1
#define __micron_crc32_clmul 1
#include "../simd/aliases/neon.hpp"
#include "../simd/aliases/neon_aes.hpp"
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

#if defined(__micron_crc32_clmul)

// reflected CRC-32 (gzip poly), 4-way 64 B/iter fold + Barrett reduction (spliced in from czlib)
//
// NOTE: do not merge these with __fold_k; you will get a plausible looking wrong answer
[[gnu::target("pclmul,sse4.1")]] static inline u32
__crc32_refl_clmul(u32 init, const u8 *p, usize len) noexcept
{
  // the (long long) casts are the declared parameter type of set_i64, not a width assumption
  const __v128 mults = __sse::set_i64((long long)0xccaa009eULL, (long long)0xae689191ULL);
  const __v128 mults4 = __sse::set_i64((long long)0x1d9513d7ULL, (long long)0x8f352d95ULL);
  const __v128 barrett = __sse::set_i64((long long)0x1db710641ULL, (long long)0xb4e5b025f7011641ULL);

  __v128 x0 = __sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p));
  x0 = __sse::xor_i128(x0, __sse::broadcast_i32_to_i128((int)(init ^ 0xFFFFFFFFu)));
  p += 16;
  len -= 16;

  if ( len >= 48 ) {
    __v128 x1 = __sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p));
    __v128 x2 = __sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p + 16));
    __v128 x3 = __sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p + 32));
    p += 48;
    len -= 48;
    while ( len >= 64 ) {
      x0 = __sse::xor_i128(__sse::xor_i128(__sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p)), __clm::clmul_64<0x00>(x0, mults4)),
                           __clm::clmul_64<0x11>(x0, mults4));
      x1 = __sse::xor_i128(
          __sse::xor_i128(__sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p + 16)), __clm::clmul_64<0x00>(x1, mults4)),
          __clm::clmul_64<0x11>(x1, mults4));
      x2 = __sse::xor_i128(
          __sse::xor_i128(__sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p + 32)), __clm::clmul_64<0x00>(x2, mults4)),
          __clm::clmul_64<0x11>(x2, mults4));
      x3 = __sse::xor_i128(
          __sse::xor_i128(__sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p + 48)), __clm::clmul_64<0x00>(x3, mults4)),
          __clm::clmul_64<0x11>(x3, mults4));
      p += 64;
      len -= 64;
    }
    x0 = __sse::xor_i128(__sse::xor_i128(x1, __clm::clmul_64<0x00>(x0, mults)), __clm::clmul_64<0x11>(x0, mults));
    x0 = __sse::xor_i128(__sse::xor_i128(x2, __clm::clmul_64<0x00>(x0, mults)), __clm::clmul_64<0x11>(x0, mults));
    x0 = __sse::xor_i128(__sse::xor_i128(x3, __clm::clmul_64<0x00>(x0, mults)), __clm::clmul_64<0x11>(x0, mults));
  }
  while ( len >= 16 ) {
    const __v128 d = __sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p));
    x0 = __sse::xor_i128(__sse::xor_i128(d, __clm::clmul_64<0x00>(x0, mults)), __clm::clmul_64<0x11>(x0, mults));
    p += 16;
    len -= 16;
  }
  x0 = __sse::xor_i128(__clm::clmul_64<0x10>(x0, mults), __sse::bsrl_i128<8>(x0));
  __v128 x1 = __clm::clmul_64<0x00>(x0, barrett);
  x1 = __clm::clmul_64<0x10>(x1, barrett);
  x0 = __sse::xor_i128(x0, x1);
  const u32 r = (u32)__sse::extract_i32_imm<2>(x0);

  return __crc32_refl_slice8(p, len, r) ^ 0xFFFFFFFFu;
}

#endif

#else

// ARM branch

namespace __nc = micron::simd::neon_crypto;
namespace __neon = micron::simd::neon;

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

// reflected CRC-32, 16 B/iter PMULL fold
static inline u32
__crc32_refl_clmul(u32 init, const u8 *p, usize len) noexcept
{
  alignas(16) static constexpr u64 k_mults[2] = { 0xae689191ULL, 0xccaa009eULL };      // x^159, x^95
  alignas(16) static constexpr u64 k_x95[2] = { 0xccaa009eULL, 0 };                    // x^95 mod G
  alignas(16) static constexpr u64 k_mu[2] = { 0xb4e5b025f7011641ULL, 0 };             // floor(x^95 / G)
  alignas(16) static constexpr u64 k_g[2] = { 0x1db710641ULL, 0 };                     // G(x)

  const __v128 mults = __nc::load_u8q(reinterpret_cast<const u8 *>(k_mults));
  const __v128 m_x95 = __nc::load_u8q(reinterpret_cast<const u8 *>(k_x95));
  const __v128 b_mu = __nc::load_u8q(reinterpret_cast<const u8 *>(k_mu));
  const __v128 b_g = __nc::load_u8q(reinterpret_cast<const u8 *>(k_g));

  __v128 x0 = __nc::load_u8q(p);
  const uint32x4_t xi = __neon::set_lane_u32<0>(init ^ 0xFFFFFFFFu, __neon::splat_u32(0));
  x0 = __nc::xor_q(x0, (__v128)xi);      // fold the conditioned register into the first 4 bytes
  p += 16;
  len -= 16;
  while ( len >= 16 ) {
    const __v128 d = __nc::load_u8q(p);
    x0 = __nc::xor_q(__nc::xor_q(d, __nc::clmul_lo(x0, mults)), __nc::clmul_hi(x0, mults));
    p += 16;
    len -= 16;
  }
  x0 = __nc::xor_q(__nc::clmul_lo(x0, m_x95), __neon::ext_u8<8>(x0, __neon::splat_u8(0)));
  __v128 t = __nc::clmul_lo(x0, b_mu);
  t = __nc::clmul_lo(t, b_g);
  x0 = __nc::xor_q(x0, t);
  const u32 r = __neon::get_lane_u32<2>((uint32x4_t)x0);

  return __crc32_refl_slice8(p, len, r) ^ 0xFFFFFFFFu;
}

#endif

inline constexpr usize __clmul_min = 64;
inline constexpr usize __clmul32_min = 16;

#endif

};      // namespace __simd
};      // namespace crc
};      // namespace micron
