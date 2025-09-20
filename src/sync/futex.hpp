//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

#include "../linux/sys/time.hpp"
#include "../syscall.hpp"
#include "../type_traits.hpp"

#include "../atomic/intrin.hpp"

namespace micron
{

constexpr static const u32 futex_wait = 0;
constexpr static const u32 futex_wake = 1;
constexpr static const u32 futex_fd = 2;
constexpr static const u32 futex_requeue = 3;
constexpr static const u32 futex_cmp_requeue = 4;
constexpr static const u32 futex_wake_op = 5;
constexpr static const u32 futex_lock_pi = 6;
constexpr static const u32 futex_unlock_pi = 7;
constexpr static const u32 futex_trylock_pi = 8;
constexpr static const u32 futex_wait_bitset = 9;
constexpr static const u32 futex_wake_bitset = 10;
constexpr static const u32 futex_wait_requeue_pi = 11;
constexpr static const u32 futex_cmp_requeue_pi = 12;
constexpr static const u32 futex_futex_lock_pi2 = 13;
constexpr static const u32 futex_private_flag = 128;
constexpr static const u32 futex_clock_realtime = 256;

void
__futex(u32 *addr, int futex, u32 val, timespec_t *timeout, u32 *addr2, u32 val2)
{
  micron::syscall(SYS_futex, addr, futex, val, timeout, addr2, val2);
}

template <typename T>
void
wait_futex(T *ptr, T val)
{
  T e = 0;
  while ( !atom::cmp_exchange_weak(ptr, &e, 1) ) {
    e = 0;
    __futex(reinterpret_cast<u32*>(ptr), futex_wait | futex_private_flag, val, nullptr, nullptr, 0);
  }
}
template <typename T>
void
release_futex(T *ptr, T val)
{
  atom::store(ptr, 0, atomic_seq_cst);
  __futex(reinterpret_cast<u32*>(ptr), futex_wake | futex_private_flag, val, nullptr, nullptr, 0);
}

template <typename T = u32, T __D = 1>
  requires(micron::is_integral_v<T>)
struct futex {
  T __value;
  ~futex() = default;
  futex(void) : __value(__D) {};
  futex(const futex &) = delete;
  futex(futex &&) = delete;
  futex &operator=(const futex &) = delete;
  futex &operator=(futex &&) = delete;

  void
  wait()
  {
    T e = __D;
    while ( !atom::cmp_exchange_weak(&__value, &e, 1) ) {
      e = 0;
      __futex(&__value, futex_wait | futex_private_flag, __D, nullptr, nullptr, 0);
    }
  }
  void
  release()
  {
    atom::store(&__value, 0, atomic_seq_cst);
    __futex(&__value, futex_wake | futex_private_flag, 1, nullptr, nullptr, 0);
  }
};

};
