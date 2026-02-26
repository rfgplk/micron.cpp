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
#include "linux/sys/types.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include "io/__std.hpp"
#include "linux/sys/signal.hpp"

#include "version.hpp"

#include "bits/__ctasserts.hpp"

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

#if defined(_WIN32) || defined(_WIN64)
#error "The micron standard library wasn't made for Windows."
#endif

#if defined(__micron_arch_arm32) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#pragma GCC warning "Limited support for ARM platforms currently. Be careful!"
#elif defined(__micron_arch_amd64) || defined(_M_X64)
#else
#error "This version of the Micron standard library is designed for amd64, with limited support for ARM platforms."
#endif
#if !defined(__GNUC__) && !defined(__clang__)
#error "Only gcc or clang are currently supported compilers. Remove this if you're willing to take risks."
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
