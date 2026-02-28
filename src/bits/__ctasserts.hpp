//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"


#if !defined(__micron_lang_cpp23 )
#error "Micron needs C++23 compiler support to compile properly"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compilers

#if defined(__micron_compiler_clang)
#define COMPILER "Clang/LLVM"
#elif defined(__micron_compiler_icc) || defined(__micron_compiler_icx)
#define COMPILER "Intel"
#elif defined(__micron_compiler_gcc)
#define GNUCC
#define COMPILER "GNU"
#if __micron_compiler_gcc_major != 15
#pragma GCC warning "This version of micron was made for GCC 15.x"
#endif
#elif defined(__micron_compiler_msvc)
#define COMPILER "MSVC"
#else
#define COMPILER "Unknown"
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// platforms
#if defined(__micron_os_windows)
#error "The micron standard library wasn't made for Windows."
#elif !defined(__micron_os_linux) && defined(__micron_os_posix)
#pragma GCC warning "Micron was developed for Linux, support on other UNIX-like kernels may not be stable!"
#endif

#if !defined(__micron_compiler_gcc_compat)
#error "Only gcc or clang are currently supported compilers. Remove this if you're willing to take risks."
#endif

#if !defined(__micron_endian_little)
#error                                                                                                                                     \
    "Micron was primarily developed and tested for little endian systems. It may, or may not run on big endian machines. Remove this error if you're willing to take risks."
#endif

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// arches
#if defined(__micron_arch_arm_any)
#pragma GCC warning "Limited support for ARM platforms currently. Be careful!"
#elif defined(__micron_arch_amd64)
#else
#error "This version of the Micron standard library is designed for amd64, with limited support for ARM platforms."
#endif

