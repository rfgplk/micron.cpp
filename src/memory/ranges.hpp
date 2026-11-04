//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

namespace micron
{

// Counts are bytes. Empty/null ranges never overlap; no pointed-to byte is read.
inline bool
byte_ranges_overlap(const void *left, usize left_size, const void *right, usize right_size) noexcept
{
  if ( !left || !right || left_size == 0 || right_size == 0 ) return false;
  const uintptr_t a = reinterpret_cast<uintptr_t>(left);
  const uintptr_t b = reinterpret_cast<uintptr_t>(right);
  return a <= b ? b - a < left_size : a - b < right_size;
}

template<typename Left, typename Right>
  requires(sizeof(Left) == 1 && sizeof(Right) == 1)
constexpr bool
byte_ranges_overlap(const Left *left, usize left_size, const Right *right, usize right_size) noexcept
{
  if consteval {
    if ( !left || !right || left_size == 0 || right_size == 0 ) return false;
    // Ordering pointers from unrelated arrays is not a constant expression.
    for ( usize index = 0; index < left_size; ++index )
      if ( static_cast<const void *>(left + index) == static_cast<const void *>(right) ) return true;
    for ( usize index = 0; index < right_size; ++index )
      if ( static_cast<const void *>(right + index) == static_cast<const void *>(left) ) return true;
    return false;
  } else {
    return byte_ranges_overlap(static_cast<const void *>(left), left_size, static_cast<const void *>(right), right_size);
  }
}

};      // namespace micron
