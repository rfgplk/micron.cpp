//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
struct default_heap {
  template <typename T>
  static __attribute__((always_inline)) inline T *
  allocate(size_t size)
  {
    return ::operator new(size);
  }
  template <typename T>
  static __attribute__((always_inline)) inline void
  deallocate(size_t, T *data)
  {
    ::operator delete(data);
  }
};

};
