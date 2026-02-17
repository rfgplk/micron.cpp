//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"

#include "bits/__pause.hpp"

#include "bits-types.hpp"
#include "linux/sys/types.hpp"
#include "type_traits.hpp"

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;

// portable cstdint for gcc, pp macros

using int8_t = __INT8_TYPE__;
using int16_t = __INT16_TYPE__;
using int32_t = __INT32_TYPE__;
using int64_t = __INT64_TYPE__;

using ptrdiff_t = __PTRDIFF_TYPE__;
using intptr_t = __INTPTR_TYPE__;
using uintptr_t = __UINTPTR_TYPE__;
using uint8_t = __UINT8_TYPE__;
using uint16_t = __UINT16_TYPE__;
using uint32_t = __UINT32_TYPE__;
using uint64_t = __UINT64_TYPE__;
using fint8_t = __UINT_FAST8_TYPE__;
using fint16_t = __UINT_FAST16_TYPE__;
using fint32_t = __UINT_FAST32_TYPE__;
using fint64_t = __UINT_FAST64_TYPE__;

using ptr_t = uintptr_t;
using addr_t = uintptr_t;

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

// time block
using clock_t = __clock_t;
using clockid_t = __clockid_t;
using timer_t = __timer_t;
using suseconds_t = __suseconds_t;

using time_t = __time_t;
using time64_t = time_t;
using slong_t = __syscall_slong_type;
using ulong_t = __syscall_ulong_type;
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

using kernel_off_t = __off_t;                // kernel_long_t
using kernel_loff_t = __off64_t;             // long long
using kernel_old_time_t = __time_t;          // kernel_long_t
using kernel_time_t = __time_t;              // kernel_long_t
using kernel_time64_t = __suseconds64_t;     // long long
using kernel_clock_t = __clock_t;            // kernel_long_t
using kernel_timer_t = int;                  // int
using kernel_clockid_t = int;                // int
using kernel_caddr_t = char *;               // char*
using kernel_uid16_t = __uid_t_type;         // unsigned short
using kernel_gid16_t = __gid_t_type;         // unsigned short

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

struct __ct_type_checker {
  static constexpr bool
  check()
  {
    static_assert(sizeof(byte) == 1, "byte must be exactly 1 byte");
    static_assert(sizeof(byte) == sizeof(unsigned char), "byte must be exactly 1 unsigned char (check arch)");
    static_assert(sizeof(int8_t) == 1, "int8_t must be 1 byte");
    static_assert(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");
    static_assert(sizeof(int16_t) == 2, "int16_t must be 2 bytes");
    static_assert(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
    static_assert(sizeof(int32_t) == 4, "int32_t must be 4 bytes");
    static_assert(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
    static_assert(sizeof(int64_t) == 8, "int64_t must be 8 bytes");
    static_assert(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
    static_assert(sizeof(uint64_t) == sizeof(size_t), "uint64_t should match size_t");
    static_assert(sizeof(uint64_t) == sizeof(umax_t), "uint64_t should match umax_t");

    static_assert(sizeof(int8_t) == sizeof(i8), "int8_t must be equaequal to i8");
    static_assert(sizeof(uint8_t) == sizeof(u8), "uint8_t must be equal to u8");
    static_assert(sizeof(int16_t) == sizeof(i16), "int16_t must be equal to i16");
    static_assert(sizeof(uint16_t) == sizeof(u16), "uint16_t must be equal to u16");
    static_assert(sizeof(int32_t) == sizeof(i32), "int32_t must be equal to i32");
    static_assert(sizeof(uint32_t) == sizeof(u32), "uint32_t must be equal to u32");
    static_assert(sizeof(int64_t) == sizeof(i64), "int64_t must be equal to i64");
    static_assert(sizeof(uint64_t) == sizeof(u64), "uint64_t must be equal to u64");

    static_assert(sizeof(intptr_t) == sizeof(void *), "intptr_t must match pointer size");
    static_assert(sizeof(uintptr_t) == sizeof(void *), "uintptr_t must match pointer size");
    static_assert(sizeof(ptr_t) == sizeof(void *), "ptr_t must match pointer size");
    static_assert(sizeof(addr_t) == sizeof(void *), "addr_t must match pointer size");
    static_assert(sizeof(size_t) == sizeof(void *), "size_t must match pointer size");
    static_assert(sizeof(ssize_t) == sizeof(ptrdiff_t), "ssize_t must match ptrdiff_t");

#if __micron_arch_amd64 || __micron_arch_x86
    static_assert(sizeof(long) == (__micron_arch_amd64 ? 8 : 4), "long must match LP64/ILP32 model");
#elif __micron_arch_arm64
    static_assert(sizeof(long) == 8, "ARM64 long must be 64-bit");
#elif __micron_arch_arm32
    static_assert(sizeof(long) == 4, "ARM32 long must be 32-bit");
#endif

#if defined(__GNUC__) && !defined(__clang__) && __cplusplus >= 202300L
    static_assert(sizeof(f16) == 2, "_Float16 must be 2 bytes");
    static_assert(sizeof(f32) == 4, "_Float32 must be 4 bytes");
    static_assert(sizeof(f64) == 8, "_Float64 must be 8 bytes");
    static_assert(sizeof(f128) == 16, "_Float128 must be 16 bytes");
#elif defined(__clang__)
    static_assert(sizeof(f16) == 4, "f16 fallback must be 4 bytes (float)");
    static_assert(sizeof(f32) == 4, "f32 fallback must be 4 bytes (float)");
    static_assert(sizeof(f64) == 8, "f64 fallback must be 8 bytes (double)");
    static_assert(sizeof(f128) >= 8, "f128 fallback must be at least 8 bytes (long double)");
#else
    static_assert(sizeof(float) == 4, "float must be 4 bytes");
    static_assert(sizeof(double) == 8, "double must be 8 bytes");
#endif

    static_assert(sizeof(flong) >= 8, "long double must be at least 8 bytes");

    static_assert(sizeof(c8) == 1, "char8_t must be 1 byte");
    static_assert(sizeof(c16) == 2, "char16_t must be 2 bytes");
    static_assert(sizeof(c32) == 4, "char32_t must be 4 bytes");

    return true;
  }
};

// Compile-time trigger
constexpr bool __type_sizes_ok = __ct_type_checker::check();
