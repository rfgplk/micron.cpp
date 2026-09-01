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

#include "metadata.hpp"

#include "../../../mem.hpp"
#include "../../../memory/cmemory.hpp"
#include "../../../simd/types.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"

namespace abc
{
template<typename T, i64 Min, i32 Mx = 64>
  requires(micron::is_trivially_constructible_v<T> and micron::is_trivially_destructible_v<T> and (bool)((Min & (Min - 1)) == 0))
struct __buddy_list {

  static_assert(Min >= __hdr_offset, "Min block size must be at least __hdr_offset");
  static_assert(Mx <= 64, "Mx must fit in u64 free_mask");

  // header is placed at the TAIL of each region
  //   [ usable region | __hdr_offset bytes ]
  //                    ^block_start + order_size - __hdr_offset
  //                    block_header lives here remaining bytes are spare for
  //                    future metadata expansion (sizeof(block_header) < __hdr_offset)
  //   ^-- block_start == user_ptr  (naturally aligned)

  struct free_block {
    free_block *next;
    free_block *prev;
  };

  static inline free_block *
  __free_block_at(void *addr) noexcept
  {
    return reinterpret_cast<free_block *>(__builtin_assume_aligned(addr, alignof(free_block)));
  }

  static constexpr int __log2_min = []() constexpr {
    int r = 0;
    i64 v = Min;
    while ( v > 1 ) {
      v >>= 1;
      ++r;
    }
    return r;
  }();

  // bit 7 = __tag_free  bits 0..6 = order.
  // (order | 0x80): free block start at this order.
  // (order): allocated block start at this order.
  // 0xFF: not a block start / uninitialised.
  static constexpr u8 __tag_free = 0x80;
  static constexpr u8 __tag_none = 0xFF;

  static constexpr i32 __cache_cap = 4;
  // ring of temporal-active addresses per order
  static constexpr i32 __active_ring = 2;
  static constexpr i32 __cold_cap = 0;      // disabled when tombstoning is off interop is messy

  byte *base;
  usize total;
  i32 max_order;
  usize allocated_bytes;
  usize tombstoned_bytes;
  u64 free_mask;        // main bitmap for o(1): bit i set iff free_lists[i] != nullptr
  u8 *block_tags;       // one tag per min-block
  usize tag_count;      // == total >> __log2_min
  bool tags_external;
  free_block *free_lists[Mx];
  free_block *active[Mx][__active_ring];      // N active temporal blocks per order (rotated)
  u8 active_rotor[Mx];                        // next slot to insert/return for order o

  free_block *tcache[Mx];
  i32 tcache_count[Mx];
  free_block *cold_cache[Mx];
  i32 cold_count[Mx];

  __attribute__((always_inline)) static inline int
  ceil_log2_u64(u64 v) noexcept
  {
    return 64 - __builtin_clzll(v - 1);
  }

  __attribute__((always_inline)) inline int
  order_for_size(usize n) const noexcept
  {
    usize units = ((n - 1) >> __log2_min) + 1;
    if ( units <= 1 ) return 0;
    return ceil_log2_u64(units);
  }

  __attribute__((always_inline)) inline usize
  order_size(i32 o) const noexcept
  {
    return static_cast<usize>(Min) << o;
  }

  __attribute__((always_inline)) inline usize
  tag_index_of(usize off) const noexcept
  {
    return off >> __log2_min;
  }

  __attribute__((always_inline)) inline usize
  tag_index(byte *addr) const noexcept
  {
    return tag_index_of((usize)(addr - base));
  }

  __attribute__((always_inline)) inline void
  tag_set_free(byte *addr, i32 o) noexcept
  {
    block_tags[tag_index(addr)] = (u8)(o | __tag_free);
  }

  __attribute__((always_inline)) inline void
  tag_set_free_at(usize tidx, i32 o) noexcept
  {
    block_tags[tidx] = (u8)(o | __tag_free);
  }

  __attribute__((always_inline)) inline void
  tag_set_alloc(byte *addr, i32 o) noexcept
  {
    block_tags[tag_index(addr)] = (u8)(o);
  }

