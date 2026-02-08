//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
u32
fnv_32(const byte *data, u32 seed, size_t sz)
{
  u64 hash = 14695981039346656037ULL + (31 * seed); 
  for ( u32 i = 0; i < sz; i++ ) {
    hash = hash ^ (unsigned char)data[i];
    hash = hash * 1099511628211ULL;
  }
  return hash;
}
}
