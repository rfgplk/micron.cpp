//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../concepts.hpp"
#include "../../simd/types.hpp"
#include "../../types.hpp"

namespace micron
{
namespace math
{
namespace mk
{

template <typename V>
concept packed_real = micron::same_as<V, simd::f128>
#if defined(__micron_arch_x86_any) || defined(__micron_arch_arm64)
                      or micron::same_as<V, simd::d128>
#endif
#if defined(__micron_x86_avx)
                      or micron::same_as<V, simd::f256> or micron::same_as<V, simd::d256>
#endif
#if defined(__micron_x86_avx512f)
                      or micron::same_as<V, simd::f512> or micron::same_as<V, simd::d512>
#endif
    ;

template <typename V> struct lanes_of {
};

#if defined(__micron_arch_x86_any) || defined(__micron_arch_arm_any)
template <> struct lanes_of<simd::f128> {
  static constexpr usize value = 4;
};
#endif
#if defined(__micron_arch_x86_any) || defined(__micron_arch_arm64)
template <> struct lanes_of<simd::d128> {
  static constexpr usize value = 2;
};
#endif
#if defined(__micron_x86_avx)
template <> struct lanes_of<simd::f256> {
  static constexpr usize value = 8;
};

template <> struct lanes_of<simd::d256> {
  static constexpr usize value = 4;
};
#endif
#if defined(__micron_x86_avx512f)
template <> struct lanes_of<simd::f512> {
  static constexpr usize value = 16;
};

template <> struct lanes_of<simd::d512> {
  static constexpr usize value = 8;
};
#endif

template <typename V> inline constexpr usize lanes_v = lanes_of<V>::value;

template <typename V> struct elem_of {
};
#if defined(__micron_arch_x86_any) || defined(__micron_arch_arm_any)
template <> struct elem_of<simd::f128> {
  using type = f32;
};
#endif
#if defined(__micron_arch_x86_any) || defined(__micron_arch_arm64)
template <> struct elem_of<simd::d128> {
  using type = f64;
};
#endif
#if defined(__micron_x86_avx)
template <> struct elem_of<simd::f256> {
  using type = f32;
};

template <> struct elem_of<simd::d256> {
  using type = f64;
};
#endif
#if defined(__micron_x86_avx512f)
template <> struct elem_of<simd::f512> {
  using type = f32;
};

template <> struct elem_of<simd::d512> {
  using type = f64;
};
#endif
template <typename V> using elem_t = typename elem_of<V>::type;

};     // namespace mk
};     // namespace math
};     // namespace micron
