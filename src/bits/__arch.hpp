//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// this is here so we don't clutter the root dir

// TODO: change all code macros to use these defs

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
#define __micron_arch_amd64 1
#define __micron_arch_width_64 1
#define __wordsize 64
#define __syscall_wordsize 64
#elif defined(__i386__) || defined(_M_IX86) || defined(__ILP32__)
#define __micron_arch_x86 1
#define __micron_arch_width_32 1
#define __wordsize 32
#define __syscall_wordsize 32
#elif defined(__aarch64__) || defined(__AARCH64EL__) || defined(__AARCH64EB__)
#define __micron_arch_arm64 1
#define __micron_arch_width_64 1
#define __syscall_wordsize 64
#define __wordsize 64
#elif defined(__arm__) || defined(__thumb__) || defined(__ARMEL__) || defined(__ARMEB__) || defined(__ARM_ARCH)
#define __micron_arch_arm32 1
#define __micron_arch_width_32 1
#define __wordsize 32
#define __syscall_wordsize 32
#else
#error "Unsupported architecture. Is your compiler working properly?"
#endif
