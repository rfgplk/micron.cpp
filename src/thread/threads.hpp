//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__abc_mt.hpp"      // autofires MICRON_ABC_MT; must precede abcmalloc

#include "../type_traits.hpp"
#include "../types.hpp"

#include "../linux/__includes.hpp"
#include "../linux/sys/micthread/threads.hpp"
#include "../linux/sys/resource.hpp"
#include "../linux/sys/system.hpp"
#include "../memory/stack_constants.hpp"

#include "../atomic/atomic.hpp"
#include "../control.hpp"
#include "../memory/cmemory.hpp"
#include "../memory/mman.hpp"
#include "../pointer.hpp"
#include "../sync/until.hpp"
#include "../sync/yield.hpp"
#include "../tags.hpp"
#include "../tuple.hpp"

#include "../linux/process/signals.hpp"

#include "thread_types/async_thread.hpp"
#include "thread_types/auto_thread.hpp"
#include "thread_types/group_thread.hpp"
#include "thread_types/reg_thread.hpp"
#include "thread_types/void_thread.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// availability
//
// WARNING: do not rehandroll this by probing /proc/self/auxv
// if chroot()ed or in a container with no /proc, probing is a false negative that disables threading for no reason
[[nodiscard]] inline bool
threads_available(void) noexcept
{
#if defined(__micron_freestanding)
  return micthread::available();
#else
  return true;
#endif
}

};      // namespace micron
