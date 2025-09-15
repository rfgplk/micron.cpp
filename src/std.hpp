//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "attributes.hpp"

#include "cmpl.hpp"
#include "control.hpp"
#include "endian.hpp"
#include "errno.hpp"
#include "type_traits.hpp"
#include "types.hpp"
#include "linux/linux_types.hpp"

#include "io/__std.hpp"
#include "linux/sys/signal.hpp"

#ifndef __MICRON
#define __MICRON
#define __MICRONTL
#endif

constexpr static const int MICRON_VERSION_MAJOR = 0x000;
constexpr static const int MICRON_VERSION_MINOR = 0x030;
constexpr static const int MICRON_VERSION_PATCH = 0x000;

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
#pragma GCC warning "This version of micron was made for GCC 15.x"
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
