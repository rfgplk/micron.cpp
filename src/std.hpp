//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "errno.hpp"
#include "cmpl.hpp"
#include "endian.hpp"
#include "control.hpp"
#include "types.hpp"
#include "type_traits.hpp"


#ifndef __MICRON
#define __MICRON
#define __MICRONTL
#endif

constexpr static const int MICRON_VERSION_MAJOR = 0x0;
constexpr static const int MICRON_VERSION_MINOR = 0x2;
constexpr static const int MICRON_VERSION_PATCH = 0x0;

#ifdef MICRON_VERSION_PP
#define MICRON_VERSION_MAJOR 0
#define MICRON_VERSION_MINOR 2
#define MICRON_VERSION_PATCH 0
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

#if defined(__clang__)
#define COMPILER "Clang/LLVM"
#elif defined(__ICC) || defined(__INTEL_COMPILER)
#define COMPILER "Intel"
#elif defined(__GNUC__) || defined(__GNUG__)
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#define GNUCC
#endif
#define COMPILER "GNU"
#if __GNUC__ != 15
#pragma GCC warning "This version of micron was made for GCC 15.0"
#endif
#elif defined(_MSC_VER)
#define COMPILER "MSVC"
#else
#define COMPILER "Unknown"
#endif

#if !defined(__x86_64__)
#error "This version of the Micron standard library is designed for amd64 only."
#endif
#if !defined(__GNUC__) && !defined(__clang__)
#error "Only G++ or Clang++ are the supported compilers. Remove this if you're willing to take risks."
#endif

typedef void (*sig_t)(int);
#define __MICRON_SELF_ASSIGNMENT_GUARD

// all the POSIX signals, in case they aren't defined

#ifndef SIG_ERR
#define SIG_ERR ((sig_t) - 1)
#endif

#ifndef SIG_DFL
#define SIG_DFL ((sig_t)0)
#endif

#ifndef SIG_IGN
#define SIG_IGN ((sig_t)1)
#endif

#ifndef SIG_HUP
#define SIG_HUP 1
#endif

#ifndef SIG_INT
#define SIG_INT 2
#endif

#ifndef SIG_QUIT
#define SIG_QUIT 3
#endif

#ifndef SIG_ILL
#define SIG_ILL 4
#endif

#ifndef SIG_TRAP
#define SIG_TRAP 5
#endif

#ifndef SIG_ABRT
#define SIG_ABRT 6
#endif

#ifndef SIG_IOT
#define SIG_IOT 6
#endif

#ifndef SIG_FPE
#define SIG_FPE 8
#endif

#ifndef SIG_KILL
#define SIG_KILL 9
#endif

#ifndef SIG_USR1
#define SIG_USR1 10
#endif
#ifndef SIG_SEGV
#define SIG_SEGV 11
#endif

#ifndef SIG_USR2
#define SIG_USR2 12
#endif

#ifndef SIG_PIPE
#define SIG_PIPE 13
#endif

#ifndef SIG_ALRM
#define SIG_ARLM 14
#endif

#ifndef SIG_TERM
#define SIG_TERM 15
#endif

#ifndef SIG_URG
#define SIG_URG 16
#endif

#ifndef SIG_CHLD
#define SIG_CHLD 17
#endif

#ifndef SIG_CONT
#define SIG_CONT 18
#endif

#ifndef SIG_STOP
#define SIG_STOP 19
#endif

#ifndef SIG_TSTP
#define SIG_TSTP 20
#endif

#ifndef SIG_TTIN
#define SIG_TTIN 21
#endif

#ifndef SIG_TTOU
#define SIG_TTOU 22
#endif

#ifndef SIG_URG
#define SIG_URG 23
#endif

#ifndef SIG_POLL
#define SIG_POLL 23
#endif

#ifndef SIG_XCPU
#define SIG_XCPU 24
#endif

#ifndef SIG_XFSZ
#define SIG_XFSZ 25
#endif

#ifndef SIG_VTALRM
#define SIG_VTALRM 26
#endif

#ifndef SIG_PROF
#define SIG_PROF 27
#endif

#ifndef SIG_WINCH
#define SIG_WINCH 28
#endif

#ifndef SIG_USR1
#define SIG_USR1 30
#endif

#ifndef SIG_USR2
#define SIG_USR2 31
#endif

template <typename T>
  requires micron::is_integral_v<T>
constexpr byte
B(T t)
{
  return (t);
}

template <typename T>
  requires micron::is_integral_v<T>
constexpr byte
KB(T t)
{
  return (t << 10);
}

template <typename T>
  requires micron::is_integral_v<T>
constexpr byte
MB(T t)
{
  return (t << 20);
}

template <typename T>
  requires micron::is_integral_v<T>
constexpr byte
GB(T t)
{
  return (t << 30);
}

namespace mc = micron;
