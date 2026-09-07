//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__kport_abi.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline i32
exec_id(void) noexcept
{
  return static_cast<i32>(::mc_kport_exec_id());
}

inline i32
process_id(void) noexcept
{
  return static_cast<i32>(::mc_kport_process_id());
}

inline bool
thread_alive(i32 __tid) noexcept
{
  (void)__tid;
  return true;
}

inline i32
cpu_id(void) noexcept
{
  return static_cast<i32>(::mc_kport_cpu_id());
}

};      // namespace port
};      // namespace micron
