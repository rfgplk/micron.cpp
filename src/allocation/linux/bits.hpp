//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

namespace micron
{

template <typename T = byte> struct alignas(16) __chunk {     // total memory allocated
  T *ptr;
  size_t len;
  bool
  zero(void) const noexcept
  {
    if ( ptr == nullptr or len == 0 )
      return true;
    return false;
  }
};
};
