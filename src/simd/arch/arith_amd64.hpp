//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "simd.hpp"
#include "types.hpp"

namespace micron
{
namespace simd
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// bitwise

template <is_simd_class T>
inline T
avx_and(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_and_si128(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_and_si256(o, b);
  }
}

template <is_simd_class T>
inline T
avx_andnot(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_andnot_si128(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_andnot_si256(o, b);
  }
}

template <is_simd_class T>
inline T
avx_or(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_or_si128(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_or_si256(o, b);
  }
}

template <is_simd_class T>
inline T
avx_xor(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_xor_si128(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_xor_si256(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shifts

template <is_simd_class T>
inline T
shiftleft_16(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_slli_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_slli_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
shiftleft_32(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_slli_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_slli_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
shiftleft_64(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_slli_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_slli_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
shift_left(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return shiftleft_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return shiftleft_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return shiftleft_64(o, b);
  }
}

template <is_simd_class T>
inline T
shiftright_logical_16(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srli_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srli_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
shiftright_logical_32(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srli_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srli_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
shiftright_logical_64(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srli_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srli_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
shift_right_logical(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return shiftright_logical_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return shiftright_logical_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return shiftright_logical_64(o, b);
  }
}

template <is_simd_class T>
inline T
shiftright_arithmetic_16(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srai_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srai_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
shiftright_arithmetic_32(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_srai_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_srai_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
shift_right_arithmetic(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return shiftright_arithmetic_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return shiftright_arithmetic_32(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// addition

template <is_simd_class T>
inline T
add_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_add_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_add_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
add_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_add_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_add_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
add_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_add_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_add_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
add_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_add_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_add_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
add(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return add_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return add_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return add_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return add_64(o, b);
  }
}

template <is_simd_class T>
inline T
adds_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_adds_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_adds_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
adds_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_adds_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_adds_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
adds(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return adds_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return adds_16(o, b);
  }
}

template <is_simd_class T>
inline T
adds_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_adds_epu8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_adds_epu8(o, b);
  }
}

template <is_simd_class T>
inline T
adds_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_adds_epu16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_adds_epu16(o, b);
  }
}

template <is_simd_class T>
inline T
adds_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return adds_u8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return adds_u16(o, b);
  }
}

template <is_simd_class T>
inline T
sub_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sub_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sub_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
sub_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sub_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sub_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
sub_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sub_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sub_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
sub_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sub_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sub_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
sub(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return sub_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return sub_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return sub_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return sub_64(o, b);
  }
}

template <is_simd_class T>
inline T
subs_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_subs_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_subs_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
subs_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_subs_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_subs_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
subs(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return subs_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return subs_16(o, b);
  }
}

template <is_simd_class T>
inline T
subs_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_subs_epu8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_subs_epu8(o, b);
  }
}

template <is_simd_class T>
inline T
subs_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_subs_epu16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_subs_epu16(o, b);
  }
}

template <is_simd_class T>
inline T
subs_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return subs_u8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return subs_u16(o, b);
  }
}

template <is_simd_class T>
inline T
hadd_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_hadd_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_hadd_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
hadd_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_hadd_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_hadd_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
hadd(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return hadd_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return hadd_32(o, b);
  }
}

template <is_simd_class T>
inline T
hadds_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_hadds_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_hadds_epi16(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// subs

template <is_simd_class T>
inline T
hsub_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_hsub_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_hsub_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
hsub_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_hsub_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_hsub_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
hsub(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return hsub_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return hsub_32(o, b);
  }
}

template <is_simd_class T>
inline T
hsubs_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_hsubs_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_hsubs_epi16(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// muls

template <is_simd_class T>
inline T
mullo_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mullo_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mullo_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
mullo_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mullo_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mullo_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
mullo_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mullo_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mullo_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
mullo(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return mullo_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return mullo_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return mullo_64(o, b);
  }
}

template <is_simd_class T>
inline T
mulhi_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mulhi_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mulhi_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
mulhi(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return mulhi_16(o, b);
  }
}

template <is_simd_class T>
inline T
mulhi_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mulhi_epu16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mulhi_epu16(o, b);
  }
}

template <is_simd_class T>
inline T
mulhi_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return mulhi_u16(o, b);
  }
}

template <is_simd_class T>
inline T
mul_32_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mul_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mul_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
mul_u32_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_mul_epu32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_mul_epu32(o, b);
  }
}

