//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../memory/actions.hpp"
#include "../new.hpp"
#include "../types.hpp"

namespace micron
{

template <size_t N> struct lambda_queue {
  struct node_base_t {
    virtual void call() = 0;
    virtual ~node_base_t() = default;
  };

  template <typename Fn> struct node_t : node_base_t {
    Fn fn;

    node_t(Fn &&f) : fn(micron::move(f)) {}

    void
    call() override
    {
      fn();
    }
  };

  u8 slots[N * 64]{};
  node_base_t *__q[N]{};
  micron::atomic_token<size_t> head{ 0 };
  micron::atomic_token<size_t> tail{ 0 };

  template <typename Fn>
  inline void
  push(Fn &&fn)
  {
    size_t idx = tail.fetch_add(1, memory_order_acquire) % N;
    // Wait if queue is full
    while ( __q[idx] != nullptr ) {
      // Spin or yield - queue is full
    }
    __q[idx] = new (&slots[idx * 64]) node_t<Fn>(micron::forward<Fn>(fn));
  }

  void
  execute()
  {
    auto __task = pop();
    if ( __task != nullptr ) [[unlikely]]
      __task->call();
  }

  inline node_base_t *
  pop()
  {
    if ( head.get(memory_order_acquire) == tail.get(memory_order_acquire) )
      return nullptr;

    size_t idx = head.fetch_add(1, memory_order_acquire) % N;
    node_base_t *task = __q[idx];
    __q[idx] = nullptr;
    return task;
  }

  inline void
  clear()
  {
    size_t h = head.get(memory_order_acquire);
    size_t t = tail.get(memory_order_acquire);

    for ( size_t i = h; i < t; i++ ) {
      size_t idx = i % N;
      if ( __q[idx] != nullptr ) {
        delete __q[idx];
        __q[idx] = nullptr;
      }
    }

    head.store(0, memory_order_release);
    tail.store(0, memory_order_release);
  }

  inline bool
  empty() const
  {
    return head.get(memory_order_acquire) == tail.get(memory_order_acquire);
  }

  inline size_t
  size() const
  {
    size_t t = tail.get(memory_order_acquire);
    size_t h = head.get(memory_order_acquire);
    return t - h;
  }

  inline size_t
  max_size() const
  {
    return N;
  }
};

};     // namespace micron
