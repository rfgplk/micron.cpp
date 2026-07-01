//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../queue/crossbeam.hpp"
#include "../sync/semaphore.hpp"
#include "../types.hpp"

namespace micron
{

template<class T, usize Cap = 256> class sync_channel
{
  micron::crossbeam<T, Cap> __ring;
  micron::basic_semaphore __space{ static_cast<i32>(Cap) };      // free slots
  micron::basic_semaphore __items{ 0 };                          // available items
  micron::atomic_token<u32> __closed{ 0 };

public:
  sync_channel() = default;
  sync_channel(const sync_channel &) = delete;
  sync_channel &operator=(const sync_channel &) = delete;

  bool
  push(T __v) noexcept
  {
    if ( __closed.get(memory_order_acquire) ) return false;
    __space.wait();
    if ( __closed.get(memory_order_acquire) ) return false;      // woken by close()'s abort
    while ( !__ring.push(micron::move(__v)) ) __cpu_pause();
    __items.flag();
    return true;
  }

  bool
  pull(T &__out) noexcept
  {
    __items.wait();
    if ( !__closed.get(memory_order_acquire) ) {
      while ( !__ring.pop(__out) ) __cpu_pause();
      __space.flag();
      return true;
    }
    if ( __ring.pop(__out) ) {      // closed: drain any still-buffered item
      __space.flag();
      return true;
    }
    return false;      // empty after wake -> closed & drained
  }

  bool
  try_push(T __v) noexcept
  {
    if ( __closed.get(memory_order_acquire) || !__space.try_wait() ) return false;
    while ( !__ring.push(micron::move(__v)) ) __cpu_pause();
    __items.flag();
    return true;
  }

  bool
  try_pull(T &__out) noexcept
  {
    if ( !__items.try_wait() ) return false;
    if ( __ring.pop(__out) ) {
      __space.flag();
      return true;
    }
    return false;
  }

  void
  close() noexcept
  {
    __closed.store(1, memory_order_release);
    __items.abort();
    __space.abort();
  }

  bool
  is_closed() const noexcept
  {
    return __closed.get(memory_order_acquire) != 0;
  }
};

};      // namespace micron
