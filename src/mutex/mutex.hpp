//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"

namespace micron
{
class mutex
{
  atomic_token<bool> tk;

  void
  reset()
  {
    tk.store(ATOMIC_OPEN);
  }

public:
  ~mutex() = default;
  mutex() = default;

  auto
  operator()()
  {
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) ) {
    };
    return &mutex::reset;
  }

  bool
  operator!()
  {
    return (tk.get() != ATOMIC_LOCKED);
  }

  auto
  lock()
  {
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) ) {
    };
    return &mutex::reset;
  }

  bool
  try_lock() noexcept
  {
    return tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED);
  }

  void
  unlock() noexcept
  {
    tk.store(ATOMIC_OPEN);
  }

  auto
  retrieve()
  {
    return &mutex::reset;
  }

  bool
  is_locked() const noexcept
  {
    return tk.get() == ATOMIC_LOCKED;
  }

  template <typename... T> friend void unlock(T &...);

  mutex(const mutex &) = delete;
  mutex(mutex &&) = delete;
  mutex &operator=(const mutex &) = delete;
};

struct mcs_node {
  atomic_token<mcs_node *> next;     // successor in the queue
  atomic_token<bool> waiting;        // true  → still blocked
                                     // false → lock handed off to us

  mcs_node() noexcept : next(nullptr), waiting(false) {}
};

class queuing_mutex
{
public:
  using node_type = mcs_node;

private:
  atomic_token<mcs_node *> tail;

  void
  do_unlock(mcs_node &node) noexcept
  {
    mcs_node *expected_next = node.next.get(memory_order::acquire);

    if ( !expected_next ) {
      mcs_node *me = &node;
      if ( tail.compare_and_swap(me, nullptr) ) {
        return;
      }
      while ( !(expected_next = node.next.get(memory_order::acquire)) )
        __cpu_pause();
    }

    expected_next->waiting.store(false, memory_order::release);
  }

public:
  queuing_mutex() noexcept : tail(nullptr) {}

  queuing_mutex(const queuing_mutex &) = delete;
  queuing_mutex(queuing_mutex &&) = delete;
  queuing_mutex &operator=(const queuing_mutex &) = delete;

  auto
  operator()(mcs_node &node) noexcept
  {
    node.next.store(nullptr, memory_order::relaxed);
    node.waiting.store(true, memory_order::relaxed);

    mcs_node *prev = tail.swap(&node);     // xchg — seq_cst

    if ( prev ) {
      prev->next.store(&node, memory_order::release);
      while ( node.waiting.get(memory_order::acquire) )
        __cpu_pause();
    }
    return &queuing_mutex::do_unlock;
  }

  void
  lock(mcs_node &node) noexcept
  {
    operator()(node);
  }

  bool
  try_lock(mcs_node &node) noexcept
  {
    node.next.store(nullptr, memory_order::relaxed);
    node.waiting.store(false, memory_order::relaxed);

    mcs_node *expected = nullptr;
    return tail.compare_and_swap(expected, &node);
  }

  void
  unlock(mcs_node &node) noexcept
  {
    do_unlock(node);
  }

  bool
  is_locked() const noexcept
  {
    return tail.get(memory_order::relaxed) != nullptr;
  }
};

};     // namespace micron
