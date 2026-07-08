//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../memory/new.hpp"
#include "../types.hpp"

#include "async_channel.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// coroutine-awaitable MPMC channel with 0 backpressure

namespace micron
{

template<class T> class unbounded_channel
{
  struct __node {
    T __val;
    __node *__next = nullptr;
  };

  __node *__head = nullptr;      // dequeue end
  __node *__tail = nullptr;      // enqueue end
  micron::atomic_token<u32> __lk{ 0 };
  micron::coro::async_semaphore __items{ 0 };
  micron::atomic_token<u32> __closed{ 0 };

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
  unbounded_channel() = default;
  unbounded_channel(const unbounded_channel &) = delete;
  unbounded_channel &operator=(const unbounded_channel &) = delete;

  ~unbounded_channel()
  {
    for ( __node *__n = __head; __n != nullptr; ) {
      __node *__nx = __n->__next;
      delete __n;
      __n = __nx;
    }
  }

  bool
  push(T __v)
  {
    if ( __closed.get(memory_order_acquire) ) return false;
    __node *__n = new __node{ micron::move(__v), nullptr };
    __lock();
    if ( __tail != nullptr )
      __tail->__next = __n;
    else
      __head = __n;
    __tail = __n;
    __unlock();
    __items.release();
    return true;
  }

  micron::task<chan_pull<T>>
  pull()
  {
    co_await __items.acquire();
    __lock();
    __node *__n = __head;
    if ( __n != nullptr ) {
      __head = __n->__next;
      if ( __head == nullptr ) __tail = nullptr;
    }
    __unlock();
    if ( __n != nullptr ) {
      T __v = micron::move(__n->__val);
      delete __n;
      co_return chan_pull<T>{ true, micron::move(__v) };
    }
    co_return chan_pull<T>{ false, T{} };      // empty -> closed & drained
  }

  chan_status
  try_pull(T &__out) noexcept
  {
    if ( !__items.try_acquire() && !__closed.get(memory_order_acquire) ) return chan_status::empty;
    __lock();
    __node *__n = __head;
    if ( __n != nullptr ) {
      __head = __n->__next;
      if ( __head == nullptr ) __tail = nullptr;
    }
    __unlock();
    if ( __n != nullptr ) {
      __out = micron::move(__n->__val);
      delete __n;
      return chan_status::ok;
    }
    return chan_status::closed;
  }

  void
  close() noexcept
  {
    __closed.store(1, memory_order_release);
    __items.abort();
  }

  bool
  is_closed() const noexcept
  {
    return __closed.get(memory_order_acquire) != 0;
  }
};

};      // namespace micron
