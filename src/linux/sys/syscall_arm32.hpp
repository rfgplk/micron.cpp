//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"

// armv7-a Linux EABI syscall ABI (ILP32)
// syscall num: r7
// arg slot 1: r0
// arg slot 2: r1
// arg slot 3: r2
// arg slot 4: r3
// arg slot 5: r4
// arg slot 6: r5
// return:     r0

namespace micron
{
namespace __impl
{

template <typename T>
concept __syscall_arg
    = (micron::is_integral_v<T> || micron::is_pointer_v<T> || micron::is_enum_v<T> || micron::is_null_pointer_v<T>) && sizeof(T) <= 8;

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

template <__tt_size_t Slot>
consteval __tt_size_t
__count_slots() noexcept
{
  return Slot;
}

template <__tt_size_t Slot, typename A0, typename... Rest>
consteval __tt_size_t
__count_slots() noexcept
{
  if constexpr ( sizeof(A0) <= sizeof(long int) ) {
    return __count_slots<Slot + 1, Rest...>();
  } else {
    constexpr __tt_size_t __aligned = (Slot + 1) & ~__tt_size_t(1);
    return __count_slots<__aligned + 2, Rest...>();
  }
}

template <__tt_size_t Slot>
constexpr void
__fill(long int *[[maybe_unused]]) noexcept
{
}

template <__tt_size_t Slot, typename A0, typename... Rest>
constexpr void
__fill(long int *__slots, A0 __a0, Rest... __rest) noexcept
{
  if constexpr ( sizeof(A0) <= sizeof(long int) ) {
    __slots[Slot] = __coerce(__a0);
    __fill<Slot + 1>(__slots, __rest...);
  } else {
    constexpr __tt_size_t __aligned = (Slot + 1) & ~__tt_size_t(1);
    unsigned long long __v = static_cast<unsigned long long>(__a0);
    __slots[__aligned] = static_cast<long int>(__v & 0xFFFFFFFFull);
    __slots[__aligned + 1] = static_cast<long int>(__v >> 32);
    __fill<__aligned + 2>(__slots, __rest...);
  }
}

};     // namespace __impl

inline __attribute__((always_inline)) long int
__do_syscall(long int __n) noexcept
{
  register long int __r7 asm("r7") = __n;
  register long int __r0 asm("r0");
  asm volatile("svc #0" : "=r"(__r0) : "r"(__r7) : "memory");
  return __r0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1) noexcept
{
  register long int __r7 asm("r7") = __n;
  register long int __r0 asm("r0") = __a1;
  asm volatile("svc #0" : "+r"(__r0) : "r"(__r7) : "memory");
  return __r0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2) noexcept
{
  register long int __r7 asm("r7") = __n;
  register long int __r0 asm("r0") = __a1;
  register long int __r1 asm("r1") = __a2;
  asm volatile("svc #0" : "+r"(__r0) : "r"(__r7), "r"(__r1) : "memory");
  return __r0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3) noexcept
{
  register long int __r7 asm("r7") = __n;
  register long int __r0 asm("r0") = __a1;
  register long int __r1 asm("r1") = __a2;
  register long int __r2 asm("r2") = __a3;
  asm volatile("svc #0" : "+r"(__r0) : "r"(__r7), "r"(__r1), "r"(__r2) : "memory");
  return __r0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4) noexcept
{
  register long int __r7 asm("r7") = __n;
  register long int __r0 asm("r0") = __a1;
  register long int __r1 asm("r1") = __a2;
  register long int __r2 asm("r2") = __a3;
  register long int __r3 asm("r3") = __a4;
  asm volatile("svc #0" : "+r"(__r0) : "r"(__r7), "r"(__r1), "r"(__r2), "r"(__r3) : "memory");
  return __r0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4, long int __a5) noexcept
{
  register long int __r7 asm("r7") = __n;
  register long int __r0 asm("r0") = __a1;
  register long int __r1 asm("r1") = __a2;
  register long int __r2 asm("r2") = __a3;
  register long int __r3 asm("r3") = __a4;
  register long int __r4 asm("r4") = __a5;
  asm volatile("svc #0" : "+r"(__r0) : "r"(__r7), "r"(__r1), "r"(__r2), "r"(__r3), "r"(__r4) : "memory");
  return __r0;
}

inline __attribute__((always_inline)) long int
__do_syscall(long int __n, long int __a1, long int __a2, long int __a3, long int __a4, long int __a5, long int __a6) noexcept
{
  register long int __r7 asm("r7") = __n;
  register long int __r0 asm("r0") = __a1;
  register long int __r1 asm("r1") = __a2;
  register long int __r2 asm("r2") = __a3;
  register long int __r3 asm("r3") = __a4;
  register long int __r4 asm("r4") = __a5;
  register long int __r5 asm("r5") = __a6;
  asm volatile("svc #0" : "+r"(__r0) : "r"(__r7), "r"(__r1), "r"(__r2), "r"(__r3), "r"(__r4), "r"(__r5) : "memory");
  return __r0;
}

namespace __impl
{

template <__tt_size_t... Is>
inline __attribute__((always_inline)) long int
__dispatch_impl(long int __n, const long int *__s, micron::index_sequence<Is...>) noexcept
{
  return __do_syscall(__n, __s[Is]...);
}

template <__tt_size_t N>
inline __attribute__((always_inline)) long int
__dispatch(long int __n, const long int *__s) noexcept
{
  return __dispatch_impl(__n, __s, micron::make_index_sequence<N>{});
}

};     // namespace __impl

template <__impl::__syscall_arg... Args>
  requires(__impl::__count_slots<0, Args...>() <= 6)
inline __attribute__((always_inline)) long int
syscall(long int __n, Args... __args) noexcept
{
  long int __slots[6] = { 0, 0, 0, 0, 0, 0 };
  __impl::__fill<0>(__slots, __args...);
  return __impl::__dispatch<__impl::__count_slots<0, Args...>()>(__n, __slots);
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
