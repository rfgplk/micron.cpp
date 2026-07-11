//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{
// canonical FNV-1a
constexpr u32 fnv1a_32_basis = 2166136261u;
constexpr u32 fnv1a_32_prime = 16777619u;
constexpr u64 fnv1a_64_basis = 14695981039346656037ULL;
constexpr u64 fnv1a_64_prime = 1099511628211ULL;

template<u32 seed = 0>
inline u32
fnv1a_32(const byte *data, usize sz)
{
  u32 hash = fnv1a_32_basis ^ seed;
  for ( usize i = 0; i < sz; ++i ) {
    hash ^= static_cast<u32>(data[i]);
    hash *= fnv1a_32_prime;
  }
  return hash;
}

inline u32
fnv1a_32(const byte *data, u32 seed, usize sz)
{
  u32 hash = fnv1a_32_basis ^ seed;
  for ( usize i = 0; i < sz; ++i ) {
    hash ^= static_cast<u32>(data[i]);
    hash *= fnv1a_32_prime;
  }
  return hash;
}

template<u64 seed = 0>
inline u64
fnv1a_64(const byte *data, usize sz)
{
  u64 hash = fnv1a_64_basis ^ seed;
  for ( usize i = 0; i < sz; ++i ) {
    hash ^= static_cast<u64>(data[i]);
    hash *= fnv1a_64_prime;
  }
  return hash;
}

inline u64
fnv1a_64(const byte *data, u64 seed, usize sz)
{
  u64 hash = fnv1a_64_basis ^ seed;
  for ( usize i = 0; i < sz; ++i ) {
    hash ^= static_cast<u64>(data[i]);
    hash *= fnv1a_64_prime;
  }
  return hash;
}
}      // namespace micron
