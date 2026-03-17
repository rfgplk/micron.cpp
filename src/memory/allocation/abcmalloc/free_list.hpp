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
template <typename T, i64 Min, i32 Mx = 64>
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
  };

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

  // __log2_tbl[v] = ceil(log2(v))
  static constexpr u8 __log2_tbl[66] = {
    0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  };

  byte *base;
  usize total;
  i64 max_order;
  usize allocated_bytes;
  usize tombstoned_bytes;
  u64 free_mask;       // main bitmap for o(1): bit i set iff free_lists[i] != nullptr
  u8 *block_tags;      // one tag per min-block
  usize tag_count;     // == total >> __log2_min
  bool tags_external;
  usize order_sizes[Mx];     // order_sizes[i] = Min << i
  free_block *free_lists[Mx];
  free_block *active[Mx];     // single active temporal block per order

  free_block *tcache[Mx];
  i32 tcache_count[Mx];

  __attribute__((always_inline)) static inline int
  ceil_log2_u64(u64 v) noexcept
  {
    if ( v <= 64 ) [[likely]]
      return __log2_tbl[v];
    return 64 - __builtin_clzll(v - 1);
  }

  __attribute__((always_inline)) inline int
  order_for_size(usize n) const noexcept
  {
    usize units = (n + Min - 1) >> __log2_min;
    if ( units <= 1 )
      return 0;
    return ceil_log2_u64(units);
  }

  __attribute__((always_inline)) inline usize
  order_size(i64 o) const noexcept
  {
    return order_sizes[o];
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
  tag_set_free(byte *addr, i64 o) noexcept
  {
    block_tags[tag_index(addr)] = (u8)(o | __tag_free);
  }

  __attribute__((always_inline)) inline void
  tag_set_free_at(usize tidx, i64 o) noexcept
  {
    block_tags[tidx] = (u8)(o | __tag_free);
  }

  __attribute__((always_inline)) inline void
  tag_set_alloc(byte *addr, i64 o) noexcept
  {
    block_tags[tag_index(addr)] = (u8)(o);
  }

  __attribute__((always_inline)) inline bool
  tag_is_free_at_off(usize off, i64 o) const noexcept
  {
    u8 expected = (u8)(o | __tag_free);
    return block_tags[off >> __log2_min] == expected;
  }

  __attribute__((always_inline)) inline bool
  tag_is_free_at(byte *addr, i64 o) const noexcept
  {
    return tag_is_free_at_off((usize)(addr - base), o);
  }

  __attribute__((always_inline)) inline void
  mask_set(i64 o) noexcept
  {
    free_mask |= (u64(1) << o);
  }

  __attribute__((always_inline)) inline void
  mask_clear_if_empty(i64 o) noexcept
  {
    u64 bit = u64(1) << o;
    u64 keep = -(u64)(free_lists[o] != nullptr);
    free_mask = (free_mask & ~bit) | (free_mask & bit & keep);
  }

  __attribute__((always_inline)) inline i64
  find_free_order(i64 o) const noexcept
  {
    u64 m = free_mask >> o;
    if ( m == 0 )
      return max_order;
    return o + __builtin_ctzll(m);
  }

  __attribute__((always_inline)) inline block_header *
  hdr_of(byte *block_start, i64 o) const noexcept
  {
    return reinterpret_cast<block_header *>(block_start + order_sizes[o] - __hdr_offset);
  }

  __attribute__((always_inline)) inline const block_header *
  hdr_of(const byte *block_start, i64 o) const noexcept
  {
    return reinterpret_cast<const block_header *>(block_start + order_sizes[o] - __hdr_offset);
  }

  __attribute__((always_inline)) inline block_header *
  hdr_of_tagged(byte *block_start) const noexcept
  {
    u8 tag = block_tags[tag_index(block_start)];
    i64 o = static_cast<i64>(tag & ~__tag_free);
    return hdr_of(block_start, o);
  }

  __attribute__((always_inline)) inline void
  freelist_remove(byte *buddy, i64 o) noexcept
  {
    free_block *prev = nullptr;
    free_block *cur = free_lists[o];
    while ( cur ) {
      if ( (byte *)cur == buddy ) {
        if ( prev )
          prev->next = cur->next;
        else
          free_lists[o] = cur->next;
        mask_clear_if_empty(o);
        return;
      }
      prev = cur;
      cur = cur->next;
    }
  }

  __attribute__((always_inline)) inline void
  freelist_push(byte *addr, i64 o) noexcept
  {
    free_block *nb = (free_block *)addr;
    nb->next = free_lists[o];
    free_lists[o] = nb;
    mask_set(o);
    tag_set_free(addr, o);
  }

  __attribute__((always_inline)) inline void
  freelist_push_off(byte *addr, i64 o, usize off) noexcept
  {
    free_block *nb = (free_block *)addr;
    nb->next = free_lists[o];
    free_lists[o] = nb;
    mask_set(o);
    tag_set_free_at(off >> __log2_min, o);
  }

  __attribute__((always_inline)) inline free_block *
  tcache_pop(i64 o) noexcept
  {
    if ( tcache_count[o] <= 0 )
      return nullptr;
    free_block *blk = tcache[o];
    tcache[o] = blk->next;
    --tcache_count[o];
    return blk;
  }

  __attribute__((always_inline)) inline bool
  tcache_push(free_block *blk, i64 o) noexcept
  {
    if ( tcache_count[o] >= __cache_cap )
      return false;
    blk->next = tcache[o];
    tcache[o] = blk;
    ++tcache_count[o];
    return true;
  }

  void
  tcache_flush(i64 o) noexcept
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
    for ( i64 i = 0; i < max_order; ++i )
      tcache_flush(i);
  }

  void
  __merge_and_free(byte *addr, i64 o) noexcept
  {
    usize off = (usize)(addr - base);

    while ( o < max_order - 1 ) {
      usize blk_sz = order_sizes[o];
      usize buddy_off = off ^ blk_sz;

      if ( !tag_is_free_at_off(buddy_off, o) )
        break;

      freelist_remove(base + buddy_off, o);
      block_tags[buddy_off >> __log2_min] = __tag_none;

      off = (buddy_off < off) ? buddy_off : off;
      addr = base + off;
      ++o;
    }

    freelist_push_off(addr, o, off);
  }

  void
  __impl_zero_arrays() noexcept
  {
    free_mask = 0;
    for ( i64 i = 0; i < Mx; ++i ) {
      order_sizes[i] = (usize)Min << i;
      free_lists[i] = nullptr;
      active[i] = nullptr;
      tcache[i] = nullptr;
      tcache_count[i] = 0;
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
      usize approx_tags = (usable + Min) / (Min + 1);
      usize tag_area = (approx_tags + Min - 1) & ~(usize)(Min - 1);
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

    int m = 0;
    usize blk = Min;
    while ( blk <= total && m < Mx ) {
      ++m;
      blk <<= 1;
    }
    max_order = m;

    __impl_zero_arrays();

    for ( usize i = 0; i < tag_count; ++i )
      block_tags[i] = __tag_none;

    free_lists[max_order - 1] = (free_block *)base;
    free_lists[max_order - 1]->next = nullptr;
    mask_set(max_order - 1);
    tag_set_free(base, max_order - 1);
  }

  ~__buddy_list() noexcept {}

  __buddy_list(void) = delete;

  __buddy_list(const T &mem) noexcept
      : base(nullptr), total(0), max_order(0), allocated_bytes(0), tombstoned_bytes(0), free_mask(0), block_tags(nullptr), tag_count(0),
        tags_external(false)
  {
    __impl_zero_arrays();
    if ( mem.zero() or mem.len < Min )
      micron::abort();
    __impl_init_memory(mem.ptr, mem.len);
  }

  __buddy_list(const T &mem, u8 *tag_buf) noexcept
      : base(nullptr), total(0), max_order(0), allocated_bytes(0), tombstoned_bytes(0), free_mask(0), block_tags(nullptr), tag_count(0),
        tags_external(true)
  {
    __impl_zero_arrays();
    if ( mem.zero() or mem.len < Min )
      micron::abort();
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

    for ( i64 i = 0; i < Mx; ++i ) {
      free_lists[i] = o.free_lists[i];
      active[i] = o.active[i];
      order_sizes[i] = o.order_sizes[i];
      tcache[i] = o.tcache[i];
      tcache_count[i] = o.tcache_count[i];
      o.free_lists[i] = nullptr;
      o.active[i] = nullptr;
      o.tcache[i] = nullptr;
      o.tcache_count[i] = 0;
    }
  }

  __buddy_list &operator=(const __buddy_list &) = delete;

  __buddy_list &
  operator=(__buddy_list &&o)
  {

    tcache_flush_all();

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

    for ( i64 i = 0; i < Mx; ++i ) {
      free_lists[i] = o.free_lists[i];
      active[i] = o.active[i];
      order_sizes[i] = o.order_sizes[i];
      tcache[i] = o.tcache[i];
      tcache_count[i] = o.tcache_count[i];
      o.free_lists[i] = nullptr;
      o.active[i] = nullptr;
      o.tcache[i] = nullptr;
      o.tcache_count[i] = 0;
    }
    return *this;
  }

  T
  allocate(usize n) noexcept
  {
    n += __hdr_offset;
    if ( n == 0 )
      n = 1;
    if ( !base )
      return { nullptr, 0 };
    usize needed = (n + Min - 1) & ~(Min - 1);
    i64 o = order_for_size(needed);
    if ( o >= max_order )
      return { nullptr, 0 };

    i64 i = find_free_order(o);
    if ( i >= max_order )
      return { nullptr, 0 };

    free_block *blk = free_lists[i];
    free_lists[i] = blk->next;
    mask_clear_if_empty(i);

    while ( i > o ) {
      --i;
      byte *right = (byte *)blk + order_sizes[i];
      freelist_push(right, i);
    }

    // write header at the tail of the block
    block_header *hdr = hdr_of((byte *)blk, o);
    hdr->order = static_cast<i32>(o);
    hdr->flags = __block_alloc;
    tag_set_alloc((byte *)blk, o);
    allocated_bytes += order_sizes[o];

    return { (byte *)blk, order_sizes[o] - __hdr_offset };
  }

  T
  temporal_allocate(usize n) noexcept
  {
    n += __hdr_offset;
    if ( n == 0 )
      n = 1;
    if ( !base )
      return { nullptr, 0 };

    usize needed = (n + Min - 1) & ~(Min - 1);
    i64 o = order_for_size(needed);
    if ( o >= max_order )
      return { nullptr, 0 };

    usize target_size = order_sizes[o];

    if ( active[o] != nullptr ) {
      return { reinterpret_cast<byte *>(active[o]), target_size - __hdr_offset };
    }

    free_block *cached = tcache_pop(o);
    if ( cached ) {
      block_header *hdr = hdr_of((byte *)cached, o);
      hdr->order = static_cast<i32>(o);
      hdr->flags = __block_alloc | __block_temporal;
      tag_set_alloc((byte *)cached, o);
      allocated_bytes += target_size;
      active[o] = cached;
      return { (byte *)cached, target_size - __hdr_offset };
    }

    i64 i = find_free_order(o);
    if ( i >= max_order )
      return { nullptr, 0 };

    free_block *blk = free_lists[i];
    free_lists[i] = blk->next;
    mask_clear_if_empty(i);

    while ( i > o ) {
      --i;
      byte *right = (byte *)blk + order_sizes[i];
      freelist_push(right, i);
    }

    block_header *hdr = hdr_of((byte *)blk, o);
    hdr->order = static_cast<i32>(o);
    hdr->flags = __block_alloc | __block_temporal;
    tag_set_alloc((byte *)blk, o);
    allocated_bytes += target_size;

    active[o] = blk;

    return { (byte *)blk, target_size - __hdr_offset };
  }

  T
  allocate_exact(usize n) noexcept
  {
    if ( !base )
      return { nullptr, 0 };
    if ( n < Min )
      return { nullptr, 0 };
    if ( (n & (n - 1)) != 0 )
      return { nullptr, 0 };
    i64 o = order_for_size(n);
    if ( o >= max_order )
      return { nullptr, 0 };
    if ( !free_lists[o] )
      return { nullptr, 0 };

    free_block *blk = free_lists[o];
    free_lists[o] = blk->next;
    mask_clear_if_empty(o);

    block_header *hdr = hdr_of((byte *)blk, o);
    hdr->order = static_cast<i32>(o);
    hdr->flags = __block_alloc;
    tag_set_alloc((byte *)blk, o);
    allocated_bytes += order_sizes[o];

    return { (byte *)blk, order_sizes[o] - __hdr_offset };
  }

  ret_flag
  tombstone(byte *ptr) noexcept
  {
    block_header *hdr = hdr_of_tagged(ptr);
    i64 o = static_cast<i64>(hdr->order);
    if ( o < 0 || o >= max_order )
      return { __flag_invalid };
    if ( !(hdr->flags & __block_alloc) )
      return { __flag_invalid };

    hdr->flags = __block_tombstone;
    allocated_bytes -= order_sizes[o];
    tombstoned_bytes += order_sizes[o];

    return __flag_tombstoned;
  }

  ret_flag
  tombstone(T &node) noexcept
  {
    if ( !node.ptr or node.len == 0 )
      return __flag_invalid;
    return tombstone(node.ptr);
  }

  bool
  is_tombstoned(byte *ptr) noexcept
  {
    block_header *hdr = hdr_of_tagged(ptr);
    return (hdr->flags & __block_tombstone) != 0;
  }

  ret_flag
  deallocate(byte *ptr) noexcept
  {
    byte *addr = ptr;
    block_header *hdr = hdr_of_tagged(addr);
    const i64 original_o = static_cast<i64>(hdr->order);
    if ( original_o < 0 || original_o >= max_order )
      return { __flag_invalid };
    if ( !base || !addr )
      return __flag_failure;

    allocated_bytes -= order_sizes[original_o];
    if ( hdr->flags & __block_tombstone )
      tombstoned_bytes -= order_sizes[original_o];

    bool was_temporal = (hdr->flags & __block_temporal) != 0;
    hdr->flags = __block_free;

    if ( active[original_o] == (free_block *)addr )
      active[original_o] = nullptr;

    if ( was_temporal && tcache_push((free_block *)addr, original_o) ) {
      block_tags[tag_index(addr)] = __tag_none;
      return { __flag_ok };
    }

    __merge_and_free(addr, original_o);
    return { __flag_ok };
  }

  ret_flag
  deallocate(T &node) noexcept
  {
    if ( !node.ptr or node.len == 0 )
      return __flag_invalid;
    return deallocate(node.ptr);
  }

  T
  reallocate(T node, usize new_size) noexcept
  {
    if ( !base )
      return { nullptr, 0 };
    if ( !node.ptr )
      return allocate(new_size);
    if ( new_size == 0 ) {
      deallocate(node);
      return { nullptr, 0 };
    }

    if ( node.len >= new_size && new_size > (node.len >> 1) )
      return node;

    T nnode = allocate(new_size);
    if ( !nnode.ptr )
      return { nullptr, 0 };

    usize to_copy = (node.len < nnode.len) ? node.len : nnode.len;
    micron::memcpy(nnode.ptr, node.ptr, to_copy);
    deallocate(node);
    return nnode;
  }

  usize
  available() const noexcept
  {
    if ( !base )
      return 0;
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
    if ( !base || !ptr )
      return 0;
    if ( ptr < base || ptr >= base + total )
      return 0;
    u8 tag = block_tags[tag_index(ptr)];
    i64 o = static_cast<i64>(tag & ~__tag_free);
    if ( (u64)o >= (u64)max_order )
      return 0;
    return order_sizes[o];
  }

  bool
  is_allocated(byte *ptr) const noexcept
  {
    if ( !ptr || !base || ptr < base || ptr >= base + total )
      return false;
    // ptr == block_start
    u8 tag = block_tags[tag_index(ptr)];
    return (tag & __tag_free) == 0 && tag < (u8)max_order;
  }

  usize
  allocated_size(byte *ptr) const noexcept
  {
    if ( !is_allocated(ptr) )
      return 0;
    return block_size(ptr);
  }
};
};     // namespace abc
