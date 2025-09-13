//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "std.hpp"

constexpr bool
is_version(int major, int minor, int patch)
{
  if constexpr ( major == MICRON_VERSION_MAJOR and minor == MICRON_VERSION_MINOR and patch == MICRON_VERSION_PATCH )
    return true;
  return false;
}

constexpr int
get_version(void)
{
  return MICRON_VERSION_MAJOR | MICRON_VERSION_MINOR | MICRON_VERSION_PATCH;
}
