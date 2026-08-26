//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%55
// arm64 neon kernels

namespace micron::math::graphs::kernels::__impl
{
using __graph_word_vector = u64 __attribute__((vector_size(16)));

inline void
word_union_arch(u64 *out, const u64 *a, const u64 *b, usize words) noexcept
{
  constexpr usize lanes = sizeof(__graph_word_vector) / sizeof(u64);
  usize i = 0;
  for ( ; i + lanes <= words; i += lanes ) {
    __graph_word_vector av{}, bv{}, value{};
    __builtin_memcpy(&av, a + i, sizeof(av));
    __builtin_memcpy(&bv, b + i, sizeof(bv));
    value = av | bv;
    __builtin_memcpy(out + i, &value, sizeof(value));
  }
  for ( ; i < words; ++i ) out[i] = a[i] | b[i];
}

inline void
word_intersection_arch(u64 *out, const u64 *a, const u64 *b, usize words) noexcept
{
  constexpr usize lanes = sizeof(__graph_word_vector) / sizeof(u64);
  usize i = 0;
  for ( ; i + lanes <= words; i += lanes ) {
    __graph_word_vector av{}, bv{}, value{};
    __builtin_memcpy(&av, a + i, sizeof(av));
    __builtin_memcpy(&bv, b + i, sizeof(bv));
    value = av & bv;
    __builtin_memcpy(out + i, &value, sizeof(value));
  }
  for ( ; i < words; ++i ) out[i] = a[i] & b[i];
}

[[nodiscard]] inline usize
popcount_arch(const u64 *words, usize count) noexcept
{
  usize result = 0;
  for ( usize i = 0; i < count; ++i ) result += static_cast<usize>(__builtin_popcountll(words[i]));
  return result;
}

[[nodiscard]] inline usize
intersection_popcount_arch(const u64 *a, const u64 *b, usize count) noexcept
{
  usize result = 0;
  for ( usize i = 0; i < count; ++i ) result += static_cast<usize>(__builtin_popcountll(a[i] & b[i]));
  return result;
}

};      // namespace micron::math::graphs::kernels::__impl