  __attribute__((always_inline)) inline bool
  tag_is_free_at_off(usize off, i32 o) const noexcept
  {
    u8 expected = (u8)(o | __tag_free);
    return block_tags[off >> __log2_min] == expected;
  }

  __attribute__((always_inline)) inline bool
  tag_is_free_at(byte *addr, i32 o) const noexcept
  {
    return tag_is_free_at_off((usize)(addr - base), o);
  }

  __attribute__((always_inline)) inline void
  mask_set(i32 o) noexcept
  {
    free_mask |= (u64(1) << o);
  }

  __attribute__((always_inline)) inline void
  mask_clear_if_empty(i32 o) noexcept
  {
    if ( free_lists[o] == nullptr ) free_mask &= ~(u64(1) << o);
  }

  __attribute__((always_inline)) inline i32
  find_free_order(i32 o) const noexcept
  {
    u64 m = free_mask >> o;
    if ( m == 0 ) return max_order;
    return o + __builtin_ctzll(m);
  }

  __attribute__((always_inline)) inline block_header *
  hdr_of(byte *block_start, i32 o) const noexcept
  {
    return micron::ptr_cast<block_header *>(block_start + order_size(o) - __hdr_offset);
  }

  __attribute__((always_inline)) inline const block_header *
  hdr_of(const byte *block_start, i32 o) const noexcept
  {
    return reinterpret_cast<const block_header *>(block_start + order_size(o) - __hdr_offset);
  }

  __attribute__((always_inline)) inline block_header *
  hdr_of_tagged(byte *block_start) const noexcept
  {
    u8 tag = block_tags[tag_index(block_start)];
    i32 o = static_cast<i32>(tag & ~__tag_free);
    return hdr_of(block_start, o);
  }

  // guard against corrupted heads/links
  __attribute__((always_inline)) inline bool
  __link_valid(const free_block *n) const noexcept
  {
    if ( n == nullptr ) return true;
    const byte *b = reinterpret_cast<const byte *>(n);
    if ( b < base || b >= base + total ) return false;
    return ((usize)(b - base) & (Min - 1)) == 0;
  }

  [[gnu::cold, gnu::noinline]] bool
  __freelist_remove_slow(byte *buddy, i32 o) noexcept
  {
    free_block *prev = nullptr;
    free_block *cur = free_lists[o];
    usize guard = 0;
    while ( cur && guard++ <= tag_count ) {
      free_block *nx = cur->next;
      if ( !__link_valid(nx) ) nx = nullptr;
      if ( (byte *)cur == buddy ) {
        if ( prev )
          prev->next = nx;
        else
          free_lists[o] = nx;
        if ( prev && nx ) nx->prev = prev;
        mask_clear_if_empty(o);
        return true;
      }
      prev = cur;
      cur = nx;
    }
    return false;
  }

  __attribute__((always_inline)) inline bool
  freelist_remove(byte *buddy, i32 o) noexcept
  {
    free_block *node = __free_block_at(buddy);
    free_block *next = node->next;
    free_block *head = free_lists[o];

    if ( head == node ) {
      if ( !__link_valid(next) ) [[unlikely]]
        return __freelist_remove_slow(buddy, o);
      free_lists[o] = next;
      if ( next == nullptr ) free_mask &= ~(u64(1) << o);
      return true;
    }

    free_block *prev = node->prev;
    if ( !__link_valid(next) ) [[unlikely]]
      return __freelist_remove_slow(buddy, o);
    if ( !__link_valid(prev) || prev == nullptr || prev->next != node || (next && next->prev != node) ) [[unlikely]]
      return __freelist_remove_slow(buddy, o);

    prev->next = next;
    if ( next ) next->prev = prev;
    return true;
  }

  __attribute__((always_inline)) inline void
  freelist_push_off(byte *addr, i32 o, usize off) noexcept
  {
    free_block *nb = __free_block_at(addr);
    free_block *head = free_lists[o];
    nb->next = head;
    if ( head )
      head->prev = nb;
    else
      mask_set(o);
    free_lists[o] = nb;
    tag_set_free_at(off >> __log2_min, o);
  }

