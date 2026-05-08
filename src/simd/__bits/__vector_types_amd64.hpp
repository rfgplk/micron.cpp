//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

#if !defined(__micron_arch_x86_any)
#error "__vector_types_amd64.hpp included on a non-x86 build"
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

// internal "v" types
typedef char __v8qi __attribute__((__vector_size__(8)));
typedef short __v4hi __attribute__((__vector_size__(8)));
typedef int __v2si __attribute__((__vector_size__(8)));
typedef long long __v1di __attribute__((__vector_size__(8)));
typedef float __v2sf __attribute__((__vector_size__(8)));

typedef char __v16qi __attribute__((__vector_size__(16)));
typedef short __v8hi __attribute__((__vector_size__(16)));
typedef int __v4si __attribute__((__vector_size__(16)));
typedef long long __v2di __attribute__((__vector_size__(16)));
typedef float __v4sf __attribute__((__vector_size__(16)));
typedef double __v2df __attribute__((__vector_size__(16)));

typedef char __v32qi __attribute__((__vector_size__(32)));
typedef short __v16hi __attribute__((__vector_size__(32)));
typedef int __v8si __attribute__((__vector_size__(32)));
typedef long long __v4di __attribute__((__vector_size__(32)));
typedef float __v8sf __attribute__((__vector_size__(32)));
typedef double __v4df __attribute__((__vector_size__(32)));

typedef char __v64qi __attribute__((__vector_size__(64)));
typedef short __v32hi __attribute__((__vector_size__(64)));
typedef int __v16si __attribute__((__vector_size__(64)));
typedef long long __v8di __attribute__((__vector_size__(64)));
typedef float __v16sf __attribute__((__vector_size__(64)));
typedef double __v8df __attribute__((__vector_size__(64)));

// unsigned variants
typedef unsigned char __v16qu __attribute__((__vector_size__(16)));
typedef unsigned short __v8hu __attribute__((__vector_size__(16)));
typedef unsigned int __v4su __attribute__((__vector_size__(16)));
typedef unsigned long long __v2du __attribute__((__vector_size__(16)));

typedef unsigned char __v32qu __attribute__((__vector_size__(32)));
typedef unsigned short __v16hu __attribute__((__vector_size__(32)));
typedef unsigned int __v8su __attribute__((__vector_size__(32)));
typedef unsigned long long __v4du __attribute__((__vector_size__(32)));

typedef unsigned char __v64qu __attribute__((__vector_size__(64)));
typedef unsigned short __v32hu __attribute__((__vector_size__(64)));
typedef unsigned int __v16su __attribute__((__vector_size__(64)));
typedef unsigned long long __v8du __attribute__((__vector_size__(64)));

// public "m" types
typedef int __m64 __attribute__((__vector_size__(8), __may_alias__));

typedef float __m128 __attribute__((__vector_size__(16), __may_alias__));
typedef double __m128d __attribute__((__vector_size__(16), __may_alias__));
typedef long long __m128i __attribute__((__vector_size__(16), __may_alias__));

typedef float __m256 __attribute__((__vector_size__(32), __may_alias__));
typedef double __m256d __attribute__((__vector_size__(32), __may_alias__));
typedef long long __m256i __attribute__((__vector_size__(32), __may_alias__));

typedef float __m512 __attribute__((__vector_size__(64), __may_alias__));
typedef double __m512d __attribute__((__vector_size__(64), __may_alias__));
typedef long long __m512i __attribute__((__vector_size__(64), __may_alias__));

// unaligned variants
typedef int __m64_u __attribute__((__vector_size__(8), __may_alias__, __aligned__(1)));

typedef float __m128_u __attribute__((__vector_size__(16), __may_alias__, __aligned__(1)));
typedef double __m128d_u __attribute__((__vector_size__(16), __may_alias__, __aligned__(1)));
typedef long long __m128i_u __attribute__((__vector_size__(16), __may_alias__, __aligned__(1)));

typedef float __m256_u __attribute__((__vector_size__(32), __may_alias__, __aligned__(1)));
typedef double __m256d_u __attribute__((__vector_size__(32), __may_alias__, __aligned__(1)));
typedef long long __m256i_u __attribute__((__vector_size__(32), __may_alias__, __aligned__(1)));

typedef float __m512_u __attribute__((__vector_size__(64), __may_alias__, __aligned__(1)));
typedef double __m512d_u __attribute__((__vector_size__(64), __may_alias__, __aligned__(1)));
typedef long long __m512i_u __attribute__((__vector_size__(64), __may_alias__, __aligned__(1)));

typedef float __x86_float_u __attribute__((__may_alias__, __aligned__(1)));
typedef double __x86_double_u __attribute__((__may_alias__, __aligned__(1)));

// half-precision (FP16)
#if defined(__FLT16_MAX__) || defined(__STDCPP_FLOAT16_T__)
typedef _Float16 __v8hf __attribute__((__vector_size__(16)));
typedef _Float16 __v16hf __attribute__((__vector_size__(32)));
typedef _Float16 __v32hf __attribute__((__vector_size__(64)));
typedef _Float16 __m128h __attribute__((__vector_size__(16), __may_alias__));
typedef _Float16 __m256h __attribute__((__vector_size__(32), __may_alias__));
typedef _Float16 __m512h __attribute__((__vector_size__(64), __may_alias__));
typedef _Float16 __m128h_u __attribute__((__vector_size__(16), __may_alias__, __aligned__(1)));
typedef _Float16 __m256h_u __attribute__((__vector_size__(32), __may_alias__, __aligned__(1)));
typedef _Float16 __m512h_u __attribute__((__vector_size__(64), __may_alias__, __aligned__(1)));
#endif

