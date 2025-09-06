//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "../concepts.hpp"
#include "intrin.hpp"

namespace micron
{
namespace simd
{
using b64 = __m64;
using f128 = __m128;
using d128 = __m128d;
using i128 = __m128i;
using f256 = __m256;
using d256 = __m256d;
using i256 = __m256i;
using f512 = __m512;
using d512 = __m512d;
using i512 = __m512i;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
template <typename T>
concept is_simd_type
    = micron::same_as<T, b64> or micron::same_as<T, f128> or micron::same_as<T, d128> or micron::same_as<T, i128>
      or micron::same_as<T, f256> or micron::same_as<T, d256> or micron::same_as<T, i256> or micron::same_as<T, f512>
      or micron::same_as<T, d512> or micron::same_as<T, i512>;
template <typename T>
concept is_simd_128_type = micron::same_as<T, f128> or micron::same_as<T, d128> or micron::same_as<T, i128>;
template <typename T>
concept is_simd_256_type = micron::same_as<T, f256> or micron::same_as<T, d256> or micron::same_as<T, i256>;
template <typename T>
concept is_simd_512_type = micron::same_as<T, f512> or micron::same_as<T, d512> or micron::same_as<T, i512>;

#pragma GCC diagnostic pop
};
};
