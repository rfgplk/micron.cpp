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

#include "../atomic/atomic.hpp"
#include "../atomic/intrin.hpp"
#include "../except.hpp"

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
constexpr static const u32 futex_futex_lock_pi2 = 13;      // >=5.14
constexpr static const u32 futex_private_flag = 128;
constexpr static const u32 futex_clock_realtime = 256;

auto
__futex(u32 *addr, int futex, u32 val, timespec_t *timeout, u32 *addr2, u32 val2)
{
  return micron::syscall(SYS_futex, addr, futex, val, timeout, addr2, val2);
}

template<typename T>
  requires(sizeof(T) == 4)
void
wait_futex(T *ptr, decay_t<T> expected)
{
  while ( atom::load(ptr, (int)memory_order_relaxed) == expected ) {
    auto ret = __futex(reinterpret_cast<u32 *>(const_cast<decay_t<T> *>(ptr)), futex_wait | futex_private_flag, static_cast<u32>(expected),
                       nullptr, nullptr, 0);
    if ( ret < 0 and ret != -11 and ret != -4 ) {
      micron::exc<except::thread_error>("futex wait failed");
    }
  }
}

template<typename T>
  requires(sizeof(T) == 4)
void
release_futex(T *ptr, decay_t<T> to_store, u32 cnt = 1)
{
  atom::store(ptr, to_store, (int)memory_order_release);
  __futex(reinterpret_cast<u32 *>(ptr), futex_wake | futex_private_flag, cnt, nullptr, nullptr, 0);
}

template<typename T>
  requires(sizeof(T) == 4)
void
wake_futex(T *ptr, int cnt = 1)
{
  __futex(reinterpret_cast<u32 *>(const_cast<decay_t<T> *>(ptr)), futex_wake | futex_private_flag, static_cast<u32>(cnt), nullptr, nullptr,
          0);
}

// __D is the unlocked value
// __L is the locked value
template<typename T = u32, T __D = 0, T __L = 1>
  requires(micron::is_integral_v<T> && __D != __L)
struct futex {
  T __value;
  ~futex() = default;
  futex(void) : __value(__D) { };
  futex(const futex &) = delete;
  futex(futex &&) = delete;
  futex &operator=(const futex &) = delete;
  futex &operator=(futex &&) = delete;

  void
  wait()
  {
    for ( ;; ) {
      T expected = __D;
      if ( atom::cmp_exchange_weak(&__value, &expected, __L) ) return;
      auto ret
          = __futex(reinterpret_cast<u32 *>(&__value), futex_wait | futex_private_flag, static_cast<u32>(expected), nullptr, nullptr, 0);
      if ( ret < 0 && ret != -11 && ret != -4 ) {
        micron::exc<except::thread_error>("futex wait failed");
      }
    }
  }

  void
  release()
  {
    atom::store(&__value, __D, atomic_seq_cst);
    __futex(reinterpret_cast<u32 *>(&__value), futex_wake | futex_private_flag, 1, nullptr, nullptr, 0);
  }
};

};      // namespace micron
