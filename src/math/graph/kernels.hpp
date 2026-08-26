//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"
#include "../../concepts.hpp"
#include "../../types.hpp"
#include "kernels/scalar.hpp"

#if defined(__micron_arch_amd64) || defined(__micron_arch_x86)
#include "kernels/arch/bitset_amd64.hpp"
#elif defined(__micron_arch_arm32)
#include "kernels/arch/bitset_arm32.hpp"
#elif defined(__micron_arch_arm64)
#include "kernels/arch/bitset_arm64.hpp"
#endif

namespace micron::math::graphs::kernels
{

inline void
set_union(u64 *out, const u64 *a, const u64 *b, usize words) noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
  __impl::word_union_arch(out, a, b, words);
#else
  __impl::word_union_scalar(out, a, b, words);
#endif
}

inline void
set_intersection(u64 *out, const u64 *a, const u64 *b, usize words) noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
  __impl::word_intersection_arch(out, a, b, words);
#else
  __impl::word_intersection_scalar(out, a, b, words);
#endif
}

[[nodiscard]] inline usize
popcount_reduce(const u64 *words, usize count) noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
  return __impl::popcount_arch(words, count);
#else
  return __impl::popcount_scalar(words, count);
#endif
}

[[nodiscard]] inline usize
common_neighbor_count(const u64 *a, const u64 *b, usize words) noexcept
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_x86) || defined(__micron_arch_arm32) || defined(__micron_arch_arm64)
  return __impl::intersection_popcount_arch(a, b, words);
#else
  return __impl::intersection_popcount_scalar(a, b, words);
#endif
}

inline void
expand_frontier(u64 *out, const u64 *frontier, const u64 *adjacency_rows, usize vertices) noexcept
{
  const usize words = (vertices + 63u) / 64u;
  for ( usize word = 0; word < words; ++word ) out[word] = 0;
  for ( usize vertex = 0; vertex < vertices; ++vertex ) {
    if ( (frontier[vertex >> 6u] & (u64(1) << (vertex & 63u))) == 0 ) continue;
    const u64 *row = adjacency_rows + vertex * words;
    set_union(out, out, row, words);
  }
}

template<micron::integral I>
[[nodiscard]] inline usize
masked_slot_scan(const u64 *mask, usize slots, I *out, usize capacity) noexcept
{
  usize count = 0;
  const usize words = (slots + 63u) / 64u;
  for ( usize word = 0; word < words; ++word ) {
    u64 bits = mask[word];
    while ( bits ) {
      const usize bit = static_cast<usize>(__builtin_ctzll(bits));
      const usize slot = word * 64u + bit;
      if ( slot < slots && count < capacity ) out[count++] = static_cast<I>(slot);
      bits &= bits - 1;
    }
  }
  return count;
}

inline void
dense_boolean_product(const u8 *a, const u8 *b, u8 *out, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    for ( usize j = 0; j < n; ++j ) {
      u8 value = 0;
      for ( usize k = 0; k < n; ++k ) value |= static_cast<u8>(a[i * n + k] && b[k * n + j]);
      out[i * n + j] = static_cast<u8>(value != 0);
    }
  }
}

template<micron::integral I>
[[nodiscard]] inline bool
csr_prefix_sum(const I *counts, I *outer, usize rows) noexcept
{
  outer[0] = I(0);
  for ( usize row = 0; row < rows; ++row ) {
    I next{};
    if ( __builtin_add_overflow(outer[row], counts[row], &next) ) return false;
    outer[row + 1] = next;
  }
  return true;
}

};      // namespace micron::math::graphs::kernels
