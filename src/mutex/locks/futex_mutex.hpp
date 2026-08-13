//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../linux/sys/time.hpp"
#include "../../sync/futex.hpp"

#include "../backoff.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// adaptive mutex
//
// bounded spin + sleep; thread adaptive; futex backed
class futex_mutex
{
  static constexpr u32 __free = 0u;
  static constexpr u32 __held = 1u;
  static constexpr u32 __contended = 2u;

  atomic_token<u32> __s;
  [[no_unique_address]] __lock_stats st;

  // bounded adaptive spin
  bool
  __spin_phase() noexcept
  {
    __counted_backoff<spin_park> bo(st);
    for ( ;; ) {
      if ( __s.get(memory_order::relaxed) == __free ) {
        u32 e = __free;
        if ( __s.compare_exchange_weak(e, __held, memory_order::acquire, memory_order::relaxed) ) return true;
      }
      if ( bo.next() == spin_step::park ) return false;
    }
  }

  static bool
  __now(timespec_t &t) noexcept
  {
    return micron::clock_gettime(clock_monotonic, t) == 0;
  }

  // *out = a - b, clamped at zero
  static bool
  __remaining(const timespec_t &a, const timespec_t &b, timespec_t &out) noexcept
  {
    time64_t s = a.tv_sec - b.tv_sec;
    long ns = static_cast<long>(a.tv_nsec) - static_cast<long>(b.tv_nsec);
    if ( ns < 0 ) {
      ns += 1000000000L;
      --s;
    }
    if ( s < 0 ) return false;
    out.tv_sec = s;
    out.tv_nsec = ns;
    return true;
  }

  void
  reset() noexcept
  {
    unlock();
  }

public:
  ~futex_mutex() = default;

  constexpr futex_mutex() noexcept : __s(__free) { }

  futex_mutex(const futex_mutex &) = delete;
  futex_mutex(futex_mutex &&) = delete;
  futex_mutex &operator=(const futex_mutex &) = delete;

  auto
  operator()() noexcept
  {
    u32 e = __free;
    if ( __s.compare_exchange_strong(e, __held, memory_order::acquire, memory_order::relaxed) ) {
      st.note_acquire();
      return &futex_mutex::reset;
    }

    if ( !__spin_phase() ) {
      // publish that a waiter exists, then sleep
      u32 c = __s.swap(__contended, memory_order::acq_rel);
      while ( c != __free ) {
        micron::__futex(__s.ptr(), futex_wait | futex_private_flag, __contended, nullptr, nullptr, 0);
        c = __s.swap(__contended, memory_order::acq_rel);
      }
    }
    st.note_acquire();
    return &futex_mutex::reset;
  }

  auto
  lock() noexcept
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    u32 e = __free;
    if ( !__s.compare_exchange_strong(e, __held, memory_order::acquire, memory_order::relaxed) ) return false;
    st.note_acquire();
    return true;
  }

  void
  unlock() noexcept
  {
    if ( __s.swap(__free, memory_order::release) == __contended ) micron::wake_futex(__s.ptr(), 1);
  }

  // absolute deadline on CLOCK_MONOTONIC
  bool
  try_lock_until(const timespec_t &deadline) noexcept
  {
    u32 e = __free;
    if ( __s.compare_exchange_strong(e, __held, memory_order::acquire, memory_order::relaxed) ) {
      st.note_acquire();
      return true;
    }
    if ( __spin_phase() ) {
      st.note_acquire();
      return true;
    }

    u32 c = __s.swap(__contended, memory_order::acq_rel);
    while ( c != __free ) {
      timespec_t now{};
      timespec_t rel{};
      if ( !__now(now) or !__remaining(deadline, now, rel) ) {
        // out of time
        if ( __s.get(memory_order::relaxed) == __contended ) micron::wake_futex(__s.ptr(), 1);
        return false;
      }
      auto r = micron::__futex(__s.ptr(), futex_wait | futex_private_flag, __contended, &rel, nullptr, 0);
      if ( r == -110 ) {      // ETIMEDOUT
        if ( __s.get(memory_order::relaxed) == __contended ) micron::wake_futex(__s.ptr(), 1);
        return false;
      }
      c = __s.swap(__contended, memory_order::acq_rel);
    }
    st.note_acquire();
    return true;
  }

  bool
  try_lock_for(u64 timeout_ns) noexcept
  {
    timespec_t deadline{};
    if ( !__now(deadline) ) return try_lock();
    deadline.tv_sec += static_cast<time64_t>(timeout_ns / 1000000000ULL);
    deadline.tv_nsec += static_cast<long>(timeout_ns % 1000000000ULL);
    if ( deadline.tv_nsec >= 1000000000L ) {
      deadline.tv_nsec -= 1000000000L;
      ++deadline.tv_sec;
    }
    return try_lock_until(deadline);
  }

  auto
  retrieve() noexcept
  {
    return &futex_mutex::reset;
  }

  bool
  operator!() const noexcept
  {
    return !is_locked();
  }

  bool
  is_locked() const noexcept
  {
    return __s.get(memory_order::relaxed) != __free;
  }

  // true when at least one waiter has recorded itself
  [[nodiscard]] bool
  contended() const noexcept
  {
    return __s.get(memory_order::relaxed) == __contended;
  }

  [[nodiscard]] const __lock_stats &
  stats() const noexcept
  {
    return st;
  }

  template<typename... T> friend void unlock(T &...);
};

using timed_mutex = futex_mutex;
using adaptive_mutex = futex_mutex;

};      // namespace micron
