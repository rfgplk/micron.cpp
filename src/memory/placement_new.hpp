//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// global templated placement-new
// is in its own header so we can easily pull this in without accidentally circularly including abcmalloc (was previously in arena)

#include "../types.hpp"

template<typename P>
void *
operator new(usize size, P *ptr)
{
  (void)size;
  return ptr;
}
