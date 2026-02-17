//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"

namespace micron
{
class mutex
{
  atomic_token<bool> tk;

  void
  reset()
  {
    tk.store(ATOMIC_OPEN);
  }

public:
  mutex() = default;
  ~mutex() = default;

  auto
  operator()()
  {
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) ) {
    };
    return &mutex::reset;
  }

  bool
  operator!()
  {
    return (tk.get() != ATOMIC_LOCKED);     // inverse if it's locked false
  }

  auto
  lock()
  {
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) ) {
    };
    return &mutex::reset;
  }

  auto
  retrieve()
  {
    return &mutex::reset;
  }

  template <typename... T> friend void unlock(T &...);
  mutex(const mutex &) = delete;
  mutex(mutex &&) = delete;
  mutex &operator=(const mutex &) = delete;
};
};
