//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../memory/cache.hpp"

#include "../backoff.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// FIFO ticket lock
// (acquisition order is arrival order)

template<spin_policy P = spin_yield> class alignas(cache_line_size()) basic_ticket_lock
{
  alignas(cache_line_size()) atomic_token<u32> __next;
  alignas(cache_line_size()) atomic_token<u32> __serving;
  [[no_unique_address]] __lock_stats st;

  static constexpr u32 __prop_step = 8u;
  static constexpr u32 __prop_cap = 512u;

  void
  reset() noexcept
  {
    unlock();
  }

public:
  ~basic_ticket_lock() = default;

  basic_ticket_lock() noexcept : __next(0), __serving(0) { }

  explicit basic_ticket_lock(u32 seed) noexcept : __next(seed), __serving(seed) { }

  basic_ticket_lock(const basic_ticket_lock &) = delete;
  basic_ticket_lock(basic_ticket_lock &&) = delete;
  basic_ticket_lock &operator=(const basic_ticket_lock &) = delete;

  auto
  operator()() noexcept
  {
    const u32 my = __next.fetch_add(1, memory_order::acq_rel);

    if ( __serving.get(memory_order::acquire) != my ) {
      __counted_backoff<P> bo(st);
      for ( ;; ) {
        const u32 s = __serving.get(memory_order::acquire);
        if ( s == my ) break;

        const u32 dist = my - s;
        u32 n = dist > (__prop_cap / __prop_step) ? __prop_cap : dist * __prop_step;
        while ( n-- ) __cpu_pause();
        bo.relax();
      }
    }
    st.note_acquire();
    return &basic_ticket_lock::reset;
  }

  auto
  lock() noexcept
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    const u32 s = __serving.get(memory_order::acquire);
    const u32 n = __next.get(memory_order::relaxed);
    if ( s != n ) return false;
    if ( !__next.compare_and_swap(n, n + 1, memory_order::acquire, memory_order::relaxed) ) return false;
    st.note_acquire();
    return true;
  }

  void
  unlock() noexcept
  {
    const u32 s = __serving.get(memory_order::relaxed);
    if ( s == __next.get(memory_order::relaxed) ) {      // serving == next: nobody holds a ticket
      __micron_lock_misuse("ticket_lock::unlock with nothing held");
      return;
    }
    __serving.store(s + 1, memory_order::release);
  }

  auto
  retrieve() noexcept
  {
    return &basic_ticket_lock::reset;
  }

  bool
  operator!() const noexcept
  {
    return !is_locked();
  }

  bool
  is_locked() const noexcept
  {
    return __next.get(memory_order::relaxed) != __serving.get(memory_order::relaxed);
  }

  [[nodiscard]] u32
  queued() const noexcept
  {
    return __next.get(memory_order::relaxed) - __serving.get(memory_order::relaxed);
  }

  [[nodiscard]] u32
  serving() const noexcept
  {
    return __serving.get(memory_order::relaxed);
  }

  [[nodiscard]] const __lock_stats &
  stats() const noexcept
  {
    return st;
  }

  template<typename... T> friend void unlock(T &...);
};

using ticket_lock = basic_ticket_lock<spin_yield>;
using ticket_spin_lock = basic_ticket_lock<spin_only>;

};      // namespace micron
