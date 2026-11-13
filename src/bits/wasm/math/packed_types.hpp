// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../simd/types.hpp"

namespace micron::math::mk
{
template<typename V>
concept packed_real = simd::is_simd_type<V> && micron::is_floating_point_v<typename simd::__wasm_packet_traits<V>::element>;

template<packed_real V> struct elem_of {
  using type = typename simd::__wasm_packet_traits<V>::element;
};

template<typename V> using elem_t = typename elem_of<V>::type;

template<packed_real V> struct lanes_of {
  static constexpr usize value = sizeof(V) / sizeof(elem_t<V>);
};

template<typename V> inline constexpr usize lanes_v = lanes_of<V>::value;
}      // namespace micron::math::mk
