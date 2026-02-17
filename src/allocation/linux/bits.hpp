//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../numerics.hpp"

namespace micron
{

template <typename T = byte> struct alignas(16) __chunk {     // total memory allocated
  T *ptr;
  size_t len;

  __chunk &
  operator=(nullptr_t t [[maybe_unused]])
  {
    ptr = nullptr;
    len = 0;
    return *this;
  }

  bool
  zero(void) const noexcept
  {
    if ( ptr == nullptr or len == 0 )
      return true;
    return false;
  }

  bool
  failed_allocation(void) const noexcept
  {
    return ((ptr == micron::numeric_limits<byte *>::max() - 1) and len == 0xFF);
  }

  bool
  invalid(void) const noexcept
  {
    return (ptr == (byte *)-1);
  }
};
};
