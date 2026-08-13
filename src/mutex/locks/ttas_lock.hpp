//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"

#include "../backoff.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%$%%%%%%%%%%%%%
// ttas_lock
//
// test-and-test-and-set with exponential backoff
template<spin_policy P = spin_yield> class basic_ttas_lock
{
  atomic_token<bool> tk;
  [[no_unique_address]] __lock_stats st;

  void
  reset()
  {
    unlock();
  }

public:
  ~basic_ttas_lock() = default;

  basic_ttas_lock() : tk(ATOMIC_OPEN) { }

  explicit basic_ttas_lock(bool state) : tk(state) { }

  basic_ttas_lock(const basic_ttas_lock &) = delete;
  basic_ttas_lock(basic_ttas_lock &&) = delete;
  basic_ttas_lock &operator=(const basic_ttas_lock &) = delete;

  auto
  operator()()
  {
    if ( tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed) ) {
      st.note_acquire();
      return &basic_ttas_lock::reset;      // uncontended: one CAS, no backoff state touched
    }

    __counted_backoff<P> bo(st);
    for ( ;; ) {
      while ( tk.get(memory_order::relaxed) == ATOMIC_LOCKED ) bo.relax();
      if ( tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed) ) break;
      bo.relax();      // lost the race to another spinner; back off rather than re-CAS immediately
    }
    st.note_acquire();
    return &basic_ttas_lock::reset;
  }

  auto
  lock()
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    if ( tk.get(memory_order::relaxed) == ATOMIC_LOCKED ) return false;
    return tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed);
  }

  void
  unlock() noexcept
  {
    tk.store(ATOMIC_OPEN, memory_order::release);
  }

  auto
  retrieve()
  {
    return &basic_ttas_lock::reset;
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

  [[nodiscard]] const __lock_stats &
  stats() const noexcept
  {
    return st;
  }

  template<typename... T> friend void unlock(T &...);
};

using ttas_lock = basic_ttas_lock<spin_yield>;

// never leaves userspace
using ttas_spin_lock = basic_ttas_lock<spin_only>;

};      // namespace micron
