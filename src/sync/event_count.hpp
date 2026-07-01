//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "futex.hpp"

#include "../atomic/atomic.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// condition counter over a futex

namespace micron
{

class event_count
{
  micron::atomic_token<u32> __epoch{ 0 };        // bumped on every notify; waiters sleep while it == their key
  micron::atomic_token<u32> __waiters{ 0 };      // count of threads between prepare_wait and commit/cancel

public:
  event_count() noexcept = default;
  event_count(const event_count &) = delete;
  event_count &operator=(const event_count &) = delete;

  [[nodiscard]] u32
  prepare_wait() noexcept
  {
    __waiters.fetch_add(1, memory_order::acq_rel);
    return __epoch.get(memory_order::acquire);
  }

  // abandon the wait
  void
  cancel_wait() noexcept
  {
    __waiters.sub_fetch(1, memory_order::acq_rel);
  }

  void
  commit_wait(u32 __key, const timespec_t *__timeout = nullptr) noexcept
  {
    while ( __epoch.get(memory_order::acquire) == __key ) {
      auto __r = micron::__futex(__epoch.ptr(), futex_wait | futex_private_flag, __key, const_cast<timespec_t *>(__timeout), nullptr, 0);
      if ( __timeout != nullptr && __r == -110 ) break;             // ETIMEDOUT: honor the finite timeout
      if ( __r < 0 && __r != -11 && __r != -4 && __r != -110 )      // not EAGAIN/EINTR/ETIMEDOUT -> bail
        break;
    }
    __waiters.sub_fetch(1, memory_order::acq_rel);
  }

  // wake one parked waiter (if any)
  void
  notify_one() noexcept
  {
    __epoch.fetch_add(1, memory_order::acq_rel);
    if ( __waiters.get(memory_order::acquire) != 0 ) micron::wake_futex(__epoch.ptr(), 1);
  }

  // wake all parked waiters (if any)
  void
  notify_all() noexcept
  {
    __epoch.fetch_add(1, memory_order::acq_rel);
    if ( __waiters.get(memory_order::acquire) != 0 ) micron::wake_futex(__epoch.ptr(), 0x7fffffff);
  }
};

};      // namespace micron
