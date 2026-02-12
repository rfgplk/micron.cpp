//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "linux/sys/signal.hpp"

namespace micron
{

void
sys_exit(int ret)
{
  micron::syscall(SYS_exit, ret);
}

constexpr static const int exit_ok = 0;

__attribute__((noreturn)) void
exit(int s = exit_ok)
{
  __builtin_exit(s);
}
__attribute__((noreturn)) void
abort(void)
{
  __builtin_abort();
}
__attribute__((noreturn)) void
abort(int ret)
{
  sys_exit(ret);
}

__attribute__((noreturn)) void
quick_exit(const int s = sig_abrt)
{
  sys_exit(s);
}

};
