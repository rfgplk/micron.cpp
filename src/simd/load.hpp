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
// NOTE: this is currently here since the compiler really likes to complain that we lose attributes when passing _mXXX
// types around through templates. no way to surpress otherwise
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

template <is_simd_type B>
inline B
stream_load(B *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_stream_load_si128(ptr);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm_stream_load_si256(ptr);
  }
}

template <is_simd_type B, typename T>
inline auto
stream_load(T *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_stream_load_si128(reinterpret_cast<i128 *>(ptr));
  }
}

template <is_simd_type B>
inline B
load(B *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_load_si128(ptr);
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_load_ps(ptr);
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_load_pd(ptr);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_load_si256(ptr);
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_load_ps(ptr);
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_load_pd(ptr);
  }
}

template <is_simd_type B, typename T>
inline auto
load(T *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_load_si128(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_load_ps(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_load_pd(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_load_si256(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_load_ps(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_load_pd(reinterpret_cast<T *>(ptr));
  }
}

template <is_simd_type B>
inline B
loadu(B *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_loadu_si128(ptr);
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_loadu_ps(ptr);
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_loadu_pd(ptr);
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_loadu_si256(ptr);
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_loadu_ps(ptr);
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_loadu_pd(ptr);
  }
}

template <is_simd_type B, typename T>
inline auto
loadu(T *ptr)
{
  if constexpr ( micron::is_same_v<B, i128> ) {
    return _mm_loadu_si128(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f128> ) {
    return _mm_loadu_ps(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d128> ) {
    return _mm_loadu_pd(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, i256> ) {
    return _mm256_loadu_si256(reinterpret_cast<i256 *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, f256> ) {
    return _mm256_loadu_ps(reinterpret_cast<T *>(ptr));
  }
  if constexpr ( micron::is_same_v<B, d256> ) {
    return _mm256_loadu_pd(reinterpret_cast<T *>(ptr));
  }
}

#pragma GCC diagnostic pop
};
};
