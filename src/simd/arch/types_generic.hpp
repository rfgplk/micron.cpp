//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../concepts.hpp"

#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../intrin.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// generic (scalar) types
// dispatched via ns

namespace micron
{
namespace simd
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// lane containers

template<typename L, usize N> struct alignas(sizeof(L) * N) __gvec {
  using lane_type = L;
  static constexpr usize lanes = N;
  L v[N];

  constexpr L &
  operator[](usize i) noexcept
  {
    return v[i];
  }

  constexpr const L &
  operator[](usize i) const noexcept
  {
    return v[i];
  }
};

using b64 = __gvec<u64, 1>;

using f128 = __gvec<f32, 4>;
using d128 = __gvec<f64, 2>;
using i128 = __gvec<u64, 2>;

using f256 = __gvec<f32, 8>;
using d256 = __gvec<f64, 4>;
using i256 = __gvec<u64, 4>;

using f512 = __gvec<f32, 16>;
using d512 = __gvec<f64, 8>;
using i512 = __gvec<u64, 8>;

template<typename T>
concept is_simd_type = micron::same_as<T, b64> or micron::same_as<T, f128> or micron::same_as<T, d128> or micron::same_as<T, i128>
                       or micron::same_as<T, f256> or micron::same_as<T, d256> or micron::same_as<T, i256> or micron::same_as<T, f512>
                       or micron::same_as<T, d512> or micron::same_as<T, i512>;

template<typename T>
concept is_simd_128_type = micron::same_as<T, f128> or micron::same_as<T, d128> or micron::same_as<T, i128>;
template<typename T>
concept is_simd_256_type = micron::same_as<T, f256> or micron::same_as<T, d256> or micron::same_as<T, i256>;
template<typename T>
concept is_simd_512_type = micron::same_as<T, f512> or micron::same_as<T, d512> or micron::same_as<T, i512>;

template<typename T>
concept is_int_flag_type
    = micron::same_as<T, __uv8> or micron::same_as<T, __uv16> or micron::same_as<T, __uv32> or micron::same_as<T, __uv64>
      or micron::same_as<T, __v8> or micron::same_as<T, __v16> or micron::same_as<T, __v32> or micron::same_as<T, __v64>;

template<typename T>
concept is_flag_type = micron::same_as<T, __vd> or micron::same_as<T, __vf> or micron::same_as<T, __v8> or micron::same_as<T, __v16>
                       or micron::same_as<T, __v32> or micron::same_as<T, __v64>;

template<typename F>
constexpr bool
__is_64_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v64> or micron::is_same_v<F, __uv64> ) return true;
  return false;
}

template<typename F>
constexpr bool
__is_32_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v32> or micron::is_same_v<F, __uv32> ) return true;
  return false;
}

template<typename F>
constexpr bool
__is_16_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v16> or micron::is_same_v<F, __uv16> ) return true;
  return false;
}

template<typename F>
constexpr bool
__is_8_wide(void)
{
  if constexpr ( micron::is_same_v<F, __v8> or micron::is_same_v<F, __uv8> ) return true;
  return false;
}

};      // namespace simd
};      // namespace micron
