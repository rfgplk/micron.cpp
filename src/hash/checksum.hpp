//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// codec checksums
//  .. crc32   gzip (RFC 1952) trailer
//  .. adler32 zlib (RFC 1950) trailer
//  .. xxh32   lz4 frame checksums
//
// WARNING: do __NOT__ include this from hash.hpp or any core header: it pulls crc.hpp, which carries an
// 8 KiB slice-by-8 table and the carry-less-fold kernels; hashmaps have no business dragging in a gzip trailer

// NOTE: this code has been copied over from czlib

#include "../concepts.hpp"
#include "../types.hpp"
#include "crc.hpp"
#include "xx.hpp"

#if defined(__micron_arch_x86_any) && defined(__micron_x86_avx2)
#define __micron_adler32_simd 1
#include "../simd/aliases/avx.hpp"
#include "../simd/aliases/avx2.hpp"
#include "../simd/aliases/sse.hpp"
#elif defined(__micron_arch_x86_any) && defined(__micron_x86_ssse3)
#define __micron_adler32_simd 1
#include "../simd/aliases/sse.hpp"
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_neon)
#define __micron_adler32_simd 1
#include "../simd/aliases/neon.hpp"
#endif

namespace micron
{

namespace adler
{

constexpr u32 divisor = 65521;      // largest prime below 2^16
constexpr usize nmax = 5552;        // largest n with 255*n*(n+1)/2 + (n+1)*(65520) < 2^32

#if defined(__micron_arch_x86_any) && defined(__micron_x86_ssse3)

[[gnu::target("ssse3")]] inline u32
__adler32_ssse3(const u8 *p, usize n, u32 init) noexcept
{
  alignas(16) static constexpr u8 wtab[16] = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
  alignas(16) static constexpr u16 onetab[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };

  namespace sse = micron::simd::sse;
  u32 s1 = init & 0xFFFFu;
  u32 s2 = (init >> 16) & 0xFFFFu;
  const __m128i zero = sse::zero_i128();
  const __m128i w = sse::load_i128(reinterpret_cast<const __m128i *>(wtab));
  const __m128i ones = sse::load_i128(reinterpret_cast<const __m128i *>(onetab));

  usize i = 0;
  while ( i < n ) {
    const usize block = (n - i < nmax) ? (n - i) : nmax;
    const usize kb = block >> 4;
    u64 s1w = s1, s2w = s2;
    if ( kb ) {
      __m128i vs1 = zero, vps = zero, vdot = zero;
      for ( usize k = 0; k < kb; ++k ) {
        const __m128i v = sse::loadu_i128(reinterpret_cast<const __m128i_u *>(p + i + (k << 4)));
        vps = sse::add_i64(vps, vs1);
        vs1 = sse::add_i64(vs1, sse::sad_u8(v, zero));
        vdot = sse::add_i32(vdot, sse::madd_i16(sse::madd_u8s8(v, w), ones));
      }
      const u64 h_s1 = (u64)sse::extract_low_i64(vs1) + (u64)sse::extract_low_i64(sse::unpack_hi_i64(vs1, vs1));
      const u64 h_ps = (u64)sse::extract_low_i64(vps) + (u64)sse::extract_low_i64(sse::unpack_hi_i64(vps, vps));
      __m128i sh = sse::add_i32(vdot, sse::shuffle_i32<0x4E>(vdot));
      sh = sse::add_i32(sh, sse::shuffle_i32<0xB1>(sh));
      const u32 h_dot = (u32)sse::extract_low_i32(sh);
      s2w += 16ull * (u64)kb * s1w + 16ull * h_ps + h_dot;
      s1w += h_s1;
    }
    for ( usize k = i + (kb << 4), e = i + block; k < e; ++k ) {
      s1w += p[k];
      s2w += s1w;
    }
    // narrow to u32 before the modulo
    s1 = static_cast<u32>(s1w) % divisor;
    s2 = static_cast<u32>(s2w) % divisor;
    i += block;
  }
  return (s2 << 16) | s1;
}

#endif

#if defined(__micron_arch_x86_any) && defined(__micron_x86_avx2)

[[gnu::target("avx2")]] inline u32
__adler32_avx2(const u8 *p, usize n, u32 init) noexcept
{
  alignas(32) static constexpr u8 wtab[32]
      = { 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
  alignas(32) static constexpr u16 onetab[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

  namespace sse = micron::simd::sse;
  namespace avx = micron::simd::avx;
  namespace avx2 = micron::simd::avx2;
  u32 s1 = init & 0xFFFFu;
  u32 s2 = (init >> 16) & 0xFFFFu;
  const __m256i zero = avx2::zero_i256();
  const __m256i w = avx::load_i256(reinterpret_cast<const __m256i *>(wtab));
  const __m256i ones = avx::load_i256(reinterpret_cast<const __m256i *>(onetab));

  usize i = 0;
  while ( i < n ) {
    const usize block = (n - i < nmax) ? (n - i) : nmax;
    const usize kb = block >> 5;
    u64 s1w = s1, s2w = s2;
    if ( kb ) {
      __m256i vs1 = zero, vps = zero, vdot = zero;
      for ( usize k = 0; k < kb; ++k ) {
        const __m256i v = avx2::loadu_i256(reinterpret_cast<const __m256i *>(p + i + (k << 5)));
        vps = avx2::add_i64(vps, vs1);
        vs1 = avx2::add_i64(vs1, avx2::sad_u8(v, zero));
        vdot = avx2::add_i32(vdot, avx2::madd_i16(avx2::madd_u8s8(v, w), ones));
      }
      // extract_f128_i256 is micron's name for _mm256_extracti128_si256 -- the integer op, despite the f128
      const __m128i s1lo = avx::cast_i256_to_lo128(vs1), s1hi = avx::extract_f128_i256<1>(vs1);
      const __m128i pslo = avx::cast_i256_to_lo128(vps), pshi = avx::extract_f128_i256<1>(vps);
      const __m128i vs1s = sse::add_i64(s1lo, s1hi);
      const __m128i vpss = sse::add_i64(pslo, pshi);
      const u64 h_s1 = (u64)sse::extract_low_i64(vs1s) + (u64)sse::extract_low_i64(sse::unpack_hi_i64(vs1s, vs1s));
      const u64 h_ps = (u64)sse::extract_low_i64(vpss) + (u64)sse::extract_low_i64(sse::unpack_hi_i64(vpss, vpss));
      __m128i sh = sse::add_i32(avx::cast_i256_to_lo128(vdot), avx::extract_f128_i256<1>(vdot));
      sh = sse::add_i32(sh, sse::shuffle_i32<0x4E>(sh));
      sh = sse::add_i32(sh, sse::shuffle_i32<0xB1>(sh));
      const u32 h_dot = (u32)sse::extract_low_i32(sh);
      s2w += 32ull * (u64)kb * s1w + 32ull * h_ps + h_dot;
      s1w += h_s1;
    }
    for ( usize k = i + (kb << 5), e = i + block; k < e; ++k ) {
      s1w += p[k];
      s2w += s1w;
    }
    // NOTE: narrow to u32 before the modulo
    s1 = static_cast<u32>(s1w) % divisor;
    s2 = static_cast<u32>(s2w) % divisor;
    i += block;
  }
  return (s2 << 16) | s1;
}

#endif

#if ( defined(__micron_arch_arm64) || defined(__micron_arch_arm32) ) && defined(__micron_arm_neon)

inline u32
__adler32_neon(const u8 *p, usize n, u32 init) noexcept
{
  alignas(16) static constexpr u8 wtab[16] = { 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

  namespace neon = micron::simd::neon;
  u32 s1 = init & 0xFFFFu;
  u32 s2 = (init >> 16) & 0xFFFFu;
  const uint8x16_t w = neon::load_u8(wtab);
  const uint8x8_t wlo = neon::low(w);
  const uint8x8_t whi = neon::high(w);

  usize i = 0;
  while ( i < n ) {
    const usize block = (n - i < nmax) ? (n - i) : nmax;
    const usize kb = block >> 4;
    u64 s1w = s1, s2w = s2;
    if ( kb ) {
      uint32x4_t vs1 = neon::splat_u32(0), vps = neon::splat_u32(0), vdot = neon::splat_u32(0);
      for ( usize k = 0; k < kb; ++k ) {
        const uint8x16_t v = neon::load_u8(p + i + (k << 4));
        vps = neon::add(vps, vs1);
        vs1 = neon::pairwise_add_acc_u16(vs1, neon::pairwise_add_long_u8(v));
        vdot = neon::pairwise_add_acc_u16(vdot, neon::mul_long(neon::low(v), wlo));
        vdot = neon::pairwise_add_acc_u16(vdot, neon::mul_long(neon::high(v), whi));
      }
      const u64 h_s1
          = (u64)neon::get_lane_u32<0>(vs1) + neon::get_lane_u32<1>(vs1) + neon::get_lane_u32<2>(vs1) + neon::get_lane_u32<3>(vs1);
      const u64 h_ps
          = (u64)neon::get_lane_u32<0>(vps) + neon::get_lane_u32<1>(vps) + neon::get_lane_u32<2>(vps) + neon::get_lane_u32<3>(vps);
      const u64 h_dot
          = (u64)neon::get_lane_u32<0>(vdot) + neon::get_lane_u32<1>(vdot) + neon::get_lane_u32<2>(vdot) + neon::get_lane_u32<3>(vdot);
      s2w += 16ull * (u64)kb * s1w + 16ull * h_ps + h_dot;
      s1w += h_s1;
    }
    for ( usize k = i + (kb << 4), e = i + block; k < e; ++k ) {
      s1w += p[k];
      s2w += s1w;
    }
    // NOTE: narrow to u32 before the modulo
    s1 = (u32)s1w % divisor;
    s2 = (u32)s2w % divisor;
    i += block;
  }
  return (s2 << 16) | s1;
}

#endif

constexpr u32
__adler32_scalar(const u8 *p, usize n, u32 init) noexcept
{
  u32 s1 = init & 0xFFFFu;
  u32 s2 = (init >> 16) & 0xFFFFu;

  usize i = 0;
  while ( i < n ) {
    const usize block = (n - i < nmax) ? (n - i) : nmax;
    const usize end4 = i + (block & ~usize(3));
    for ( ; i < end4; i += 4 ) {
      const u32 b0 = p[i], b1 = p[i + 1], b2 = p[i + 2], b3 = p[i + 3];
      s2 += 4 * s1 + 4 * b0 + 3 * b1 + 2 * b2 + b3;
      s1 += b0 + b1 + b2 + b3;
    }
    for ( const usize e = i + (block & 3); i < e; ++i ) {
      s1 += p[i];
      s2 += s1;
    }
    s1 %= divisor;
    s2 %= divisor;
  }
  return (s2 << 16) | s1;
}

};      // namespace adler

constexpr u32
adler32(u32 init, const u8 *buf, usize len) noexcept
{
  if !consteval {
#if defined(__micron_arch_x86_any) && defined(__micron_x86_avx2)
    if ( len >= 64 ) return adler::__adler32_avx2(buf, len, init);
#elif defined(__micron_arch_x86_any) && defined(__micron_x86_ssse3)
    if ( len >= 64 ) return adler::__adler32_ssse3(buf, len, init);
#elif (defined(__micron_arch_arm64) || defined(__micron_arch_arm32)) && defined(__micron_arm_neon)
    if ( len >= 64 ) return adler::__adler32_neon(buf, len, init);
#endif
  }
  return adler::__adler32_scalar(buf, len, init);
}

template<is_iterable C>
constexpr u32
adler32(u32 init, const C &src) noexcept
{
  return adler32(init, reinterpret_cast<const u8 *>(src.begin()), src.size() * sizeof(typename C::value_type));
}

template<addressable T>
constexpr u32
adler32(u32 init, const T &obj) noexcept
{
  return adler32(init, reinterpret_cast<const u8 *>(&obj), sizeof(T));
}

};      // namespace micron
