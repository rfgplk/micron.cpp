//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/sysinfo.hpp"
#include "config.hpp"

namespace abc
{

inline auto
check_oom(void) -> bool
{
  if constexpr ( __default_oom_enable ) {
    micron::resources rs;
    size_t total_ram = rs.total_memory;
    size_t free_ram = rs.free_memory;
    if ( ((float)free_ram / (float)total_ram) <= __default_oom_limit_error ) {
      return true;
    } else if ( ((float)free_ram / (float)total_ram) <= __default_oom_limit_warn ) {
      // TODO: add an actual warning here, false for now
      return false;
    }
  }
  return false;
}

};
