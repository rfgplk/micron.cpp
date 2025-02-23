#pragma once

#include <stdint.h>
#include <type_traits>


#if __WORDSIZE == 64
#ifndef __intptr_t_defined
typedef long int intptr_t;
typedef unsigned long int uintptr_t;
#define __intptr_t_defined
#endif
#endif

#if defined(__intptr_t_defined)
typedef uintptr_t ptr_t;
#endif


// portable cstdint for gcc, pp macros

typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;

typedef __INTPTR_TYPE__ intptr_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __UINT_FAST8_TYPE__ fint8_t;
typedef __UINT_FAST16_TYPE__ fint16_t;
typedef __UINT_FAST32_TYPE__ fint32_t;
typedef __UINT_FAST64_TYPE__ fint64_t;

typedef __INTMAX_TYPE__ max_t;
typedef __UINTMAX_TYPE__ umax_t;

typedef max_t ssize_t;
typedef umax_t size_t;
typedef umax_t word;

using quad_t = int64_t;
using uquad_t = uint64_t;

// using complex = _Complex;
// using imaginary = _Imaginary;

using c8 = char8_t;
using c16 = char16_t;
using c32 = char32_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using p8 = fint8_t;
using p16 = fint16_t;
using p32 = fint32_t;
using p64 = fint64_t;
using byte = uint8_t;
using p64 = uintptr_t;

typedef umax_t __attribute__ ((__may_alias__)) word;
constexpr u32 byte_width = 8;

using pnt_t = byte;

#if defined(__GNUC__) && !defined(__clang__) && defined(__cplusplus) && __cplusplus >= 202300L
typedef _Float16 f16;
typedef _Float32 f32;
typedef _Float64 f64;
typedef _Float128 f128;
#elif(defined(__clang__))
typedef float f16;
typedef float f32;
typedef double f64;
typedef long double f128;
#endif
typedef long double flong;
typedef float ff;
typedef double df;

