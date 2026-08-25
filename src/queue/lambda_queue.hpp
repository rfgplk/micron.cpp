//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../memory/actions.hpp"
#include "../memory/cache.hpp"
#include "../new.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

namespace micron
{

template<usize N>
  requires(N > 0 and N <= (static_cast<usize>(-1) / 2 + 1))
struct lambda_queue {
  static constexpr usize slot_bytes = 64;

  struct node_base_t {
    using invoke_t = void (*)(node_base_t *);
    using finish_t = void (*)(node_base_t *) noexcept;
    using transfer_t = void (*)(node_base_t *, lambda_queue &);

    invoke_t __invoke;
    finish_t __finish;
    transfer_t __transfer;

    constexpr node_base_t(invoke_t invoke, finish_t finish, transfer_t transfer) noexcept
        : __invoke(invoke), __finish(finish), __transfer(transfer)
    {
    }

    node_base_t(const node_base_t &) = delete;
    node_base_t &operator=(const node_base_t &) = delete;

    inline void
    call()
    {
      __invoke(this);
    }

    ~node_base_t() noexcept
    {
      finish_t finish = __finish;
      __finish = nullptr;
      if ( finish ) finish(this);
    }
  };

private:
  static constexpr usize __cache_line = cache_line_size();

  enum __slot_state : u8 { __free = 0, __claimed = 1, __ready = 2, __running = 3, __cancelled = 4 };

  struct alignas(__cache_line) __slot {
    union {
      node_base_t node;
    };

    micron::atomic_token<usize> sequence;
    micron::atomic_token<u8> state;
    usize ticket;
    alignas(__cache_line) byte storage[slot_bytes];

    __slot() noexcept : sequence(0), state(__free), ticket(0) { }

    ~__slot() noexcept { }
  };

  alignas(__cache_line) __slot __slots[N];

  struct alignas(__cache_line) __producer_state {
    micron::atomic_token<usize> tail;
  } __producer{ 0 };

  struct alignas(__cache_line) __consumer_state {
    micron::atomic_token<usize> head;
  } __consumer{ 0 };

  template<typename Fn>
  [[gnu::always_inline]] static inline Fn *
  __callable(__slot &slot) noexcept
  {
    return reinterpret_cast<Fn *>(slot.storage);
  }

  template<typename Fn>
  static void
  __invoke_node(node_base_t *node)
  {
    __slot &slot = *reinterpret_cast<__slot *>(node);
    (*__callable<Fn>(slot))();
  }

  template<typename Fn>
  static void
  __finish_node(node_base_t *node) noexcept
  {
    __slot &slot = *reinterpret_cast<__slot *>(node);
    const usize ticket = slot.ticket;
    __callable<Fn>(slot)->~Fn();
    slot.sequence.store(ticket + N, memory_order_release);
    slot.state.store(__free, memory_order_release);
  }

  template<typename Fn>
  static void
  __transfer_node(node_base_t *node, lambda_queue &destination)
  {
    __slot &slot = *reinterpret_cast<__slot *>(node);
    destination.push(micron::move(*__callable<Fn>(slot)));
  }

  inline void
  __reset_empty() noexcept
  {
    __consumer.head.store(0, memory_order_relaxed);
    __producer.tail.store(0, memory_order_relaxed);
    for ( usize i = 0; i < N; ++i ) {
      __slots[i].state.store(__free, memory_order_relaxed);
      __slots[i].sequence.store(i, memory_order_relaxed);
    }
  }

  inline void
  __move_from(lambda_queue &source)
  {
    while ( node_base_t *task = source.pop() ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
        task->__transfer(task, *this);
      } catch ( ... ) {
        task->~node_base_t();
        throw;
      }
#else
      task->__transfer(task, *this);
#endif
      task->~node_base_t();
    }
    source.__reset_empty();
  }

public:
  lambda_queue()
  {
    for ( usize i = 0; i < N; ++i ) {
      __slots[i].sequence.store(i, memory_order_relaxed);
      __slots[i].state.store(__free, memory_order_relaxed);
    }
  }