  __attribute__((always_inline)) inline free_block *
  __take_and_split(i32 from, i32 target) noexcept
  {
    free_block *blk = free_lists[from];
    free_block *next = blk->next;
    u64 mask = free_mask;
    const u64 old_mask = mask;

    if ( !__link_valid(next) ) [[unlikely]]
      next = nullptr;      // hard drop a corrupted tail
    free_lists[from] = next;
    if ( next == nullptr ) mask &= ~(u64(1) << from);

    usize split_size = order_size(from);
    while ( from > target ) {
      --from;
      split_size >>= 1;
      byte *right = reinterpret_cast<byte *>(blk) + split_size;
      free_block *node = __free_block_at(right);
      free_block *head = free_lists[from];
      node->next = head;
      if ( head )
        head->prev = node;
      else
        mask |= (u64(1) << from);
      free_lists[from] = node;
      block_tags[tag_index(right)] = static_cast<u8>(from | __tag_free);
    }

    if ( mask != old_mask ) free_mask = mask;
    return blk;
  }

  __attribute__((always_inline)) inline free_block *
  tcache_pop(i32 o) noexcept
  {
    if ( tcache_count[o] <= 0 ) return nullptr;
    free_block *blk = tcache[o];
    free_block *__nx = blk->next;
    if ( __link_valid(__nx) ) {
      tcache[o] = __nx;
      --tcache_count[o];
    } else {
      tcache[o] = nullptr;
      tcache_count[o] = 0;
    }      // hard drop corrupted cache tail
    return blk;
  }

  __attribute__((always_inline)) inline bool
  tcache_push(free_block *blk, i32 o) noexcept
  {
    if ( tcache_count[o] >= __cache_cap ) return false;
    blk->next = tcache[o];
    tcache[o] = blk;
    ++tcache_count[o];
    return true;
  }

  void
  tcache_flush(i32 o) noexcept
  {
    while ( tcache[o] ) {
      free_block *blk = tcache[o];
      tcache[o] = blk->next;
      --tcache_count[o];

      byte *addr = (byte *)blk;
      __merge_and_free(addr, o);
    }
  }

  void
  tcache_flush_all() noexcept
  {
    for ( i32 i = 0; i < max_order; ++i ) tcache_flush(i);
  }

  __attribute__((always_inline)) inline free_block *
  cold_pop(i32 o) noexcept
  {
    if ( cold_count[o] <= 0 ) return nullptr;
    free_block *blk = cold_cache[o];
    free_block *__nx = blk->next;
    if ( __link_valid(__nx) ) {
      cold_cache[o] = __nx;
      --cold_count[o];
    } else {
      cold_cache[o] = nullptr;
      cold_count[o] = 0;
    }      // hard drop corrupted cache tail
    return blk;
  }

  __attribute__((always_inline)) inline bool
  cold_push(free_block *blk, i32 o) noexcept
  {
    if ( cold_count[o] >= __cold_cap ) return false;
    blk->next = cold_cache[o];
    cold_cache[o] = blk;
    ++cold_count[o];
    return true;
  }

  void
  cold_drain_merge(i32 o) noexcept
  {
    while ( cold_cache[o] ) {
      free_block *blk = cold_cache[o];
      cold_cache[o] = blk->next;
      --cold_count[o];

      byte *addr = (byte *)blk;
      __merge_and_free(addr, o);
    }
  }

  void
  cold_flush_all() noexcept
  {
    for ( i32 i = 0; i < max_order; ++i ) cold_drain_merge(i);
  }

  void
  __merge_and_free(byte *addr, i32 o) noexcept
  {
    usize off = (usize)(addr - base);

    while ( o < max_order - 1 ) {
      usize blk_sz = order_size(o);
      usize buddy_off = off ^ blk_sz;

      if ( !tag_is_free_at_off(buddy_off, o) ) break;

      if ( !freelist_remove(base + buddy_off, o) ) [[unlikely]]
        break;
      const usize interior_off = (buddy_off > off) ? buddy_off : off;
      block_tags[interior_off >> __log2_min] = __tag_none;

      off = (buddy_off < off) ? buddy_off : off;
      ++o;
    }

    freelist_push_off(base + off, o, off);
  }

