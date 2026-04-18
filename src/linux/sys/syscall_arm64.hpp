//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"

// NOTE: currently unused/untested (no aarch64 support so far, just a placeholder)

// aarch64 (ARMv8-A) Linux syscall ABI
// syscall num: x8
// arg 1: x0
// arg 2: x1
// arg 3: x2
// arg 4: x3
// arg 5: x4
// arg 6: x5
// return:  x0

namespace micron
{

namespace __impl
{

template <typename T>
concept __syscall_arg = (micron::is_integral_v<T> || micron::is_pointer_v<T> || micron::is_enum_v<T> || micron::is_null_pointer_v<T>)
                        && sizeof(T) <= sizeof(long int);

template <__syscall_arg T>
inline __attribute__((always_inline)) long int
__coerce(T __v) noexcept
{
  if constexpr ( micron::is_null_pointer_v<T> )
    return 0L;
  else if constexpr ( micron::is_pointer_v<T> )
    return static_cast<long int>(reinterpret_cast<__UINTPTR_TYPE__>(__v));
  else
    return static_cast<long int>(__v);
}

};     // namespace __impl

inline __attribute__((always_inline)) long int
__do_syscall(long int __n) noexcept
{
  register long int __x8 asm("x8") = __n;
  register long int __x0 asm("x0");
  asm volatile("svc #0" : "=r"(__x0) : "r"(__x8) : "memory");
  return __x0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1) noexcept
{
  register long int __x8 asm("x8") = __n;
  register long int __x0 asm("x0") = __a1;
  asm volatile("svc #0" : "+r"(__x0) : "r"(__x8) : "memory");
  return __x0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2) noexcept
{
  register long int __x8 asm("x8") = __n;
  register long int __x0 asm("x0") = __a1;
  register long int __x1 asm("x1") = __a2;
  asm volatile("svc #0" : "+r"(__x0) : "r"(__x8), "r"(__x1) : "memory");
  return __x0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3) noexcept
{
  register long int __x8 asm("x8") = __n;
  register long int __x0 asm("x0") = __a1;
  register long int __x1 asm("x1") = __a2;
  register long int __x2 asm("x2") = __a3;
  asm volatile("svc #0" : "+r"(__x0) : "r"(__x8), "r"(__x1), "r"(__x2) : "memory");
  return __x0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4) noexcept
{
  register long int __x8 asm("x8") = __n;
  register long int __x0 asm("x0") = __a1;
  register long int __x1 asm("x1") = __a2;
  register long int __x2 asm("x2") = __a3;
  register long int __x3 asm("x3") = __a4;
  asm volatile("svc #0" : "+r"(__x0) : "r"(__x8), "r"(__x1), "r"(__x2), "r"(__x3) : "memory");
  return __x0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4, long int __a5) noexcept
{
  register long int __x8 asm("x8") = __n;
  register long int __x0 asm("x0") = __a1;
  register long int __x1 asm("x1") = __a2;
  register long int __x2 asm("x2") = __a3;
  register long int __x3 asm("x3") = __a4;
  register long int __x4 asm("x4") = __a5;
  asm volatile("svc #0" : "+r"(__x0) : "r"(__x8), "r"(__x1), "r"(__x2), "r"(__x3), "r"(__x4) : "memory");
  return __x0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4, long int __a5, long int __a6) noexcept
{
  register long int __x8 asm("x8") = __n;
  register long int __x0 asm("x0") = __a1;
  register long int __x1 asm("x1") = __a2;
  register long int __x2 asm("x2") = __a3;
  register long int __x3 asm("x3") = __a4;
  register long int __x4 asm("x4") = __a5;
  register long int __x5 asm("x5") = __a6;
  asm volatile("svc #0" : "+r"(__x0) : "r"(__x8), "r"(__x1), "r"(__x2), "r"(__x3), "r"(__x4), "r"(__x5) : "memory");
  return __x0;
}

template <__impl::__syscall_arg... Args>
  requires(sizeof...(Args) <= 6)
inline __attribute__((always_inline)) long int
syscall(long int __n, Args... __args) noexcept
{
  return __do_syscall(__n, __impl::__coerce(__args)...);
}

inline __attribute__((always_inline)) bool
syscall_failed(long int __r) noexcept
{
  return static_cast<unsigned long int>(__r) >= static_cast<unsigned long int>(-4095L);
}

inline __attribute__((always_inline)) int
syscall_errno(long int __r) noexcept
{
  return static_cast<int>(-__r);
}

};     // namespace micron
