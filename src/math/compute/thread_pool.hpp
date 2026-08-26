//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../thread/pool.hpp"
#include "../compute.hpp"

// thread pool for the graph compute module

namespace micron::math::compute
{

class thread_pool_executor
{
  micron::concurrent_arena *__arena{};

public:
  explicit thread_pool_executor(micron::concurrent_arena &arena) noexcept : __arena(micron::addressof(arena)) { }

  [[nodiscard]] bool
  submit(void (*function)(void *) noexcept, void *argument) noexcept
  {
    if ( !__arena || !function ) return false;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      __arena->add(function, argument);
    } catch ( ... ) {
      return false;
    }
#else
    __arena->add(function, argument);
#endif
    return true;
  }
};

};      // namespace micron::math::compute
