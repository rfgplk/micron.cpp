//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "futex.hpp"

#include "../atomic/atomic.hpp"
#include "../mutex/token.hpp"
#include "../types.hpp"

#include "../memory/actions.hpp"

namespace micron
{
// synchronization mechanism across threads or resources
class basic_semaphore
{
  micron::atomic_token<i32> counter;
  micron::atomic_token<u32> wakeups;

public:
  ~basic_semaphore() = default;

  basic_semaphore(void) : counter(0), wakeups(0) { }

  basic_semaphore(i32 __init) : counter(__init), wakeups(0) { }

  basic_semaphore(const basic_semaphore &) = delete;
  basic_semaphore &operator=(const basic_semaphore &) = delete;

  basic_semaphore(basic_semaphore &&o) : counter(micron::move(o.counter)), wakeups(micron::move(o.wakeups)) { }

  basic_semaphore &
  operator=(basic_semaphore &&o)
  {
    counter = micron::move(o.counter);
    wakeups = micron::move(o.wakeups);
    return *this;
  }

  void
  wait(void) noexcept
  {
    if ( counter.sub_fetch(1, memory_order::acquire) >= 0 ) return;      // a permit was there for us

    for ( ;; ) {
      u32 w = wakeups.get(memory_order::acquire);
      while ( w > 0 ) {
        if ( wakeups.compare_exchange_weak(w, w - 1, memory_order::acquire, memory_order::relaxed) ) return;
      }
      auto r = micron::__futex(wakeups.ptr(), futex_wait | futex_private_flag, 0u, nullptr, nullptr, 0);
      if ( r < 0 and r != -11 and r != -4 ) return;      // not EAGAIN/EINTR
    }
  }

  bool
  try_wait() noexcept
  {
    i32 old = counter.get(memory_order::relaxed);
    while ( old > 0 ) {
      if ( counter.compare_exchange_weak(old, old - 1, memory_order::acquire, memory_order::relaxed) ) return true;
    }
    return false;
  }

  void
  flag() noexcept
  {
    i32 o = counter.add_fetch(1, memory_order::release);
    if ( o <= 0 ) {      // this permit landed on a parked waiter: hand it a token, then wake one
      wakeups.add_fetch(1, memory_order::release);
      wake_futex(wakeups.ptr(), 1);
    }
  }

  i32
  value() const noexcept
  {
    return counter.get(memory_order::relaxed);
  }

  void
  reset(i32 init = 0) noexcept
  {
    counter.store(init, memory_order::relaxed);
    wakeups.store(0, memory_order::relaxed);
  }

  // for coroutines (WIP)
  void
  abort() noexcept
  {
    counter.store(0x40000000, memory_order::release);
    wakeups.store(0x7fffffffu, memory_order::release);      // every parked waiter finds a token
    wake_futex(wakeups.ptr(), 0x7fffffff);
  }
};

template<auto Fn> class semaphore
{
  basic_semaphore sem;

  void
  __trigger()
  {
    sem.flag();
  }

public:
  ~semaphore() = default;

  semaphore(void) : sem(1) { }

  semaphore(const semaphore &) = delete;
  semaphore &operator=(const semaphore &) = delete;

  semaphore(semaphore &&o) : sem(micron::move(o.sem)) { }

  semaphore &
  operator=(semaphore &&o)
  {
    sem = micron::move(o.sem);
    return *this;
  }

  template<typename... Args>
  void
  run(Args &&...args)
  {
    sem.wait();
    Fn(micron::forward<Args>(args)...);
  }

  auto
  get_permit()
  {
    return &semaphore<Fn>::__trigger;
  }

  void
  permit()
  {
    sem.flag();
  }

  void
  permit_ahead(u64 n)
  {
    for ( ; n > 0; --n ) sem.flag();
  }
};

};      // namespace micron
