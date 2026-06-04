//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../memory/actions.hpp"
#include "../new.hpp"
#include "../types.hpp"

namespace micron
{

template<usize N> struct lambda_queue {
  struct node_base_t {
    virtual void call() = 0;
    virtual ~node_base_t() = default;
  };

  template<typename Fn> struct node_t: node_base_t {
    Fn fn;

    node_t(Fn &&f) : fn(micron::move(f)) { }

    void
    call() override
    {
      fn();
    }
  };

  static constexpr usize slot_bytes = 64;

  u8 slots[N * slot_bytes]{};
  micron::atomic_ptr<node_base_t *> __q[N]{};
  micron::atomic_token<usize> head{ 0 };
  micron::atomic_token<usize> tail{ 0 };

  template<typename Fn>
  inline void
  push(Fn &&fn)
  {
    static_assert(sizeof(node_t<micron::decay_t<Fn>>) <= slot_bytes, "lambda_queue: captured callable too large for the 64-byte slot");
    // claim a unique ring slot (acq_rel: orders the slot publish below w.r.t. other producers)
    usize idx = tail.fetch_add(1, memory_order_acq_rel) % N;
    while ( __q[idx].get(memory_order_acquire) != nullptr ) {
      ::__cpu_pause();
    }
    node_base_t *node = new (&slots[idx * slot_bytes]) node_t<micron::decay_t<Fn>>(micron::forward<Fn>(fn));
    __q[idx].store(node, memory_order_release);
  }

  void
  execute()
  {
    node_base_t *task = pop();
    if ( task != nullptr ) {
      task->call();
      task->~node_base_t();      // placement-new'd into slots[]: destruct in place, do NOT delete
    }
  }

  inline node_base_t *
  pop()
  {
    if ( head.get(memory_order_acquire) == tail.get(memory_order_acquire) ) return nullptr;

    usize idx = head.fetch_add(1, memory_order_acquire) % N;
    // a producer increments tail before publishing __q[idx]; wait (acquire) for the publish
    node_base_t *task;
    while ( (task = __q[idx].get(memory_order_acquire)) == nullptr ) {
      ::__cpu_pause();
    }
    __q[idx].store(nullptr, memory_order_release);      // free the slot for reuse
    return task;
  }

  inline void
  clear()
  {
    for ( usize i = 0; i < N; ++i ) {
      node_base_t *n = __q[i].get(memory_order_acquire);
      if ( n != nullptr ) {
        n->~node_base_t();      // placement-destruct; storage is the slots[] buffer (never `delete`)
        __q[i].store(nullptr, memory_order_release);
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

  inline usize
  size() const
  {
    usize t = tail.get(memory_order_acquire);
    usize h = head.get(memory_order_acquire);
    return t - h;
  }

  inline usize
  max_size() const
  {
    return N;
  }
};

};      // namespace micron
