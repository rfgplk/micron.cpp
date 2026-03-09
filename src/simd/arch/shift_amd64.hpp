//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "types.hpp"

namespace micron
{
namespace simd
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// byteshifts

template <is_simd_class T>
inline T
byte_shift_left(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_bslli_si128(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_bslli_epi128(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_bslli_epi128(o, imm8);
  }
}

template <is_simd_class T>
inline T
byte_shift_right(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_bsrli_si128(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_bsrli_epi128(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_bsrli_epi128(o, imm8);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shift lefts

template <is_simd_class T>
inline T
slli_si(T &o, int imm8)
{
  return byte_shift_left(o, imm8);
}

template <is_simd_class T>
inline T
srli_si(T &o, int imm8)
{
  return byte_shift_right(o, imm8);
}

template <is_simd_class T>
inline T
sll_16(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sll_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sll_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sll_epi16(o, count);
  }
}

template <is_simd_class T>
inline T
sll_32(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sll_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sll_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sll_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
sll_64(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sll_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sll_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sll_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
sll(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return sll_16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return sll_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return sll_64(o, count);
  }
}

template <is_simd_class T>
inline T
slli_16(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_slli_epi16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_slli_epi16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_slli_epi16(o, imm8);
  }
}

template <is_simd_class T>
inline T
slli_32(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_slli_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_slli_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_slli_epi32(o, imm8);
  }
}

template <is_simd_class T>
inline T
slli_64(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_slli_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_slli_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_slli_epi64(o, imm8);
  }
}

template <is_simd_class T>
inline T
slli(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return slli_16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return slli_32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return slli_64(o, imm8);
  }
}

template <is_simd_class T>
inline T
sllv_16(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sllv_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sllv_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sllv_epi16(o, count);
  }
}

template <is_simd_class T>
inline T
sllv_32(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sllv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sllv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sllv_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
sllv_64(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sllv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sllv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sllv_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
sllv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return sllv_16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return sllv_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return sllv_64(o, count);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shift rights

template <is_simd_class T>
inline T
srl_16(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srl_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srl_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srl_epi16(o, count);
  }
}

template <is_simd_class T>
inline T
srl_32(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srl_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srl_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srl_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
srl_64(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srl_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srl_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srl_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
srl(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return srl_16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return srl_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return srl_64(o, count);
  }
}

template <is_simd_class T>
inline T
srli_16(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srli_epi16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srli_epi16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srli_epi16(o, imm8);
  }
}

template <is_simd_class T>
inline T
srli_32(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srli_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srli_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srli_epi32(o, imm8);
  }
}

template <is_simd_class T>
inline T
srli_64(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srli_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srli_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srli_epi64(o, imm8);
  }
}

template <is_simd_class T>
inline T
srli(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return srli_16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return srli_32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return srli_64(o, imm8);
  }
}

template <is_simd_class T>
inline T
srlv_16(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srlv_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srlv_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srlv_epi16(o, count);
  }
}

template <is_simd_class T>
inline T
srlv_32(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srlv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srlv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srlv_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
srlv_64(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srlv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srlv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srlv_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
srlv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return srlv_16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return srlv_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return srlv_64(o, count);
  }
}

template <is_simd_class T>
inline T
sra_16(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sra_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sra_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sra_epi16(o, count);
  }
}

template <is_simd_class T>
inline T
sra_32(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sra_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sra_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sra_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
sra_64(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sra_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sra_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_sra_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
sra(T &o, i128 count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return sra_16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return sra_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return sra_64(o, count);
  }
}

template <is_simd_class T>
inline T
srai_16(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srai_epi16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srai_epi16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srai_epi16(o, imm8);
  }
}

template <is_simd_class T>
inline T
srai_32(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srai_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srai_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srai_epi32(o, imm8);
  }
}

template <is_simd_class T>
inline T
srai_64(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srai_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srai_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srai_epi64(o, imm8);
  }
}

template <is_simd_class T>
inline T
srai(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return srai_16(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return srai_32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return srai_64(o, imm8);
  }
}

template <is_simd_class T>
inline T
srav_16(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srav_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srav_epi16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srav_epi16(o, count);
  }
}

template <is_simd_class T>
inline T
srav_32(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srav_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srav_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srav_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
srav_64(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srav_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srav_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_srav_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
srav(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return srav_16(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return srav_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return srav_64(o, count);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// rotates

template <is_simd_class T>
inline T
rol_32(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_rol_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_rol_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_rol_epi32(o, imm8);
  }
}

template <is_simd_class T>
inline T
rol_64(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_rol_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_rol_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_rol_epi64(o, imm8);
  }
}

template <is_simd_class T>
inline T
rol(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return rol_32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return rol_64(o, imm8);
  }
}

template <is_simd_class T>
inline T
rolv_32(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_rolv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_rolv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_rolv_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
rolv_64(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_rolv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_rolv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_rolv_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
rolv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return rolv_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return rolv_64(o, count);
  }
}

template <is_simd_class T>
inline T
ror_32(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_ror_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_ror_epi32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_ror_epi32(o, imm8);
  }
}

template <is_simd_class T>
inline T
ror_64(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_ror_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_ror_epi64(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_ror_epi64(o, imm8);
  }
}

template <is_simd_class T>
inline T
ror(T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return ror_32(o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return ror_64(o, imm8);
  }
}

