//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "allocation/__internal.hpp"
#include "types.hpp"

namespace micron
{
template <typename T = void>
T *
alloc(size_t sz)
{
  return reinterpret_cast<T *>(micron::__alloc(sz));
}

template <typename T = void>
void
free(T *ptr)
{
  micron::__free(ptr);
}
};
