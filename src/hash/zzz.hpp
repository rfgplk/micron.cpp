//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#if defined(__micron_arch_arm_any)
#include "zzz_arm.hpp"
#else

#include "../simd/aliases.hpp"
#include "../simd/simd.hpp"
#include "../types.hpp"

#include "../tuple.hpp"

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

// hard seed
template<i64 Seed>
void
z(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = simd::avx::splat_i64(Seed);

  __m256i S0 = simd::avx2::xor_i256(IV, seedv);
  __m256i S1 = simd::avx2::xor_i256(simd::avx2::permute2x128_i256<0x01>(IV, IV), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = simd::avx::splat_i64(0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) [[unlikely]] {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];      // fill with static zero
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    __m256i S0_next = simd::avx2::xor_i256(S0, block);
    __m256i S1_next = simd::avx2::xor_i256(S1, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T0 = simd::avx2::shl_i64(S0_next, 3);
      __m256i A0 = simd::avx2::sub_i64(S0_next, T0);
      S0_next = simd::avx2::and_i256(A0, mask64);

      __m256i T1 = simd::avx2::shl_i64(S1_next, 2);
      __m256i A1 = simd::avx2::sub_i64(S1_next, T1);
      S1_next = simd::avx2::and_i256(A1, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
  }

  __m256i S = simd::avx2::xor_i256(S0, S1);
  simd::avx::store_i256(reinterpret_cast<__m256i *>(out), S);
}

void
z(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = simd::avx::splat_i64(seed);

  __m256i S0 = simd::avx2::xor_i256(IV, seedv);
  __m256i S1 = simd::avx2::xor_i256(simd::avx2::permute2x128_i256<0x01>(IV, IV), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = simd::avx::splat_i64(0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) [[unlikely]] {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];      // fill with static zero
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    __m256i S0_next = simd::avx2::xor_i256(S0, block);
    __m256i S1_next = simd::avx2::xor_i256(S1, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T0 = simd::avx2::shl_i64(S0_next, 3);
      __m256i A0 = simd::avx2::sub_i64(S0_next, T0);
      S0_next = simd::avx2::and_i256(A0, mask64);

      __m256i T1 = simd::avx2::shl_i64(S1_next, 2);
      __m256i A1 = simd::avx2::sub_i64(S1_next, T1);
      S1_next = simd::avx2::and_i256(A1, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
  }

  __m256i S = simd::avx2::xor_i256(S0, S1);
  simd::avx::store_i256(reinterpret_cast<__m256i *>(out), S);
}

// hard seed version
template<i64 Seed>
void
zz(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256i shift_vec = simd::avx::set_i64(13, 29, 7, 19);
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);

  __m256i S0 = IV;
  __m256i S1 = simd::avx2::permute2x128_i256<0x01>(IV, IV);
  __m256i S2 = simd::avx2::xor_i256(IV, simd::avx2::shl_i64(IV, 1));
  __m256i S3 = simd::avx2::add_i64(IV, simd::avx2::shl_i64(IV, 2));

  const __m256i seedv = simd::avx::splat_i64(Seed);
  S0 = simd::avx2::xor_i256(S0, seedv);
  S1 = simd::avx2::xor_i256(S1, seedv);
  S2 = simd::avx2::xor_i256(S2, seedv);
  S3 = simd::avx2::xor_i256(S3, seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    S0 = simd::avx2::xor_i256(S0, block);
    S1 = simd::avx2::add_i64(S1, block);
    S2 = simd::avx2::xor_i256(S2, simd::avx2::shl_i64(block, 1));
    S3 = simd::avx2::add_i64(S3, simd::avx2::shr_i64(block, 1));

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T, U;

      // s0
      T = simd::avx2::shl_per_i64(S0, shift_vec);
      S0 = simd::avx2::xor_i256(S0, T);
      U = simd::avx2::permute2x128_i256<0x01>(S0, S0);
      S0 = simd::avx2::sub_sat_i8(S0, U);
      T = simd::avx2::shl_i16(S0, 3);
      S0 = simd::avx2::xor_i256(S0, T);

      // s1
      T = simd::avx2::shl_per_i64(S1, shift_vec);
      S1 = simd::avx2::xor_i256(S1, T);
      U = simd::avx2::permute2x128_i256<0x01>(S1, S1);
      S1 = simd::avx2::sub_sat_i8(S1, U);
      T = simd::avx2::shl_i16(S1, 3);
      S1 = simd::avx2::xor_i256(S1, T);

      // s2
      T = simd::avx2::shl_per_i64(S2, shift_vec);
      S2 = simd::avx2::xor_i256(S2, T);
      U = simd::avx2::permute2x128_i256<0x01>(S2, S2);
      S2 = simd::avx2::sub_sat_i8(S2, U);
      T = simd::avx2::shl_i16(S2, 3);
      S2 = simd::avx2::xor_i256(S2, T);

      // s3
      T = simd::avx2::shl_per_i64(S3, shift_vec);
      S3 = simd::avx2::xor_i256(S3, T);
      U = simd::avx2::permute2x128_i256<0x01>(S3, S3);
      S3 = simd::avx2::sub_sat_i8(S3, U);
      T = simd::avx2::shl_i16(S3, 3);
      S3 = simd::avx2::xor_i256(S3, T);
    }
  }

  __m256i S_final = simd::avx2::xor_i256(simd::avx2::xor_i256(S0, S1), simd::avx2::xor_i256(S2, S3));
  simd::avx::store_i256(reinterpret_cast<__m256i *>(out), S_final);
}

// rt seed version
void
zz(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256i shift_vec = simd::avx::set_i64(13, 29, 7, 19);
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);

  __m256i S0 = IV;
  __m256i S1 = simd::avx2::permute2x128_i256<0x01>(IV, IV);
  __m256i S2 = simd::avx2::xor_i256(IV, simd::avx2::shl_i64(IV, 1));
  __m256i S3 = simd::avx2::add_i64(IV, simd::avx2::shl_i64(IV, 2));

  const __m256i seedv = simd::avx::splat_i64(seed);
  S0 = simd::avx2::xor_i256(S0, seedv);
  S1 = simd::avx2::xor_i256(S1, seedv);
  S2 = simd::avx2::xor_i256(S2, seedv);
  S3 = simd::avx2::xor_i256(S3, seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    S0 = simd::avx2::xor_i256(S0, block);
    S1 = simd::avx2::add_i64(S1, block);
    S2 = simd::avx2::xor_i256(S2, simd::avx2::shl_i64(block, 1));
    S3 = simd::avx2::add_i64(S3, simd::avx2::shr_i64(block, 1));

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T, U;

      T = simd::avx2::shl_per_i64(S0, shift_vec);
      S0 = simd::avx2::xor_i256(S0, T);
      U = simd::avx2::permute2x128_i256<0x01>(S0, S0);
      S0 = simd::avx2::sub_sat_i8(S0, U);
      T = simd::avx2::shl_i16(S0, 3);
      S0 = simd::avx2::xor_i256(S0, T);

      T = simd::avx2::shl_per_i64(S1, shift_vec);
      S1 = simd::avx2::xor_i256(S1, T);
      U = simd::avx2::permute2x128_i256<0x01>(S1, S1);
      S1 = simd::avx2::sub_sat_i8(S1, U);
      T = simd::avx2::shl_i16(S1, 3);
      S1 = simd::avx2::xor_i256(S1, T);

      T = simd::avx2::shl_per_i64(S2, shift_vec);
      S2 = simd::avx2::xor_i256(S2, T);
      U = simd::avx2::permute2x128_i256<0x01>(S2, S2);
      S2 = simd::avx2::sub_sat_i8(S2, U);
      T = simd::avx2::shl_i16(S2, 3);
      S2 = simd::avx2::xor_i256(S2, T);

      T = simd::avx2::shl_per_i64(S3, shift_vec);
      S3 = simd::avx2::xor_i256(S3, T);
      U = simd::avx2::permute2x128_i256<0x01>(S3, S3);
      S3 = simd::avx2::sub_sat_i8(S3, U);
      T = simd::avx2::shl_i16(S3, 3);
      S3 = simd::avx2::xor_i256(S3, T);
    }
  }

  __m256i S_final = simd::avx2::xor_i256(simd::avx2::xor_i256(S0, S1), simd::avx2::xor_i256(S2, S3));
  simd::avx::store_i256(reinterpret_cast<__m256i *>(out), S_final);
}

