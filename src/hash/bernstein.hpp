//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
inline u32
bernstein_32(const byte *data, usize sz)
{
  u32 hash = 5381;
  for ( usize i = 0; i < sz; ++i ) {
    hash = (hash * 33) + static_cast<u32>(data[i]);
  }
  return hash;
}

};      // namespace micron
