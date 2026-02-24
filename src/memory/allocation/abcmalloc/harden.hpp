//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../memory/cmemory.hpp"
#include "../../../simd/types.hpp"
#include "../../../types.hpp"
#include "config.hpp"

namespace abc
{

__attribute__((noreturn)) void
abort_state(void)
{
  micron::abort();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"

bool
fail_state(void)
{
  if constexpr ( __default_fail_result == 0 ) {
    abort_state();
  } else if constexpr ( __default_fail_result == 1 ) {
    abort_state();
  } else if constexpr ( __default_fail_result == 2 ) {
    return false;
  }
  return true;
}

#pragma GCC diagnostic pop

inline __attribute__((always_inline)) auto
check_constraint(const size_t sz) -> bool
{
  if constexpr ( __is_constrained ) {
    if ( sz > __alloc_limit and __alloc_limit != 0 )
      return true;
  }
  return false;
}

inline __attribute__((always_inline)) void
sanitize_on_alloc(byte *addr, size_t sz = 0)
{
  // addr will always be valid
  if constexpr ( __default_sanitize ) {
    if ( sz == 0 ) {
      sz = *reinterpret_cast<i64 *>(addr - sizeof(micron::simd::i256));
    }
    micron::memset(addr, __default_sanitize_with_on_alloc, sz);
  }
}

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
};     // namespace abc
