//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/cmemory.hpp"
#include "../../simd/types.hpp"
#include "../../types.hpp"
#include "config.hpp"

namespace abc
{

inline __attribute__((always_inline)) void
zero_on_alloc(byte *addr, size_t sz = 0)
{
  // addr will always be valid
  if constexpr ( __default_zero_on_alloc ) {
    if ( sz == 0 ) {
      sz = *reinterpret_cast<i64 *>(addr - sizeof(micron::simd::i256));
    }
    micron::bzero(addr, sz);
  }
}

inline __attribute__((always_inline)) void
zero_on_free(byte *addr, size_t sz = 0)
{
  // addr will always be valid
  if constexpr ( __default_zero_on_free ) {
    if ( sz == 0 ) {
      sz = *reinterpret_cast<i64 *>(addr - sizeof(micron::simd::i256));
    }
    micron::bzero(addr, sz);
  }
}

inline __attribute__((always_inline)) void
full_on_free(byte *addr, size_t sz = 0)
{
  // addr will always be valid
  if constexpr ( __default_full_on_free ) {
    if ( sz == 0 ) {
      sz = *reinterpret_cast<i64 *>(addr - sizeof(micron::simd::i256));
    }
    micron::memset(addr, 0xFF, sz);
  }
}
// TODO: add page guards and protected memory later
};
