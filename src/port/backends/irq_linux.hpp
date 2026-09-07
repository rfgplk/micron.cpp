//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// interrupt masking, in userspace linux
//
// (noop)

#include "../../types.hpp"

namespace micron
{
namespace port
{

using irq_state = usize;

[[gnu::always_inline]] inline irq_state
irq_save(void) noexcept
{
  return 0;
}

[[gnu::always_inline]] inline void
irq_restore(irq_state __state) noexcept
{
  (void)__state;
}

};      // namespace port
};      // namespace micron
