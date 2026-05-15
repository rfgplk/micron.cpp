//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"

#if !defined(__micron_arch_arm32)
#error "__vector_types_arm32.hpp included on a non-armv7 build"
#endif

namespace micron
{
namespace simd
{
namespace __bits
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#pragma GCC diagnostic ignored "-Wpsabi"
#pragma GCC diagnostic ignored "-Wpedantic"

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__ARM_FP16_FORMAT_ALTERNATIVE) || defined(__micron_arm_fp16)
using float16_t = __fp16;
#endif
using float32_t = float;

using poly8_t = ::i8;
using poly16_t = ::i16;

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)
using poly64_t = ::i64;
#endif

#if defined(__ARM_FEATURE_BF16) || defined(__micron_arm_bf16)
using bfloat16_t = __bf16;
#endif

typedef signed char int8x8_t __attribute__((__vector_size__(8), __may_alias__));
typedef signed short int16x4_t __attribute__((__vector_size__(8), __may_alias__));
typedef signed int int32x2_t __attribute__((__vector_size__(8), __may_alias__));
typedef signed long long int64x1_t __attribute__((__vector_size__(8), __may_alias__));

typedef unsigned char uint8x8_t __attribute__((__vector_size__(8), __may_alias__));
typedef unsigned short uint16x4_t __attribute__((__vector_size__(8), __may_alias__));
typedef unsigned int uint32x2_t __attribute__((__vector_size__(8), __may_alias__));
typedef unsigned long long uint64x1_t __attribute__((__vector_size__(8), __may_alias__));

typedef poly8_t poly8x8_t __attribute__((__vector_size__(8), __may_alias__));
typedef poly16_t poly16x4_t __attribute__((__vector_size__(8), __may_alias__));

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)
typedef poly64_t poly64x1_t __attribute__((__vector_size__(8), __may_alias__));
#endif

typedef float32_t float32x2_t __attribute__((__vector_size__(8), __may_alias__));

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__micron_arm_fp16)
typedef float16_t float16x4_t __attribute__((__vector_size__(8), __may_alias__));
#endif

#if defined(__ARM_FEATURE_BF16) || defined(__micron_arm_bf16)
typedef bfloat16_t bfloat16x4_t __attribute__((__vector_size__(8), __may_alias__));
#endif

typedef signed char int8x16_t __attribute__((__vector_size__(16), __may_alias__));
typedef signed short int16x8_t __attribute__((__vector_size__(16), __may_alias__));
typedef signed int int32x4_t __attribute__((__vector_size__(16), __may_alias__));
typedef signed long long int64x2_t __attribute__((__vector_size__(16), __may_alias__));

typedef unsigned char uint8x16_t __attribute__((__vector_size__(16), __may_alias__));
typedef unsigned short uint16x8_t __attribute__((__vector_size__(16), __may_alias__));
typedef unsigned int uint32x4_t __attribute__((__vector_size__(16), __may_alias__));
typedef unsigned long long uint64x2_t __attribute__((__vector_size__(16), __may_alias__));

typedef poly8_t poly8x16_t __attribute__((__vector_size__(16), __may_alias__));
typedef poly16_t poly16x8_t __attribute__((__vector_size__(16), __may_alias__));

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)
typedef poly64_t poly64x2_t __attribute__((__vector_size__(16), __may_alias__));
#endif

typedef float32_t float32x4_t __attribute__((__vector_size__(16), __may_alias__));

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__micron_arm_fp16)
typedef float16_t float16x8_t __attribute__((__vector_size__(16), __may_alias__));
#endif

#if defined(__ARM_FEATURE_BF16) || defined(__micron_arm_bf16)
typedef bfloat16_t bfloat16x8_t __attribute__((__vector_size__(16), __may_alias__));
#endif

#define __micron_neon_tuple(BASE, N)                                                                                                       \
  typedef struct BASE##x##N##_t {                                                                                                          \
    BASE##_t val[N];                                                                                                                       \
  } BASE##x##N##_t

#define __micron_neon_tuples_for(BASE)                                                                                                     \
  __micron_neon_tuple(BASE, 2);                                                                                                            \
  __micron_neon_tuple(BASE, 3);                                                                                                            \
  __micron_neon_tuple(BASE, 4)

__micron_neon_tuples_for(int8x8);
__micron_neon_tuples_for(int16x4);
__micron_neon_tuples_for(int32x2);
__micron_neon_tuples_for(int64x1);
__micron_neon_tuples_for(uint8x8);
__micron_neon_tuples_for(uint16x4);
__micron_neon_tuples_for(uint32x2);
__micron_neon_tuples_for(uint64x1);
__micron_neon_tuples_for(float32x2);
__micron_neon_tuples_for(poly8x8);
__micron_neon_tuples_for(poly16x4);

