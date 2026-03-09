//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../simd/simd.hpp"
#include "../types.hpp"

namespace micron
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

void
z(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256i IV = _mm256_set_epi64x(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = _mm256_set1_epi64x(seed);

  __m256i S0 = _mm256_xor_si256(IV, seedv);
  __m256i S1 = _mm256_xor_si256(_mm256_permute2x128_si256(IV, IV, 0x01), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = _mm256_set1_epi64x(0xFFFFFFFFFFFFULL);     // VPAND mask

  while ( ptr + 32 <= end ) {
    __m256i block = _mm256_load_si256(reinterpret_cast<const __m256i *>(ptr));
    ptr += 32;

    // Parallel S0/S1 XOR with input
    __m256i S0_next = _mm256_xor_si256(S0, block);
    __m256i S1_next = _mm256_xor_si256(S1, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      // --- S0 pipeline ---
      __m256i T0 = _mm256_slli_epi64(S0_next, 3);     // shift left
      __m256i A0 = _mm256_sub_epi64(S0_next, T0);     // subtract
      __m256i B0 = _mm256_and_si256(A0, mask64);      // truncate
      S0_next = B0;

      // --- S1 pipeline (parallel independent) ---
      __m256i T1 = _mm256_slli_epi64(S1_next, 2);
      __m256i A1 = _mm256_sub_epi64(S1_next, T1);
      __m256i B1 = _mm256_and_si256(A1, mask64);
      S1_next = B1;
    }

    S0 = S0_next;
    S1 = S1_next;
  }

  __m256i S = _mm256_xor_si256(S0, S1);
  _mm256_store_si256(reinterpret_cast<__m256i *>(out), S);
}

template <i64 Seed>
void
z(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256i IV = _mm256_set_epi64x(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = _mm256_set1_epi64x(Seed);

  __m256i S0 = _mm256_xor_si256(IV, seedv);
  __m256i S1 = _mm256_xor_si256(_mm256_permute2x128_si256(IV, IV, 0x01), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = _mm256_set1_epi64x(0xFFFFFFFFFFFFULL);     // VPAND mask

  while ( ptr + 32 <= end ) {
    __m256i block = _mm256_load_si256(reinterpret_cast<const __m256i *>(ptr));
    ptr += 32;

    __m256i S0_next = _mm256_xor_si256(S0, block);
    __m256i S1_next = _mm256_xor_si256(S1, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T0 = _mm256_slli_epi64(S0_next, 3);
      __m256i A0 = _mm256_sub_epi64(S0_next, T0);
      __m256i B0 = _mm256_and_si256(A0, mask64);
      S0_next = B0;

      __m256i T1 = _mm256_slli_epi64(S1_next, 2);
      __m256i A1 = _mm256_sub_epi64(S1_next, T1);
      __m256i B1 = _mm256_and_si256(A1, mask64);
      S1_next = B1;
    }

    S0 = S0_next;
    S1 = S1_next;
  }

  __m256i S = _mm256_xor_si256(S0, S1);
  _mm256_store_si256(reinterpret_cast<__m256i *>(out), S);
}

// hard seed version
template <i64 Seed>
void
zz(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256i shift_vec = _mm256_set_epi64x(13, 29, 7, 19);
  const __m256i IV = _mm256_set_epi64x(iv64_3, iv64_2, iv64_1, iv64_0);

  __m256i S0 = IV;
  __m256i S1 = _mm256_permute2x128_si256(IV, IV, 0x01);
  __m256i S2 = _mm256_xor_si256(IV, _mm256_slli_epi64(IV, 1));
  __m256i S3 = _mm256_add_epi64(IV, _mm256_slli_epi64(IV, 2));

  const __m256i seedv = _mm256_set1_epi64x(Seed);
  S0 = _mm256_xor_si256(S0, seedv);
  S1 = _mm256_xor_si256(S1, seedv);
  S2 = _mm256_xor_si256(S2, seedv);
  S3 = _mm256_xor_si256(S3, seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr + 32 <= end ) {
    // accumulator setup
    __m256i block = _mm256_load_si256(reinterpret_cast<const __m256i *>(ptr));
    S0 = _mm256_xor_si256(S0, block);
    S1 = _mm256_add_epi64(S1, block);
    S2 = _mm256_xor_si256(S2, _mm256_slli_epi64(block, 1));
    S3 = _mm256_add_epi64(S3, _mm256_srli_epi64(block, 1));
    ptr += 32;

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T, U;

      // s0

      // VPSLLVQ
      // p0 .. p23 <= haswell-ep
      // p01 .. p23A <= rl/al/rl?
      // FP23 <= zen4
      T = _mm256_sllv_epi64(S0, shift_vec);
      // VPXOR (YMM)
      // p015 <= haswell-ep/rl/al/rl?
      // fp0123 <= zen4
      S0 = _mm256_xor_si256(S0, T);     // mixer
      // VPERM2I128
      // p5(!!!!) <= haswell-ep/rl/al
      // fp12 <= zen4
      U = _mm256_permute2x128_si256(S0, S0, 0x01);     // permutation/shuffle
      // VPSUBSB
      // p15 <= haswell-ep
      // p01 rl/al
      // fp01 <= zen4
      S0 = _mm256_subs_epi8(S0, U);     // diffusion
      // PSLLW
      // p0 .. p5 <= haswell-ep
      // p01 .. p5 <= rl/al
      // fp01 <= zen4
      T = _mm256_slli_epi16(S0, 3);
      // VPXOR
      // p015 <= haswell-ep/rl/al/rl?
      // fp0123 <= zen4
      S0 = _mm256_xor_si256(S0, T);

      // s1
      T = _mm256_sllv_epi64(S1, shift_vec);
      S1 = _mm256_xor_si256(S1, T);
      U = _mm256_permute2x128_si256(S1, S1, 0x01);
      S1 = _mm256_subs_epi8(S1, U);
      T = _mm256_slli_epi16(S1, 3);
      S1 = _mm256_xor_si256(S1, T);

      // s2
      T = _mm256_sllv_epi64(S2, shift_vec);
      S2 = _mm256_xor_si256(S2, T);
      U = _mm256_permute2x128_si256(S2, S2, 0x01);
      S2 = _mm256_subs_epi8(S2, U);
      T = _mm256_slli_epi16(S2, 3);
      S2 = _mm256_xor_si256(S2, T);

      // s3
      T = _mm256_sllv_epi64(S3, shift_vec);
      S3 = _mm256_xor_si256(S3, T);
      U = _mm256_permute2x128_si256(S3, S3, 0x01);
      S3 = _mm256_subs_epi8(S3, U);
      T = _mm256_slli_epi16(S3, 3);
      S3 = _mm256_xor_si256(S3, T);
    }
  }

  // NOTE: must be 256b aligned
  __m256i S_final = _mm256_xor_si256(_mm256_xor_si256(S0, S1), _mm256_xor_si256(S2, S3));
  _mm256_store_si256(reinterpret_cast<__m256i *>(out), S_final);
}

// rt seed version
void
zz(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256i shift_vec = _mm256_set_epi64x(13, 29, 7, 19);
  const __m256i IV = _mm256_set_epi64x(iv64_3, iv64_2, iv64_1, iv64_0);

  __m256i S0 = IV;
  __m256i S1 = _mm256_permute2x128_si256(IV, IV, 0x01);
  __m256i S2 = _mm256_xor_si256(IV, _mm256_slli_epi64(IV, 1));
  __m256i S3 = _mm256_add_epi64(IV, _mm256_slli_epi64(IV, 2));

  const __m256i seedv = _mm256_set1_epi64x(seed);
  S0 = _mm256_xor_si256(S0, seedv);
  S1 = _mm256_xor_si256(S1, seedv);
  S2 = _mm256_xor_si256(S2, seedv);
  S3 = _mm256_xor_si256(S3, seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;

  while ( ptr + 32 <= end ) {
    // accumulator setup
    __m256i block = _mm256_load_si256(reinterpret_cast<const __m256i *>(ptr));
    S0 = _mm256_xor_si256(S0, block);
    S1 = _mm256_add_epi64(S1, block);
    S2 = _mm256_xor_si256(S2, _mm256_slli_epi64(block, 1));
    S3 = _mm256_add_epi64(S3, _mm256_srli_epi64(block, 1));
    ptr += 32;

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T, U;

      // s0

      // VPSLLVQ
      // p0 .. p23 <= haswell-ep
      // p01 .. p23A <= rl/al/rl?
      // FP23 <= zen4
      T = _mm256_sllv_epi64(S0, shift_vec);
      // VPXOR (YMM)
      // p015 <= haswell-ep/rl/al/rl?
      // fp0123 <= zen4
      S0 = _mm256_xor_si256(S0, T);     // mixer
      // VPERM2I128
      // p5(!!!!) <= haswell-ep/rl/al
      // fp12 <= zen4
      U = _mm256_permute2x128_si256(S0, S0, 0x01);     // permutation/shuffle
      // VPSUBSB
      // p15 <= haswell-ep
      // p01 rl/al
      // fp01 <= zen4
      S0 = _mm256_subs_epi8(S0, U);     // diffusion
      // PSLLW
      // p0 .. p5 <= haswell-ep
      // p01 .. p5 <= rl/al
      // fp01 <= zen4
      T = _mm256_slli_epi16(S0, 3);
      // VPXOR
      // p015 <= haswell-ep/rl/al/rl?
      // fp0123 <= zen4
      S0 = _mm256_xor_si256(S0, T);

      // s1
      T = _mm256_sllv_epi64(S1, shift_vec);
      S1 = _mm256_xor_si256(S1, T);
      U = _mm256_permute2x128_si256(S1, S1, 0x01);
      S1 = _mm256_subs_epi8(S1, U);
      T = _mm256_slli_epi16(S1, 3);
      S1 = _mm256_xor_si256(S1, T);

      // s2
      T = _mm256_sllv_epi64(S2, shift_vec);
      S2 = _mm256_xor_si256(S2, T);
      U = _mm256_permute2x128_si256(S2, S2, 0x01);
      S2 = _mm256_subs_epi8(S2, U);
      T = _mm256_slli_epi16(S2, 3);
      S2 = _mm256_xor_si256(S2, T);

      // s3
      T = _mm256_sllv_epi64(S3, shift_vec);
      S3 = _mm256_xor_si256(S3, T);
      U = _mm256_permute2x128_si256(S3, S3, 0x01);
      S3 = _mm256_subs_epi8(S3, U);
      T = _mm256_slli_epi16(S3, 3);
      S3 = _mm256_xor_si256(S3, T);
    }
  }

  // NOTE: must be 256b aligned
  __m256i S_final = _mm256_xor_si256(_mm256_xor_si256(S0, S1), _mm256_xor_si256(S2, S3));
  _mm256_store_si256(reinterpret_cast<__m256i *>(out), S_final);
}

void
zzz(const u8 *__restrict data, i64 seed, usize sz, u64 *__restrict out)
{
  const __m256i IV = _mm256_set_epi64x(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = _mm256_set1_epi64x(seed);

  __m256i S0 = _mm256_xor_si256(IV, seedv);
  __m256i S1 = _mm256_xor_si256(_mm256_permute2x128_si256(IV, IV, 0x01), seedv);
  __m256i S2 = _mm256_xor_si256(_mm256_slli_epi64(IV, 1), seedv);
  __m256i S3 = _mm256_xor_si256(_mm256_srli_epi64(IV, 1), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = _mm256_set1_epi64x(0xFFFFFFFFFFFFULL);     // optional truncation mask

  while ( ptr + 32 <= end ) {
    __m256i block = _mm256_load_si256(reinterpret_cast<const __m256i *>(ptr));
    ptr += 32;

    __m256i S0_next = _mm256_xor_si256(S0, block);
    __m256i S1_next = _mm256_xor_si256(S1, block);
    __m256i S2_next = _mm256_xor_si256(S2, block);
    __m256i S3_next = _mm256_xor_si256(S3, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      // S0
      __m256i T0 = _mm256_slli_epi64(S0_next, 3);
      S0_next = _mm256_sub_epi64(S0_next, T0);
      T0 = _mm256_srli_epi64(S0_next, 2);
      S0_next = _mm256_sub_epi64(S0_next, T0);
      T0 = _mm256_slli_epi64(S0_next, 5);
      S0_next = _mm256_add_epi64(S0_next, T0);
      T0 = _mm256_srli_epi64(S0_next, 4);
      S0_next = _mm256_sub_epi64(S0_next, T0);
      S0_next = _mm256_and_si256(S0_next, mask64);

      // S1
      __m256i T1 = _mm256_slli_epi64(S1_next, 3);
      S1_next = _mm256_sub_epi64(S1_next, T1);
      T1 = _mm256_srli_epi64(S1_next, 2);
      S1_next = _mm256_sub_epi64(S1_next, T1);
      T1 = _mm256_slli_epi64(S1_next, 5);
      S1_next = _mm256_add_epi64(S1_next, T1);
      T1 = _mm256_srli_epi64(S1_next, 4);
      S1_next = _mm256_sub_epi64(S1_next, T1);
      S1_next = _mm256_and_si256(S1_next, mask64);

      // S2
      __m256i T2 = _mm256_slli_epi64(S2_next, 2);
      S2_next = _mm256_sub_epi64(S2_next, T2);
      T2 = _mm256_srli_epi64(S2_next, 3);
      S2_next = _mm256_sub_epi64(S2_next, T2);
      T2 = _mm256_slli_epi64(S2_next, 4);
      S2_next = _mm256_add_epi64(S2_next, T2);
      T2 = _mm256_srli_epi64(S2_next, 5);
      S2_next = _mm256_sub_epi64(S2_next, T2);
      S2_next = _mm256_and_si256(S2_next, mask64);

      // S3
      __m256i T3 = _mm256_slli_epi64(S3_next, 3);
      S3_next = _mm256_sub_epi64(S3_next, T3);
      T3 = _mm256_srli_epi64(S3_next, 1);
      S3_next = _mm256_sub_epi64(S3_next, T3);
      T3 = _mm256_slli_epi64(S3_next, 6);
      S3_next = _mm256_add_epi64(S3_next, T3);
      T3 = _mm256_srli_epi64(S3_next, 3);
      S3_next = _mm256_sub_epi64(S3_next, T3);
      S3_next = _mm256_and_si256(S3_next, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
    S2 = S2_next;
    S3 = S3_next;
  }

  __m256i S = _mm256_xor_si256(_mm256_xor_si256(S0, S1), _mm256_xor_si256(S2, S3));
  _mm256_store_si256(reinterpret_cast<__m256i *>(out), S);
}

template <i64 Seed>
void
zzz(const u8 *__restrict data, usize sz, u64 *__restrict out)
{
  const __m256i IV = _mm256_set_epi64x(iv64_3, iv64_2, iv64_1, iv64_0);
  const __m256i seedv = _mm256_set1_epi64x(Seed);

  __m256i S0 = _mm256_xor_si256(IV, seedv);
  __m256i S1 = _mm256_xor_si256(_mm256_permute2x128_si256(IV, IV, 0x01), seedv);
  __m256i S2 = _mm256_xor_si256(_mm256_slli_epi64(IV, 1), seedv);
  __m256i S3 = _mm256_xor_si256(_mm256_srli_epi64(IV, 1), seedv);

  const u8 *ptr = data;
  const u8 *end = data + sz;
  const __m256i mask64 = _mm256_set1_epi64x(0xFFFFFFFFFFFFULL);     // optional truncation mask

  while ( ptr + 32 <= end ) {
    __m256i block = _mm256_load_si256(reinterpret_cast<const __m256i *>(ptr));
    ptr += 32;

    __m256i S0_next = _mm256_xor_si256(S0, block);
    __m256i S1_next = _mm256_xor_si256(S1, block);
    __m256i S2_next = _mm256_xor_si256(S2, block);
    __m256i S3_next = _mm256_xor_si256(S3, block);

    for ( u64 r = 0; r < __zzzrounds; ++r ) {
      __m256i T0 = _mm256_slli_epi64(S0_next, 3);
      S0_next = _mm256_sub_epi64(S0_next, T0);
      T0 = _mm256_srli_epi64(S0_next, 2);
      S0_next = _mm256_sub_epi64(S0_next, T0);
      T0 = _mm256_slli_epi64(S0_next, 5);
      S0_next = _mm256_add_epi64(S0_next, T0);
      T0 = _mm256_srli_epi64(S0_next, 4);
      S0_next = _mm256_sub_epi64(S0_next, T0);
      S0_next = _mm256_and_si256(S0_next, mask64);

      __m256i T1 = _mm256_slli_epi64(S1_next, 3);
      S1_next = _mm256_sub_epi64(S1_next, T1);
      T1 = _mm256_srli_epi64(S1_next, 2);
      S1_next = _mm256_sub_epi64(S1_next, T1);
      T1 = _mm256_slli_epi64(S1_next, 5);
      S1_next = _mm256_add_epi64(S1_next, T1);
      T1 = _mm256_srli_epi64(S1_next, 4);
      S1_next = _mm256_sub_epi64(S1_next, T1);
      S1_next = _mm256_and_si256(S1_next, mask64);

      __m256i T2 = _mm256_slli_epi64(S2_next, 2);
      S2_next = _mm256_sub_epi64(S2_next, T2);
      T2 = _mm256_srli_epi64(S2_next, 3);
      S2_next = _mm256_sub_epi64(S2_next, T2);
      T2 = _mm256_slli_epi64(S2_next, 4);
      S2_next = _mm256_add_epi64(S2_next, T2);
      T2 = _mm256_srli_epi64(S2_next, 5);
      S2_next = _mm256_sub_epi64(S2_next, T2);
      S2_next = _mm256_and_si256(S2_next, mask64);

      __m256i T3 = _mm256_slli_epi64(S3_next, 3);
      S3_next = _mm256_sub_epi64(S3_next, T3);
      T3 = _mm256_srli_epi64(S3_next, 1);
      S3_next = _mm256_sub_epi64(S3_next, T3);
      T3 = _mm256_slli_epi64(S3_next, 6);
      S3_next = _mm256_add_epi64(S3_next, T3);
      T3 = _mm256_srli_epi64(S3_next, 3);
      S3_next = _mm256_sub_epi64(S3_next, T3);
      S3_next = _mm256_and_si256(S3_next, mask64);
    }

    S0 = S0_next;
    S1 = S1_next;
    S2 = S2_next;
    S3 = S3_next;
  }

  __m256i S = _mm256_xor_si256(_mm256_xor_si256(S0, S1), _mm256_xor_si256(S2, S3));
  _mm256_store_si256(reinterpret_cast<__m256i *>(out), S);
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

}     // namespace micron
