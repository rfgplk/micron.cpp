//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__syscall.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline i32
exec_id(void) noexcept
{
  return static_cast<i32>(micron::syscall(SYS_gettid));
}

inline i32
process_id(void) noexcept
{
  return static_cast<i32>(micron::syscall(SYS_getpid));
}

inline bool
thread_alive(i32 __tid) noexcept
{
  return micron::syscall(SYS_tgkill, process_id(), __tid, 0) == 0;
}

inline i32
cpu_id(void) noexcept
{
  u32 __cpu = 0;
  u32 __node = 0;
  if ( micron::syscall(SYS_getcpu, &__cpu, &__node, nullptr) != 0 ) return -1;
  return static_cast<i32>(__cpu);
}

};      // namespace port
};      // namespace micron
