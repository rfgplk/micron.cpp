//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/sys/system.hpp"

namespace micron
{
bool
is_root(void)
{
  // NOTE: only a surface level check, root may not have a UID of 0
  return (posix::getuid() == 0 or posix::geteuid() == 0);
}
};