  void
  __impl_zero_arrays() noexcept
  {
    free_mask = 0;
    for ( i32 i = 0; i < Mx; ++i ) {
      free_lists[i] = nullptr;
      for ( i32 r = 0; r < __active_ring; ++r ) active[i][r] = nullptr;
      active_rotor[i] = 0;
      tcache[i] = nullptr;
      tcache_count[i] = 0;
      cold_cache[i] = nullptr;
      cold_count[i] = 0;
    }
  }

  void
  __impl_init_memory(byte *_ptr, usize _len, u8 *ext_tags = nullptr)
  {
    uintptr_t ptr = (uintptr_t)_ptr;
    uintptr_t a = alignof(void *);
    uintptr_t r = (ptr + (a - 1)) & ~(a - 1);
    usize adjust = r - ptr;
    if ( _len <= adjust ) {
      base = nullptr;
      total = 0;
      max_order = 0;
      tag_count = 0;
      block_tags = nullptr;
      tags_external = false;
      return;
    }

    byte *aligned = (byte *)r;
    usize usable = _len - adjust;

    if ( ext_tags ) {

      tags_external = true;
      block_tags = ext_tags;
      base = aligned;
      usize data_usable = (usable / Min) * Min;
      if ( data_usable < Min ) {
        base = nullptr;
        total = 0;
        max_order = 0;
        tag_count = 0;
        block_tags = nullptr;
        tags_external = false;
        return;
      }
      total = data_usable;
    } else {

      tags_external = false;
      constexpr usize min_sz = static_cast<usize>(Min);
      usize approx_tags = (usable + min_sz) / (min_sz + 1);
      usize tag_area = (approx_tags + min_sz - 1) & ~(min_sz - 1);
      if ( tag_area >= usable ) {
        base = nullptr;
        total = 0;
        max_order = 0;
        tag_count = 0;
        block_tags = nullptr;
        return;
      }
      block_tags = aligned;
      base = aligned + tag_area;
      usize data_usable = usable - tag_area;
      data_usable = (data_usable / Min) * Min;
      if ( data_usable < Min ) {
        base = nullptr;
        total = 0;
        max_order = 0;
        tag_count = 0;
        block_tags = nullptr;
        return;
      }
      total = data_usable;
    }

    tag_count = total >> __log2_min;

    const usize units = total >> __log2_min;
    max_order = static_cast<i32>(63 - __builtin_clzll(units)) + 1;
    if ( max_order > Mx ) max_order = Mx;

    __impl_zero_arrays();

    micron::memset(block_tags, __tag_none, tag_count);

    free_lists[max_order - 1] = __free_block_at(base);
    free_lists[max_order - 1]->next = nullptr;
    mask_set(max_order - 1);
    tag_set_free(base, max_order - 1);
  }

  ~__buddy_list() noexcept { }

  __buddy_list(void) = delete;

  __buddy_list(const T &mem) noexcept
      : base(nullptr), total(0), max_order(0), allocated_bytes(0), tombstoned_bytes(0), free_mask(0), block_tags(nullptr), tag_count(0),
        tags_external(false)
  {
    __impl_zero_arrays();
    if ( mem.zero() or mem.len < Min ) micron::abort();
    __impl_init_memory(mem.ptr, mem.len);
  }

  __buddy_list(const T &mem, u8 *tag_buf) noexcept
      : base(nullptr), total(0), max_order(0), allocated_bytes(0), tombstoned_bytes(0), free_mask(0), block_tags(nullptr), tag_count(0),
        tags_external(true)
  {
    __impl_zero_arrays();
    if ( mem.zero() or mem.len < Min ) micron::abort();
    __impl_init_memory(mem.ptr, mem.len, tag_buf);
  }

  __buddy_list(const __buddy_list &) = delete;

