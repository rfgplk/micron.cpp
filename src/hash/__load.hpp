//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// unaligned little-endian punning loads for the checksum kernels
namespace micron
{
namespace hashes
{

constexpr u32
__load32(const u8 *p) noexcept
{
#if defined(__micron_endian_little)
  if !consteval {
    u32 v;
    __builtin_memcpy(&v, p, 4);
    return v;
  }
#endif
  return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

constexpr u64
__load64(const u8 *p) noexcept
{
#if defined(__micron_endian_little)
  if !consteval {
    u64 v;
    __builtin_memcpy(&v, p, 8);
    return v;
  }
#endif
  return u64(p[0]) | (u64(p[1]) << 8) | (u64(p[2]) << 16) | (u64(p[3]) << 24) | (u64(p[4]) << 32) | (u64(p[5]) << 40) | (u64(p[6]) << 48)
         | (u64(p[7]) << 56);
}

};      // namespace hashes
};      // namespace micron
