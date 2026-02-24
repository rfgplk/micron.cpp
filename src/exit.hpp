//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "types.hpp"

#include "syscall.hpp"

namespace micron
{

__attribute__((noreturn)) void
sys_exit(int ret)
{
  micron::syscall(SYS_exit, ret);
  __builtin_unreachable();
}

constexpr static const int exit_ok = 0;

__attribute__((noreturn)) void
exit(int s = exit_ok)
{
  sys_exit(s);
  __builtin_unreachable();
}

__attribute__((noreturn)) void
abort(void)
{
  sys_exit(6);
  __builtin_unreachable();
}

__attribute__((noreturn)) void
abort(int ret)
{
  sys_exit(ret);
  __builtin_unreachable();
}

__attribute__((noreturn)) void
quick_exit(const int s)
{
  sys_exit(s);
  __builtin_unreachable();
}

};     // namespace micron
