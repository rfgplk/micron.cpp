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
#include "__sys.hpp"
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

struct __oor_sheet {
  uintptr_t lo;
  uintptr_t hi;      // exclusive
  __arena *owner;
  __oor_sheet *next;
};

inline micron::atomic_token<__oor_sheet *> __oor_head{ nullptr };
inline micron::atomic_token<u32> __oor_live{ 0 };      // lock-free empty probe; 0 == skip the walk

inline void
__oor_insert(__arena *arena, const void *base, usize len) noexcept
{
  const uintptr_t lo = reinterpret_cast<uintptr_t>(base);
  const uintptr_t hi = lo + len;
  for ( __oor_sheet *n = __oor_head.get(micron::memory_order_acquire); n != nullptr; n = n->next ) {
    if ( n->lo != lo || n->hi != hi ) continue;
    __arena *expect = nullptr;
    if ( __atomic_compare_exchange_n(&n->owner, &expect, arena, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE) ) {
      __oor_live.fetch_add(1, micron::memory_order_acq_rel);
      return;
    }
  }
  byte *mem = micron::sys_allocator<byte>::alloc(sizeof(__oor_sheet));
  if ( !mem ) [[unlikely]]
    return;      // cannot track it; degrades to the old behaviour for this sheet only
  __oor_sheet *n = new (mem) __oor_sheet{ lo, hi, arena, nullptr };
  n->next = __oor_head.get(micron::memory_order_acquire);
  while ( !__oor_head.compare_exchange_weak(n->next, n, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
  }
  __oor_live.fetch_add(1, micron::memory_order_acq_rel);
}

inline void
__oor_retire(const void *base, usize len) noexcept
{
  if ( __oor_live.get(micron::memory_order_acquire) == 0 ) return;
  const uintptr_t lo = reinterpret_cast<uintptr_t>(base);
  for ( __oor_sheet *n = __oor_head.get(micron::memory_order_acquire); n != nullptr; n = n->next ) {
    if ( n->lo == lo && n->hi == lo + len && __atomic_load_n(&n->owner, __ATOMIC_ACQUIRE) != nullptr ) {
      __atomic_store_n(&n->owner, static_cast<__arena *>(nullptr), __ATOMIC_RELEASE);
      __oor_live.sub_fetch(1, micron::memory_order_acq_rel);
      return;
    }
  }
}

[[gnu::cold, gnu::noinline]] inline __arena *
__oor_owner_of(const void *p) noexcept
{
  if ( __oor_live.get(micron::memory_order_acquire) == 0 ) return nullptr;
  const uintptr_t a = reinterpret_cast<uintptr_t>(p);
  for ( __oor_sheet *n = __oor_head.get(micron::memory_order_acquire); n != nullptr; n = n->next ) {
    if ( a >= n->lo && a < n->hi ) return __atomic_load_n(&n->owner, __ATOMIC_ACQUIRE);
  }
  return nullptr;
}

inline void
__sheet_register(__arena *arena, const void *base, usize len) noexcept
{
  if ( !__va_contains(base) ) {
    __oor_insert(arena, base, len);
    return;
  }
  const u64 first = __block_index(base);
  const usize blocks = (len + __sheet_align_mask) >> __sheet_align_log2;
  for ( usize i = 0; i < blocks; ++i ) {
    __atomic_store_n(&__block_owner_table[first + i], arena, __ATOMIC_RELEASE);
  }
}

inline void
__sheet_unregister(const void *base, usize len) noexcept
{
  if ( !__va_contains(base) ) {
    __oor_retire(base, len);
    return;
  }
  const u64 first = __block_index(base);
  const usize blocks = (len + __sheet_align_mask) >> __sheet_align_log2;
  for ( usize i = 0; i < blocks; ++i ) {
    __atomic_store_n(&__block_owner_table[first + i], static_cast<__arena *>(nullptr), __ATOMIC_RELEASE);
  }
}

[[gnu::always_inline]] inline __arena *
__owner_of(const void *p) noexcept
{
  if ( !__va_contains(p) ) [[unlikely]]
    return __oor_owner_of(p);
  return __atomic_load_n(&__block_owner_table[__block_index(p)], __ATOMIC_ACQUIRE);
}

};      // namespace abc
