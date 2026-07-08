//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../types.hpp"

#include "waiter_list.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// coroutine reader/writer lock

namespace micron
{
namespace coro
{

class async_rwlock
{
  struct __rw_node {
    micron::atomic_token<usize> __st{ 0 };      // 0 idle | 1 granted | else parked frame pointer
    __rw_node *__next = nullptr;
  };

  micron::atomic_token<u32> __lk{ 0 };      // internal spinlock for the fields below
  i64 __readers = 0;                        // active readers
  bool __writer = false;                    // a writer holds
  u32 __wwaiting = 0;                       // writers parked
  __rw_node *__rhead = nullptr;             // parked readers
  __rw_node *__whead = nullptr;             // parked writers

  void
  __acq() noexcept
  {
    u32 __e = 0;
    while ( !__lk.compare_exchange_weak(__e, 1u, memory_order_acq_rel, memory_order_acquire) ) {
      __e = 0;
      __cpu_pause();
    }
  }

  void
  __rel() noexcept
  {
    __lk.store(0u, memory_order_release);
  }

  static void
  __grant(__rw_node *__w) noexcept
  {
    const usize __old = __w->__st.swap(1u, memory_order_acq_rel);
    if ( __old != 0u ) __resume_waiter(reinterpret_cast<__frame_base *>(__old));
  }

public:
  async_rwlock() noexcept = default;
  async_rwlock(const async_rwlock &) = delete;
  async_rwlock &operator=(const async_rwlock &) = delete;

  struct [[nodiscard]] __rlock_awaiter {
    async_rwlock *__rw;
    __rw_node __node{};

    bool
    await_ready() noexcept
    {
      __rw->__acq();
      if ( !__rw->__writer && __rw->__wwaiting == 0u ) {
        ++__rw->__readers;
        __rw->__rel();
        return true;
      }
      __node.__next = __rw->__rhead;
      __rw->__rhead = &__node;
      __rw->__rel();
      return false;
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      usize __exp = 0u;
      const usize __f = reinterpret_cast<usize>(static_cast<__frame_base *>(&__h.promise()));
      return __node.__st.compare_exchange_strong(__exp, __f, memory_order_acq_rel, memory_order_acquire);
    }

    void
    await_resume() const noexcept
    {
    }
  };

  __rlock_awaiter
  lock_shared() noexcept
  {
    return __rlock_awaiter{ this };
  }

  void
  unlock_shared() noexcept
  {
    __acq();
    --__readers;
    if ( __readers == 0 && __wwaiting > 0u ) {
      __rw_node *__w = __whead;
      __whead = __w->__next;
      --__wwaiting;
      __writer = true;
      __rel();
      __grant(__w);
      return;
    }
    __rel();
  }

  struct [[nodiscard]] __wlock_awaiter {
    async_rwlock *__rw;
    __rw_node __node{};

    bool
    await_ready() noexcept
    {
      __rw->__acq();
      if ( !__rw->__writer && __rw->__readers == 0 ) {
        __rw->__writer = true;
        __rw->__rel();
        return true;
      }
      ++__rw->__wwaiting;
      __node.__next = __rw->__whead;
      __rw->__whead = &__node;
      __rw->__rel();
      return false;
    }

    template<class P>
    bool
    await_suspend(std::coroutine_handle<P> __h) noexcept
    {
      usize __exp = 0u;
      const usize __f = reinterpret_cast<usize>(static_cast<__frame_base *>(&__h.promise()));
      return __node.__st.compare_exchange_strong(__exp, __f, memory_order_acq_rel, memory_order_acquire);
    }

    void
    await_resume() const noexcept
    {
    }
  };

  __wlock_awaiter
  lock() noexcept
  {
    return __wlock_awaiter{ this };
  }

  void
  unlock() noexcept
  {
    __acq();
    __writer = false;
    if ( __wwaiting > 0u ) {
      __rw_node *__w = __whead;
      __whead = __w->__next;
      --__wwaiting;
      __writer = true;
      __rel();
      __grant(__w);
      return;
    }

    __rw_node *__rs = __rhead;
    __rhead = nullptr;
    i64 __cnt = 0;
    for ( __rw_node *__p = __rs; __p != nullptr; __p = __p->__next ) ++__cnt;
    __readers = __cnt;
    __rel();
    while ( __rs != nullptr ) {
      __rw_node *__nx = __rs->__next;
      __grant(__rs);
      __rs = __nx;
    }
  }
};

};      // namespace coro
};      // namespace micron
