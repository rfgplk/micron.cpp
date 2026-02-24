//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../sync/yield.hpp"

#include "../../linux/sys/__threads.hpp"

namespace micron
{

class recursive_lock
{
  static constexpr size_t __ownerless = 0;

  atomic_token<size_t> owner;
  atomic_token<size_t> depth;

  static size_t
  current_thread() noexcept
  {
    return static_cast<size_t>(pthread::self());
  }

  void
  reset()
  {

    size_t d = depth.get(memory_order::relaxed);
    if ( d > 1 ) {
      depth.store(d - 1, memory_order::relaxed);
    } else {
      depth.store(0, memory_order::release);
      owner.store(__ownerless, memory_order::release);
    }
  }

public:
  recursive_lock() noexcept : owner(__ownerless), depth(0) {}

  recursive_lock(const recursive_lock &) = delete;
  recursive_lock(recursive_lock &&) = delete;
  recursive_lock &operator=(const recursive_lock &) = delete;

  auto
  operator()()
  {
    const size_t me = current_thread();

    for ( ;; ) {

      size_t cur = owner.get(memory_order::acquire);
      if ( cur == me ) {
        depth.fetch_add(1, memory_order::relaxed);
        return &recursive_lock::reset;
      }

      if ( cur == __ownerless ) {
        if ( owner.compare_and_swap(__ownerless, me) ) {
          depth.store(1, memory_order::release);
          return &recursive_lock::reset;
        }
      }

      __cpu_pause();
    }
  }

  auto
  lock()
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    const size_t me = current_thread();

    size_t cur = owner.get(memory_order::acquire);
    if ( cur == me ) {
      depth.fetch_add(1, memory_order::relaxed);
      return true;
    }
    if ( cur == __ownerless && owner.compare_and_swap(__ownerless, me) ) {
      depth.store(1, memory_order::release);
      return true;
    }
    return false;
  }

  void
  unlock() noexcept
  {
    reset();
  }

  auto
  retrieve()
  {
    return &recursive_lock::reset;
  }

  bool
  operator!() const noexcept
  {
    return owner.get(memory_order::relaxed) == __ownerless;
  }

  bool
  is_locked() const noexcept
  {
    return owner.get(memory_order::relaxed) != __ownerless;
  }

  size_t
  lock_depth() const noexcept
  {
    return depth.get(memory_order::relaxed);
  }

  template <typename... T> friend void unlock(T &...);
};

}     // namespace micron