__micron_neon_tuples_for(int8x16);
__micron_neon_tuples_for(int16x8);
__micron_neon_tuples_for(int32x4);
__micron_neon_tuples_for(int64x2);
__micron_neon_tuples_for(uint8x16);
__micron_neon_tuples_for(uint16x8);
__micron_neon_tuples_for(uint32x4);
__micron_neon_tuples_for(uint64x2);
__micron_neon_tuples_for(float32x4);
__micron_neon_tuples_for(poly8x16);
__micron_neon_tuples_for(poly16x8);

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)
__micron_neon_tuples_for(poly64x1);
__micron_neon_tuples_for(poly64x2);
#endif

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__micron_arm_fp16)
__micron_neon_tuples_for(float16x4);
__micron_neon_tuples_for(float16x8);
#endif

#if defined(__ARM_FEATURE_BF16) || defined(__micron_arm_bf16)
__micron_neon_tuples_for(bfloat16x4);
__micron_neon_tuples_for(bfloat16x8);
#endif

#undef __micron_neon_tuples_for
#undef __micron_neon_tuple

template<typename T> inline constexpr unsigned width_v = sizeof(T) * 8;

template<typename T, typename L> inline constexpr unsigned lane_count_v = sizeof(T) / sizeof(L);

template<typename T>
concept v64 = (sizeof(T) == 8);

template<typename T>
concept v128 = (sizeof(T) == 16);

static_assert(sizeof(int8x8_t) == 8);
static_assert(sizeof(int8x16_t) == 16);
static_assert(sizeof(int16x8_t) == 16);
static_assert(sizeof(int32x4_t) == 16);
static_assert(sizeof(uint8x16_t) == 16);
static_assert(sizeof(float32x4_t) == 16);
static_assert(sizeof(int8x16x2_t) == 32 && sizeof(int8x16x4_t) == 64);

#pragma GCC diagnostic pop

};      // namespace __bits
};      // namespace simd
};      // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_TYPES)

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__ARM_FP16_FORMAT_ALTERNATIVE) || defined(__micron_arm_fp16)
using ::micron::simd::__bits::float16_t;
#endif
using ::micron::simd::__bits::float32_t;
using ::micron::simd::__bits::poly16_t;
using ::micron::simd::__bits::poly8_t;

using ::micron::simd::__bits::float32x2_t;
using ::micron::simd::__bits::int16x4_t;
using ::micron::simd::__bits::int32x2_t;
using ::micron::simd::__bits::int64x1_t;
using ::micron::simd::__bits::int8x8_t;
using ::micron::simd::__bits::poly16x4_t;
using ::micron::simd::__bits::poly8x8_t;
using ::micron::simd::__bits::uint16x4_t;
using ::micron::simd::__bits::uint32x2_t;
using ::micron::simd::__bits::uint64x1_t;
using ::micron::simd::__bits::uint8x8_t;

using ::micron::simd::__bits::float32x4_t;
using ::micron::simd::__bits::int16x8_t;
using ::micron::simd::__bits::int32x4_t;
using ::micron::simd::__bits::int64x2_t;
using ::micron::simd::__bits::int8x16_t;
using ::micron::simd::__bits::poly16x8_t;
using ::micron::simd::__bits::poly8x16_t;
using ::micron::simd::__bits::uint16x8_t;
using ::micron::simd::__bits::uint32x4_t;
using ::micron::simd::__bits::uint64x2_t;
using ::micron::simd::__bits::uint8x16_t;

using ::micron::simd::__bits::float32x2x2_t;
using ::micron::simd::__bits::float32x2x3_t;
using ::micron::simd::__bits::float32x2x4_t;
using ::micron::simd::__bits::int16x4x2_t;
using ::micron::simd::__bits::int16x4x3_t;
using ::micron::simd::__bits::int16x4x4_t;
using ::micron::simd::__bits::int32x2x2_t;
using ::micron::simd::__bits::int32x2x3_t;
using ::micron::simd::__bits::int32x2x4_t;
using ::micron::simd::__bits::int64x1x2_t;
using ::micron::simd::__bits::int64x1x3_t;
using ::micron::simd::__bits::int64x1x4_t;
using ::micron::simd::__bits::int8x8x2_t;
using ::micron::simd::__bits::int8x8x3_t;
using ::micron::simd::__bits::int8x8x4_t;
using ::micron::simd::__bits::poly16x4x2_t;
using ::micron::simd::__bits::poly16x4x3_t;
using ::micron::simd::__bits::poly16x4x4_t;
using ::micron::simd::__bits::poly8x8x2_t;
using ::micron::simd::__bits::poly8x8x3_t;
using ::micron::simd::__bits::poly8x8x4_t;
using ::micron::simd::__bits::uint16x4x2_t;
using ::micron::simd::__bits::uint16x4x3_t;
using ::micron::simd::__bits::uint16x4x4_t;
using ::micron::simd::__bits::uint32x2x2_t;
using ::micron::simd::__bits::uint32x2x3_t;
using ::micron::simd::__bits::uint32x2x4_t;
using ::micron::simd::__bits::uint64x1x2_t;
using ::micron::simd::__bits::uint64x1x3_t;
using ::micron::simd::__bits::uint64x1x4_t;
using ::micron::simd::__bits::uint8x8x2_t;
using ::micron::simd::__bits::uint8x8x3_t;
using ::micron::simd::__bits::uint8x8x4_t;

