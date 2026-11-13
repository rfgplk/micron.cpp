//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../except.hpp"
#include "wasmalloc.hpp"

namespace micron::wasmalloc
{
template<typename T> struct __abc_allocator {
  static chunk<byte>
  calloc(usize bytes)
  {
    const auto result = zalloc(bytes);
    if ( bytes != 0 && result.ptr == nullptr ) exc<except::memory_error>("wasmalloc: heap exhausted");
    return result;
  }

  static chunk<byte>
  allocate_aligned(usize bytes, usize alignment)
  {
    const auto result = aligned_balloc(alignment, bytes);
    if ( bytes != 0 && result.ptr == nullptr ) exc<except::memory_error>("wasmalloc: aligned allocation failed");
    return result;
  }

  static void
  dealloc(T *ptr, usize = 0) noexcept
  {
    wasmalloc::dealloc(static_cast<void *>(ptr));
  }

  static void
  dealloc_aligned(T *ptr, usize) noexcept
  {
    wasmalloc::dealloc(static_cast<void *>(ptr));
  }
};
};      // namespace micron::wasmalloc
