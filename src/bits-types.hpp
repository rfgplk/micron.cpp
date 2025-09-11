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
#define __STD_TYPE __extension__ typedef
#elif __WORDSIZE == 64
#define __SQUAD_TYPE long int
#define __UQUAD_TYPE unsigned long int
#define __SWORD_TYPE long int
#define __UWORD_TYPE unsigned long int
#define __SLONG32_TYPE int
#define __ULONG32_TYPE unsigned int
#define __S64_TYPE long int
#define __U64_TYPE unsigned long int
#define __STD_TYPE typedef
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

__STD_TYPE __DEV_T_TYPE __dev_t;     /* Type of device numbers.  */
__STD_TYPE __UID_T_TYPE __uid_t;     /* Type of user identifications.  */
__STD_TYPE __GID_T_TYPE __gid_t;     /* Type of group identifications.  */
__STD_TYPE __INO_T_TYPE __ino_t;     /* Type of file serial numbers.  */
__STD_TYPE __INO64_T_TYPE __ino64_t; /* Type of file serial numbers (LFS).*/
__STD_TYPE __MODE_T_TYPE __mode_t;   /* Type of file attribute bitmasks.  */
__STD_TYPE __NLINK_T_TYPE __nlink_t; /* Type of file link counts.  */
__STD_TYPE __OFF_T_TYPE __off_t;     /* Type of file sizes and offsets.  */
__STD_TYPE __OFF64_T_TYPE __off64_t; /* Type of file sizes and offsets (LFS).  */
__STD_TYPE __PID_T_TYPE __pid_t;     /* Type of process identifications.  */
//__STD_TYPE __FSID_T_TYPE __fsid_t;	/* Type of file system IDs.  */
__STD_TYPE __CLOCK_T_TYPE __clock_t; /* Type of CPU usage counts.  */
__STD_TYPE __CLOCKID_T_TYPE __clockid_t;
__STD_TYPE __RLIM_T_TYPE __rlim_t;           /* Type for resource measurement.  */
__STD_TYPE __RLIM64_T_TYPE __rlim64_t;       /* Type for resource measurement (LFS).  */
__STD_TYPE __ID_T_TYPE __id_t;               /* General type for IDs.  */
__STD_TYPE __TIME_T_TYPE __time_t;           /* Seconds since the Epoch.  */
__STD_TYPE __TIMER_T_TYPE __timer_t;         /* Seconds since the Epoch.  */
__STD_TYPE __USECONDS_T_TYPE __useconds_t;   /* Count of microseconds.  */
__STD_TYPE __SUSECONDS_T_TYPE __suseconds_t; /* Signed count of microseconds.  */
__STD_TYPE __SUSECONDS64_T_TYPE __suseconds64_t;

__STD_TYPE __DADDR_T_TYPE __daddr_t; /* The type of a disk address.  */
__STD_TYPE __KEY_T_TYPE __key_t;     /* Type of an IPC key.  */

#undef __STD_TYPE