template <is_simd_class T>
inline T
madd_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_madd_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_madd_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
madd(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return madd_16(o, b);
  }
}

template <is_simd_class T>
inline T
maddubs_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_maddubs_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_maddubs_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
maddubs(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return maddubs_8(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// avgs

template <is_simd_class T>
inline T
avg_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_avg_epu8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_avg_epu8(o, b);
  }
}

template <is_simd_class T>
inline T
avg_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_avg_epu16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_avg_epu16(o, b);
  }
}

template <is_simd_class T>
inline T
avg_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return avg_u8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return avg_u16(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// signs

template <is_simd_class T>
inline T
sign_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sign_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sign_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
sign_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sign_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sign_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
sign_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sign_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sign_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
sign(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return sign_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return sign_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return sign_32(o, b);
  }
}

template <is_simd_class T>
inline T
min_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
min_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
min_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
min(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return min_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return min_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return min_32(o, b);
  }
}

template <is_simd_class T>
inline T
min_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epu8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epu8(o, b);
  }
}

template <is_simd_class T>
inline T
min_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epu16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epu16(o, b);
  }
}

template <is_simd_class T>
inline T
min_u32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_min_epu32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_min_epu32(o, b);
  }
}

template <is_simd_class T>
inline T
min_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return min_u8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return min_u16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return min_u32(o, b);
  }
}

template <is_simd_class T>
inline T
max_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
max_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
max_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
max(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return max_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return max_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return max_32(o, b);
  }
}

template <is_simd_class T>
inline T
max_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epu8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epu8(o, b);
  }
}

template <is_simd_class T>
inline T
max_u16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epu16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epu16(o, b);
  }
}

template <is_simd_class T>
inline T
max_u32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epu32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epu32(o, b);
  }
}

template <is_simd_class T>
inline T
max_unsigned(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return max_u8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return max_u16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return max_u32(o, b);
  }
}

template <is_simd_class T>
inline T
abs_8(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_abs_epi8(o);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_abs_epi8(o);
  }
}

template <is_simd_class T>
inline T
abs_16(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_abs_epi16(o);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_abs_epi16(o);
  }
}

template <is_simd_class T>
inline T
abs_32(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_abs_epi32(o);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_abs_epi32(o);
  }
}

template <is_simd_class T>
inline T
abs(T &o)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return abs_8(o);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return abs_16(o);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return abs_32(o);
  }
}

template <is_simd_class T>
inline T
sad_u8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sad_epu8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sad_epu8(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cmps

template <is_simd_class T>
inline T
cmpeq_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpeq_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpeq_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
cmpeq_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpeq_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpeq_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
cmpeq_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpeq_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpeq_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
cmpeq_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpeq_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpeq_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
cmpeq(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return cmpeq_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return cmpeq_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return cmpeq_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return cmpeq_64(o, b);
  }
}

template <is_simd_class T>
inline T
cmpgt_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpgt_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpgt_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
cmpgt_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpgt_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpgt_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
cmpgt_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpgt_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpgt_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
cmpgt_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_cmpgt_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_cmpgt_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
cmpgt(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return cmpgt_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return cmpgt_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return cmpgt_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return cmpgt_64(o, b);
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// packs

template <is_simd_class T>
inline T
packs_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_packs_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_packs_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
packs_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_packs_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_packs_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
packs(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return packs_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return packs_32(o, b);
  }
}

template <is_simd_class T>
inline T
packus_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_packus_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_packus_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
packus_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_packus_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_packus_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
packus(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return packus_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return packus_32(o, b);
  }
}

template <is_simd_class T>
inline T
unpacklo_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpacklo_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpacklo_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
unpacklo_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpacklo_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpacklo_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
unpacklo_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpacklo_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpacklo_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
unpacklo_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpacklo_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpacklo_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
unpacklo(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return unpacklo_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return unpacklo_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return unpacklo_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return unpacklo_64(o, b);
  }
}

template <is_simd_class T>
inline T
unpackhi_8(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpackhi_epi8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpackhi_epi8(o, b);
  }
}

template <is_simd_class T>
inline T
unpackhi_16(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpackhi_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpackhi_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
unpackhi_32(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpackhi_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpackhi_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
unpackhi_64(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_unpackhi_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_unpackhi_epi64(o, b);
  }
}

template <is_simd_class T>
inline T
unpackhi(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return unpackhi_8(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return unpackhi_16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return unpackhi_32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v64> ) {
    return unpackhi_64(o, b);
  }
}

};     // namespace simd
};     // namespace micron

#pragma GCC diagnostic pop
