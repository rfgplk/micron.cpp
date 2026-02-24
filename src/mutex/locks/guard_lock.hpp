//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex.hpp"

#include "../../atomic/atomic.hpp"
#include "../../sync/yield.hpp"

namespace micron
{

template <class M = mutex> class lock_guard
{
  M *mtx;
  void (micron::mutex::*rptr)();

public:
  lock_guard(M &m, adopt_lock_t a) : mtx(&m), rptr(m.retrieve()) {};
  lock_guard(M &m) : mtx(&m), rptr(m()) {};
  lock_guard(M *m, adopt_lock_t a) : mtx(m), rptr(m->retrieve()) {};
  lock_guard(M *m) : mtx(m), rptr(m()) {};

  ~lock_guard() { (mtx->*rptr)(); }
};

template <memory_order Aq = memory_order::acquire, memory_order Rl = memory_order::release> class free_guard
{
  atomic_flag *flag;
  bool owned;

  bool
  try_acquire() noexcept
  {
    return !flag->test_and_set(Aq);
  }

  void
  acquire() noexcept
  {
    while ( !try_acquire() )
      ;
  }

public:
  explicit free_guard(atomic_flag *f) : flag(f), owned(false)
  {
    acquire();
    owned = true;
  }

  free_guard(atomic_flag &f, adopt_lock_t) : flag(&f), owned(true) {}

  free_guard(atomic_flag &f, defer_lock_t) : flag(&f), owned(false) {}

  free_guard(atomic_flag &f, try_to_lock_t) : flag(&f), owned(try_acquire()) {}

  free_guard(atomic_flag *f, adopt_lock_t) : flag(f), owned(true) {}

  free_guard(atomic_flag *f, defer_lock_t) : flag(f), owned(false) {}

  free_guard(atomic_flag *f, try_to_lock_t) : flag(f), owned(try_acquire()) {}

  // move construction — drain the source completely, leaving it inert
  free_guard(free_guard &&o) noexcept : flag(o.flag), owned(o.owned)
  {
    o.flag = nullptr;
    o.owned = false;
  }

  // move assignment — release any lock we currently hold, then steal from source
  free_guard &
  operator=(free_guard &&o) noexcept
  {
    if ( this == &o )
      return *this;

    // drop our current acquisition before overwriting state
    if ( owned && flag )
      flag->clear(Rl);

    flag = o.flag;
    owned = o.owned;
    o.flag = nullptr;
    o.owned = false;

    return *this;
  }

  ~free_guard()
  {
    if ( owned && flag )
      flag->clear(Rl);
  }

  bool
  owns() const noexcept
  {
    return owned;
  }

  bool
  lock() noexcept
  {
    if ( owned || !flag )
      return false;
    acquire();
    owned = true;
    return true;
  }

  bool
  try_lock() noexcept
  {
    if ( owned || !flag )
      return false;
    owned = try_acquire();
    return owned;
  }

  void
  unlock() noexcept
  {
    if ( owned && flag ) {
      flag->clear(Rl);
      owned = false;
    }
  }

  atomic_flag *
  release() noexcept
  {
    owned = false;
    atomic_flag *tmp = flag;
    flag = nullptr;
    return tmp;
  }

  // returns true if the guard is in a valid (non-moved-from) state
  bool
  valid() const noexcept
  {
    return flag != nullptr;
  }

  explicit
  operator bool() const noexcept
  {
    return owned;
  }

  free_guard(const free_guard &) = delete;
  free_guard &operator=(const free_guard &) = delete;
};

};     // namespace micron
