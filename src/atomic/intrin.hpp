//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// intrinsics mapped to fn's
namespace micron
{
constexpr static const int atomic_seq_cst = __ATOMIC_SEQ_CST;
constexpr static const int atomic_consume = __ATOMIC_CONSUME;
constexpr static const int atomic_acquire = __ATOMIC_ACQUIRE;
constexpr static const int atomic_release = __ATOMIC_RELEASE;
constexpr static const int atomic_acq_rel = __ATOMIC_ACQ_REL;
constexpr static const int atomic_hle_acquire = __ATOMIC_HLE_ACQUIRE;
constexpr static const int atomic_hle_release = __ATOMIC_HLE_RELEASE;

namespace atom
{
template <typename T>
constexpr __attribute__((always_inline)) inline T
load(T *ptr, int memorder)
{
  return __atomic_load_n(ptr, memorder);
}
template <typename T>
constexpr __attribute__((always_inline)) inline void
store(T *ptr, T val, int memorder)
{
  __atomic_store_n(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
exchange(T *ptr, T val, int memorder)
{
  return __atomic_exchange_n(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline bool
cmp_exchange_strong(T *ptr, T *expected, T desired)
{
  return __atomic_compare_exchange_n(ptr, expected, desired, false, atomic_seq_cst, atomic_seq_cst);
}
template <typename T>
constexpr __attribute__((always_inline)) inline bool
compare_exchange_strong(T *ptr, T *expected, T desired)
{
  return __atomic_compare_exchange_n(ptr, expected, desired, false, atomic_seq_cst, atomic_seq_cst);
}
template <typename T>
constexpr __attribute__((always_inline)) inline bool
cmp_exchange_weak(T *ptr, T *expected, T desired)
{
  return __atomic_compare_exchange_n(ptr, expected, desired, true, atomic_seq_cst, atomic_seq_cst);
}
template <typename T>
constexpr __attribute__((always_inline)) inline bool
compare_exchange_weak(T *ptr, T *expected, T desired)
{
  return __atomic_compare_exchange_n(ptr, expected, desired, true, atomic_seq_cst, atomic_seq_cst);
}
template <typename T>
constexpr __attribute__((always_inline)) inline bool
cmp_exchange(T *ptr, T *expected, T desired, bool weak, int success_memorder, int failure_memorder)
{
  return __atomic_compare_exchange_n(ptr, expected, desired, weak, success_memorder, failure_memorder);
}
template <typename T>
constexpr __attribute__((always_inline)) inline bool
compare_exchange(T *ptr, T *expected, T desired, bool weak, int success_memorder, int failure_memorder)
{
  return __atomic_compare_exchange_n(ptr, expected, desired, weak, success_memorder, failure_memorder);
}
template <typename T>
constexpr __attribute__((always_inline)) inline T
add_fetch(T *ptr, T val, int memorder)
{
  return __atomic_add_fetch(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
sub_fetch(T *ptr, T val, int memorder)
{
  return __atomic_sub_fetch(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
and_fetch(T *ptr, T val, int memorder)
{
  return __atomic_and_fetch(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
xor_fetch(T *ptr, T val, int memorder)
{
  return __atomic_xor_fetch(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
or_fetch(T *ptr, T val, int memorder)
{
  return __atomic_or_fetch(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
nand_fetch(T *ptr, T val, int memorder)
{
  return __atomic_nand_fetch(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
fetch_add(T *ptr, T val, int memorder)
{
  return __atomic_fetch_ad(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
fetch_sub(T *ptr, T val, int memorder)
{
  return __atomic_fetch_sub(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
fetch_and(T *ptr, T val, int memorder)
{
  return __atomic_fetch_and(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
fetch_xor(T *ptr, T val, int memorder)
{
  return __atomic_fetch_xor(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
fetch_or(T *ptr, T val, int memorder)
{
  return __atomic_fetch_or(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline T
fetch_nand(T *ptr, T val, int memorder)
{
  return __atomic_fetch_nand(ptr, val, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline bool
test_and_set(T *ptr, int memorder)
{
  return __atomic_test_and_set(ptr, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline void
clear(T *ptr, int memorder)
{
  __atomic_clear(ptr, memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline void
thread_fence(int memorder)
{
  return __atomic_thread_fence(memorder);
}

template <typename T>
constexpr __attribute__((always_inline)) inline void
signal_fence(int memorder)
{
  return __atomic_signal_fence(memorder);
}
};

};
