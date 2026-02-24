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
bernstein_32(const byte *data, size_t sz)
{
  u32 hash = 5381;
  while ( --sz ) {
    hash = (hash * 33) + static_cast<u32>(*(data + sz));     // moves backwords
  }
  return hash;
}

};     // namespace micron
