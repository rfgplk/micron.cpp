//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../syscall.hpp"
#include <unistd.h>

namespace micron
{

int
yield()
{
  micron::syscall(SYS_sched_yield);
  return 0;
}

};
