//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{

struct __set_empty_v {
  constexpr bool
  operator==(const __set_empty_v &) const noexcept
  {
    return true;
  }

  constexpr bool
  operator!=(const __set_empty_v &) const noexcept
  {
    return false;
  }
};

};      // namespace micron
