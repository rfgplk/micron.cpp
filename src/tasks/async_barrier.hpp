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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a reusable (cyclic) barrier

namespace micron
{
namespace coro
{

class async_barrier
{
  const u32 __threshold;
  micron::atomic_token<u32> __count;      // arrivals still needed this generation
  micron::atomic_token<u32> __gen;        // generation counter (prevents arrive/fire ABA)
  waiter_list __waiters;

public:
  explicit async_barrier(u32 __n) noexcept : __threshold(__n), __count(__n), __gen(0) { }

  async_barrier(const async_barrier &) = delete;
  async_barrier &operator=(const async_barrier &) = delete;

  struct [[nodiscard]] __arrive_awaiter {
    async_barrier *__b;
    u32 __mygen = 0;
    waiter_node __node{};

    bool
    await_ready() noexcept
    {
      __mygen = __b->__gen.get(memory_order_acquire);
      const u32 __prev = __b->__count.fetch_sub(1, memory_order_acq_rel);
      if ( __prev == 1u ) {      // last arriver: complete the generation
        __b->__count.store(__b->__threshold, memory_order_relaxed);
        __b->__gen.fetch_add(1, memory_order_acq_rel);
        __wake_all(__b->__waiters.swap_all());
        return true;      // last arriver continues without suspending
      }
      return false;      // not last: suspend in await_suspend
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      __node.__frame = &__h.promise();
      // park unless the generation already advanced (barrier fired between our fetch_sub and here)
      return __b->__waiters.push_unless(&__node, [__bp = __b, __g = __mygen]() { return __bp->__gen.get(memory_order_acquire) != __g; });
    }

    void
    await_resume() const noexcept
    {
    }
  };

  __arrive_awaiter
  arrive_and_wait() noexcept
  {
    return __arrive_awaiter{ this };
  }
};

};      // namespace coro
};      // namespace micron
