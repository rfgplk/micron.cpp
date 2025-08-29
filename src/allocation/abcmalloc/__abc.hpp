//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

#include "../linux/kmemory.hpp"

#include "../../except.hpp"
#include "../../type_traits.hpp"

#include "malloc.hpp"

namespace abc
{

template <typename T>
  requires(micron::is_integral_v<T>)
struct __abc_allocator {
  inline static T *
  alloc(size_t n)     // allocate 'smartly'
  {
    T *ptr = abc::alloc(n);
    if ( ptr == (void *)-1 )
      throw micron::except::memory_error("std_allocator::alloc(): mmap() failed");
    return ptr;
  };

  inline static void
  dealloc(T *mem, size_t len)
  {     // deallocate at location N
    if ( mem == nullptr )
      throw micron::except::memory_error("std_allocator::dealloc(): nullptr was provided");
    abc::dealloc(mem, len);
    mem = nullptr;
  }
};

};
