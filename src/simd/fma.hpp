
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "simd.hpp"
#include "types.hpp"

// WARNING: x86 only, no NEON version yet
#if defined(__micron_arch_x86_any)

namespace micron
{
namespace simd
{

namespace fma
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"

template<is_simd_class T>
inline typename T::bit_width
__unwrap(const T &o) noexcept
{
  static_assert(__is_standard_layout(T) && sizeof(T) == sizeof(typename T::bit_width),
                "fma: wrapper must be standard-layout with a sole register member");
  return *reinterpret_cast<const typename T::bit_width *>(&o);
}

template<is_simd_class T>
inline T
__wrap(typename T::bit_width r) noexcept
{
  T o;
  *reinterpret_cast<typename T::bit_width *>(&o) = r;
  return o;
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d128>)
inline T
fma(T &o, T &b, T &c)
{
  return __wrap<T>(_mm_fmadd_pd(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d256>)
inline T
fma(T &o, T &b, T &c)
{
  return __wrap<T>(_mm256_fmadd_pd(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f128>)
inline T
fma(T &o, T &b, T &c)
{
  return __wrap<T>(_mm_fmadd_ps(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f256>)
inline T
fma(T &o, T &b, T &c)
{
  return __wrap<T>(_mm256_fmadd_ps(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d128>)
inline T
fmas(T &o, T &b, T &c)
{
  return __wrap<T>(_mm_fmaddsub_pd(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d256>)
inline T
fmas(T &o, T &b, T &c)
{
  return __wrap<T>(_mm256_fmaddsub_pd(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f128>)
inline T
fmas(T &o, T &b, T &c)
{
  return __wrap<T>(_mm_fmaddsub_ps(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f256>)
inline T
fmas(T &o, T &b, T &c)
{
  return __wrap<T>(_mm256_fmaddsub_ps(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d128>)
inline T
fms(T &o, T &b, T &c)
{
  return __wrap<T>(_mm_fmsub_pd(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, d256>)
inline T
fms(T &o, T &b, T &c)
{
  return __wrap<T>(_mm256_fmsub_pd(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f128>)
inline T
fms(T &o, T &b, T &c)
{
  return __wrap<T>(_mm_fmsub_ps(__unwrap(o), __unwrap(b), __unwrap(c)));
}

template<is_simd_class T>
  requires(micron::is_same_v<typename T::bit_width, f256>)
inline T
fms(T &o, T &b, T &c)
{
  return __wrap<T>(_mm256_fmsub_ps(__unwrap(o), __unwrap(b), __unwrap(c)));
}

};      // namespace fma
};      // namespace simd
};      // namespace micron

#pragma GCC diagnostic pop

#endif
