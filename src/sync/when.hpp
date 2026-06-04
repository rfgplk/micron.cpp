//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../mutex/locks/spin_lock.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../atomic/atomic.hpp"

#include "pause.hpp"

#include "../thread/spawn.hpp"

// (when) is a framework for enabling asynchronous work. similar to 'async' in the STL and other languages, but with a
// twist the general usage pattern is you select some boolean condition (which must evaluate to true) with a
// function/workload attached. upon calling when a background thread is launched which periodically checks if the
// condition has been met, and if it has, launches the task

namespace micron
{

template<typename Fn, typename... Args>
void
when(atomic_token<bool> *result, Fn fn, Args... args)
{
  go([result, fn = micron::move(fn), ... args = micron::move(args)]() mutable {
    while ( result->get(memory_order::acquire) != true ) cpu_pause<500>();
    fn(args...);
  });
}

template<typename Fn, typename... Flags>
  requires(micron::is_invocable_v<Fn> && (micron::is_same_v<Flags, atomic_token<bool>> && ...))
void
when_any(Fn fn, Flags *...flags)
{
  (go([f = flags, fn = fn]() mutable {
     while ( f->get(memory_order::acquire) != true ) cpu_pause<500>();
     fn();
   }),
   ...);
}

};      // namespace micron
