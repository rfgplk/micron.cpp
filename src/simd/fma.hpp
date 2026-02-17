
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

namespace fma
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d128>)
inline T
fma(T &o, T &b, T &c)
{
  return _mm_fmadd_pd(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d256>)
inline T
fma(T &o, T &b, T &c)
{
  return _mm256_fmadd_pd(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f128>)
inline T
fma(T &o, T &b, T &c)
{
  return _mm_fmadd_ps(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f256>)
inline T
fma(T &o, T &b, T &c)
{
  return _mm256_fmadd_ps(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d128>)
inline T
fmas(T &o, T &b, T &c)
{
  return _mm_fmaddsub_pd(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d256>)
inline T
fmas(T &o, T &b, T &c)
{
  return _mm256_fmaddsub_pd(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f128>)
inline T
fmas(T &o, T &b, T &c)
{
  return _mm_fmaddsub_ps(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f256>)
inline T
fmas(T &o, T &b, T &c)
{
  return _mm256_fmaddsub_ps(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d128>)
inline T
fms(T &o, T &b, T &c)
{
  return _mm_fmsub_pd(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d256>)
inline T
fms(T &o, T &b, T &c)
{
  return _mm256_fmsub_pd(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f128>)
inline T
fms(T &o, T &b, T &c)
{
  return _mm_fmsub_ps(o, b, c);
}

template <is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f256>)
inline T
fms(T &o, T &b, T &c)
{
  return _mm256_fmsub_ps(o, b, c);
}

};
};
};

#pragma GCC diagnostic pop