  __buddy_list(__buddy_list &&o)
      : base(o.base), total(o.total), max_order(o.max_order), allocated_bytes(o.allocated_bytes), tombstoned_bytes(o.tombstoned_bytes),
        free_mask(o.free_mask), block_tags(o.block_tags), tag_count(o.tag_count), tags_external(o.tags_external)
  {
    o.base = nullptr;
    o.total = 0;
    o.max_order = 0;
    o.allocated_bytes = 0;
    o.tombstoned_bytes = 0;
    o.free_mask = 0;
    o.block_tags = nullptr;
    o.tag_count = 0;
    o.tags_external = false;

    for ( i32 i = 0; i < Mx; ++i ) {
      free_lists[i] = o.free_lists[i];
      tcache[i] = o.tcache[i];
      tcache_count[i] = o.tcache_count[i];
      cold_cache[i] = o.cold_cache[i];
      cold_count[i] = o.cold_count[i];
      o.free_lists[i] = nullptr;
      o.tcache[i] = nullptr;
      o.tcache_count[i] = 0;
      o.cold_cache[i] = nullptr;
      o.cold_count[i] = 0;
      for ( i32 r = 0; r < __active_ring; ++r ) {
        active[i][r] = o.active[i][r];
        o.active[i][r] = nullptr;
      }
      active_rotor[i] = o.active_rotor[i];
      o.active_rotor[i] = 0;
    }
  }

  __buddy_list &operator=(const __buddy_list &) = delete;

  __buddy_list &
  operator=(__buddy_list &&o)
  {

    tcache_flush_all();
    cold_flush_all();

    base = o.base;
    total = o.total;
    max_order = o.max_order;
    allocated_bytes = o.allocated_bytes;
    tombstoned_bytes = o.tombstoned_bytes;
    free_mask = o.free_mask;
    block_tags = o.block_tags;
    tag_count = o.tag_count;
    tags_external = o.tags_external;

    o.base = nullptr;
    o.total = 0;
    o.max_order = 0;
    o.allocated_bytes = 0;
    o.tombstoned_bytes = 0;
    o.free_mask = 0;
    o.block_tags = nullptr;
    o.tag_count = 0;
    o.tags_external = false;

    for ( i32 i = 0; i < Mx; ++i ) {
      free_lists[i] = o.free_lists[i];
      tcache[i] = o.tcache[i];
      tcache_count[i] = o.tcache_count[i];
      cold_cache[i] = o.cold_cache[i];
      cold_count[i] = o.cold_count[i];
      o.free_lists[i] = nullptr;
      o.tcache[i] = nullptr;
      o.tcache_count[i] = 0;
      o.cold_cache[i] = nullptr;
      o.cold_count[i] = 0;
      for ( i32 r = 0; r < __active_ring; ++r ) {
        active[i][r] = o.active[i][r];
        o.active[i][r] = nullptr;
      }
      active_rotor[i] = o.active_rotor[i];
      o.active_rotor[i] = 0;
    }
    return *this;
  }

  T
  allocate(usize n) noexcept
  {
    if ( n > micron::numeric_limits<usize>::max() - __hdr_offset ) return { nullptr, 0 };
    n += __hdr_offset;
    if ( !base ) return { nullptr, 0 };
    i32 o = order_for_size(n);
    if ( o >= max_order ) return { nullptr, 0 };
    const usize target_size = order_size(o);

    if constexpr ( __cold_cap > 0 ) {
      if ( free_block *cold = cold_pop(o) ) {
        block_header *hdr = hdr_of((byte *)cold, o);
        hdr->order = static_cast<i32>(o);
        hdr->flags = __block_alloc;
        tag_set_alloc((byte *)cold, o);
        allocated_bytes += target_size;
        return { (byte *)cold, target_size - __hdr_offset };
      }
    }

    i32 i = find_free_order(o);
    if ( i >= max_order ) {
      if constexpr ( __cold_cap > 0 ) {
        cold_flush_all();
        i = find_free_order(o);
      }
      if ( i >= max_order ) return { nullptr, 0 };
    }

    free_block *blk = __take_and_split(i, o);

    // write header at the tail of the block
    block_header *hdr = hdr_of((byte *)blk, o);
    hdr->order = static_cast<i32>(o);
    hdr->flags = __block_alloc;
    tag_set_alloc((byte *)blk, o);
    allocated_bytes += target_size;

    return { (byte *)blk, target_size - __hdr_offset };
  }

