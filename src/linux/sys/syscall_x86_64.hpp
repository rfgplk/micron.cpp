//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"

// cpp23 rewrite

// amd64 abi
// syscall num: rax
// arg 1: rdi
// arg 2: rsi
// arg 3: rdx
// arg 4: r10  (NOT rcx kernel overwrites rcx with user RIP)
// arg 5: r8
// arg 6: r9
// clob: rcx   (user RIP across transition)
// clib: r11   (user RFLAGS across transition)

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

inline __attribute__((always_inline)) long
__do_syscall(long n) noexcept
{
  long ret;
  asm volatile("syscall" : "=a"(ret) : "a"(n) : "rcx", "r11", "memory");
  return ret;
}

inline __attribute__((always_inline)) long
__do_syscall(long n, long a1) noexcept
{
  long ret;
  asm volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
  return ret;
}

inline __attribute__((always_inline)) long
__do_syscall(long n, long a1, long a2) noexcept
{
  long ret;
  asm volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "memory");
  return ret;
}

inline __attribute__((always_inline)) long
__do_syscall(long n, long a1, long a2, long a3) noexcept
{
  long ret;
  asm volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
  return ret;
}

inline __attribute__((always_inline)) long
__do_syscall(long n, long a1, long a2, long a3, long a4) noexcept
{
  long ret;
  register long r10 asm("r10") = a4;

  asm volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10) : "rcx", "r11", "memory");
  return ret;
}

inline __attribute__((always_inline)) long
__do_syscall(long n, long a1, long a2, long a3, long a4, long a5) noexcept
{
  long ret;
  register long r10 asm("r10") = a4;
  register long r8 asm("r8") = a5;

  asm volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
  return ret;
}

inline __attribute__((always_inline)) long
__do_syscall(long n, long a1, long a2, long a3, long a4, long a5, long a6) noexcept
{
  long ret;
  register long r10 asm("r10") = a4;
  register long r8 asm("r8") = a5;
  register long r9 asm("r9") = a6;

  asm volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
  return ret;
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