  ~lambda_queue() { clear(); }

  lambda_queue(const lambda_queue &) = delete;
  lambda_queue &operator=(const lambda_queue &) = delete;

  lambda_queue(lambda_queue &&source) : lambda_queue()
  {
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      __move_from(source);
    } catch ( ... ) {
      clear();
      throw;
    }
#else
    __move_from(source);
#endif
  }

  lambda_queue &
  operator=(lambda_queue &&source)
  {
    if ( this == micron::addr(source) ) return *this;
    clear();
    __move_from(source);
    return *this;
  }

  template<typename Fn>
  inline void
  push(Fn &&fn)
  {
    using callable_t = micron::decay_t<Fn>;
    static_assert(sizeof(callable_t) <= slot_bytes, "lambda_queue: captured callable too large for the 64-byte slot");
    static_assert(alignof(callable_t) <= __cache_line, "lambda_queue: captured callable is over-aligned for the slot");

    const usize ticket = __producer.tail.fetch_add(1, memory_order_acq_rel);
    __slot &slot = __slots[ticket % N];
    while ( slot.sequence.get(memory_order_acquire) != ticket || slot.state.get(memory_order_acquire) != __free ) ::__cpu_pause();

    slot.ticket = ticket;
    slot.state.store(__claimed, memory_order_relaxed);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      new (slot.storage) callable_t(micron::forward<Fn>(fn));
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      slot.state.store(__cancelled, memory_order_relaxed);
      slot.sequence.store(ticket + 1, memory_order_release);
      usize expected = ticket;
      if ( __consumer.head.compare_exchange_strong(expected, ticket + 1, memory_order_acq_rel, memory_order_relaxed) ) {
        slot.sequence.store(ticket + N, memory_order_release);
        slot.state.store(__free, memory_order_release);
      }
      throw;
    }
#endif
    new (reinterpret_cast<node_base_t *>(micron::addr(slot)))
        node_base_t(&__invoke_node<callable_t>, &__finish_node<callable_t>, &__transfer_node<callable_t>);
    slot.state.store(__ready, memory_order_relaxed);
    slot.sequence.store(ticket + 1, memory_order_release);
  }

  inline node_base_t *
  pop()
  {
    usize ticket = __consumer.head.get(memory_order_relaxed);
    for ( ;; ) {
      if ( ticket == __producer.tail.get(memory_order_acquire) ) return nullptr;
      if ( !__consumer.head.compare_exchange_weak(ticket, ticket + 1, memory_order_acq_rel, memory_order_relaxed) ) continue;
      __slot &slot = __slots[ticket % N];
      while ( slot.sequence.get(memory_order_acquire) != ticket + 1 ) ::__cpu_pause();

      const u8 state = slot.state.get(memory_order_relaxed);
      if ( state == __cancelled ) {
        slot.sequence.store(ticket + N, memory_order_release);
        slot.state.store(__free, memory_order_release);
        continue;
      }
      slot.state.store(__running, memory_order_relaxed);
      return reinterpret_cast<node_base_t *>(micron::addr(slot));
    }
  }

  inline void
  execute()
  {
    node_base_t *task = pop();
    if ( task == nullptr ) return;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      task->call();
    } catch ( ... ) {
      task->~node_base_t();
      throw;
    }
#else
    task->call();
#endif
    task->~node_base_t();
  }

  // Quiescent-only: running tasks must finish before clear/destruction.
  inline void
  clear()
  {
    while ( node_base_t *task = pop() ) task->~node_base_t();
    __reset_empty();
  }

  inline bool
  empty() const
  {
    const usize h = __consumer.head.get(memory_order_acquire);
    return h == __producer.tail.get(memory_order_acquire);
  }

  inline usize
  size() const
  {
    const usize h = __consumer.head.get(memory_order_acquire);
    const usize t = __producer.tail.get(memory_order_acquire);
    const usize n = t - h;
    return n < N ? n : N;
  }

  static constexpr usize
  max_size() noexcept
  {
    return N;
  }
};

};      // namespace micron
