//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// ARMv7-a NEON port of the 8086 avx256 z/zz/zzz hash family.
#include "../simd/simd.hpp"
#include "../types.hpp"

#include "../tuple.hpp"

#if !defined(__micron_arm_neon)
#error "zzz hash NEON port requires ARM NEON (ARMv7-a+ with -mfpu=neon)."
#endif

#if defined(__micron_endian_big)
#error "zzz hash NEON port assumes little-endian ARM."
#endif

namespace micron
{
// same as other algs
namespace hashes
{
// T = [int(256*((i*0.6180339887)%1))-128 for i in range(32)]
constexpr static const i8 iv[32] = { 127, -113, 94, -72, 55, -39, 21, -7, 118, -101, 83, -65, 47, -29, 11, -3,
                                     120, -109, 91, -74, 58, -42, 24, -9, 115, -98,  80, -63, 44, -27, 9,  -1 };

constexpr i64
pack_iv64(usize off)
{
  return ((i64)(u8)iv[off + 0] << 0) | ((i64)(u8)iv[off + 1] << 8) | ((i64)(u8)iv[off + 2] << 16) | ((i64)(u8)iv[off + 3] << 24)
         | ((i64)(u8)iv[off + 4] << 32) | ((i64)(u8)iv[off + 5] << 40) | ((i64)(u8)iv[off + 6] << 48) | ((i64)(u8)iv[off + 7] << 56);
}

constexpr static const u64 __zzzrounds = 1;
constexpr static const u64 __zzrounds = 1;

constexpr i64 iv64_0 = pack_iv64(0);
constexpr i64 iv64_1 = pack_iv64(8);
constexpr i64 iv64_2 = pack_iv64(16);
constexpr i64 iv64_3 = pack_iv64(24);

alignas(32) static const u8 __zero_block[32] = { 0 };

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// 256-bit state emulation on top of two 128-bit NEON Q registers
struct __m256_neon {
  uint64x2_t lo;     // lanes 0, 1  (bytes  0..15)
  uint64x2_t hi;     // lanes 2, 3  (bytes 16..31)
};

// _mm256_set1_epi64x
__attribute__((always_inline)) static inline __m256_neon
__zset1_epi64(i64 v)
{
  uint64x2_t x = vdupq_n_u64((u64)v);
  return { x, x };
}

// _mm256_set_epi64x(e3,e2,e1,e0)
__attribute__((always_inline)) static inline __m256_neon
__zset_epi64(i64 e3, i64 e2, i64 e1, i64 e0)
{
  uint64x2_t lo = vcombine_u64(vcreate_u64((u64)e0), vcreate_u64((u64)e1));
  uint64x2_t hi = vcombine_u64(vcreate_u64((u64)e2), vcreate_u64((u64)e3));
  return { lo, hi };
}

__attribute__((always_inline)) static inline __m256_neon
__zxor(__m256_neon a, __m256_neon b)
{
  return { veorq_u64(a.lo, b.lo), veorq_u64(a.hi, b.hi) };
}

__attribute__((always_inline)) static inline __m256_neon
__zand(__m256_neon a, __m256_neon b)
{
  return { vandq_u64(a.lo, b.lo), vandq_u64(a.hi, b.hi) };
}

// matches _mm256_add_epi64
__attribute__((always_inline)) static inline __m256_neon
__zadd64(__m256_neon a, __m256_neon b)
{
  return { vreinterpretq_u64_s64(vaddq_s64(vreinterpretq_s64_u64(a.lo), vreinterpretq_s64_u64(b.lo))),
           vreinterpretq_u64_s64(vaddq_s64(vreinterpretq_s64_u64(a.hi), vreinterpretq_s64_u64(b.hi))) };
}

// matches _mm256_sub_epi64
__attribute__((always_inline)) static inline __m256_neon
__zsub64(__m256_neon a, __m256_neon b)
{
  return { vreinterpretq_u64_s64(vsubq_s64(vreinterpretq_s64_u64(a.lo), vreinterpretq_s64_u64(b.lo))),
           vreinterpretq_u64_s64(vsubq_s64(vreinterpretq_s64_u64(a.hi), vreinterpretq_s64_u64(b.hi))) };
}

// matches _mm256_subs_epi8
__attribute__((always_inline)) static inline __m256_neon
__zsubs_epi8(__m256_neon a, __m256_neon b)
{
  int8x16_t lo = vqsubq_s8(vreinterpretq_s8_u64(a.lo), vreinterpretq_s8_u64(b.lo));
  int8x16_t hi = vqsubq_s8(vreinterpretq_s8_u64(a.hi), vreinterpretq_s8_u64(b.hi));
  return { vreinterpretq_u64_s8(lo), vreinterpretq_u64_s8(hi) };
}

// _mm256_permute2x128_si256(x, x, 0x01)
__attribute__((always_inline)) static inline __m256_neon
__zperm_swap(__m256_neon a)
{
  return { a.hi, a.lo };
}

__attribute__((always_inline)) static inline __m256_neon
__zload(const u8 *p)
{
  return { vld1q_u64(reinterpret_cast<const u64 *>(p)), vld1q_u64(reinterpret_cast<const u64 *>(p + 16)) };
}

__attribute__((always_inline)) static inline void
__zstore(u8 *p, __m256_neon v)
{
  vst1q_u64(reinterpret_cast<u64 *>(p), v.lo);
  vst1q_u64(reinterpret_cast<u64 *>(p + 16), v.hi);
}

// matches _mm256_slli_epi64 with imm8 N
template <int N>
__attribute__((always_inline)) static inline __m256_neon
__zslli64(__m256_neon a)
{
  return { vreinterpretq_u64_s64(vshlq_n_s64(vreinterpretq_s64_u64(a.lo), N)),
           vreinterpretq_u64_s64(vshlq_n_s64(vreinterpretq_s64_u64(a.hi), N)) };
}

// matches _mm256_srli_epi64
template <int N>
__attribute__((always_inline)) static inline __m256_neon
__zsrli64(__m256_neon a)
{
  return { vshrq_n_u64(a.lo, N), vshrq_n_u64(a.hi, N) };
}

// matches _mm256_slli_epi16
template <int N>
__attribute__((always_inline)) static inline __m256_neon
__zslli16(__m256_neon a)
{
  int16x8_t lo = vshlq_n_s16(vreinterpretq_s16_u64(a.lo), N);
  int16x8_t hi = vshlq_n_s16(vreinterpretq_s16_u64(a.hi), N);
  return { vreinterpretq_u64_s16(lo), vreinterpretq_u64_s16(hi) };
}

// match _mm256_sllv_epi64
__attribute__((always_inline)) static inline __m256_neon
__zsllv64(__m256_neon a, __m256_neon shifts)
{
  return { vshlq_u64(a.lo, vreinterpretq_s64_u64(shifts.lo)), vshlq_u64(a.hi, vreinterpretq_s64_u64(shifts.hi)) };
}

__attribute__((always_inline)) static inline __m256_neon
__zload_tail(const u8 *ptr, usize remaining)
{
  alignas(16) u8 tmp[32];
  for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
  for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];
  return __zload(tmp);
}

// hard seed
template <i64 Seed>
void
z(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256_neon IV = __zset_epi64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256_neon seedv = __zset1_epi64(Seed);

  __m256_neon S0 = __zxor(IV, seedv);
  __m256_neon S1 = __zxor(__zperm_swap(IV), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256_neon mask64 = __zset1_epi64((i64)0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256_neon block;
    usize remaining = (usize)(end - ptr);
    if ( remaining >= 32 ) {
      block = __zload(ptr);
      ptr += 32;
    } else {
      block = __zload_tail(ptr, remaining);
      ptr += remaining;
    }

    __m256_neon S0_next = __zxor(S0, block);
    __m256_neon S1_next = __zxor(S1, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256_neon T0 = __zslli64<3>(S0_next);
      __m256_neon A0 = __zsub64(S0_next, T0);
      S0_next = __zand(A0, mask64);

      __m256_neon T1 = __zslli64<2>(S1_next);
      __m256_neon A1 = __zsub64(S1_next, T1);
      S1_next = __zand(A1, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
  }

  __m256_neon S = __zxor(S0, S1);
  __zstore(reinterpret_cast<u8 *>(out), S);
}

// rt seed
void
z(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256_neon IV = __zset_epi64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256_neon seedv = __zset1_epi64(seed);

  __m256_neon S0 = __zxor(IV, seedv);
  __m256_neon S1 = __zxor(__zperm_swap(IV), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256_neon mask64 = __zset1_epi64((i64)0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256_neon block;
    usize remaining = (usize)(end - ptr);
    if ( remaining >= 32 ) {
      block = __zload(ptr);
      ptr += 32;
    } else {
      block = __zload_tail(ptr, remaining);
      ptr += remaining;
    }

    __m256_neon S0_next = __zxor(S0, block);
    __m256_neon S1_next = __zxor(S1, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256_neon T0 = __zslli64<3>(S0_next);
      __m256_neon A0 = __zsub64(S0_next, T0);
      S0_next = __zand(A0, mask64);

      __m256_neon T1 = __zslli64<2>(S1_next);
      __m256_neon A1 = __zsub64(S1_next, T1);
      S1_next = __zand(A1, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
  }

  __m256_neon S = __zxor(S0, S1);
  __zstore(reinterpret_cast<u8 *>(out), S);
}

// _mm256_set_epi64x(13, 29, 7, 19)
// = [19, 7, 29, 13]
// NEON: lo = {19, 7},  hi = {29, 13}
__attribute__((always_inline)) static inline __m256_neon
__zz_shift_vec()
{
  uint64x2_t lo = vcombine_u64(vcreate_u64((u64)19), vcreate_u64((u64)7));
  uint64x2_t hi = vcombine_u64(vcreate_u64((u64)29), vcreate_u64((u64)13));
  return { lo, hi };
}

// hard seed version
template <i64 Seed>
void
zz(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256_neon shift_vec = __zz_shift_vec();
  const __m256_neon IV = __zset_epi64(iv64_3, iv64_2, iv64_1, iv64_0);

  __m256_neon S0 = IV;
  __m256_neon S1 = __zperm_swap(IV);
  __m256_neon S2 = __zxor(IV, __zslli64<1>(IV));
  __m256_neon S3 = __zadd64(IV, __zslli64<2>(IV));

  const __m256_neon seedv = __zset1_epi64(Seed);
  S0 = __zxor(S0, seedv);
  S1 = __zxor(S1, seedv);
  S2 = __zxor(S2, seedv);
  S3 = __zxor(S3, seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr < end ) {
    __m256_neon block;
    usize remaining = (usize)(end - ptr);
    if ( remaining >= 32 ) {
      block = __zload(ptr);
      ptr += 32;
    } else {
      block = __zload_tail(ptr, remaining);
      ptr += remaining;
    }

    S0 = __zxor(S0, block);
    S1 = __zadd64(S1, block);
    S2 = __zxor(S2, __zslli64<1>(block));
    S3 = __zadd64(S3, __zsrli64<1>(block));

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256_neon T, U;

      // s0
      T = __zsllv64(S0, shift_vec);
      S0 = __zxor(S0, T);
      U = __zperm_swap(S0);
      S0 = __zsubs_epi8(S0, U);
      T = __zslli16<3>(S0);
      S0 = __zxor(S0, T);

      // s1
      T = __zsllv64(S1, shift_vec);
      S1 = __zxor(S1, T);
      U = __zperm_swap(S1);
      S1 = __zsubs_epi8(S1, U);
      T = __zslli16<3>(S1);
      S1 = __zxor(S1, T);

      // s2
      T = __zsllv64(S2, shift_vec);
      S2 = __zxor(S2, T);
      U = __zperm_swap(S2);
      S2 = __zsubs_epi8(S2, U);
      T = __zslli16<3>(S2);
      S2 = __zxor(S2, T);

      // s3
      T = __zsllv64(S3, shift_vec);
      S3 = __zxor(S3, T);
      U = __zperm_swap(S3);
      S3 = __zsubs_epi8(S3, U);
      T = __zslli16<3>(S3);
      S3 = __zxor(S3, T);
    }
  }

  __m256_neon S_final = __zxor(__zxor(S0, S1), __zxor(S2, S3));
  __zstore(reinterpret_cast<u8 *>(out), S_final);
}

// rt seed version
void
zz(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256_neon shift_vec = __zz_shift_vec();
  const __m256_neon IV = __zset_epi64(iv64_3, iv64_2, iv64_1, iv64_0);

  __m256_neon S0 = IV;
  __m256_neon S1 = __zperm_swap(IV);
  __m256_neon S2 = __zxor(IV, __zslli64<1>(IV));
  __m256_neon S3 = __zadd64(IV, __zslli64<2>(IV));

  const __m256_neon seedv = __zset1_epi64(seed);
  S0 = __zxor(S0, seedv);
  S1 = __zxor(S1, seedv);
  S2 = __zxor(S2, seedv);
  S3 = __zxor(S3, seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr < end ) {
    __m256_neon block;
    usize remaining = (usize)(end - ptr);
    if ( remaining >= 32 ) {
      block = __zload(ptr);
      ptr += 32;
    } else {
      block = __zload_tail(ptr, remaining);
      ptr += remaining;
    }

    S0 = __zxor(S0, block);
    S1 = __zadd64(S1, block);
    S2 = __zxor(S2, __zslli64<1>(block));
    S3 = __zadd64(S3, __zsrli64<1>(block));

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256_neon T, U;

      T = __zsllv64(S0, shift_vec);
      S0 = __zxor(S0, T);
      U = __zperm_swap(S0);
      S0 = __zsubs_epi8(S0, U);
      T = __zslli16<3>(S0);
      S0 = __zxor(S0, T);

      T = __zsllv64(S1, shift_vec);
      S1 = __zxor(S1, T);
      U = __zperm_swap(S1);
      S1 = __zsubs_epi8(S1, U);
      T = __zslli16<3>(S1);
      S1 = __zxor(S1, T);

      T = __zsllv64(S2, shift_vec);
      S2 = __zxor(S2, T);
      U = __zperm_swap(S2);
      S2 = __zsubs_epi8(S2, U);
      T = __zslli16<3>(S2);
      S2 = __zxor(S2, T);

      T = __zsllv64(S3, shift_vec);
      S3 = __zxor(S3, T);
      U = __zperm_swap(S3);
      S3 = __zsubs_epi8(S3, U);
      T = __zslli16<3>(S3);
      S3 = __zxor(S3, T);
    }
  }

  __m256_neon S_final = __zxor(__zxor(S0, S1), __zxor(S2, S3));
  __zstore(reinterpret_cast<u8 *>(out), S_final);
}

// rt seed version
void
zzz(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256_neon IV = __zset_epi64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256_neon seedv = __zset1_epi64(seed);

  __m256_neon S0 = __zxor(IV, seedv);
  __m256_neon S1 = __zxor(__zperm_swap(IV), seedv);
  __m256_neon S2 = __zxor(__zslli64<1>(IV), seedv);
  __m256_neon S3 = __zxor(__zsrli64<1>(IV), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256_neon mask64 = __zset1_epi64((i64)0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256_neon block;
    usize remaining = (usize)(end - ptr);
    if ( remaining >= 32 ) {
      block = __zload(ptr);
      ptr += 32;
    } else {
      block = __zload_tail(ptr, remaining);
      ptr += remaining;
    }

    __m256_neon S0_next = __zxor(S0, block);
    __m256_neon S1_next = __zxor(S1, block);
    __m256_neon S2_next = __zxor(S2, block);
    __m256_neon S3_next = __zxor(S3, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256_neon T0 = __zslli64<3>(S0_next);
      S0_next = __zsub64(S0_next, T0);
      T0 = __zsrli64<2>(S0_next);
      S0_next = __zsub64(S0_next, T0);
      T0 = __zslli64<5>(S0_next);
      S0_next = __zadd64(S0_next, T0);
      T0 = __zsrli64<4>(S0_next);
      S0_next = __zsub64(S0_next, T0);
      S0_next = __zand(S0_next, mask64);

      __m256_neon T1 = __zslli64<3>(S1_next);
      S1_next = __zsub64(S1_next, T1);
      T1 = __zsrli64<2>(S1_next);
      S1_next = __zsub64(S1_next, T1);
      T1 = __zslli64<5>(S1_next);
      S1_next = __zadd64(S1_next, T1);
      T1 = __zsrli64<4>(S1_next);
      S1_next = __zsub64(S1_next, T1);
      S1_next = __zand(S1_next, mask64);

      __m256_neon T2 = __zslli64<2>(S2_next);
      S2_next = __zsub64(S2_next, T2);
      T2 = __zsrli64<3>(S2_next);
      S2_next = __zsub64(S2_next, T2);
      T2 = __zslli64<4>(S2_next);
      S2_next = __zadd64(S2_next, T2);
      T2 = __zsrli64<5>(S2_next);
      S2_next = __zsub64(S2_next, T2);
      S2_next = __zand(S2_next, mask64);

      __m256_neon T3 = __zslli64<3>(S3_next);
      S3_next = __zsub64(S3_next, T3);
      T3 = __zsrli64<1>(S3_next);
      S3_next = __zsub64(S3_next, T3);
      T3 = __zslli64<6>(S3_next);
      S3_next = __zadd64(S3_next, T3);
      T3 = __zsrli64<3>(S3_next);
      S3_next = __zsub64(S3_next, T3);
      S3_next = __zand(S3_next, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
    S2 = S2_next;
    S3 = S3_next;
  }

  __m256_neon S = __zxor(__zxor(S0, S1), __zxor(S2, S3));
  __zstore(reinterpret_cast<u8 *>(out), S);
}

// hard seed template
template <i64 Seed>
void
zzz(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256_neon IV = __zset_epi64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256_neon seedv = __zset1_epi64(Seed);

  __m256_neon S0 = __zxor(IV, seedv);
  __m256_neon S1 = __zxor(__zperm_swap(IV), seedv);
  __m256_neon S2 = __zxor(__zslli64<1>(IV), seedv);
  __m256_neon S3 = __zxor(__zsrli64<1>(IV), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256_neon mask64 = __zset1_epi64((i64)0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256_neon block;
    usize remaining = (usize)(end - ptr);
    if ( remaining >= 32 ) {
      block = __zload(ptr);
      ptr += 32;
    } else {
      block = __zload_tail(ptr, remaining);
      ptr += remaining;
    }

    __m256_neon S0_next = __zxor(S0, block);
    __m256_neon S1_next = __zxor(S1, block);
    __m256_neon S2_next = __zxor(S2, block);
    __m256_neon S3_next = __zxor(S3, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256_neon T0 = __zslli64<3>(S0_next);
      S0_next = __zsub64(S0_next, T0);
      T0 = __zsrli64<2>(S0_next);
      S0_next = __zsub64(S0_next, T0);
      T0 = __zslli64<5>(S0_next);
      S0_next = __zadd64(S0_next, T0);
      T0 = __zsrli64<4>(S0_next);
      S0_next = __zsub64(S0_next, T0);
      S0_next = __zand(S0_next, mask64);

      __m256_neon T1 = __zslli64<3>(S1_next);
      S1_next = __zsub64(S1_next, T1);
      T1 = __zsrli64<2>(S1_next);
      S1_next = __zsub64(S1_next, T1);
      T1 = __zslli64<5>(S1_next);
      S1_next = __zadd64(S1_next, T1);
      T1 = __zsrli64<4>(S1_next);
      S1_next = __zsub64(S1_next, T1);
      S1_next = __zand(S1_next, mask64);

      __m256_neon T2 = __zslli64<2>(S2_next);
      S2_next = __zsub64(S2_next, T2);
      T2 = __zsrli64<3>(S2_next);
      S2_next = __zsub64(S2_next, T2);
      T2 = __zslli64<4>(S2_next);
      S2_next = __zadd64(S2_next, T2);
      T2 = __zsrli64<5>(S2_next);
      S2_next = __zsub64(S2_next, T2);
      S2_next = __zand(S2_next, mask64);

      __m256_neon T3 = __zslli64<3>(S3_next);
      S3_next = __zsub64(S3_next, T3);
      T3 = __zsrli64<1>(S3_next);
      S3_next = __zsub64(S3_next, T3);
      T3 = __zslli64<6>(S3_next);
      S3_next = __zadd64(S3_next, T3);
      T3 = __zsrli64<3>(S3_next);
      S3_next = __zsub64(S3_next, T3);
      S3_next = __zand(S3_next, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
    S2 = S2_next;
    S3 = S3_next;
  }

  __m256_neon S = __zxor(__zxor(S0, S1), __zxor(S2, S3));
  __zstore(reinterpret_cast<u8 *>(out), S);
}

u64
z64(const u8 *data, i64 seed, usize sz)
{
  alignas(32) u64 out[4];
  z(data, seed, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

template <i64 Seed>
u64
z64(const u8 *data, usize sz)
{
  alignas(32) u64 out[4];
  z<Seed>(data, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

u64
zz64(const u8 *data, i64 seed, usize sz)
{
  alignas(32) u64 out[4];
  zz(data, seed, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

template <i64 Seed>
u64
zz64(const u8 *data, usize sz)
{
  alignas(32) u64 out[4];
  zz<Seed>(data, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

// 64-bit helper for the rt seed version
u64
zzz64(const u8 *data, i64 seed, usize sz)
{
  alignas(32) u64 out[4];
  zzz(data, seed, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

// 64-bit helper, zzz by default produces a 256-bit hash
template <i64 Seed>
u64
zzz64(const u8 *data, usize sz)
{
  alignas(32) u64 out[4];
  zzz<Seed>(data, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

// 128-bit helper for the rt seed version
micron::pair<u64, u64>
zzz128(const u8 *data, i64 seed, usize sz)
{
  alignas(32) u64 out[4];
  zzz(data, seed, sz, out);
  return { out[0] ^ out[1], out[2] ^ out[3] };
}

// 128-bit helper, zzz by default produces a 256-bit hash
template <i64 Seed>
micron::pair<u64, u64>
zzz128(const u8 *data, usize sz)
{
  alignas(32) u64 out[4];
  zzz<Seed>(data, sz, out);
  return { out[0] ^ out[1], out[2] ^ out[3] };
}

};     // namespace hashes
};     // namespace micron
