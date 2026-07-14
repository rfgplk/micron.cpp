//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/actions.hpp"
#include "../memory/addr.hpp"
#include "../memory/allocation/resources.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shared node storage for our arena backed trees

namespace micron
{
namespace __tree_store
{

using node_idx = u32;
inline constexpr node_idx nil = static_cast<node_idx>(~0u);

template<typename T>
inline constexpr bool is_byte_relocatable_v = micron::is_trivially_copyable_v<T> && micron::is_trivially_destructible_v<T>;

template<typename T>
__attribute__((always_inline)) inline void
shift_right(T *base, usize first, usize last) noexcept
{
  if ( first >= last ) return;
  if constexpr ( is_byte_relocatable_v<T> ) {
    const usize cnt = last - first;
    using __ab = unsigned char __attribute__((may_alias));
    __ab *d = reinterpret_cast<__ab *>(base + first + 1);
    const __ab *s = reinterpret_cast<const __ab *>(base + first);
    for ( usize i = cnt * sizeof(T); i > 0; --i ) d[i - 1] = s[i - 1];
  } else {
    for ( usize i = last; i > first; --i ) {
      new (micron::addr(base[i])) T(micron::move(base[i - 1]));
      base[i - 1].~T();
    }
  }
}

template<typename T>
__attribute__((always_inline)) inline void
erase_at(T *base, usize first, usize last) noexcept
{
  if ( first >= last ) return;
  if constexpr ( is_byte_relocatable_v<T> ) {
    const usize cnt = (first + 1 < last) ? (last - first - 1) : 0;
    if ( cnt ) {
      using __ab = unsigned char __attribute__((may_alias));
      __ab *d = reinterpret_cast<__ab *>(base + first);
      const __ab *s = reinterpret_cast<const __ab *>(base + first + 1);
      for ( usize i = 0; i < cnt * sizeof(T); ++i ) d[i] = s[i];
    }
  } else {
    base[first].~T();
    for ( usize i = first + 1; i < last; ++i ) {
      new (micron::addr(base[i - 1])) T(micron::move(base[i]));
      base[i].~T();
    }
  }
}

template<typename T>
__attribute__((always_inline)) inline void
relocate_n(T *dest, T *src, usize cnt)
{
  if constexpr ( is_byte_relocatable_v<T> ) {
    using __ab = unsigned char __attribute__((may_alias));
    __ab *d = reinterpret_cast<__ab *>(dest);
    const __ab *s = reinterpret_cast<const __ab *>(src);
    for ( usize i = 0; i < cnt * sizeof(T); ++i ) d[i] = s[i];
  } else {
    for ( usize i = 0; i < cnt; ++i ) {
      new (micron::addr(dest[i])) T(micron::move(src[i]));
      src[i].~T();
    }
  }
}

template<usize B, usize Al> struct alignas(Al) raw_slot {
  alignas(Al) byte raw[B];
};

template<usize B, usize Al = 64, class Alloc = allocator_serial<>> class node_arena
{
  using slot_t = raw_slot<B, Al>;
  static_assert(B >= sizeof(node_idx), "slot too small to hold the free-list link");

  __immutable_memory_resource<slot_t, Alloc> mem_;
  node_idx free_head_;

  void
  grow()
  {
    const usize old = mem_.capacity;
    mem_.expand(old == 0 ? 16u : old);
  }

public:
  node_arena() : mem_(), free_head_(nil) { }

  node_arena(const node_arena &) = delete;
  node_arena &operator=(const node_arena &) = delete;

  node_arena(node_arena &&o) noexcept : mem_(micron::move(o.mem_)), free_head_(o.free_head_) { o.free_head_ = nil; }

  node_arena &
  operator=(node_arena &&o) noexcept
  {
    if ( this != &o ) {
      mem_ = micron::move(o.mem_);
      free_head_ = o.free_head_;
      o.free_head_ = nil;
    }
    return *this;
  }

  [[gnu::always_inline]] byte *
  raw(node_idx i) noexcept
  {
    return mem_.memory[i].raw;
  }

  [[gnu::always_inline]] const byte *
  raw(node_idx i) const noexcept
  {
    return mem_.memory[i].raw;
  }

  node_idx
  allocate()
  {
    if ( free_head_ != nil ) {
      const node_idx i = free_head_;
      free_head_ = *reinterpret_cast<node_idx *>(mem_.memory[i].raw);
      return i;
    }
    if ( mem_.length >= mem_.capacity ) grow();
    return static_cast<node_idx>(mem_.length++);
  }

  void
  deallocate(node_idx i) noexcept
  {
    *reinterpret_cast<node_idx *>(mem_.memory[i].raw) = free_head_;
    free_head_ = i;
  }

  void
  reset() noexcept
  {
    mem_.length = 0;
    free_head_ = nil;
  }

  // pre-size the slab so subsequent allocate() calls never grow (the noexcept-hot-path
  // discipline: reserve at init where a throw is acceptable, then insert freely)
  void
  reserve(usize n_slots)
  {
    if ( mem_.capacity < n_slots ) mem_.expand(n_slots - mem_.capacity);
  }

  // high-water slot count (reset()/rebuild-friendly arenas never deallocate, so this is
  // exactly the number of live slots) + the reserved capacity, both in SLOT units
  [[nodiscard, gnu::always_inline]] usize
  slots_used() const noexcept
  {
    return mem_.length;
  }

  [[nodiscard, gnu::always_inline]] usize
  slots_reserved() const noexcept
  {
    return mem_.capacity;
  }
};

};      // namespace __tree_store
};      // namespace micron
