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
enum class states { RED, YELLOW, GREEN };

enum class priority { low, regular, high, realtime };

// synchronization mechanism across threads or resources
class basic_semaphore
{
  micron::atomic_token<i32> counter;

public:
  ~basic_semaphore() = default;
  basic_semaphore(void) : counter(0) {}
  basic_semaphore(i32 __init) : counter(__init) {}
  basic_semaphore(const basic_semaphore &) = delete;
  basic_semaphore &operator=(const basic_semaphore &) = delete;
  basic_semaphore(basic_semaphore &&o) : counter(micron::move(o.counter)) {}
  basic_semaphore &
  operator=(basic_semaphore &&o)
  {
    counter = micron::move(o.counter);
    return *this;
  }
  void
  wait(void) noexcept
  {
    i32 v = counter.sub_fetch(1, memory_order::acquire);
    if ( v < 0 ) {
      wait_futex(counter.ptr(), v);
    }
  }
  bool
  try_wait() noexcept
  {
    i32 old = counter.get(memory_order::relaxed);
    while ( old > 0 ) {
      if ( counter.compare_exchange_weak(old, old - 1, memory_order::acquire, memory_order::relaxed) )
        return true;
    }
    return false;
  }
  void
  flag() noexcept
  {
    i32 o = counter.add_fetch(1, memory_order::release);
    if ( o < 0 ) {
      release_futex(counter.ptr(), 1);
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
  }
};

template <auto Fn> class semaphore
{
  basic_semaphore sem;

  void
  __trigger()
  {
    sem.flag();
  }

public:
  ~semaphore() = default;
  semaphore(void) : sem(1) {}
  semaphore(const semaphore &) = delete;
  semaphore &operator=(const semaphore &) = delete;
  semaphore(semaphore &&o) : sem(micron::move(o.sem)) {}
  semaphore &
  operator=(semaphore &&o)
  {
    sem = micron::move(o.sem);
    return *this;
  }
  template <typename... Args>
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
    for ( ; n > 0; --n )
      sem.flag();
  }
};

};     // namespace micron
