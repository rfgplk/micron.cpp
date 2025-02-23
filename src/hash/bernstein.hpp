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

};
