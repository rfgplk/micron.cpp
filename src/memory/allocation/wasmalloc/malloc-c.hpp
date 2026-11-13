//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../errno.hpp"
#include "wasmalloc.hpp"

#define __MICRON_WASM_ALLOC_C __attribute__((weak, used))

extern "C" {
__MICRON_WASM_ALLOC_C void *
malloc(usize bytes) noexcept
{
  void *result = micron::wasmalloc::malloc(bytes);
  if ( result == nullptr && bytes != 0 ) errno = 12;
  return result;
}

__MICRON_WASM_ALLOC_C void *
calloc(usize count, usize bytes) noexcept
{
  void *result = micron::wasmalloc::calloc(count, bytes);
  if ( result == nullptr && count != 0 && bytes != 0 ) errno = 12;
  return result;
}

__MICRON_WASM_ALLOC_C void *
realloc(void *ptr, usize bytes) noexcept
{
  void *result = micron::wasmalloc::realloc(ptr, bytes);
  if ( result == nullptr && bytes != 0 ) errno = 12;
  return result;
}

__MICRON_WASM_ALLOC_C void
free(void *ptr) noexcept
{
  micron::wasmalloc::dealloc(ptr);
}

__MICRON_WASM_ALLOC_C void *
aligned_alloc(usize alignment, usize bytes) noexcept
{
  if ( alignment == 0 || (alignment & (alignment - 1)) != 0 || bytes % alignment != 0 ) {
    errno = 22;
    return nullptr;
  }
  void *result = micron::wasmalloc::aligned_alloc(alignment, bytes);
  if ( result == nullptr && bytes != 0 ) errno = 12;
  return result;
}

__MICRON_WASM_ALLOC_C void *
memalign(usize alignment, usize bytes) noexcept
{
  if ( alignment == 0 || (alignment & (alignment - 1)) != 0 ) {
    errno = 22;
    return nullptr;
  }
  void *result = micron::wasmalloc::aligned_balloc(alignment, bytes).ptr;
  if ( result == nullptr && bytes != 0 ) errno = 12;
  return result;
}

__MICRON_WASM_ALLOC_C int
posix_memalign(void **out, usize alignment, usize bytes) noexcept
{
  if ( out == nullptr || alignment < sizeof(void *) || (alignment & (alignment - 1)) != 0 ) return 22;
  void *result = micron::wasmalloc::aligned_balloc(alignment, bytes).ptr;
  if ( result == nullptr && bytes != 0 ) return 12;
  *out = result;
  return 0;
}

__MICRON_WASM_ALLOC_C void *
valloc(usize bytes) noexcept
{
  return memalign(__micron_wasm_allocation_granularity, bytes);
}

__MICRON_WASM_ALLOC_C void *
pvalloc(usize bytes) noexcept
{
  constexpr usize granularity = __micron_wasm_allocation_granularity;
  if ( bytes > static_cast<usize>(-1) - granularity + 1 ) {
    errno = 12;
    return nullptr;
  }
  const usize rounded = (bytes + granularity - 1) & ~(granularity - 1);
  return memalign(granularity, rounded == 0 ? granularity : rounded);
}
}

#undef __MICRON_WASM_ALLOC_C
