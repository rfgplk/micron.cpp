//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/spinlock.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../thread/pause.hpp"

#include "../thread/spawn.hpp"

// (when) is a framework for enabling asynchronous work. similar to 'async' in the STL and other languages, but with a
// twist the general usage pattern is you select some boolean condition (which must evaluate to true) with a
// function/workload attached. upon calling when a background thread is launched which periodically checks if the
// condition has been met, and if it has, launches the task

namespace micron
{

template <typename D, typename Fn, typename... Args>
void
when(D *result, Fn &&fn, Args &&...args)
{
  go(
      [&](D *res) {
        while ( *res != true )
          cpu_pause<500>();
        fn(micron::forward<Args>(args)...);
      },
      result);
}

template <typename... D, typename Fn, typename... Args>
void
when_any(Fn &&fn, Args &&...args, D *...result)
{
  (go([&] {
     while ( *result != true )
       cpu_pause<500>();
     fn(micron::forward<Args>(args)...);
   }),
   ...);
}

};
