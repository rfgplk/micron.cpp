//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"

#include "../types.hpp"

namespace micron
{

inline constexpr size_t
cache_line_size()
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
  return 64;
#elif defined(__micron_arch_arm64) || defined(__micron_arch_arm32)
  return 64;
#else
  return 64;
#endif
}

inline constexpr size_t
constructive_interference_size()
{
  return cache_line_size();
}

inline constexpr size_t
destructive_interference_size()
{
  return cache_line_size();
}

};     // namespace micron
