// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "../../../atomic/atomic.hpp"
#include "../../../types.hpp"
#include "va_reserve.hpp"

namespace abc
{

class __arena;

constexpr static const usize __num_blocks = __va_reservation_size >> __sheet_align_log2;

inline __arena *__block_owner_table[__num_blocks]{};

[[gnu::always_inline]] inline u64
__block_index(const void *p) noexcept
{
  addr_t *base = __va_base.get(micron::memory_order_relaxed);
  return (reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(base)) >> __sheet_align_log2;
}

inline void
__sheet_register(__arena *arena, const void *base, usize len) noexcept
{
  if ( !__va_contains(base) ) return;
  const u64 first = __block_index(base);
  const usize blocks = (len + __sheet_align_mask) >> __sheet_align_log2;
  for ( usize i = 0; i < blocks; ++i ) {
    __atomic_store_n(&__block_owner_table[first + i], arena, __ATOMIC_RELEASE);
  }
}

inline void
__sheet_unregister(const void *base, usize len) noexcept
{
  if ( !__va_contains(base) ) return;
  const u64 first = __block_index(base);
  const usize blocks = (len + __sheet_align_mask) >> __sheet_align_log2;
  for ( usize i = 0; i < blocks; ++i ) {
    __atomic_store_n(&__block_owner_table[first + i], static_cast<__arena *>(nullptr), __ATOMIC_RELEASE);
  }
}

[[gnu::always_inline]] inline __arena *
__owner_of(const void *p) noexcept
{
  if ( !__va_contains(p) ) return nullptr;
  return __atomic_load_n(&__block_owner_table[__block_index(p)], __ATOMIC_ACQUIRE);
}

};      // namespace abc
