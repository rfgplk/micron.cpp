//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../atomic/atomic.hpp"
#include "../types.hpp"

#include "waiter_list.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// single use countdown latch

namespace micron
{
namespace coro
{

class async_latch
{
  micron::atomic_token<i64> __count;
  waiter_list __waiters;

public:
  explicit async_latch(i64 __n) noexcept : __count(__n) { }

  async_latch(const async_latch &) = delete;
  async_latch &operator=(const async_latch &) = delete;

  [[nodiscard]] bool
  try_wait() const noexcept
  {
    return __count.get(memory_order_acquire) <= 0;
  }

  void
  count_down(i64 __n = 1) noexcept
  {
    const i64 __old = __count.fetch_sub(__n, memory_order_acq_rel);
    if ( __old > 0 && __old - __n <= 0 ) __wake_all(__waiters.swap_all());
  }

  struct [[nodiscard]] __wait_awaiter {
    async_latch *__l;
    waiter_node __node{};

    bool
    await_ready() const noexcept
    {
      return __l->__count.get(memory_order_acquire) <= 0;
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      __node.__frame = &__h.promise();

      return __l->__waiters.push_unless(&__node, [__lp = __l]() { return __lp->__count.get(memory_order_acquire) <= 0; });
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
};

};      // namespace coro
};      // namespace micron