  T
  temporal_allocate(usize n) noexcept
  {
    if ( n > micron::numeric_limits<usize>::max() - __hdr_offset ) return { nullptr, 0 };
    n += __hdr_offset;
    if ( !base ) return { nullptr, 0 };

    i32 o = order_for_size(n);
    if ( o >= max_order ) return { nullptr, 0 };

    usize target_size = order_size(o);

    u8 r0 = active_rotor[o] & (__active_ring - 1);
    free_block *active_block = active[o][r0];
    if ( !active_block ) {
      r0 ^= 1u;
      active_block = active[o][r0];
    }
    if ( active_block ) {
      active_rotor[o] = r0 ^ 1u;
      return { reinterpret_cast<byte *>(active_block), target_size - __hdr_offset };
    }

    free_block *cached = tcache_pop(o);
    if ( cached ) {
      block_header *hdr = hdr_of((byte *)cached, o);
      hdr->order = static_cast<i32>(o);
      hdr->flags = __block_alloc | __block_temporal;
      tag_set_alloc((byte *)cached, o);
      allocated_bytes += target_size;
      active[o][r0] = cached;
      active_rotor[o] = r0 ^ 1u;
      return { (byte *)cached, target_size - __hdr_offset };
    }

    i32 i = find_free_order(o);
    if ( i >= max_order ) return { nullptr, 0 };

    free_block *blk = __take_and_split(i, o);

    block_header *hdr = hdr_of((byte *)blk, o);
    hdr->order = static_cast<i32>(o);
    hdr->flags = __block_alloc | __block_temporal;
    tag_set_alloc((byte *)blk, o);
    allocated_bytes += target_size;

    active[o][r0] = blk;
    active_rotor[o] = r0 ^ 1u;

    return { (byte *)blk, target_size - __hdr_offset };
  }

  T
  allocate_exact(usize n) noexcept
  {
    if ( !base ) return { nullptr, 0 };
    if ( n < Min ) return { nullptr, 0 };
    if ( (n & (n - 1)) != 0 ) return { nullptr, 0 };
    i32 o = order_for_size(n);
    if ( o >= max_order ) return { nullptr, 0 };
    if ( !free_lists[o] ) return { nullptr, 0 };

    free_block *blk = __take_and_split(o, o);
    const usize target_size = order_size(o);

    block_header *hdr = hdr_of((byte *)blk, o);
    hdr->order = static_cast<i32>(o);
    hdr->flags = __block_alloc;
    tag_set_alloc((byte *)blk, o);
    allocated_bytes += target_size;

    return { (byte *)blk, target_size - __hdr_offset };
  }

  ret_flag
  tombstone(byte *ptr) noexcept
  {
    if ( !is_allocated(ptr) ) return { __flag_invalid };
    block_header *hdr = hdr_of_tagged(ptr);
    i32 o = hdr->order;
    if ( o < 0 || o >= max_order ) return { __flag_invalid };
    if ( !(hdr->flags & __block_alloc) ) return { __flag_invalid };

    hdr->flags = __block_tombstone;
    const usize target_size = order_size(o);
    allocated_bytes -= target_size;
    tombstoned_bytes += target_size;

    return __flag_tombstoned;
  }

  ret_flag
  tombstone(T &node) noexcept
  {
    if ( !node.ptr or node.len == 0 ) return __flag_invalid;
    return tombstone(node.ptr);
  }

  bool
  is_tombstoned(byte *ptr) const noexcept
  {
    block_header *hdr = hdr_of_tagged(ptr);
    return (hdr->flags & __block_tombstone) != 0;
  }

