//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits-types.hpp"
#include "linux/linux_types.hpp"
#include "type_traits.hpp"

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;

typedef long int intptr_t;
typedef unsigned long int uintptr_t;
using ptr_t = uintptr_t;
using addr_t = uintptr_t;

// portable cstdint for gcc, pp macros

using int8_t = __INT8_TYPE__;
using int16_t = __INT16_TYPE__;
using int32_t = __INT32_TYPE__;
using int64_t = __INT64_TYPE__;

using intptr_t = __INTPTR_TYPE__;
using uint8_t = __UINT8_TYPE__;
using uint16_t = __UINT16_TYPE__;
using uint32_t = __UINT32_TYPE__;
using uint64_t = __UINT64_TYPE__;
using fint8_t = __UINT_FAST8_TYPE__;
using fint16_t = __UINT_FAST16_TYPE__;
using fint32_t = __UINT_FAST32_TYPE__;
using fint64_t = __UINT_FAST64_TYPE__;

using int_least8_t = int8_t;
using uint_least8_t = uint8_t;
using int_least16_t = int16_t;
using uint_least16_t = uint16_t;
using int_least32_t = int32_t;
using uint_least32_t = uint32_t;
using int_least64_t = int64_t;
using uint_least64_t = uint64_t;

using intmax_t = __INTMAX_TYPE__;
using uintmax_t = __UINTMAX_TYPE__;
using max_t = __INTMAX_TYPE__;
using umax_t = __UINTMAX_TYPE__;
using ssize_t = max_t;
using size_t = umax_t;
using word = umax_t;

// time block
using clock_t = __clock_t;
using clockid_t = __clockid_t;
using timer_t = __timer_t;
using suseconds_t = __suseconds_t;

using time_t = __time_t;
using time64_t = time_t;
using slong_t = __SYSCALL_SLONG_TYPE;
using ulong_t = __SYSCALL_ULONG_TYPE;
using quad_t = int64_t;
using uquad_t = uint64_t;

using kernel_long_t = long;
using kernel_ulong_t = unsigned long;
using kernel_ino_t = kernel_ulong_t;
using kernel_mode_t = unsigned int;
using kernel_pid_t = int;
using kernel_ipc_pid_t = int;
using kernel_uid32_t = unsigned int;
using kernel_gid32_t = unsigned int;
using kernel_uid_t = unsigned int;
using kernel_gid_t = unsigned int;
using kernel_old_uid_t = unsigned int;
using kernel_old_gid_t = unsigned int;
using kernel_suseconds_t = kernel_long_t;
using kernel_daddr_t = int;
using kernel_old_dev_t = unsigned int;

using kernel_size_t = kernel_ulong_t;
using kernel_ssize_t = kernel_long_t;
using kernel_ptrdiff_t = kernel_long_t;

typedef kernel_long_t kernel_off_t;
typedef long long kernel_loff_t;
typedef kernel_long_t kernel_old_time_t;
typedef kernel_long_t kernel_time_t;
typedef long long kernel_time64_t;
typedef kernel_long_t kernel_clock_t;
typedef int kernel_timer_t;
typedef int kernel_clockid_t;
typedef char *kernel_caddr_t;
typedef unsigned short kernel_uid16_t;
typedef unsigned short kernel_gid16_t;

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

typedef umax_t __attribute__((__may_alias__)) word;
constexpr u32 byte_width = 8;

using pnt_t = byte;
// using time_t = uint64_t;
// TODO: implement time.h

#if defined(__GNUC__) && !defined(__clang__) && defined(__cplusplus) && __cplusplus >= 202300L
using f16 = _Float16;
using f32 = _Float32;
using f64 = _Float64;
using f128 = _Float128;
#elif (defined(__clang__))
using f16 = float;
using f32 = float;
using f64 = double;
typedef long double f128;
#endif
typedef long double flong;
using ff = float;
using df = double;

using nullptr_t = decltype(nullptr);

#define naked_fn __attribute__((naked)) void
