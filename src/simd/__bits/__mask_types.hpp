//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../type_traits.hpp"

// for avx512

namespace micron
{
namespace simd
{
namespace __bits
{

using __mmask8 = unsigned char;
using __mmask16 = unsigned short;
using __mmask32 = unsigned int;
using __mmask64 = unsigned long long;

static_assert(sizeof(__mmask8) == 1);
static_assert(sizeof(__mmask16) == 2);
static_assert(sizeof(__mmask32) == 4);
static_assert(sizeof(__mmask64) == 8);

template<typename M> inline constexpr unsigned mask_lanes_v = sizeof(M) * 8;

template<typename M>
concept k_mask_type = ::micron::is_same_v<M, __mmask8> or ::micron::is_same_v<M, __mmask16> or ::micron::is_same_v<M, __mmask32>
                      or ::micron::is_same_v<M, __mmask64>;

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_TYPES)
using ::micron::simd::__bits::__mmask16;
using ::micron::simd::__bits::__mmask32;
using ::micron::simd::__bits::__mmask64;
using ::micron::simd::__bits::__mmask8;
#endif
