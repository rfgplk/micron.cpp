//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../mutex/locks.hpp"
#include "../mutex/locks/spin_lock.hpp"
#include "../stack.hpp"

namespace micron
{

template<typename T> class channel
{
  // micron::spin_lock _spin;
  micron::mutex _lock;
  micron::stack<T> obj;

public:
  channel(void) : obj{} { }

  template<typename... Args> channel(Args &&...args)      // : _spin{}
  {
    (obj.emplace(args), ...);
  }

  channel &
  operator>>(T &&o)
  {
    micron::lock_guard m(_lock);
    obj.emplace(o);
    return *this;
  }

  channel &
  operator>>(T &o)
  {
    micron::lock_guard m(_lock);
    obj.push(o);
    return *this;
  }

  channel &
  operator<<(T &to)
  {
    for ( ;; ) {
      {
        micron::lock_guard m(_lock);
        if ( !obj.empty() ) {
          to = obj();
          return *this;
        }
      }
      __cpu_pause();
    }
  }

  inline bool
  operator!(void)
  {
    return obj.empty();
  }
};

};      // namespace micron
