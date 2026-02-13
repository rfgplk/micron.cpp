//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../mutex/locks.hpp"
#include "../mutex/spinlock.hpp"
#include "../stack.hpp"

namespace micron
{

template <typename T> class channel
{
  // micron::spin_lock _spin;
  micron::mutex _lock;
  micron::stack<T> obj;
  auto
  __wait(void)
  {
    // NOTE: On exit from a scope (however accomplished), objects with automatic storage duration (6.7.5.3) that have
    // been constructed in that scope are destroyed in the reverse order of their construction. [Note: For temporaries,
    // see 6.7.7. — end note] Transfer out of a loop, out of a block, or back past an initialized variable with automatic
    // storage duration involves the destruction of objects with automatic storage duration that are in scope at the
    // point transferred from but not at the point transferred to.
  rst: {     // doing it like this to prevent needing two separate locks
    __cpu_pause();
    micron::lock_guard m(_lock);
    if ( obj.empty() ) {
      goto rst;
    }
    return m;     // prevent _lock from being released;
  }
  }

public:
  channel(void) : obj{} {}
  template <typename... Args> channel(Args &&...args)     // : _spin{}
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
    auto m = __wait();     // keeps lock throughout scope, NOTE: although lock_guards are immovable, ellision allows this
                           // operation
    to = obj();
    return *this;
  }
  inline bool
  operator!(void)
  {
    return obj.empty();
  }
};

};
