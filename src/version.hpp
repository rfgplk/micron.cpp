//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "std.hpp"

#ifndef __MICRON
#define __MICRON
#define __MICRONTL
#endif

constexpr static const int MICRON_VERSION_MAJOR = 0x000;
constexpr static const int MICRON_VERSION_MINOR = 0x050;
constexpr static const int MICRON_VERSION_PATCH = 0x004;

template <int __major, int __minor, int __patch>
constexpr bool
is_version()
{
  if constexpr ( __major == MICRON_VERSION_MAJOR and __minor == MICRON_VERSION_MINOR
                 and __patch == MICRON_VERSION_PATCH )
    return true;
  return false;
}

constexpr int
get_version(void)
{
  return MICRON_VERSION_MAJOR | MICRON_VERSION_MINOR | MICRON_VERSION_PATCH;
}
