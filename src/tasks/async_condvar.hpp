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

#include "async_sync.hpp"
#include "waiter_list.hpp"

namespace micron
{
namespace coro
{

class async_condvar
{
  struct __cv_waiter {
    micron::atomic_token<usize> __st{ 0 };
    __cv_waiter *__next = nullptr;
  };

  __cv_waiter *__head = nullptr;
  micron::atomic_token<u32> __lk{ 0 };

  void
  __lock() noexcept
  {
    u32 __e = 0;
    while ( !__lk.compare_exchange_weak(__e, 1u, memory_order_acq_rel, memory_order_acquire) ) {
      __e = 0;
      __cpu_pause();
    }
  }

  void
  __unlock() noexcept
  {
    __lk.store(0u, memory_order_release);
  }

  void
  __push(__cv_waiter *__w) noexcept
  {
    __lock();
    __w->__next = __head;
    __head = __w;
    __unlock();
  }

  __cv_waiter *
  __pop() noexcept
  {
    __lock();
    __cv_waiter *__w = __head;
    if ( __w != nullptr ) __head = __w->__next;
    __unlock();
    return __w;
  }

  __cv_waiter *
  __swap_all() noexcept
  {
    __lock();
    __cv_waiter *__w = __head;
    __head = nullptr;
    __unlock();
    return __w;
  }

  static void
  __signal(__cv_waiter *__w) noexcept
  {
    const usize __old = __w->__st.swap(1u, memory_order_acq_rel);
    if ( __old != 0u ) __resume_waiter(reinterpret_cast<__frame_base *>(__old));
  }

  struct [[nodiscard]] __park_awaiter {
    __cv_waiter *__w;

    bool
    await_ready() const noexcept
    {
      return __w->__st.get(memory_order_acquire) == 1u;
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      usize __exp = 0u;
      const usize __f = reinterpret_cast<usize>(static_cast<__frame_base *>(&__h.promise()));

      return __w->__st.compare_exchange_strong(__exp, __f, memory_order_acq_rel, memory_order_acquire);
    }

    void
    await_resume() const noexcept
    {
    }
  };

public:
  async_condvar() noexcept = default;
  async_condvar(const async_condvar &) = delete;
  async_condvar &operator=(const async_condvar &) = delete;

  micron::task<void>
  wait(async_mutex &__m)
  {
    __cv_waiter __w;
    __push(&__w);
    __m.unlock();
    co_await __park_awaiter{ &__w };
    co_await __m.lock();
  }

  void
  notify_one() noexcept
  {
    __cv_waiter *__w = __pop();
    if ( __w != nullptr ) __signal(__w);
  }

  void
  notify_all() noexcept
  {
    __cv_waiter *__w = __swap_all();
    while ( __w != nullptr ) {
      __cv_waiter *__nx = __w->__next;
      __signal(__w);
      __w = __nx;
    }
  }
};

};      // namespace coro
};      // namespace micron
