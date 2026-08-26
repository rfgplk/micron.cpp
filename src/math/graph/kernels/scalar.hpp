//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// scalar baseline kernels

namespace micron::math::graphs::kernels::__impl
{

inline void
word_union_scalar(u64 *out, const u64 *a, const u64 *b, usize words) noexcept
{
  for ( usize i = 0; i < words; ++i ) out[i] = a[i] | b[i];
}

inline void
word_intersection_scalar(u64 *out, const u64 *a, const u64 *b, usize words) noexcept
{
  for ( usize i = 0; i < words; ++i ) out[i] = a[i] & b[i];
}

[[nodiscard]] inline usize
popcount_scalar(const u64 *words, usize count) noexcept
{
  usize result = 0;
  for ( usize i = 0; i < count; ++i ) result += static_cast<usize>(__builtin_popcountll(words[i]));
  return result;
}

[[nodiscard]] inline usize
intersection_popcount_scalar(const u64 *a, const u64 *b, usize count) noexcept
{
  usize result = 0;
  for ( usize i = 0; i < count; ++i ) result += static_cast<usize>(__builtin_popcountll(a[i] & b[i]));
  return result;
}

};      // namespace micron::math::graphs::kernels::__impl
