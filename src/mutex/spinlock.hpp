//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../sync/yield.hpp"
#include "mutex.hpp"

namespace micron
{
class spin_lock
{
  atomic_token<bool> _lock;

public:
  spin_lock(void) : _lock(ATOMIC_OPEN) {}
  spin_lock(bool state) : _lock(state) {}
  // TTAS
  void
  operator()(void)
  {
    for ( ;; ) {
      while ( _lock.get() ) {
#if defined(__GNUC__)
        __cpu_pause();
#else
        yield();
#endif
        // this is really GCC only, i'm afraid
        // TODO: add the same for other compilers
      }
      if ( !_lock.swap(ATOMIC_LOCKED) )
        break;
    }
  }
  void
  operator()(bool state)
  {
    for ( ;; ) {
      while ( _lock.get() != state ) {
#if defined(__GNUC__)
        __cpu_pause();
#else
        yield();
#endif
        // this is really GCC only, i'm afraid
        // TODO: add the same for other compiles
      }
      if ( !_lock.swap(!state) )
        break;
    }
  }
  void
  unlock()
  {
    _lock.store(ATOMIC_OPEN);
  }
  void
  try_unlock()
  {
    for ( ;; ) {
      if ( _lock.swap(ATOMIC_OPEN) )
        break;
      __cpu_pause();
    }
  }
};
};
