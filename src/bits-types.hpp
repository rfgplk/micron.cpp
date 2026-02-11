//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// translated everything to a cpp-style using system

#if defined(__x86_64__) && !defined(__ILP32__)
#define __WORDSIZE 64
#define __SYSCALL_WORDSIZE 64
#else
#define __WORDSIZE 32     // add experimental 32-bit support
#endif

using __s16_type = short int;
using __u16_type = unsigned short int;
using __s32_type = int;
using __u32_type = unsigned int;
using __slongword_type = long int;
using __ulongword_type = unsigned long int;

// for pthreads compatibility, we don't really use this at all

#ifdef __x86_64__
#if __WORDSIZE == 64
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
#else
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

#if __WORDSIZE == 32
using __squad_type = __INT64_TYPE__;
using __uquad_type = __UINT64_TYPE__;
using __sword_type = int;
using __uword_type = unsigned int;
using __slong32_type = long int;
using __ulong32_type = unsigned long int;
using __s64_type = __INT64_TYPE__;
using __u64_type = __UINT64_TYPE__;
using __ptrdiff_type = signed int;
#elif __WORDSIZE == 64
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
#error "__WORDSIZE invalid"
#endif

#if defined(__x86_64__) && defined(__ILP32__)
using __syscall_slong_type = __squad_type;
using __syscall_ulong_type = __uquad_type;
#else
using __syscall_slong_type = __slongword_type;
using __syscall_ulong_type = __ulongword_type;
#endif

using __dev_t_type = __uquad_type;
using __uid_t_type = __u32_type;
using __gid_t_type = __u32_type;
using __ino_t_type = __syscall_ulong_type;
using __ino64_t_type = __uquad_type;
using __mode_t_type = __u32_type;

#ifdef __x86_64__
using __nlink_t_type = __syscall_ulong_type;
using __fsword_t_type = __syscall_slong_type;
#else
using __nlink_t_type = __uword_type;
using __fsword_t_type = __sword_type;
#endif

using __off_t_type = __syscall_slong_type;
using __off64_t_type = __squad_type;
using __pid_t_type = __s32_type;
using __rlim_t_type = __syscall_ulong_type;
using __rlim64_t_type = __uquad_type;
using __blkcnt_t_type = __syscall_slong_type;
using __blkcnt64_t_type = __squad_type;
using __fsblkcnt_t_type = __syscall_ulong_type;
using __fsblkcnt64_t_type = __uquad_type;
using __fsfilcnt_t_type = __syscall_ulong_type;
using __fsfilcnt64_t_type = __uquad_type;
using __id_t_type = __u32_type;
using __clock_t_type = __syscall_slong_type;
using __clockid_t_type = __s32_type;
using __time_t_type = __syscall_slong_type;
using __timer_t_type = void *;
using __useconds_t_type = __u32_type;
using __suseconds_t_type = __syscall_slong_type;
using __suseconds64_t_type = __squad_type;
using __daddr_t_type = __s32_type;
using __key_t_type = __s32_type;
using __blksize_t_type = __syscall_slong_type;
using __ssize_t_type = __sword_type;
using __cpu_mask_type = __syscall_ulong_type;

using __dev_t = __dev_t_type;     /* Type of device numbers.  */
using __uid_t = __uid_t_type;     /* Type of user identifications.  */
using __gid_t = __gid_t_type;     /* Type of group identifications.  */
using __ino_t = __ino_t_type;     /* Type of file serial numbers.  */
using __ino64_t = __ino64_t_type; /* Type of file serial numbers (LFS).*/
using __mode_t = __mode_t_type;   /* Type of file attribute bitmasks.  */
using __nlink_t = __nlink_t_type; /* Type of file link counts.  */
using __off_t = __off_t_type;     /* Type of file sizes and offsets.  */
using __off64_t = __off64_t_type; /* Type of file sizes and offsets (LFS).  */
using __pid_t = __pid_t_type;     /* Type of process identifications.  */
// using  __fsid_t __fsid_t_type;	/* Type of file system IDs.  */
using __clock_t = __clock_t_type; /* Type of CPU usage counts.  */
using __clockid_t = __clockid_t_type;
using __rlim_t = __rlim_t_type;           /* Type for resource measurement.  */
using __rlim64_t = __rlim64_t_type;       /* Type for resource measurement (LFS).  */
using __id_t = __id_t_type;               /* General type for IDs.  */
using __time_t = __time_t_type;           /* Seconds since the Epoch.  */
using __timer_t = __timer_t_type;         /* Seconds since the Epoch.  */
using __useconds_t = __useconds_t_type;   /* Count of microseconds.  */
using __suseconds_t = __suseconds_t_type; /* Signed count of microseconds.  */
using __suseconds64_t = __suseconds64_t_type;

using __daddr_t = __daddr_t_type; /* The type of a disk address.  */
using __key_t = __key_t_type;     /* Type of an IPC key.  */