  bool
  is_temporal(byte *ptr) noexcept
  {
    block_header *hdr = hdr_of_tagged(ptr);
    return (hdr->flags & __block_temporal) != 0;
  }

  ret_flag
  deallocate(byte *ptr) noexcept
  {
    // WARNING: validate a min-block-aligned, authoritatively-allocated block START before any header access
    // protects against forged/out-of-range ptrs
    if ( !is_allocated(ptr) ) return { __flag_invalid };
    byte *addr = ptr;
    block_header *hdr = hdr_of_tagged(addr);
    const i32 original_o = hdr->order;
    if ( original_o < 0 || original_o >= max_order ) return { __flag_invalid };
    if ( !base || !addr ) return __flag_failure;
    const i32 flags = hdr->flags;
    if ( flags == __block_free ) return { __flag_invalid };

    const usize target_size = order_size(original_o);
    allocated_bytes -= target_size;
    if ( flags & __block_tombstone ) tombstoned_bytes -= target_size;

    bool was_temporal = (flags & __block_temporal) != 0;
    if ( was_temporal ) {
      free_block *free_addr = __free_block_at(addr);
      for ( i32 r = 0; r < __active_ring; ++r ) {
        if ( active[original_o][r] == free_addr ) {
          active[original_o][r] = nullptr;
          break;
        }
      }
      if ( tcache_push(free_addr, original_o) ) {
        block_tags[tag_index(addr)] = __tag_none;
        return { __flag_ok };
      }
    }

    if constexpr ( __cold_cap > 0 ) {
      if ( !was_temporal && cold_push(__free_block_at(addr), original_o) ) {
        block_tags[tag_index(addr)] = __tag_none;
        return { __flag_ok };
      }
    }

    __merge_and_free(addr, original_o);
    return { __flag_ok };
  }

  ret_flag
  deallocate(T &node) noexcept
  {
    if ( !node.ptr or node.len == 0 ) return __flag_invalid;
    return deallocate(node.ptr);
  }

  T
  reallocate(T node, usize new_size) noexcept
  {
    if ( !base ) return { nullptr, 0 };
    if ( !node.ptr ) return allocate(new_size);
    if ( new_size == 0 ) {
      deallocate(node);
      return { nullptr, 0 };
    }

    if ( node.len >= new_size && new_size > (node.len >> 1) ) return node;

    T nnode = allocate(new_size);
    if ( !nnode.ptr ) return { nullptr, 0 };

    usize to_copy = (node.len < nnode.len) ? node.len : nnode.len;
    micron::memcpy(nnode.ptr, node.ptr, to_copy);
    deallocate(node);
    return nnode;
  }

  usize
  available() const noexcept
  {
    if ( !base ) return 0;
    return total - allocated_bytes;
  }

  usize
  __total() const noexcept
  {
    return total;
  }

  usize
  tombstoned() const noexcept
  {
    return tombstoned_bytes;
  }

  usize
  used() const noexcept
  {
    return allocated_bytes;
  }

  usize
  block_size(byte *ptr) const noexcept
  {
    if ( !base || !ptr ) return 0;
    if ( ptr < base || ptr >= base + total ) return 0;
    if ( ((usize)(ptr - base) & (Min - 1)) != 0 ) return 0;
    u8 tag = block_tags[tag_index(ptr)];
    i32 o = static_cast<i32>(tag & ~__tag_free);
    if ( (u64)o >= (u64)max_order ) return 0;
    return order_size(o);
  }

  bool
  is_allocated(byte *ptr) const noexcept
  {
    if ( !ptr || !base || ptr < base || ptr >= base + total ) return false;
    if ( ((usize)(ptr - base) & (Min - 1)) != 0 ) return false;
    u8 tag = block_tags[tag_index(ptr)];
    return (tag & __tag_free) == 0 && tag < (u8)max_order;
  }

