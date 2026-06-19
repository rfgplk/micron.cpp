//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"

#if !defined(__micron_freestanding)
#include "../linux/sys/__threads.hpp"
#endif

namespace micron
{

// NOTE: intentionally NOT noexcept when a deferred cancel is pending, we perform a forced unwind that propagates through this call
inline __attribute__((always_inline)) void
__testcancel(void)
{
#if !defined(__micron_freestanding)
  pthread::cancel();
#endif
}

};      // namespace micron
