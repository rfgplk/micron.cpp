//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#define __WORDSIZE 64     // only for 64bit

#define __S16_TYPE short int
#define __U16_TYPE unsigned short int
#define __S32_TYPE int
#define __U32_TYPE unsigned int
#define __SLONGWORD_TYPE long int
#define __ULONGWORD_TYPE unsigned long int
#if __WORDSIZE == 32
#define __SQUAD_TYPE int64_t
#define __UQUAD_TYPE uint64_t
#define __SWORD_TYPE int
#define __UWORD_TYPE unsigned int
#define __SLONG32_TYPE long int
#define __ULONG32_TYPE unsigned long int
#define __S64_TYPE int64_t
#define __U64_TYPE uint64_t
#elif __WORDSIZE == 64
#define __SQUAD_TYPE long int
#define __UQUAD_TYPE unsigned long int
#define __SWORD_TYPE long int
#define __UWORD_TYPE unsigned long int
#define __SLONG32_TYPE int
#define __ULONG32_TYPE unsigned int
#define __S64_TYPE long int
#define __U64_TYPE unsigned long int
#else
#error
#endif
#if defined __x86_64__ && defined __ILP32__
#define __SYSCALL_SLONG_TYPE __SQUAD_TYPE
#define __SYSCALL_ULONG_TYPE __UQUAD_TYPE
#else
#define __SYSCALL_SLONG_TYPE __SLONGWORD_TYPE
#define __SYSCALL_ULONG_TYPE __ULONGWORD_TYPE
#endif

#define __DEV_T_TYPE __UQUAD_TYPE
#define __UID_T_TYPE __U32_TYPE
#define __GID_T_TYPE __U32_TYPE
#define __INO_T_TYPE __SYSCALL_ULONG_TYPE
#define __INO64_T_TYPE __UQUAD_TYPE
#define __MODE_T_TYPE __U32_TYPE
#ifdef __x86_64__
#define __NLINK_T_TYPE __SYSCALL_ULONG_TYPE
#define __FSWORD_T_TYPE __SYSCALL_SLONG_TYPE
#else
#define __NLINK_T_TYPE __UWORD_TYPE
#define __FSWORD_T_TYPE __SWORD_TYPE
#endif
#define __OFF_T_TYPE __SYSCALL_SLONG_TYPE
#define __OFF64_T_TYPE __SQUAD_TYPE
#define __PID_T_TYPE __S32_TYPE
#define __RLIM_T_TYPE __SYSCALL_ULONG_TYPE
#define __RLIM64_T_TYPE __UQUAD_TYPE
#define __BLKCNT_T_TYPE __SYSCALL_SLONG_TYPE
#define __BLKCNT64_T_TYPE __SQUAD_TYPE
#define __FSBLKCNT_T_TYPE __SYSCALL_ULONG_TYPE
#define __FSBLKCNT64_T_TYPE __UQUAD_TYPE
#define __FSFILCNT_T_TYPE __SYSCALL_ULONG_TYPE
#define __FSFILCNT64_T_TYPE __UQUAD_TYPE
#define __ID_T_TYPE __U32_TYPE
#define __CLOCK_T_TYPE __SYSCALL_SLONG_TYPE
#define __TIME_T_TYPE __SYSCALL_SLONG_TYPE
#define __USECONDS_T_TYPE __U32_TYPE
#define __SUSECONDS_T_TYPE __SYSCALL_SLONG_TYPE
#define __SUSECONDS64_T_TYPE __SQUAD_TYPE
#define __DADDR_T_TYPE __S32_TYPE
#define __KEY_T_TYPE __S32_TYPE
#define __CLOCKID_T_TYPE __S32_TYPE
#define __TIMER_T_TYPE void *
#define __BLKSIZE_T_TYPE __SYSCALL_SLONG_TYPE
// #define __FSID_T_TYPE		struct { int __val[2]; }
#define __SSIZE_T_TYPE __SWORD_TYPE
#define __CPU_MASK_TYPE __SYSCALL_ULONG_TYPE

using __dev_t = __DEV_T_TYPE;     /* Type of device numbers.  */
using __uid_t = __UID_T_TYPE;     /* Type of user identifications.  */
using __gid_t = __GID_T_TYPE;     /* Type of group identifications.  */
using __ino_t = __INO_T_TYPE;     /* Type of file serial numbers.  */
using __ino64_t = __INO64_T_TYPE; /* Type of file serial numbers (LFS).*/
using __mode_t = __MODE_T_TYPE;   /* Type of file attribute bitmasks.  */
using __nlink_t = __NLINK_T_TYPE; /* Type of file link counts.  */
using __off_t = __OFF_T_TYPE;     /* Type of file sizes and offsets.  */
using __off64_t = __OFF64_T_TYPE; /* Type of file sizes and offsets (LFS).  */
using __pid_t = __PID_T_TYPE;     /* Type of process identifications.  */
// using  __fsid_t __FSID_T_TYPE;	/* Type of file system IDs.  */
using __clock_t = __CLOCK_T_TYPE; /* Type of CPU usage counts.  */
using __clockid_t = __CLOCKID_T_TYPE;
using __rlim_t = __RLIM_T_TYPE;           /* Type for resource measurement.  */
using __rlim64_t = __RLIM64_T_TYPE;       /* Type for resource measurement (LFS).  */
using __id_t = __ID_T_TYPE;               /* General type for IDs.  */
using __time_t = __TIME_T_TYPE;           /* Seconds since the Epoch.  */
using __timer_t = __TIMER_T_TYPE;         /* Seconds since the Epoch.  */
using __useconds_t = __USECONDS_T_TYPE;   /* Count of microseconds.  */
using __suseconds_t = __SUSECONDS_T_TYPE; /* Signed count of microseconds.  */
using __suseconds64_t = __SUSECONDS64_T_TYPE;

using __daddr_t = __DADDR_T_TYPE; /* The type of a disk address.  */
using __key_t = __KEY_T_TYPE;     /* Type of an IPC key.  */
