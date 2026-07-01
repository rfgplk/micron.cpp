//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../queue/crossbeam.hpp"
#include "../types.hpp"

#include "async_sync.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// a bounded, coroutine-awaitable MPMC channel

namespace micron
{

enum class chan_status : u8 { ok = 0, empty = 1, closed = 2 };

template<class T> struct chan_pull {
  bool ok;
  T value;
};

template<class T, usize Cap = 256> class async_channel
{
  micron::crossbeam<T, Cap> __ring;
  micron::coro::async_semaphore __space{ static_cast<i64>(Cap) };      // free slots (<= ring capacity)
  micron::coro::async_semaphore __items{ 0 };                          // available items
  micron::atomic_token<u32> __closed{ 0 };

public:
  async_channel() = default;
  async_channel(const async_channel &) = delete;
  async_channel &operator=(const async_channel &) = delete;

  micron::task<bool>
  push(T __v)
  {
    co_await __space.acquire();
    if ( __closed.get(memory_order_acquire) ) co_return false;
    while ( !__ring.push(micron::move(__v)) ) __cpu_pause();      // holds a space permit -> succeeds
    __items.release();
    co_return true;
  }

  micron::task<chan_pull<T>>
  pull()
  {
    const bool __got = co_await __items.acquire();
    T __v{};
    if ( __got ) {
      while ( !__ring.pop(__v) ) __cpu_pause();      // permit held -> an item is guaranteed (may be mid-publish)
      __space.release();
      co_return chan_pull<T>{ true, micron::move(__v) };
    }
    // aborted/closed: no permit
    if ( __ring.pop(__v) ) {
      __space.release();
      co_return chan_pull<T>{ true, micron::move(__v) };
    }
    co_return chan_pull<T>{ false, T{} };      // empty -> closed & drained
  }

  micron::task<usize>
  push_bulk(const T *__src, usize __n)
  {
    usize __done = 0;
    while ( __done < __n ) {
      if ( __closed.get(memory_order_acquire) ) break;
      const i64 __got = __space.try_acquire_many(static_cast<i64>(__n - __done));
      if ( __got > 0 ) {
        for ( i64 __k = 0; __k < __got; ++__k ) {
          while ( !__ring.push(__src[__done]) ) __cpu_pause();
          ++__done;
        }
        __items.release(__got);
      } else {
        co_await __space.acquire();      // backpressure: block for one permit
        if ( __closed.get(memory_order_acquire) ) break;
        while ( !__ring.push(__src[__done]) ) __cpu_pause();
        ++__done;
        __items.release(1);
      }
    }
    co_return __done;
  }

  micron::task<usize>
  pull_bulk(T *__dst, usize __n)
  {
    usize __done = 0;
    while ( __done < __n ) {
      const i64 __got = __items.try_acquire_many(static_cast<i64>(__n - __done));
      if ( __got > 0 ) {
        for ( i64 __k = 0; __k < __got; ++__k ) {
          T __v{};
          while ( !__ring.pop(__v) ) __cpu_pause();
          __dst[__done] = micron::move(__v);
          ++__done;
        }
        __space.release(__got);
      } else {
        const bool __got_one = co_await __items.acquire();      // block for one item (or wake on close)
        T __v{};
        if ( __got_one ) {
          while ( !__ring.pop(__v) ) __cpu_pause();      // permit held -> an item is guaranteed
          __dst[__done] = micron::move(__v);
          ++__done;
          __space.release(1);
        } else if ( __ring.pop(__v) ) {      // closed: drain any still-buffered item
          __dst[__done] = micron::move(__v);
          ++__done;
          __space.release(1);
        } else {
          break;      // closed & drained
        }
      }
    }
    co_return __done;
  }

  // non-blocking enqueue
  bool
  post(T __v) noexcept
  {
    if ( __closed.get(memory_order_acquire) || !__space.try_acquire() ) return false;
    while ( !__ring.push(micron::move(__v)) ) __cpu_pause();
    __items.release();
    return true;
  }

  // non-blocking dequeue
  chan_status
  try_pull(T &__out) noexcept
  {
    if ( __items.try_acquire() ) {
      while ( !__ring.pop(__out) ) __cpu_pause();      // permit held -> an item is guaranteed
      __space.release();
      return chan_status::ok;
    }
    if ( __closed.get(memory_order_acquire) ) {
      if ( __ring.pop(__out) ) {
        __space.release();
        return chan_status::ok;
      }
      return chan_status::closed;
    }
    return chan_status::empty;
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
