//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#if defined(__micron_arch_wasm32)
#include "../bits/wasm/host.hpp"

namespace micron
{
inline void
yield() noexcept
{
  wasm::yield();
}
}      // namespace micron
#else

#include "../linux/sys/sched.hpp"

namespace micron
{
inline void
yield()
{
  posix::sched_yield();
}
};      // namespace micron

#endif
