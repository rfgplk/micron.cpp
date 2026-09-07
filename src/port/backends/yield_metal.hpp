//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// yielding, on bare metal
//
// yield - cpu_relax

#include "../../bits/__pause.hpp"
#include "../../types.hpp"

namespace micron
{
namespace port
{

[[gnu::always_inline]] inline void
yield() noexcept
{
  __cpu_pause();
}

[[gnu::always_inline]] inline void
cpu_relax() noexcept
{
  __cpu_pause();
}

};      // namespace port
};      // namespace micron