struct __ct_type_checker {
  static constexpr bool
  check()
  {
    // WARNING: these typechecks are absolutely architecturally critical; the entire library essentially assumes that the following
    // assertions are all true. if for some reason they aren't, everything breaks
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
    static_assert(sizeof(uint64_t) == sizeof(usize), "uint64_t should match usize");
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
    static_assert(sizeof(usize) == sizeof(void *), "usize must match pointer size");
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

  static constexpr bool
  check_posix()
  {
    static_assert(sizeof(__s16_type) == 2, "__s16_type must be 2 bytes");
    static_assert(sizeof(__u16_type) == 2, "__u16_type must be 2 bytes");
    static_assert(sizeof(__s32_type) == 4, "__s32_type must be 4 bytes");
    static_assert(sizeof(__u32_type) == 4, "__u32_type must be 4 bytes");
    static_assert(sizeof(__slongword_type) == sizeof(long), "__slongword_type must match long");
    static_assert(sizeof(__ulongword_type) == sizeof(unsigned long), "__ulongword_type must match unsigned long");

    static_assert(sizeof(__sword_type) == sizeof(ssize_t), "__sword_type must match ssize_t");
    static_assert(sizeof(__uword_type) == sizeof(usize), "__uword_type must match usize");
    static_assert(sizeof(__squad_type) == 8 || sizeof(__squad_type) == sizeof(long), "__squad_type must be 8 bytes or long");
    static_assert(sizeof(__uquad_type) == 8 || sizeof(__uquad_type) == sizeof(unsigned long),
                  "__uquad_type must be 8 bytes or unsigned long");

    static_assert(sizeof(__slong32_type) == 4, "__slong32_type must be 4 bytes");
    static_assert(sizeof(__ulong32_type) == 4, "__ulong32_type must be 4 bytes");
    static_assert(sizeof(__s64_type) == 8, "__s64_type must be 8 bytes");
    static_assert(sizeof(__u64_type) == 8, "__u64_type must be 8 bytes");

    static_assert(sizeof(__ptrdiff_type) == sizeof(ptrdiff_t), "__ptrdiff_type must match ptrdiff_t");
    static_assert(sizeof(__syscall_slong_type) == sizeof(long), "__syscall_slong_type must match long");
    static_assert(sizeof(__syscall_ulong_type) == sizeof(unsigned long), "__syscall_ulong_type must match unsigned long");

    static_assert(sizeof(__dev_t_type) == sizeof(__uquad_type), "__dev_t_type must match __uquad_type");
    static_assert(sizeof(__uid_t_type) == 4, "__uid_t_type must be 4 bytes");
    static_assert(sizeof(__gid_t_type) == 4, "__gid_t_type must be 4 bytes");
    static_assert(sizeof(__ino_t_type) == sizeof(__syscall_ulong_type), "__ino_t_type must match __syscall_ulong_type");
    static_assert(sizeof(__ino64_t_type) == sizeof(__uquad_type), "__ino64_t_type must match __uquad_type");
    static_assert(sizeof(__mode_t_type) == 4, "__mode_t_type must be 4 bytes");

    static_assert(sizeof(__nlink_t_type) == sizeof(__syscall_ulong_type), "__nlink_t_type must match __syscall_ulong_type");
    static_assert(sizeof(__fsword_t_type) == sizeof(__syscall_slong_type), "__fsword_t_type must match __syscall_slong_type");

    static_assert(sizeof(__off_t_type) == sizeof(__syscall_slong_type), "__off_t_type must match __syscall_slong_type");
    static_assert(sizeof(__off64_t_type) == sizeof(__squad_type), "__off64_t_type must match __squad_type");
    static_assert(sizeof(__pid_t_type) == 4, "__pid_t_type must be 4 bytes");

    static_assert(sizeof(__rlim_t_type) == sizeof(__syscall_ulong_type), "__rlim_t_type must match __syscall_ulong_type");
    static_assert(sizeof(__rlim64_t_type) == sizeof(__uquad_type), "__rlim64_t_type must match __uquad_type");

    static_assert(sizeof(__blkcnt_t_type) == sizeof(__syscall_slong_type), "__blkcnt_t_type must match __syscall_slong_type");
    static_assert(sizeof(__blkcnt64_t_type) == sizeof(__squad_type), "__blkcnt64_t_type must match __squad_type");

    static_assert(sizeof(__fsblkcnt_t_type) == sizeof(__syscall_ulong_type), "__fsblkcnt_t_type must match __syscall_ulong_type");
    static_assert(sizeof(__fsblkcnt64_t_type) == sizeof(__uquad_type), "__fsblkcnt64_t_type must match __uquad_type");

    static_assert(sizeof(__fsfilcnt_t_type) == sizeof(__syscall_ulong_type), "__fsfilcnt_t_type must match __syscall_ulong_type");
    static_assert(sizeof(__fsfilcnt64_t_type) == sizeof(__uquad_type), "__fsfilcnt64_t_type must match __uquad_type");

    static_assert(sizeof(__id_t_type) == 4, "__id_t_type must be 4 bytes");

    static_assert(sizeof(__clock_t_type) == sizeof(__syscall_slong_type), "__clock_t_type must match __syscall_slong_type");
    static_assert(sizeof(__clockid_t_type) == 4, "__clockid_t_type must be 4 bytes");
    static_assert(sizeof(__time_t_type) == sizeof(__syscall_slong_type), "__time_t_type must match __syscall_slong_type");

    static_assert(sizeof(__timer_t_type) == sizeof(void *), "__timer_t_type must match void*");

    static_assert(sizeof(__useconds_t_type) == 4, "__useconds_t_type must be 4 bytes");
    static_assert(sizeof(__suseconds_t_type) == sizeof(__syscall_slong_type), "__suseconds_t_type must match __syscall_slong_type");
    static_assert(sizeof(__suseconds64_t_type) == sizeof(__squad_type), "__suseconds64_t_type must match __squad_type");

    static_assert(sizeof(__daddr_t_type) == 4, "__daddr_t_type must be 4 bytes");
    static_assert(sizeof(__key_t_type) == 4, "__key_t_type must be 4 bytes");
    static_assert(sizeof(__blksize_t_type) == sizeof(__syscall_slong_type), "__blksize_t_type must match __syscall_slong_type");
    static_assert(sizeof(__ssize_t_type) == sizeof(__sword_type), "__ssize_t_type must match __sword_type");
    static_assert(sizeof(__cpu_mask_type) == sizeof(__syscall_ulong_type), "__cpu_mask_type must match __syscall_ulong_type");

    return true;
  }
};

// Compile-time trigger
constexpr bool __type_sizes_ok = __ct_type_checker::check();
constexpr bool __posix_type_sizes_ok = __ct_type_checker::check_posix();
