//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// interrupt masking, inside a Linux kernel module

#include "__kport_abi.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

using irq_state = usize;

inline irq_state
irq_save(void) noexcept
{
  return static_cast<irq_state>(::mc_kport_irq_save());
}

inline void
irq_restore(irq_state __state) noexcept
{
  ::mc_kport_irq_restore(static_cast<mc_kport_usize>(__state));
}

};      // namespace port
};      // namespace micron
