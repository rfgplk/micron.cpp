//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::port; blocking and wake-up
//
// wait(u32 *, u32, i64) - wake(u32 *, i32) - __futex_linux(...) [linux only]

#include "__backend.hpp"

#if defined(__micron_port_linux)
#include "backends/wait_linux.hpp"
#elif defined(__micron_port_kernel)
#include "backends/wait_kernel.hpp"
#elif defined(__micron_port_metal)
#include "backends/wait_metal.hpp"
#else
#error "micron port: __backend.hpp selected no backend. This is a bug in __backend.hpp, not in your build."
#endif

namespace micron
{
namespace port
{

[[nodiscard]] inline bool
wait_unavailable(i64 __r) noexcept
{
  return __r < 0 && __r != -11 && __r != -4 && __r != -110;      // not -EAGAIN / -EINTR / -ETIMEDOUT
}

};      // namespace port
};      // namespace micron
