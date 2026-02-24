//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../sync/yield.hpp"

namespace micron
{

class spin_lock
{
  atomic_token<bool> tk;

  void
  reset()
  {
    tk.store(ATOMIC_OPEN);
  }

public:
  ~spin_lock() = default;

  spin_lock() : tk(ATOMIC_OPEN) {}

  explicit spin_lock(bool state) : tk(state) {}

  spin_lock(const spin_lock &) = delete;
  spin_lock(spin_lock &&) = delete;
  spin_lock &operator=(const spin_lock &) = delete;

  // TTAS
  auto
  operator()()
  {
    for ( ;; ) {
      while ( tk.get(memory_order::acquire) )
        __cpu_pause();
      if ( tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) )
        break;
    }
    return &spin_lock::reset;
  }

  auto
  lock()
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    if ( tk.get(memory_order::relaxed) )
      return false;
    return tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED);
  }

  void
  unlock() noexcept
  {
    tk.store(ATOMIC_OPEN, memory_order::release);
  }

  auto
  retrieve()
  {
    return &spin_lock::reset;
  }

  bool
  operator!() const
  {
    return tk.get(memory_order::relaxed) != ATOMIC_LOCKED;
  }

  bool
  is_locked() const noexcept
  {
    return tk.get(memory_order::relaxed) == ATOMIC_LOCKED;
  }
};
};     // namespace micron
