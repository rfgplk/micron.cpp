#pragma once

#include "../../types.hpp"

namespace abc
{
constexpr static size_t __hdr_offset = sizeof(micron::simd::i256);
inline addr_t*
get_metadata_addr(addr_t *ptr)
{
  return reinterpret_cast<addr_t*>(reinterpret_cast<byte*>(ptr) - __hdr_offset);
}


};