// brain-float (BF16)
#if defined(__BFLT16_MAX__) || defined(__BF16_MIN__) || defined(__AVX512BF16__) || defined(__AVX10_2__)
typedef __bf16 __v8bf __attribute__((__vector_size__(16)));
typedef __bf16 __v16bf __attribute__((__vector_size__(32)));
typedef __bf16 __v32bf __attribute__((__vector_size__(64)));
typedef __bf16 __m128bh __attribute__((__vector_size__(16), __may_alias__));
typedef __bf16 __m256bh __attribute__((__vector_size__(32), __may_alias__));
typedef __bf16 __m512bh __attribute__((__vector_size__(64), __may_alias__));
typedef __bf16 __bfloat16;
#endif

template <typename T> inline constexpr unsigned width_v = sizeof(T) * 8;

template <typename T, typename L> inline constexpr unsigned lane_count_v = sizeof(T) / sizeof(L);

template <typename T>
concept v128 = (sizeof(T) == 16);

template <typename T>
concept v256 = (sizeof(T) == 32);

template <typename T>
concept v512 = (sizeof(T) == 64);

template <typename T>
concept v64 = (sizeof(T) == 8);

static_assert(sizeof(__m64) == 8);
static_assert(sizeof(__m128) == 16 && alignof(__m128) == 16);
static_assert(sizeof(__m128d) == 16 && alignof(__m128d) == 16);
static_assert(sizeof(__m128i) == 16 && alignof(__m128i) == 16);
static_assert(sizeof(__m256) == 32);
static_assert(sizeof(__m256d) == 32);
static_assert(sizeof(__m256i) == 32);
static_assert(sizeof(__m512) == 64);
static_assert(sizeof(__m512d) == 64);
static_assert(sizeof(__m512i) == 64);

#if defined(__micron_x86_avx2) || defined(__AVX2__)
static_assert(alignof(__m256) == 32);
static_assert(alignof(__m256d) == 32);
static_assert(alignof(__m256i) == 32);
#endif

#if defined(__micron_x86_avx512f) || defined(__AVX512F__)
static_assert(alignof(__m512) == 64);
static_assert(alignof(__m512d) == 64);
static_assert(alignof(__m512i) == 64);
#endif

static_assert(alignof(__m128i_u) == 1);
static_assert(alignof(__m256i_u) == 1);
static_assert(alignof(__m512i_u) == 1);
static_assert(sizeof(__m128i_u) == 16);
static_assert(sizeof(__m256i_u) == 32);
static_assert(sizeof(__m512i_u) == 64);

static_assert(sizeof(__v16qi) == 16 && sizeof(__v32qi) == 32 && sizeof(__v64qi) == 64);
static_assert(sizeof(__v8hi) == 16 && sizeof(__v16hi) == 32 && sizeof(__v32hi) == 64);
static_assert(sizeof(__v4si) == 16 && sizeof(__v8si) == 32 && sizeof(__v16si) == 64);
static_assert(sizeof(__v2di) == 16 && sizeof(__v4di) == 32 && sizeof(__v8di) == 64);
static_assert(sizeof(__v4sf) == 16 && sizeof(__v8sf) == 32 && sizeof(__v16sf) == 64);
static_assert(sizeof(__v2df) == 16 && sizeof(__v4df) == 32 && sizeof(__v8df) == 64);

#pragma GCC diagnostic pop

};     // namespace __bits
};     // namespace simd
};     // namespace micron

#if defined(MICRON_SIMD_INJECT_INTRIN_TYPES)

using ::micron::simd::__bits::__m128;
using ::micron::simd::__bits::__m128d;
using ::micron::simd::__bits::__m128i;
using ::micron::simd::__bits::__m256;
using ::micron::simd::__bits::__m256d;
using ::micron::simd::__bits::__m256i;
using ::micron::simd::__bits::__m512;
using ::micron::simd::__bits::__m512d;
using ::micron::simd::__bits::__m512i;
using ::micron::simd::__bits::__m64;

using ::micron::simd::__bits::__m128_u;
using ::micron::simd::__bits::__m128d_u;
using ::micron::simd::__bits::__m128i_u;
using ::micron::simd::__bits::__m256_u;
using ::micron::simd::__bits::__m256d_u;
using ::micron::simd::__bits::__m256i_u;
using ::micron::simd::__bits::__m512_u;
using ::micron::simd::__bits::__m512d_u;
using ::micron::simd::__bits::__m512i_u;
using ::micron::simd::__bits::__m64_u;

#if defined(__FLT16_MAX__) || defined(__STDCPP_FLOAT16_T__)
using ::micron::simd::__bits::__m128h;
using ::micron::simd::__bits::__m128h_u;
using ::micron::simd::__bits::__m256h;
using ::micron::simd::__bits::__m256h_u;
using ::micron::simd::__bits::__m512h;
using ::micron::simd::__bits::__m512h_u;
#endif

#if defined(__BFLT16_MAX__) || defined(__BF16_MIN__) || defined(__AVX512BF16__) || defined(__AVX10_2__)
using ::micron::simd::__bits::__bfloat16;
using ::micron::simd::__bits::__m128bh;
using ::micron::simd::__bits::__m256bh;
using ::micron::simd::__bits::__m512bh;
#endif

#endif
