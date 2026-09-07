//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scheduler hand-off, on userspace Linux
// NOTE: a seccomp policy must keep SYS_sched_yield permitted (sec/groups.hpp:61 allowlists it)

#include "../../bits/__pause.hpp"

#include "__syscall.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

// give up the rest of this timeslice. cond_resched() in a module; wfe or a nop on a single-core MCU.
[[gnu::always_inline]] inline void
yield() noexcept
{
  (void)micron::syscall(SYS_sched_yield);
}

// hint to the core that this is a spin-wait. never a syscall, on any backend.
[[gnu::always_inline]] inline void
cpu_relax() noexcept
{
  __cpu_pause();
}

};      // namespace port
};      // namespace micron
