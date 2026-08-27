//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#define MICRON_ALLOW_GLIBC_MALLOC 1
#include "../../src/allocator.hpp"

int
main()
{
  int *value = micron::stl_allocator<int>::allocate(1);
  *value = 7;
  const int result = *value;
  micron::stl_allocator<int>::deallocate(value, 1);
  return result;
}
