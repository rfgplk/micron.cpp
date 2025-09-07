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

template <is_simd_class T>
inline T
avx_xor(T &o, T &b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_xor_si256(o, b);
  }
}

template <is_simd_class T>
inline T
shiftleft_16(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sll_epi16(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sll_epi16(o, b);
  }
}

template <is_simd_class T>
inline T
shiftleft_32(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sll_epi32(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sll_epi32(o, b);
  }
}

template <is_simd_class T>
inline T
shiftleft_64(T &o, int b)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_sll_epi64(o, b);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_sll_epi64(o, b);
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
max_8(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi8(o);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi8(o);
  }
}

template <is_simd_class T>
inline T
max_16(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi16(o);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi16(o);
  }
}

template <is_simd_class T>
inline T
max_32(T &o)
{
  if constexpr ( micron::is_same_v<typename T::bit_width, i128> ) {
    return _mm_max_epi32(o);
  }
  if constexpr ( micron::is_same_v<typename T::bit_width, i256> ) {
    return _mm256_max_epi32(o);
  }
}

template <is_simd_class T>
inline T
max(T &o)
{
  if constexpr ( micron::is_same_v<typename T::lane_width, __v8> ) {
    return max_8(o);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v16> ) {
    return max_16(o);
  }
  if constexpr ( micron::is_same_v<typename T::lane_width, __v32> ) {
    return max_32(o);
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
};
};
#pragma GCC diagnostic pop
