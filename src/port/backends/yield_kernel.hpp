//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scheduling hints, inside a Linux kernel module

#include "__kport_abi.hpp"

#include "../../bits/__pause.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

// cond_resched()
inline void
yield() noexcept
{
  ::mc_kport_yield();
}

// hint to the core that this is a spin-wait
[[gnu::always_inline]] inline void
cpu_relax() noexcept
{
  __cpu_pause();
}

};      // namespace port
};      // namespace micron
