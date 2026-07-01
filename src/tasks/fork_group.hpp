//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../types.hpp"

#include "coroutine/cl_sched.hpp"

#include "cancellation.hpp"
#include "fork_join.hpp"
#include "task.hpp"
#include "waiter_list.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dynamic task groups (Go/WaitGroup inspired)
//
// a fork_group owns its own completion counter, so you can spawn() incrementally and co_await join() from
// any frame; each spawned task runs as an independent root on the pool

namespace micron
{
namespace coro
{

struct __group_bridge {
  struct promise_type: __frame_base {
    __group_bridge
    get_return_object() noexcept
    {
      auto __h = std::coroutine_handle<promise_type>::from_promise(*this);
      this->__self = __h;
      return __group_bridge{ __h };
    }

    std::suspend_always
    initial_suspend() noexcept
    {
      return {};
    }

    std::suspend_never
    final_suspend() noexcept
    {
      return {};
    }

    void
    return_void() noexcept
    {
    }

    void
    unhandled_exception() noexcept
    {
      __builtin_trap();
    }

    static void *
    operator new(usize __n) noexcept
    {
      return ::operator new(__n);
    }

    static void
    operator delete(void *__p, usize) noexcept
    {
      ::operator delete(__p);
    }

    static __group_bridge
    get_return_object_on_allocation_failure() noexcept
    {
      return {};
    }
  };

  std::coroutine_handle<promise_type> __h{};
  __group_bridge() noexcept = default;

  explicit __group_bridge(std::coroutine_handle<promise_type> __hh) noexcept : __h(__hh) { }
};

class fork_group
{
  micron::atomic_token<i64> __outstanding{ 0 };
  waiter_list __joiners;
  micron::atomic_token<u32> __cancelled{ 0 };

public:
  fork_group() noexcept = default;
  fork_group(const fork_group &) = delete;
  fork_group &operator=(const fork_group &) = delete;

  void
  __complete() noexcept
  {
    if ( __outstanding.sub_fetch(1, memory_order_acq_rel) == 0 ) __wake_all(__joiners.swap_all());
  }

  [[nodiscard]] i64
  outstanding() const noexcept
  {
    return __outstanding.get(memory_order_acquire);
  }

  // cancellation
  void
  request_cancel() noexcept
  {
    __cancelled.store(1u, memory_order_release);
  }

  [[nodiscard]] bool
  cancelled() const noexcept
  {
    return __cancelled.get(memory_order_relaxed) != 0u;
  }

  [[nodiscard]] cancellation_token
  token() const noexcept
  {
    return cancellation_token(&__cancelled);
  }

  template<class Fn, class... Args>
  void
  spawn(Fn __fn, Args... __args)
  {
    __outstanding.fetch_add(1, memory_order_acq_rel);
    start_coroutine_runtime();
    const micron::atomic_token<u32> *__cflag = &__cancelled;
    fork_group *__self = this;
    __group_bridge __b = [](fork_group *__g, const micron::atomic_token<u32> *__cf, Fn __f, Args... __a) -> __group_bridge {
      auto __t = __f(micron::move(__a)...);                           // the spawned task<R>
      if ( __t.valid() ) __t.handle().promise().__cancel = __cf;      // children inherit the group cancel flag
      co_await micron::move(__t);
      __g->__complete();
    }(__self, __cflag, micron::move(__fn), micron::move(__args)...);
    if ( __b.__h )
      __global_engine->submit(&__b.__h.promise());
    else
      __complete();      // bridge allocation failed: undo the outstanding increment so join() still completes
  }

  struct [[nodiscard]] __join_awaiter {
    fork_group *__g;
    waiter_node __node{};

    bool
    await_ready() const noexcept
    {
      return __g->__outstanding.get(memory_order_acquire) <= 0;
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      __node.__frame = &__h.promise();
      return __g->__joiners.push_unless(&__node, [__gp = __g]() { return __gp->__outstanding.get(memory_order_acquire) <= 0; });
    }

    void
    await_resume() const noexcept
    {
    }
  };

  __join_awaiter
  join() noexcept
  {
    return __join_awaiter{ this };
  }
};

};      // namespace coro
};      // namespace micron
