//  Copyright (c) 2026- David Lucius Severus
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__thread_exit_hook.hpp"
#include "runtime.hpp"

namespace micron
{
inline void
cancellation_point() noexcept
{
  __micron_park_checkpoint();
}

inline void
park() noexcept
{
  __micron_park_checkpoint();
}

[[noreturn]] inline void
crash() noexcept
{
  wasm::panic(134);
}

[[noreturn]] inline void
halt() noexcept
{
  wasm::panic(0);
}

inline void
mark() noexcept
{
  __atomic_signal_fence(__ATOMIC_SEQ_CST);
}
}      // namespace micron
