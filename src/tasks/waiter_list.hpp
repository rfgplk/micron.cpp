//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../types.hpp"

#include "coroutine/cl_sched.hpp"

#include "frame.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shared waiter list for the suspending  sync primitives
// (async_latch / async_barrier / async_condvar / async_rwlock / fork_group)
//
// list is spinlock guarded

namespace micron
{
namespace coro
{

[[gnu::always_inline]] inline void
__resume_waiter(__frame_base *__f) noexcept
{
  if ( __global_engine != nullptr )
    __global_engine->submit(__f);
  else
    __f->__self.resume();
}

struct waiter_node {
  __frame_base *__frame = nullptr;
  waiter_node *__next = nullptr;
};

inline void
__wake_one(waiter_node *__n) noexcept
{
  if ( __n != nullptr ) __resume_waiter(__n->__frame);
}

inline void
__wake_all(waiter_node *__h) noexcept
{
  while ( __h != nullptr ) {
    waiter_node *__nx = __h->__next;
    __resume_waiter(__h->__frame);
    __h = __nx;
  }
}

class waiter_list
{
  waiter_node *__head = nullptr;
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

public:
  waiter_list() noexcept = default;
  waiter_list(const waiter_list &) = delete;
  waiter_list &operator=(const waiter_list &) = delete;

  void
  push(waiter_node *__n) noexcept
  {
    __lock();
    __n->__next = __head;
    __head = __n;
    __unlock();
  }

  template<class Pred>
  bool
  push_unless(waiter_node *__n, Pred __open) noexcept
  {
    __lock();
    if ( __open() ) {
      __unlock();
      return false;
    }
    __n->__next = __head;
    __head = __n;
    __unlock();
    return true;
  }

  waiter_node *
  pop() noexcept
  {
    __lock();
    waiter_node *__h = __head;
    if ( __h != nullptr ) __head = __h->__next;
    __unlock();
    return __h;
  }

  waiter_node *
  pop_blocking() noexcept
  {
    for ( ;; ) {
      __lock();
      if ( __head != nullptr ) {
        waiter_node *__h = __head;
        __head = __h->__next;
        __unlock();
        return __h;
      }
      __unlock();
      __cpu_pause();
    }
  }

  waiter_node *
  swap_all() noexcept
  {
    __lock();
    waiter_node *__h = __head;
    __head = nullptr;
    __unlock();
    return __h;
  }

  bool
  empty() noexcept
  {
    __lock();
    const bool __e = (__head == nullptr);
    __unlock();
    return __e;
  }
};

};      // namespace coro
};      // namespace micron