// rt seed version
void
zzz(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = simd::avx::splat_i64(seed);

  __m256i S0 = simd::avx2::xor_i256(IV, seedv);
  __m256i S1 = simd::avx2::xor_i256(simd::avx2::permute2x128_i256<0x01>(IV, IV), seedv);
  __m256i S2 = simd::avx2::xor_i256(simd::avx2::shl_i64(IV, 1), seedv);
  __m256i S3 = simd::avx2::xor_i256(simd::avx2::shr_i64(IV, 1), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = simd::avx::splat_i64(0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    __m256i S0_next = simd::avx2::xor_i256(S0, block);
    __m256i S1_next = simd::avx2::xor_i256(S1, block);
    __m256i S2_next = simd::avx2::xor_i256(S2, block);
    __m256i S3_next = simd::avx2::xor_i256(S3, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T0 = simd::avx2::shl_i64(S0_next, 3);
      S0_next = simd::avx2::sub_i64(S0_next, T0);
      T0 = simd::avx2::shr_i64(S0_next, 2);
      S0_next = simd::avx2::sub_i64(S0_next, T0);
      T0 = simd::avx2::shl_i64(S0_next, 5);
      S0_next = simd::avx2::add_i64(S0_next, T0);
      T0 = simd::avx2::shr_i64(S0_next, 4);
      S0_next = simd::avx2::sub_i64(S0_next, T0);
      S0_next = simd::avx2::and_i256(S0_next, mask64);

      __m256i T1 = simd::avx2::shl_i64(S1_next, 3);
      S1_next = simd::avx2::sub_i64(S1_next, T1);
      T1 = simd::avx2::shr_i64(S1_next, 2);
      S1_next = simd::avx2::sub_i64(S1_next, T1);
      T1 = simd::avx2::shl_i64(S1_next, 5);
      S1_next = simd::avx2::add_i64(S1_next, T1);
      T1 = simd::avx2::shr_i64(S1_next, 4);
      S1_next = simd::avx2::sub_i64(S1_next, T1);
      S1_next = simd::avx2::and_i256(S1_next, mask64);

      __m256i T2 = simd::avx2::shl_i64(S2_next, 2);
      S2_next = simd::avx2::sub_i64(S2_next, T2);
      T2 = simd::avx2::shr_i64(S2_next, 3);
      S2_next = simd::avx2::sub_i64(S2_next, T2);
      T2 = simd::avx2::shl_i64(S2_next, 4);
      S2_next = simd::avx2::add_i64(S2_next, T2);
      T2 = simd::avx2::shr_i64(S2_next, 5);
      S2_next = simd::avx2::sub_i64(S2_next, T2);
      S2_next = simd::avx2::and_i256(S2_next, mask64);

      __m256i T3 = simd::avx2::shl_i64(S3_next, 3);
      S3_next = simd::avx2::sub_i64(S3_next, T3);
      T3 = simd::avx2::shr_i64(S3_next, 1);
      S3_next = simd::avx2::sub_i64(S3_next, T3);
      T3 = simd::avx2::shl_i64(S3_next, 6);
      S3_next = simd::avx2::add_i64(S3_next, T3);
      T3 = simd::avx2::shr_i64(S3_next, 3);
      S3_next = simd::avx2::sub_i64(S3_next, T3);
      S3_next = simd::avx2::and_i256(S3_next, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
    S2 = S2_next;
    S3 = S3_next;
  }

  __m256i S = simd::avx2::xor_i256(simd::avx2::xor_i256(S0, S1), simd::avx2::xor_i256(S2, S3));
  simd::avx::store_i256(reinterpret_cast<__m256i *>(out), S);
}

// hard seed template
template<i64 Seed>
void
zzz(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = simd::avx::splat_i64(Seed);

  __m256i S0 = simd::avx2::xor_i256(IV, seedv);
  __m256i S1 = simd::avx2::xor_i256(simd::avx2::permute2x128_i256<0x01>(IV, IV), seedv);
  __m256i S2 = simd::avx2::xor_i256(simd::avx2::shl_i64(IV, 1), seedv);
  __m256i S3 = simd::avx2::xor_i256(simd::avx2::shr_i64(IV, 1), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = simd::avx::splat_i64(0xFFFFFFFFFFFFULL);

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    __m256i S0_next = simd::avx2::xor_i256(S0, block);
    __m256i S1_next = simd::avx2::xor_i256(S1, block);
    __m256i S2_next = simd::avx2::xor_i256(S2, block);
    __m256i S3_next = simd::avx2::xor_i256(S3, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T0 = simd::avx2::shl_i64(S0_next, 3);
      S0_next = simd::avx2::sub_i64(S0_next, T0);
      T0 = simd::avx2::shr_i64(S0_next, 2);
      S0_next = simd::avx2::sub_i64(S0_next, T0);
      T0 = simd::avx2::shl_i64(S0_next, 5);
      S0_next = simd::avx2::add_i64(S0_next, T0);
      T0 = simd::avx2::shr_i64(S0_next, 4);
      S0_next = simd::avx2::sub_i64(S0_next, T0);
      S0_next = simd::avx2::and_i256(S0_next, mask64);

      __m256i T1 = simd::avx2::shl_i64(S1_next, 3);
      S1_next = simd::avx2::sub_i64(S1_next, T1);
      T1 = simd::avx2::shr_i64(S1_next, 2);
      S1_next = simd::avx2::sub_i64(S1_next, T1);
      T1 = simd::avx2::shl_i64(S1_next, 5);
      S1_next = simd::avx2::add_i64(S1_next, T1);
      T1 = simd::avx2::shr_i64(S1_next, 4);
      S1_next = simd::avx2::sub_i64(S1_next, T1);
      S1_next = simd::avx2::and_i256(S1_next, mask64);

      __m256i T2 = simd::avx2::shl_i64(S2_next, 2);
      S2_next = simd::avx2::sub_i64(S2_next, T2);
      T2 = simd::avx2::shr_i64(S2_next, 3);
      S2_next = simd::avx2::sub_i64(S2_next, T2);
      T2 = simd::avx2::shl_i64(S2_next, 4);
      S2_next = simd::avx2::add_i64(S2_next, T2);
      T2 = simd::avx2::shr_i64(S2_next, 5);
      S2_next = simd::avx2::sub_i64(S2_next, T2);
      S2_next = simd::avx2::and_i256(S2_next, mask64);

      __m256i T3 = simd::avx2::shl_i64(S3_next, 3);
      S3_next = simd::avx2::sub_i64(S3_next, T3);
      T3 = simd::avx2::shr_i64(S3_next, 1);
      S3_next = simd::avx2::sub_i64(S3_next, T3);
      T3 = simd::avx2::shl_i64(S3_next, 6);
      S3_next = simd::avx2::add_i64(S3_next, T3);
      T3 = simd::avx2::shr_i64(S3_next, 3);
      S3_next = simd::avx2::sub_i64(S3_next, T3);
      S3_next = simd::avx2::and_i256(S3_next, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
    S2 = S2_next;
    S3 = S3_next;
  }

  __m256i S = simd::avx2::xor_i256(simd::avx2::xor_i256(S0, S1), simd::avx2::xor_i256(S2, S3));
  simd::avx::store_i256(reinterpret_cast<__m256i *>(out), S);
}

u64
z64(const u8 *data, i64 seed, usize sz)
{
  alignas(32) u64 out[4];
  z(data, seed, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

template<i64 Seed>
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

template<i64 Seed>
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
template<i64 Seed>
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
template<i64 Seed>
micron::pair<u64, u64>
zzz128(const u8 *data, usize sz)
{
  alignas(32) u64 out[4];
  zzz<Seed>(data, sz, out);
  return { out[0] ^ out[1], out[2] ^ out[3] };
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// zzzf
//
// true full diffusion hash
__attribute__((always_inline)) static inline __m256i
__zzzf_brev(__m256i v)
{
  const __m256i m
      = simd::avx::setr_i8(7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
  return simd::avx2::shuffle_v_i8_256(v, m);
}

template<int A, int B, int C, int D>
__attribute__((always_inline)) static inline __m256i
__zzzf_mix(__m256i s)
{
  __m256i t = simd::avx2::shl_i64(s, A);
  s = simd::avx2::sub_i64(s, t);
  t = simd::avx2::shr_i64(s, B);
  s = simd::avx2::xor_i256(s, t);
  s = __zzzf_brev(s);
  t = simd::avx2::shl_i64(s, C);
  s = simd::avx2::add_i64(s, t);
  t = simd::avx2::shr_i64(s, D);
  s = simd::avx2::xor_i256(s, t);
  return s;
}

// rt seed version
inline void
zzzf(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = simd::avx::splat_i64(seed);

  __m256i S0 = simd::avx2::xor_i256(IV, seedv);
  __m256i S1 = simd::avx2::xor_i256(simd::avx2::permute2x128_i256<0x01>(IV, IV), seedv);
  __m256i S2 = simd::avx2::xor_i256(simd::avx2::shl_i64(IV, 1), seedv);
  __m256i S3 = simd::avx2::xor_i256(simd::avx2::shr_i64(IV, 1), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      alignas(32) u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];
      block = simd::avx::load_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    S0 = __zzzf_mix<3, 31, 5, 29>(simd::avx2::xor_i256(S0, block));
    S1 = __zzzf_mix<5, 29, 7, 31>(simd::avx2::xor_i256(S1, block));
    S2 = __zzzf_mix<7, 27, 3, 33>(simd::avx2::xor_i256(S2, block));
    S3 = __zzzf_mix<9, 33, 5, 27>(simd::avx2::xor_i256(S3, block));
  }

  __m256i F = simd::avx2::xor_i256(simd::avx2::xor_i256(S0, simd::avx2::permute4x64_i64<0x39>(S1)),
                                   simd::avx2::xor_i256(simd::avx2::permute4x64_i64<0x4E>(S2), simd::avx2::permute4x64_i64<0x93>(S3)));
  F = simd::avx2::xor_i256(F, seedv);                              // sole seed carrier for sz == 0
  F = simd::avx2::xor_i256(F, simd::avx::splat_i64((i64)sz));      // zero-padded tails of different lengths stay distinct
  F = __zzzf_mix<3, 29, 5, 31>(F);
  F = __zzzf_mix<7, 31, 9, 27>(F);
  simd::avx::storeu_i256(reinterpret_cast<__m256i_u *>(out), F);
}

// hard seed template
template<i64 Seed>
void
zzzf(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256i IV = simd::avx::set_i64(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = simd::avx::splat_i64(Seed);

  __m256i S0 = simd::avx2::xor_i256(IV, seedv);
  __m256i S1 = simd::avx2::xor_i256(simd::avx2::permute2x128_i256<0x01>(IV, IV), seedv);
  __m256i S2 = simd::avx2::xor_i256(simd::avx2::shl_i64(IV, 1), seedv);
  __m256i S3 = simd::avx2::xor_i256(simd::avx2::shr_i64(IV, 1), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr < end ) {
    __m256i block;
    usize remaining = end - ptr;
    if ( remaining >= 32 ) {
      block = simd::avx2::loadu_i256(reinterpret_cast<const __m256i *>(ptr));
      ptr += 32;
    } else {
      alignas(32) u8 tmp[32];
      for ( usize i = 0; i < remaining; ++i ) tmp[i] = ptr[i];
      for ( usize i = remaining; i < 32; ++i ) tmp[i] = __zero_block[i];
      block = simd::avx::load_i256(reinterpret_cast<const __m256i *>(tmp));
      ptr += remaining;
    }

    S0 = __zzzf_mix<3, 31, 5, 29>(simd::avx2::xor_i256(S0, block));
    S1 = __zzzf_mix<5, 29, 7, 31>(simd::avx2::xor_i256(S1, block));
    S2 = __zzzf_mix<7, 27, 3, 33>(simd::avx2::xor_i256(S2, block));
    S3 = __zzzf_mix<9, 33, 5, 27>(simd::avx2::xor_i256(S3, block));
  }

  __m256i F = simd::avx2::xor_i256(simd::avx2::xor_i256(S0, simd::avx2::permute4x64_i64<0x39>(S1)),
                                   simd::avx2::xor_i256(simd::avx2::permute4x64_i64<0x4E>(S2), simd::avx2::permute4x64_i64<0x93>(S3)));
  F = simd::avx2::xor_i256(F, seedv);
  F = simd::avx2::xor_i256(F, simd::avx::splat_i64((i64)sz));
  F = __zzzf_mix<3, 29, 5, 31>(F);
  F = __zzzf_mix<7, 31, 9, 27>(F);
  simd::avx::storeu_i256(reinterpret_cast<__m256i_u *>(out), F);
}

// 64-bit helper for the rt seed version
inline u64
zzzf64(const u8 *data, i64 seed, usize sz)
{
  alignas(32) u64 out[4];
  zzzf(data, seed, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

template<i64 Seed>
u64
zzzf64(const u8 *data, usize sz)
{
  alignas(32) u64 out[4];
  zzzf<Seed>(data, sz, out);
  return out[0] ^ out[1] ^ out[2] ^ out[3];
}

inline micron::pair<u64, u64>
zzzf128(const u8 *data, i64 seed, usize sz)
{
  alignas(32) u64 out[4];
  zzzf(data, seed, sz, out);
  return { out[0] ^ out[1], out[2] ^ out[3] };
}

template<i64 Seed>
micron::pair<u64, u64>
zzzf128(const u8 *data, usize sz)
{
  alignas(32) u64 out[4];
  zzzf<Seed>(data, sz, out);
  return { out[0] ^ out[1], out[2] ^ out[3] };
}

};      // namespace hashes
};      // namespace micron

#endif      // !__micron_arch_arm_any
