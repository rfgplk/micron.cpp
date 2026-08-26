//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// amd64 kernels

namespace micron::math::graphs::kernels::__impl
{
#if defined(__micron_x86_avx2)
using __graph_word_vector = u64 __attribute__((vector_size(32)));
#else
using __graph_word_vector = u64 __attribute__((vector_size(16)));
#endif

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
  constexpr usize lanes = sizeof(__graph_word_vector) / sizeof(u64);
  usize result = 0;
  usize i = 0;
  for ( ; i + lanes <= count; i += lanes ) {
    __graph_word_vector value{};
    u64 lane[lanes]{};
    __builtin_memcpy(&value, words + i, sizeof(value));
    __builtin_memcpy(lane, &value, sizeof(value));
    for ( usize j = 0; j < lanes; ++j ) result += static_cast<usize>(__builtin_popcountll(lane[j]));
  }
  for ( ; i < count; ++i ) result += static_cast<usize>(__builtin_popcountll(words[i]));
  return result;
}

[[nodiscard]] inline usize
intersection_popcount_arch(const u64 *a, const u64 *b, usize count) noexcept
{
  constexpr usize lanes = sizeof(__graph_word_vector) / sizeof(u64);
  usize result = 0;
  usize i = 0;
  for ( ; i + lanes <= count; i += lanes ) {
    __graph_word_vector av{}, bv{}, value{};
    u64 lane[lanes]{};
    __builtin_memcpy(&av, a + i, sizeof(av));
    __builtin_memcpy(&bv, b + i, sizeof(bv));
    value = av & bv;
    __builtin_memcpy(lane, &value, sizeof(value));
    for ( usize j = 0; j < lanes; ++j ) result += static_cast<usize>(__builtin_popcountll(lane[j]));
  }
  for ( ; i < count; ++i ) result += static_cast<usize>(__builtin_popcountll(a[i] & b[i]));
  return result;
}

};      // namespace micron::math::graphs::kernels::__impl