template <is_simd_class T>
inline T
rorv_32(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_rorv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_rorv_epi32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_rorv_epi32(o, count);
  }
}

template <is_simd_class T>
inline T
rorv_64(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_rorv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_rorv_epi64(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_rorv_epi64(o, count);
  }
}

template <is_simd_class T>
inline T
rorv(T &o, T &count)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return rorv_32(o, count);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return rorv_64(o, count);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// double width shleft

template <is_simd_class T>
inline T
shldi_16(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shldi_epi16(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shldi_epi16(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shldi_epi16(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shldi_32(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shldi_epi32(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shldi_epi32(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shldi_epi32(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shldi_64(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shldi_epi64(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shldi_epi64(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shldi_epi64(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shldi(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return shldi_16(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return shldi_32(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return shldi_64(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shldv_16(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shldv_epi16(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shldv_epi16(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shldv_epi16(a, b, c);
  }
}

template <is_simd_class T>
inline T
shldv_32(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shldv_epi32(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shldv_epi32(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shldv_epi32(a, b, c);
  }
}

template <is_simd_class T>
inline T
shldv_64(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shldv_epi64(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shldv_epi64(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shldv_epi64(a, b, c);
  }
}

template <is_simd_class T>
inline T
shldv(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return shldv_16(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return shldv_32(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return shldv_64(a, b, c);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// double width shift right

template <is_simd_class T>
inline T
shrdi_16(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shrdi_epi16(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shrdi_epi16(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shrdi_epi16(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shrdi_32(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shrdi_epi32(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shrdi_epi32(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shrdi_epi32(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shrdi_64(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shrdi_epi64(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shrdi_epi64(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shrdi_epi64(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shrdi(T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return shrdi_16(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return shrdi_32(a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return shrdi_64(a, b, imm8);
  }
}

template <is_simd_class T>
inline T
shrdv_16(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shrdv_epi16(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shrdv_epi16(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shrdv_epi16(a, b, c);
  }
}

template <is_simd_class T>
inline T
shrdv_32(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shrdv_epi32(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shrdv_epi32(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shrdv_epi32(a, b, c);
  }
}

template <is_simd_class T>
inline T
shrdv_64(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_shrdv_epi64(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_shrdv_epi64(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_shrdv_epi64(a, b, c);
  }
}

template <is_simd_class T>
inline T
shrdv(T &a, T &b, T &c)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return shrdv_16(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return shrdv_32(a, b, c);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return shrdv_64(a, b, c);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// masks (avx512ext)

template <is_simd_class T, typename M>
inline T
mask_slli_16(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_slli_epi16(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_slli_epi16(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_slli_epi16(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_slli_16(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_slli_epi16(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_slli_epi16(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_slli_epi16(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_slli_32(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_slli_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_slli_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_slli_epi32(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_slli_32(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_slli_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_slli_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_slli_epi32(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_slli_64(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_slli_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_slli_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_slli_epi64(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_slli_64(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_slli_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_slli_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_slli_epi64(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_srli_16(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_srli_epi16(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_srli_epi16(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_srli_epi16(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_srli_16(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_srli_epi16(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_srli_epi16(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_srli_epi16(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_srli_32(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_srli_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_srli_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_srli_epi32(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_srli_32(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_srli_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_srli_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_srli_epi32(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_srli_64(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_srli_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_srli_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_srli_epi64(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_srli_64(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_srli_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_srli_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_srli_epi64(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_srai_16(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_srai_epi16(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_srai_epi16(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_srai_epi16(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_srai_16(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_srai_epi16(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_srai_epi16(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_srai_epi16(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_srai_32(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_srai_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_srai_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_srai_epi32(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_srai_32(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_srai_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_srai_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_srai_epi32(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_srai_64(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_srai_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_srai_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_srai_epi64(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_srai_64(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_srai_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_srai_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_srai_epi64(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_rol_32(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_rol_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_rol_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_rol_epi32(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_rol_32(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_rol_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_rol_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_rol_epi32(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_rol_64(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_rol_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_rol_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_rol_epi64(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_rol_64(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_rol_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_rol_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_rol_epi64(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_ror_32(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_ror_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_ror_epi32(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_ror_epi32(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_ror_32(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_ror_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_ror_epi32(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_ror_epi32(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_ror_64(T &src, M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_ror_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_ror_epi64(src, k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_ror_epi64(src, k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_ror_64(M k, T &o, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_ror_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_ror_epi64(k, o, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_ror_epi64(k, o, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_shldi_64(T &src, M k, T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_shldi_epi64(src, k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_shldi_epi64(src, k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_shldi_epi64(src, k, a, b, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_shldi_64(M k, T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_shldi_epi64(k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_shldi_epi64(k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_shldi_epi64(k, a, b, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
mask_shrdi_64(T &src, M k, T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mask_shrdi_epi64(src, k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mask_shrdi_epi64(src, k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_mask_shrdi_epi64(src, k, a, b, imm8);
  }
}

template <is_simd_class T, typename M>
inline T
maskz_shrdi_64(M k, T &a, T &b, int imm8)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maskz_shrdi_epi64(k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maskz_shrdi_epi64(k, a, b, imm8);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i512> ) {
    return _mm512_maskz_shrdi_epi64(k, a, b, imm8);
  }
}

#pragma GCC diagnostic pop

};     // namespace simd
};     // namespace micron
