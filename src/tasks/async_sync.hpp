//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../types.hpp"

#include "coroutine/cl_sched.hpp"

#include "frame.hpp"
#include "waiter_list.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// suspending synchronization primitives
// async_mutex, manual_reset_event, async_semaphore

namespace micron
{
namespace coro
{

class async_mutex
{

  static constexpr usize __not_locked = 1;
  static constexpr usize __locked_no_waiters = 0;
  micron::atomic_token<usize> __state{ __not_locked };

public:
  async_mutex() noexcept = default;
  async_mutex(const async_mutex &) = delete;
  async_mutex &operator=(const async_mutex &) = delete;

  struct [[nodiscard]] __lock_awaiter {
    async_mutex *__m;
    __frame_base *__frame = nullptr;
    __lock_awaiter *__next = nullptr;

    bool
    await_ready() noexcept
    {
      usize __exp = __not_locked;
      return __m->__state.compare_exchange_strong(__exp, __locked_no_waiters, memory_order_acq_rel, memory_order_acquire);
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      __frame = &__h.promise();
      usize __old = __m->__state.get(memory_order_acquire);
      for ( ;; ) {
        if ( __old == __not_locked ) {
          usize __exp = __not_locked;
          if ( __m->__state.compare_exchange_weak(__exp, __locked_no_waiters, memory_order_acq_rel, memory_order_acquire) )
            return false;      // acquired uncontended after all; don't suspend
          __old = __exp;
          continue;
        }
        __next = (__old == __locked_no_waiters) ? nullptr : reinterpret_cast<__lock_awaiter *>(__old);
        usize __desired = reinterpret_cast<usize>(this);
        if ( __m->__state.compare_exchange_weak(__old, __desired, memory_order_acq_rel, memory_order_acquire) )
          return true;      // suspended; unlock() will hand us the lock
      }
    }

    void
    await_resume() const noexcept
    {
    }
  };

  __lock_awaiter
  lock() noexcept
  {
    return __lock_awaiter{ this };
  }

  bool
  try_lock() noexcept
  {
    usize __exp = __not_locked;
    return __state.compare_exchange_strong(__exp, __locked_no_waiters, memory_order_acq_rel, memory_order_acquire);
  }

  void
  unlock() noexcept
  {
    usize __old = __state.get(memory_order_acquire);
    for ( ;; ) {
      if ( __old == __locked_no_waiters ) {
        usize __exp = __locked_no_waiters;
        if ( __state.compare_exchange_weak(__exp, __not_locked, memory_order_acq_rel, memory_order_acquire) )
          return;      // no waiters: released
        __old = __exp;
        continue;
      }
      __lock_awaiter *__head = reinterpret_cast<__lock_awaiter *>(__old);
      usize __after = (__head->__next == nullptr) ? __locked_no_waiters : reinterpret_cast<usize>(__head->__next);
      if ( __state.compare_exchange_weak(__old, __after, memory_order_acq_rel, memory_order_acquire) ) {
        __resume_waiter(__head->__frame);      // hand the lock to the popped waiter and resume it
        return;
      }
    }
  }
};

class manual_reset_event
{
  static constexpr usize __set = 1;

  micron::atomic_token<usize> __state;

public:
  explicit manual_reset_event(bool __initially_set = false) noexcept : __state(__initially_set ? __set : 0u) { }

  manual_reset_event(const manual_reset_event &) = delete;
  manual_reset_event &operator=(const manual_reset_event &) = delete;

  bool
  is_set() const noexcept
  {
    return __state.get(memory_order_acquire) == __set;
  }

  struct [[nodiscard]] __wait_awaiter {
    manual_reset_event *__e;
    __frame_base *__frame = nullptr;
    __wait_awaiter *__next = nullptr;

    bool
    await_ready() const noexcept
    {
      return __e->__state.get(memory_order_acquire) == __set;
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      __frame = &__h.promise();
      usize __old = __e->__state.get(memory_order_acquire);
      for ( ;; ) {
        if ( __old == __set ) return false;
        __next = (__old == 0u) ? nullptr : reinterpret_cast<__wait_awaiter *>(__old);
        usize __desired = reinterpret_cast<usize>(this);
        if ( __e->__state.compare_exchange_weak(__old, __desired, memory_order_acq_rel, memory_order_acquire) ) return true;
      }
    }

