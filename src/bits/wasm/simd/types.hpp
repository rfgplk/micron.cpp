// Copyright (c) 2026- David Lucius Severus
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"

namespace micron::simd
{
using __vf = float;
using __vd = double;
using __v8 = i8;
using __v16 = i16;
using __v32 = i32;
using __v64 = i64;
using __uv8 = u8;
using __uv16 = u16;
using __uv32 = u32;
using __uv64 = u64;

template<typename F, usize N> struct __wasm_lanes {
  F lane[N];
};

template<typename F, usize Bytes> struct alignas(Bytes < 16 ? Bytes : 16) __wasm_storage {
  F lane[Bytes / sizeof(F)];
};

#if defined(__micron_wasm_simd128)
template<typename F> struct __wasm_native {
  typedef F type __attribute__((vector_size(16)));
};

using f128 = __wasm_native<float>::type;
using d128 = __wasm_native<double>::type;
using i128 = __wasm_native<i32>::type;
#else
using f128 = __wasm_storage<float, 16>;
using d128 = __wasm_storage<double, 16>;
using i128 = __wasm_storage<i32, 16>;
#endif

using b64 = __wasm_storage<u8, 8>;

template<typename B, usize N> struct alignas(16) __wasm_composite {
  B block[N];
};

using f256 = __wasm_composite<f128, 2>;
using d256 = __wasm_composite<d128, 2>;
using i256 = __wasm_composite<i128, 2>;
using f512 = __wasm_composite<f128, 4>;
using d512 = __wasm_composite<d128, 4>;
using i512 = __wasm_composite<i128, 4>;

template<typename B> struct __wasm_packet_traits;

template<> struct __wasm_packet_traits<f128> {
  using element = float;
};

template<> struct __wasm_packet_traits<d128> {
  using element = double;
};

template<> struct __wasm_packet_traits<i128> {
  using element = i32;
};

template<> struct __wasm_packet_traits<b64> {
  using element = u8;
};

template<typename B, usize N> struct __wasm_packet_traits<__wasm_composite<B, N>>: __wasm_packet_traits<B> {
};

template<typename B>
concept is_simd_type = requires { typename __wasm_packet_traits<B>::element; };
template<typename B>
concept is_simd_128_type = is_simd_type<B> && sizeof(B) == 16;
template<typename B>
concept is_simd_256_type = is_simd_type<B> && sizeof(B) == 32;
template<typename B>
concept is_simd_512_type = is_simd_type<B> && sizeof(B) == 64;
template<typename F>
concept is_int_flag_type
    = micron::is_same_v<F, __v8> || micron::is_same_v<F, __v16> || micron::is_same_v<F, __v32> || micron::is_same_v<F, __v64>
      || micron::is_same_v<F, __uv8> || micron::is_same_v<F, __uv16> || micron::is_same_v<F, __uv32> || micron::is_same_v<F, __uv64>;
template<typename F>
concept is_flag_type = is_int_flag_type<F> || __is_same(F, float) || __is_same(F, double);

template<typename F>
constexpr bool
__is_64_wide()
{
  return sizeof(F) == 8;
}

template<typename F>
constexpr bool
__is_32_wide()
{
  return sizeof(F) == 4;
}

template<typename F>
constexpr bool
__is_16_wide()
{
  return sizeof(F) == 2;
}

template<typename F>
constexpr bool
__is_8_wide()
{
  return sizeof(F) == 1;
}

template<typename F, is_simd_type B>
[[gnu::always_inline]] inline constexpr auto
__wasm_unpack(B value) noexcept
{
  using lanes = __wasm_lanes<F, sizeof(B) / sizeof(F)>;
  return __builtin_bit_cast(lanes, value);
}

template<is_simd_type B, typename F, usize N>
[[gnu::always_inline]] inline constexpr B
__wasm_pack(__wasm_lanes<F, N> value) noexcept
{
  static_assert(sizeof(value) == sizeof(B));
  return __builtin_bit_cast(B, value);
}

template<is_simd_type B, typename F>
[[gnu::always_inline]] inline constexpr B
__wasm_splat(F value) noexcept
{
  __wasm_lanes<F, sizeof(B) / sizeof(F)> lanes{};
  for ( auto &lane : lanes.lane ) lane = value;
  return __wasm_pack<B>(lanes);
}
};      // namespace micron::simd
