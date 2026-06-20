//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../bits/__thread_exit_hook.hpp"

namespace micron
{

// the native cancellation/park checkpoint
inline __attribute__((always_inline)) void
__testcancel(void)
{
  // NOTE: no longer testcancel, our threads don't use that mechanism anymore
  micron::__micron_park_checkpoint();
}

};      // namespace micron
