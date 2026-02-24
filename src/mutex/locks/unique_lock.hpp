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
template <lock_starts S, class M = mutex> class unique_lock
{
  M *mtx;
  void (micron::mutex::*rptr)();

  void
  __verify()
  {
    if ( !mtx )
      exc<except::library_error>("micron::unique_lock lock() no mtx");
  }

public:
  ~unique_lock()
  {
    if ( rptr )
      if ( mtx )
        (mtx->*rptr)();
  }

  unique_lock(M *m)
    requires(S == lock_starts::locked)
      : mtx(m), rptr((*m)()) {};
  unique_lock(M *m)
    requires(S == lock_starts::adopt)
      : mtx(m), rptr(m->retrieve()) {};
  unique_lock(M *m)
    requires(S == lock_starts::unlocked)
      : mtx(m), rptr(nullptr) {};
  unique_lock(M *m)
    requires(S == lock_starts::defer)
      : mtx(m), rptr(nullptr) {};
  unique_lock(M &m)
    requires(S == lock_starts::locked)
      : mtx(&m), rptr(m()) {};
  unique_lock(M &m)
    requires(S == lock_starts::adopt)
      : mtx(&m), rptr(m.retrieve()) {};
  unique_lock(M &m)
    requires(S == lock_starts::unlocked)
      : mtx(&m), rptr(nullptr) {};
  unique_lock(M &m)
    requires(S == lock_starts::defer)
      : mtx(&m), rptr(nullptr) {};
  unique_lock(const unique_lock &) = delete;

  unique_lock(unique_lock &&o) : mtx(o.mtx), rptr(o.rptr)
  {
    o.mtx = nullptr;
    o.rptr = nullptr;
  }

  unique_lock &operator=(const unique_lock &) = delete;

  unique_lock &
  operator=(unique_lock &&o)
  {
    mtx = o.mtx;
    rptr = o.rptr;
    o.mtx = nullptr;
    o.rptr = nullptr;
    return *this;
  }

  void
  lock()
  {
    __verify();
    rptr = (*mtx).operator()();     // like this so it's explicit
  }

  void
  try_lock()
  {
    __verify();
    if ( !rptr ) {
      if ( (*mtx).operator!() )
        rptr = (*mtx).operator()();
    }
  }

  void
  unlocks()
  {
    __verify();
    if ( rptr )
      (mtx->*rptr)();
    rptr = nullptr;
  }

  auto
  release() -> M *
  {
    __verify();
    M *t = mtx;
    mtx = nullptr;
    rptr = nullptr;
    return t;
  }

  void
  swap(unique_lock &o)
  {
    auto tm = mtx;
    auto tp = rptr;
    mtx = o.mtx;
    rptr = o.rptr;
    o.mtx = tm;
    o.rptr = tp;
  }
};

};     // namespace micron
