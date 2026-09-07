//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__posix_time_types.hpp"

#include "__syscall.hpp"

#include "../../types.hpp"

#if defined(__micron_arch_width_32)
#include "../../atomic/intrin.hpp"
#endif

namespace micron
{
namespace port
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

#if defined(__micron_arch_width_32)

struct __futex_old_ts {
  i32 tv_sec;
  i32 tv_nsec;
};

inline u32 __futex_time64_demoted = 0;
#endif

inline auto
__futex_linux(u32 *addr, int futex, u32 val, micron::timespec_t *timeout, u32 *addr2, u32 val2)
{
#if defined(__micron_arch_width_32)
  if ( timeout != nullptr ) {
    if ( atom::load(&__futex_time64_demoted, micron::atomic_relaxed) == 0 ) [[likely]] {
      const long r = micron::syscall(SYS_futex_time64, addr, futex, val, timeout, addr2, val2);
      if ( r != -38L ) return r;
      atom::store(&__futex_time64_demoted, 1u, micron::atomic_relaxed);
    }
    const __futex_old_ts __old{ static_cast<i32>(timeout->tv_sec > 0x7FFF'FFFFll ? 0x7FFF'FFFFll : timeout->tv_sec),
                                static_cast<i32>(timeout->tv_nsec) };
    return micron::syscall(SYS_futex, addr, futex, val, &__old, addr2, val2);
  }
#endif
  return micron::syscall(SYS_futex, addr, futex, val, timeout, addr2, val2);
}

inline i64
wait(u32 *addr, u32 expected, i64 timeout_ns) noexcept
{
  if ( timeout_ns < 0 ) return static_cast<i64>(__futex_linux(addr, futex_wait | futex_private_flag, expected, nullptr, nullptr, 0));
  micron::timespec_t __ts{};
  __ts.tv_sec = static_cast<decltype(__ts.tv_sec)>(timeout_ns / 1'000'000'000);
  __ts.tv_nsec = static_cast<decltype(__ts.tv_nsec)>(timeout_ns % 1'000'000'000);
  return static_cast<i64>(__futex_linux(addr, futex_wait | futex_private_flag, expected, &__ts, nullptr, 0));
}

inline i64
wake(u32 *addr, i32 n) noexcept
{
  const u32 __n = (n < 0) ? 0x7FFF'FFFFu : static_cast<u32>(n);
  return static_cast<i64>(__futex_linux(addr, futex_wake | futex_private_flag, __n, nullptr, nullptr, 0));
}

};      // namespace port
};      // namespace micron
