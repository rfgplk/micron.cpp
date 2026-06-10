//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// moved arch/wordsizes to here
#include "bits/__arch.hpp"

// translated everything to a cpp-style using system

using __s16_type = short int;
using __u16_type = unsigned short int;
using __s32_type = int;
using __u32_type = unsigned int;
using __slongword_type = long int;
using __ulongword_type = unsigned long int;

// for pthreads compatibility, we don't really use this at all

#ifdef __micron_arch_amd64
#if __wordsize == 64
constexpr static const int __sizeof_pthread_mutex_t = 40;
constexpr static const int __sizeof_pthread_attr_t = 56;
constexpr static const int __sizeof_pthread_rwlock_t = 56;
constexpr static const int __sizeof_pthread_barrier_t = 32;
#else
constexpr static const int __sizeof_pthread_mutex_t = 32;
constexpr static const int __sizeof_pthread_attr_t = 32;
constexpr static const int __sizeof_pthread_rwlock_t = 44;
constexpr static const int __sizeof_pthread_barrier_t = 20;
#endif
#elif defined(__micron_arch_arm64)
// glibc aarch64 sizes
constexpr static const int __sizeof_pthread_mutex_t = 48;
constexpr static const int __sizeof_pthread_attr_t = 64;
constexpr static const int __sizeof_pthread_rwlock_t = 56;
constexpr static const int __sizeof_pthread_barrier_t = 32;
#else
// i386 / arm32 (ILP32)
constexpr static const int __sizeof_pthread_mutex_t = 24;
constexpr static const int __sizeof_pthread_attr_t = 36;
constexpr static const int __sizeof_pthread_rwlock_t = 32;
constexpr static const int __sizeof_pthread_barrier_t = 20;
#endif
constexpr static const int __sizeof_pthread_mutexattr_t = 4;
constexpr static const int __sizeof_pthread_cond_t = 48;
constexpr static const int __sizeof_pthread_condattr_t = 4;
constexpr static const int __sizeof_pthread_rwlockattr_t = 8;
constexpr static const int __sizeof_pthread_barrierattr_t = 4;

#if __wordsize == 32
using __squad_type = __INT64_TYPE__;
using __uquad_type = __UINT64_TYPE__;
using __sword_type = int;
using __uword_type = unsigned int;
using __slong32_type = long int;
using __ulong32_type = unsigned long int;
using __s64_type = __INT64_TYPE__;
using __u64_type = __UINT64_TYPE__;
using __ptrdiff_type = signed int;
#elif __wordsize == 64
using __squad_type = long int;
using __uquad_type = unsigned long int;
using __sword_type = long int;
using __uword_type = unsigned long int;
using __slong32_type = int;
using __ulong32_type = unsigned int;
using __s64_type = long int;
using __u64_type = unsigned long int;
using __ptrdiff_type = signed long long;
#else
#error "__wordsize invalid"
#endif

#if defined(__micron_arch_amd64) && defined(__ILP32__)
using __syscall_slong_type = __squad_type;
using __syscall_ulong_type = __uquad_type;
#else
using __syscall_slong_type = __slongword_type;
using __syscall_ulong_type = __ulongword_type;
#endif

using __dev_t_type = __uquad_type;
using __uid_t_type = __u32_type;
using __gid_t_type = __u32_type;
using __ino_t_type = __uquad_type;
using __ino64_t_type = __uquad_type;
using __mode_t_type = __u32_type;

#ifdef __micron_arch_amd64
using __nlink_t_type = __syscall_ulong_type;
using __fsword_t_type = __syscall_slong_type;
#else
using __nlink_t_type = __u32_type;
using __fsword_t_type = __sword_type;
#endif

using __off_t_type = __squad_type;
using __off64_t_type = __squad_type;
using __pid_t_type = __s32_type;
using __rlim_t_type = __uquad_type;
using __rlim64_t_type = __uquad_type;
using __blkcnt_t_type = __squad_type;
using __blkcnt64_t_type = __squad_type;
using __fsblkcnt_t_type = __uquad_type;
using __fsblkcnt64_t_type = __uquad_type;
using __fsfilcnt_t_type = __uquad_type;
using __fsfilcnt64_t_type = __uquad_type;
using __id_t_type = __u32_type;
using __clock_t_type = __syscall_slong_type;      // stays word-size even under time64 (glibc __SLONGWORD_TYPE)
using __clockid_t_type = __s32_type;
using __time_t_type = __squad_type;      // time64
using __timer_t_type = void *;
using __useconds_t_type = __u32_type;
using __suseconds_t_type = __squad_type;
using __suseconds64_t_type = __squad_type;
using __daddr_t_type = __s32_type;
using __key_t_type = __s32_type;
using __blksize_t_type = __syscall_slong_type;      // page-size scale; word-size is fine
using __ssize_t_type = __sword_type;
using __cpu_mask_type = __syscall_ulong_type;      // cpu_set word == unsigned long (word-size)

// NOTE: do NOT re-alias these to glibc's reserved names (__off_t, __time_t, __nlink_t, ...)
