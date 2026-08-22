//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

// intrinsics mapped to fn's
namespace micron
{
constexpr static const i32 atomic_relaxed = __ATOMIC_RELAXED;
constexpr static const i32 atomic_seq_cst = __ATOMIC_SEQ_CST;
constexpr static const i32 atomic_consume = __ATOMIC_CONSUME;
constexpr static const i32 atomic_acquire = __ATOMIC_ACQUIRE;
constexpr static const i32 atomic_release = __ATOMIC_RELEASE;
constexpr static const i32 atomic_acq_rel = __ATOMIC_ACQ_REL;
// gcc only
#if defined(__micron_arch_amd64) && defined(__micron_compiler_gcc)
constexpr static const i32 atomic_hle_acquire = __ATOMIC_HLE_ACQUIRE;
constexpr static const i32 atomic_hle_release = __ATOMIC_HLE_RELEASE;
#endif

namespace atom
{
#if defined(__clang__) && defined(__micron_arch_x86)
typedef unsigned long long __clang_i386_atomic_word __attribute__((aligned(8)));

template<typename T>
constexpr __attribute__((always_inline)) inline __clang_i386_atomic_word
__clang_i386_atomic_bits(T value) noexcept
{
  return __builtin_bit_cast(__clang_i386_atomic_word, value);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
__clang_i386_atomic_value(__clang_i386_atomic_word value) noexcept
{
  return __builtin_bit_cast(T, value);
}

template<typename T>
__attribute__((always_inline)) inline T
__clang_i386_atomic_load(const T *ptr) noexcept
{
  auto *word = reinterpret_cast<volatile __clang_i386_atomic_word *>(const_cast<T *>(ptr));
  return __clang_i386_atomic_value<T>(__sync_val_compare_and_swap(word, 0, 0));
}

template<typename T>
__attribute__((always_inline)) inline T
__clang_i386_atomic_exchange(T *ptr, T value) noexcept
{
  auto *word = reinterpret_cast<volatile __clang_i386_atomic_word *>(ptr);
  return __clang_i386_atomic_value<T>(__sync_lock_test_and_set(word, __clang_i386_atomic_bits(value)));
}

template<typename T>
__attribute__((always_inline)) inline bool
__clang_i386_atomic_compare_exchange(T *ptr, T *expected, T desired) noexcept
{
  auto *word = reinterpret_cast<volatile __clang_i386_atomic_word *>(ptr);
  const __clang_i386_atomic_word wanted = __clang_i386_atomic_bits(*expected);
  const __clang_i386_atomic_word observed = __sync_val_compare_and_swap(word, wanted, __clang_i386_atomic_bits(desired));
  if ( observed == wanted ) return true;
  *expected = __clang_i386_atomic_value<T>(observed);
  return false;
}

enum class __clang_i386_atomic_op { add, sub, bit_and, bit_xor, bit_or, bit_nand };

template<bool ReturnOld, __clang_i386_atomic_op Op, typename T>
__attribute__((always_inline)) inline T
__clang_i386_atomic_rmw(T *ptr, T value) noexcept
{
  auto *word = reinterpret_cast<volatile __clang_i386_atomic_word *>(ptr);
  const __clang_i386_atomic_word operand = __clang_i386_atomic_bits(value);
  __clang_i386_atomic_word old = __sync_val_compare_and_swap(word, 0, 0);
  for ( ;; ) {
    __clang_i386_atomic_word next;
    if constexpr ( Op == __clang_i386_atomic_op::add )
      next = old + operand;
    else if constexpr ( Op == __clang_i386_atomic_op::sub )
      next = old - operand;
    else if constexpr ( Op == __clang_i386_atomic_op::bit_and )
      next = old & operand;
    else if constexpr ( Op == __clang_i386_atomic_op::bit_xor )
      next = old ^ operand;
    else if constexpr ( Op == __clang_i386_atomic_op::bit_or )
      next = old | operand;
    else
      next = ~(old & operand);
    const __clang_i386_atomic_word observed = __sync_val_compare_and_swap(word, old, next);
    if ( observed == old ) return __clang_i386_atomic_value<T>(ReturnOld ? old : next);
    old = observed;
  }
}
#endif

template<typename T>
constexpr __attribute__((always_inline)) inline T
load(const T *ptr, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_load(ptr);
#endif
#if defined(__clang__)
  if constexpr ( __is_enum(T) ) {
    T value;
    __atomic_load(ptr, &value, memorder);
    return value;
  } else
#endif
    return __atomic_load_n(ptr, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline void
store(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) {
    (void)__clang_i386_atomic_exchange(ptr, val);
    return;
  }
#endif
#if defined(__clang__)
  if constexpr ( __is_enum(T) ) {
    __atomic_store(ptr, &val, memorder);
  } else
#endif
    __atomic_store_n(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
exchange(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_exchange(ptr, val);
#endif
#if defined(__clang__)
  if constexpr ( __is_enum(T) ) {
    T result;
    __atomic_exchange(ptr, &val, &result, memorder);
    return result;
  } else
#endif
    return __atomic_exchange_n(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline bool
cmp_exchange_strong(T *ptr, T *expected, T desired)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_compare_exchange(ptr, expected, desired);
#endif
  return __atomic_compare_exchange_n(ptr, expected, desired, false, atomic_seq_cst, atomic_seq_cst);
}

template<typename T>
constexpr __attribute__((always_inline)) inline bool
compare_exchange_strong(T *ptr, T *expected, T desired)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_compare_exchange(ptr, expected, desired);
#endif
  return __atomic_compare_exchange_n(ptr, expected, desired, false, atomic_seq_cst, atomic_seq_cst);
}

template<typename T>
constexpr __attribute__((always_inline)) inline bool
cmp_exchange_weak(T *ptr, T *expected, T desired)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_compare_exchange(ptr, expected, desired);
#endif
  return __atomic_compare_exchange_n(ptr, expected, desired, true, atomic_seq_cst, atomic_seq_cst);
}

template<typename T>
constexpr __attribute__((always_inline)) inline bool
compare_exchange_weak(T *ptr, T *expected, T desired)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_compare_exchange(ptr, expected, desired);
#endif
  return __atomic_compare_exchange_n(ptr, expected, desired, true, atomic_seq_cst, atomic_seq_cst);
}

template<typename T>
constexpr __attribute__((always_inline)) inline bool
cmp_exchange(T *ptr, T *expected, T desired, bool weak, i32 success_memorder, i32 failure_memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_compare_exchange(ptr, expected, desired);
#endif
#if defined(__clang__)
  if constexpr ( __is_enum(T) )
    return __atomic_compare_exchange(ptr, expected, &desired, weak, success_memorder, failure_memorder);
  else
#endif
    return __atomic_compare_exchange_n(ptr, expected, desired, weak, success_memorder, failure_memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline bool
compare_exchange(T *ptr, T *expected, T desired, bool weak, i32 success_memorder, i32 failure_memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) ) return __clang_i386_atomic_compare_exchange(ptr, expected, desired);
#endif
#if defined(__clang__)
  if constexpr ( __is_enum(T) )
    return __atomic_compare_exchange(ptr, expected, &desired, weak, success_memorder, failure_memorder);
  else
#endif
    return __atomic_compare_exchange_n(ptr, expected, desired, weak, success_memorder, failure_memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
add_fetch(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<false, __clang_i386_atomic_op::add>(ptr, val);
#endif
  return __atomic_add_fetch(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
sub_fetch(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<false, __clang_i386_atomic_op::sub>(ptr, val);
#endif
  return __atomic_sub_fetch(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
and_fetch(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<false, __clang_i386_atomic_op::bit_and>(ptr, val);
#endif
  return __atomic_and_fetch(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
xor_fetch(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<false, __clang_i386_atomic_op::bit_xor>(ptr, val);
#endif
  return __atomic_xor_fetch(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
or_fetch(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<false, __clang_i386_atomic_op::bit_or>(ptr, val);
#endif
  return __atomic_or_fetch(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
nand_fetch(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<false, __clang_i386_atomic_op::bit_nand>(ptr, val);
#endif
  return __atomic_nand_fetch(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
fetch_add(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<true, __clang_i386_atomic_op::add>(ptr, val);
#endif
  return __atomic_fetch_add(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
fetch_sub(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<true, __clang_i386_atomic_op::sub>(ptr, val);
#endif
  return __atomic_fetch_sub(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
fetch_and(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<true, __clang_i386_atomic_op::bit_and>(ptr, val);
#endif
  return __atomic_fetch_and(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
fetch_xor(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<true, __clang_i386_atomic_op::bit_xor>(ptr, val);
#endif
  return __atomic_fetch_xor(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
fetch_or(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<true, __clang_i386_atomic_op::bit_or>(ptr, val);
#endif
  return __atomic_fetch_or(ptr, val, memorder);
}

template<typename T>
constexpr __attribute__((always_inline)) inline T
fetch_nand(T *ptr, T val, i32 memorder)
{
#if defined(__clang__) && defined(__micron_arch_x86)
  if constexpr ( sizeof(T) == sizeof(__clang_i386_atomic_word) )
    return __clang_i386_atomic_rmw<true, __clang_i386_atomic_op::bit_nand>(ptr, val);
#endif
  return __atomic_fetch_nand(ptr, val, memorder);
}

template<typename T>
  requires(sizeof(T) == 1)      // __atomic_test_and_set only touches one byte; reject wider T (broken mutual exclusion)
constexpr __attribute__((always_inline)) inline bool
test_and_set(T *ptr, i32 memorder)
{
  return __atomic_test_and_set(ptr, memorder);
}

template<typename T>
  requires(sizeof(T) == 1)      // __atomic_clear only zeroes one byte; reject wider T (would leave high bytes set)
constexpr __attribute__((always_inline)) inline void
clear(T *ptr, i32 memorder)
{
  __atomic_clear(ptr, memorder);
}

constexpr __attribute__((always_inline)) inline void
thread_fence(i32 memorder)
{
  return __atomic_thread_fence(memorder);
}

constexpr __attribute__((always_inline)) inline void
signal_fence(i32 memorder)
{
  return __atomic_signal_fence(memorder);
}
};      // namespace atom

};      // namespace micron
