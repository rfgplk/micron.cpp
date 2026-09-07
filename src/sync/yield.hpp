//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// NOTE: this used to include linux/sys/sched.hpp for one sched_yield, and paid 139 transitive
// headers for it. port/yield.hpp costs 16 and emits the same instruction.
#include "../port/yield.hpp"

namespace micron
{
inline void
yield()
{
  micron::port::yield();
}
};      // namespace micron