using ::micron::simd::__bits::float32x4x2_t;
using ::micron::simd::__bits::float32x4x3_t;
using ::micron::simd::__bits::float32x4x4_t;
using ::micron::simd::__bits::int16x8x2_t;
using ::micron::simd::__bits::int16x8x3_t;
using ::micron::simd::__bits::int16x8x4_t;
using ::micron::simd::__bits::int32x4x2_t;
using ::micron::simd::__bits::int32x4x3_t;
using ::micron::simd::__bits::int32x4x4_t;
using ::micron::simd::__bits::int64x2x2_t;
using ::micron::simd::__bits::int64x2x3_t;
using ::micron::simd::__bits::int64x2x4_t;
using ::micron::simd::__bits::int8x16x2_t;
using ::micron::simd::__bits::int8x16x3_t;
using ::micron::simd::__bits::int8x16x4_t;
using ::micron::simd::__bits::poly16x8x2_t;
using ::micron::simd::__bits::poly16x8x3_t;
using ::micron::simd::__bits::poly16x8x4_t;
using ::micron::simd::__bits::poly8x16x2_t;
using ::micron::simd::__bits::poly8x16x3_t;
using ::micron::simd::__bits::poly8x16x4_t;
using ::micron::simd::__bits::uint16x8x2_t;
using ::micron::simd::__bits::uint16x8x3_t;
using ::micron::simd::__bits::uint16x8x4_t;
using ::micron::simd::__bits::uint32x4x2_t;
using ::micron::simd::__bits::uint32x4x3_t;
using ::micron::simd::__bits::uint32x4x4_t;
using ::micron::simd::__bits::uint64x2x2_t;
using ::micron::simd::__bits::uint64x2x3_t;
using ::micron::simd::__bits::uint64x2x4_t;
using ::micron::simd::__bits::uint8x16x2_t;
using ::micron::simd::__bits::uint8x16x3_t;
using ::micron::simd::__bits::uint8x16x4_t;

#if defined(__ARM_FEATURE_CRYPTO) || defined(__micron_arm_crypto)
// poly128_t is omitted on arm32 (see above note in the type-decl block).
using ::micron::simd::__bits::poly64_t;
using ::micron::simd::__bits::poly64x1_t;
using ::micron::simd::__bits::poly64x1x2_t;
using ::micron::simd::__bits::poly64x1x3_t;
using ::micron::simd::__bits::poly64x1x4_t;
using ::micron::simd::__bits::poly64x2_t;
using ::micron::simd::__bits::poly64x2x2_t;
using ::micron::simd::__bits::poly64x2x3_t;
using ::micron::simd::__bits::poly64x2x4_t;
#endif

#if defined(__ARM_FP16_FORMAT_IEEE) || defined(__micron_arm_fp16)
using ::micron::simd::__bits::float16x4_t;
using ::micron::simd::__bits::float16x4x2_t;
using ::micron::simd::__bits::float16x4x3_t;
using ::micron::simd::__bits::float16x4x4_t;
using ::micron::simd::__bits::float16x8_t;
using ::micron::simd::__bits::float16x8x2_t;
using ::micron::simd::__bits::float16x8x3_t;
using ::micron::simd::__bits::float16x8x4_t;
#endif

#if defined(__ARM_FEATURE_BF16) || defined(__micron_arm_bf16)
using ::micron::simd::__bits::bfloat16_t;
using ::micron::simd::__bits::bfloat16x4_t;
using ::micron::simd::__bits::bfloat16x4x2_t;
using ::micron::simd::__bits::bfloat16x4x3_t;
using ::micron::simd::__bits::bfloat16x4x4_t;
using ::micron::simd::__bits::bfloat16x8_t;
using ::micron::simd::__bits::bfloat16x8x2_t;
using ::micron::simd::__bits::bfloat16x8x3_t;
using ::micron::simd::__bits::bfloat16x8x4_t;
#endif

#endif