  usize
  allocated_size(byte *ptr) const noexcept
  {
    if ( !is_allocated(ptr) ) return 0;
    return block_size(ptr);
  }

#if defined(ABCMALLOC_DOCTOR_HELP)
  // deep corruption walk
  template<class V>
  void
  __doctor_walk(V &v)
  {
    if ( !base || !block_tags ) return;

    usize idx = 0;
    usize guard = 0;
    const usize maxg = tag_count + 4;
    while ( idx < tag_count && guard++ < maxg ) {
      u8 tag = block_tags[idx];
      if ( tag == __tag_none ) {      // interior/padding at a boundary; skip one min-block
        ++idx;
        continue;
      }
      i32 o = static_cast<i32>(tag & ~__tag_free);
      byte *blk = base + (idx << __log2_min);
      ++v.blocks;
      if ( o < 0 || o >= max_order ) {
        v.note("buddy: block tag order out of range", blk);
        ++idx;
        continue;
      }
      usize osz = order_size(o);
      if ( (usize)(blk - base) + osz > total ) {
        v.note("buddy: block extends past sheet bounds", blk);
        break;
      }
      if ( ((usize)(blk - base) & (osz - 1)) != 0 ) v.note("buddy: block not aligned to its order size", blk);

      const bool tag_free = (tag & __tag_free) != 0;
      // the tail block_header is a maintained redundant copy
      // only for allocated blocks
      if ( !tag_free ) {
        block_header *h = hdr_of(blk, o);
        if ( h->order != static_cast<i32>(o) ) {
          v.note("buddy: allocated block tail header order != block_tags order", blk);
          if ( v.repair ) {
            h->order = static_cast<i32>(o);
            v.did_repair("buddy: rewrote tail header order from tag", blk);
          }
        }
        const bool flags_alloc
            = (h->flags == __block_alloc || h->flags == (__block_alloc | __block_temporal) || h->flags == __block_tombstone);
        if ( !flags_alloc ) {
          v.note("buddy: allocated block tail header flags not allocated", blk);
          if ( v.repair ) {
            h->flags = __block_alloc | (h->flags & __block_temporal);
            v.did_repair("buddy: reset tail header flags to allocated form", blk);
          }
        }
      }
      idx += (osz >> __log2_min);
    }
    if ( guard >= maxg ) v.note("buddy: tiling walk overran (corrupt tag stream)", base);

    // per order free lists
    for ( i32 o = 0; o < max_order; ++o ) {
      const bool has = (free_lists[o] != nullptr);
      const bool bit = ((free_mask >> o) & 1u) != 0;
      if ( has != bit ) {
        v.note("buddy: free_mask bit disagrees with free_lists[o]", base);
        if ( v.repair ) {
          if ( has )
            free_mask |= (u64(1) << o);
          else
            free_mask &= ~(u64(1) << o);
          v.did_repair("buddy: fixed free_mask bit", base);
        }
      }
      free_block *n = free_lists[o];
      free_block *prev = nullptr;
      usize gc = 0;
      while ( n && gc++ < tag_count + 4 ) {
        ++v.freelist_nodes;
        if ( !__link_valid(n) ) {
          v.note("buddy: free-list node out of bounds / misaligned", n);
          if ( v.repair ) {
            if ( prev )
              prev->next = nullptr;
            else
              free_lists[o] = nullptr;
            mask_clear_if_empty(o);
            v.did_repair("buddy: truncated free-list at bad node", n);
          }
          break;
        }
        if ( !tag_is_free_at(reinterpret_cast<byte *>(n), o) ) v.note("buddy: free-list node not tagged free at its order", n);
        if ( prev && n->prev != prev ) {
          v.note("buddy: free-list prev back-link mismatch", n);
          if ( v.repair ) {
            n->prev = prev;
            v.did_repair("buddy: fixed free-list prev back-link", n);
          }
        }
        if ( n->next && !__link_valid(n->next) ) {
          v.note("buddy: forged free-list next link", n);
          if ( v.repair ) {
            n->next = nullptr;
            v.did_repair("buddy: nulled forged free-list link", n);
          }
          break;
        }
        prev = n;
        n = n->next;
      }
      if ( gc >= tag_count + 4 ) v.note("buddy: free-list cycle / overrun at order", base);
    }
  }
#endif
};
};      // namespace abc
