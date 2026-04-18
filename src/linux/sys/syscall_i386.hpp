//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"

// cpp23 rewrite

// i386 (x86-32, ILP32) Linux syscall abi
// syscall num: eax
// arg 1: ebx        (reserved as GOT under -fPIC)
// arg 2: ecx
// arg 3: edx
// arg 4: esi
// arg 5: edi
// arg 6: ebp        (reserved as frame ptr under -fno-omit-frame-pointer)
// return:  eax
// clob: memory, cc

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
  long int __r;
  asm volatile("int $0x80" : "=a"(__r) : "0"(__n) : "memory", "cc");
  return __r;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1) noexcept
{
  long int __r;
  asm volatile("xchgl %%ebx, %[a1]\n\t"
               "int   $0x80\n\t"
               "xchgl %%ebx, %[a1]"
               : "=a"(__r), [a1] "+r"(__a1)
               : "0"(__n)
               : "memory", "cc");
  return __r;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2) noexcept
{
  long int __r;
  register long int __r2 asm("ecx") = __a2;
  asm volatile("xchgl %%ebx, %[a1]\n\t"
               "int   $0x80\n\t"
               "xchgl %%ebx, %[a1]"
               : "=a"(__r), [a1] "+r"(__a1)
               : "0"(__n), "r"(__r2)
               : "memory", "cc");
  return __r;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3) noexcept
{
  long int __r;
  register long int __r2 asm("ecx") = __a2;
  register long int __r3 asm("edx") = __a3;
  asm volatile("xchgl %%ebx, %[a1]\n\t"
               "int   $0x80\n\t"
               "xchgl %%ebx, %[a1]"
               : "=a"(__r), [a1] "+r"(__a1)
               : "0"(__n), "r"(__r2), "r"(__r3)
               : "memory", "cc");
  return __r;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4) noexcept
{
  long int __r;
  register long int __r2 asm("ecx") = __a2;
  register long int __r3 asm("edx") = __a3;
  register long int __r4 asm("esi") = __a4;
  asm volatile("xchgl %%ebx, %[a1]\n\t"
               "int   $0x80\n\t"
               "xchgl %%ebx, %[a1]"
               : "=a"(__r), [a1] "+r"(__a1)
               : "0"(__n), "r"(__r2), "r"(__r3), "r"(__r4)
               : "memory", "cc");
  return __r;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4, long int __a5) noexcept
{
  long int __r;
  register long int __r2 asm("ecx") = __a2;
  register long int __r3 asm("edx") = __a3;
  register long int __r4 asm("esi") = __a4;
  register long int __r5 asm("edi") = __a5;
  asm volatile("xchgl %%ebx, %[a1]\n\t"
               "int   $0x80\n\t"
               "xchgl %%ebx, %[a1]"
               : "=a"(__r), [a1] "+r"(__a1)
               : "0"(__n), "r"(__r2), "r"(__r3), "r"(__r4), "r"(__r5)
               : "memory", "cc");
  return __r;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4, long int __a5, long int __a6) noexcept
{
  struct __args_t {
    long int a1, a6;
  };

  __args_t __s = { __a1, __a6 };
  long int __r;
  register long int __r2 asm("ecx") = __a2;
  register long int __r3 asm("edx") = __a3;
  register long int __r4 asm("esi") = __a4;
  register long int __r5 asm("edi") = __a5;
  asm volatile("pushl %%ebp\n\t"
               "pushl %%ebx\n\t"
               "pushl  (%[s])\n\t"     // push a1
               "pushl 4(%[s])\n\t"     // push a6
               "popl  %%ebp\n\t"       // ebp = a6
               "popl  %%ebx\n\t"       // ebx = a1
               "int   $0x80\n\t"
               "popl  %%ebx\n\t"
               "popl  %%ebp"
               : "=a"(__r)
               : "0"(__n), [s] "r"(&__s), "r"(__r2), "r"(__r3), "r"(__r4), "r"(__r5)
               : "memory", "cc");
  return __r;
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