    void
    await_resume() const noexcept
    {
    }
  };

  __wait_awaiter
  operator co_await() noexcept
  {
    return __wait_awaiter{ this };
  }

  void
  set() noexcept
  {
    usize __old = __state.swap(__set, memory_order_acq_rel);
    if ( __old == __set || __old == 0u ) return;
    __wait_awaiter *__w = reinterpret_cast<__wait_awaiter *>(__old);
    while ( __w != nullptr ) {
      __wait_awaiter *__nx = __w->__next;
      __resume_waiter(__w->__frame);
      __w = __nx;
    }
  }

  void
  reset() noexcept
  {
    usize __exp = __set;
    __state.compare_exchange_strong(__exp, 0u, memory_order_acq_rel, memory_order_acquire);
  }
};

class async_semaphore
{
public:
  struct __acq_awaiter;

private:
  micron::atomic_token<i64> __count;
  __acq_awaiter *__whead = nullptr;
  micron::atomic_token<u32> __wlk{ 0 };
  micron::atomic_token<u32> __aborted{ 0 };

  void
  __lock() noexcept
  {
    u32 __e = 0;
    while ( !__wlk.compare_exchange_weak(__e, 1u, memory_order_acq_rel, memory_order_acquire) ) {
      __e = 0;
      __cpu_pause();
    }
  }

  void
  __unlock() noexcept
  {
    __wlk.store(0u, memory_order_release);
  }

public:
  explicit async_semaphore(i64 __init = 0) noexcept : __count(__init) { }

  async_semaphore(const async_semaphore &) = delete;
  async_semaphore &operator=(const async_semaphore &) = delete;

  struct [[nodiscard]] __acq_awaiter {
    async_semaphore *__s;
    __acq_awaiter *__next = nullptr;
    __frame_base *__frame = nullptr;
    bool __has_permit = false;

    bool
    await_ready() noexcept
    {
      if ( __s->__aborted.get(memory_order_acquire) ) return true;

      if ( __s->__count.fetch_sub(1, memory_order_acq_rel) > 0 ) {
        __has_permit = true;
        return true;
      }
      return false;
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      __frame = &__h.promise();
      __s->__lock();
      __next = __s->__whead;
      __s->__whead = this;
      __s->__unlock();
      return true;
    }

    bool
    await_resume() const noexcept
    {
      return __has_permit;
    }
  };

  __acq_awaiter
  acquire() noexcept
  {
    return __acq_awaiter{ this };
  }

  bool
  try_acquire() noexcept
  {
    if ( __aborted.get(memory_order_acquire) ) return false;
    i64 __old = __count.get(memory_order_acquire);
    while ( __old > 0 ) {
      if ( __count.compare_exchange_weak(__old, __old - 1, memory_order_acq_rel, memory_order_acquire) ) return true;
    }
    return false;
  }

  i64
  try_acquire_many(i64 __n) noexcept
  {
    if ( __n <= 0 || __aborted.get(memory_order_acquire) ) return 0;
    i64 __old = __count.get(memory_order_acquire);
    for ( ;; ) {
      if ( __old <= 0 ) return 0;
      const i64 __take = (__old < __n) ? __old : __n;
      if ( __count.compare_exchange_weak(__old, __old - __take, memory_order_acq_rel, memory_order_acquire) ) return __take;
    }
  }

  void
  abort() noexcept
  {
    __aborted.store(1, memory_order_release);

    for ( ;; ) {
      i64 __c = __count.get(memory_order_acquire);
      if ( __c >= 256 ) break;
      __post(1, false);
    }
  }

  void
  release(i64 __n = 1) noexcept
  {
    __post(__n, true);
  }

private:
  void
  __post(i64 __n, bool __with_permit) noexcept
  {
    for ( i64 __i = 0; __i < __n; ++__i ) {
      i64 __old = __count.fetch_add(1, memory_order_acq_rel);
      if ( __old < 0 ) {

        __acq_awaiter *__w = nullptr;
        for ( ;; ) {
          __lock();
          if ( __whead != nullptr ) {
            __w = __whead;
            __whead = __w->__next;
            __unlock();
            break;
          }
          __unlock();
          __cpu_pause();
        }
        __w->__has_permit = __with_permit;
        __resume_waiter(__w->__frame);
      }
    }
  }
};

};      // namespace coro
};      // namespace micron
