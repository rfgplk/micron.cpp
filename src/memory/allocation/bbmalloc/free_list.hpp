// Copyright (c) 2026 David Lucius Severus
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

#include "config.hpp"
#include "metadata.hpp"

#include "../../../memory/allocation/kmemory.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"

namespace bb
{

struct __attribute__((__may_alias__)) __buddy_link {
  __buddy_link *next;
  __buddy_link *prev;
};

template <i32 Mx, bool On> struct __buddy_cache {
  __buddy_link *head[Mx];
  u8 count[Mx];
};

template <i32 Mx> struct __buddy_cache<Mx, false> {
};

template <usize Min, i32 Mx> struct __buddy_list {
  static_assert((Min & (Min - 1)) == 0, "bb: Min must be a power of two");
  static_assert(Min >= 2 * sizeof(void *), "bb: Min must hold two links");
  static_assert(Mx >= 1 && Mx <= 63, "bb: Mx out of range");

  using chunk_t = micron::__chunk<byte>;
  using mask_t = micron::conditional_t<(Mx <= 32), u32, u64>;
  using free_block = __buddy_link;

  static constexpr i32 __log2_min = __bits::__log2(Min);
  static constexpr u32 __cache_cap = __default_order_cache_cap;
  static constexpr i32 __cache_max_order = __default_order_cache_max_order;
  static constexpr bool __check_links = __default_link_checks;
  static constexpr usize __max_total = static_cast<usize>(0xFFFFFFF0u);

  byte *base;
  usize total;
  u8 *block_tags;
  i32 max_order;
  mask_t free_mask;
  usize allocated_bytes;
  free_block *free_lists[Mx];
  free_block __scratch;
  usize base_align;
  usize tag_count;
  byte *raw_mem;
  usize raw_len;
  u8 *ext_tags;
  usize ext_cap;
  usize want_align;
  [[no_unique_address]] __buddy_cache<Mx, (__cache_cap > 0)> cache;

  static constexpr usize
  order_size(i32 o) noexcept
  {
    return Min << o;
  }

  static constexpr mask_t
  __bit(i32 o) noexcept
  {
    return static_cast<mask_t>(1) << o;
  }

  static constexpr mask_t
  __all_if(bool c) noexcept
  {
    return static_cast<mask_t>(0) - static_cast<mask_t>(c);
  }

  __bb_hot static i32
  order_for_size(usize n) noexcept
  {
    const usize x = (n - 1) >> __log2_min;
    return __bits::__fls(x | 1) + (x != 0 ? 1 : 0);
  }

  static inline free_block *
  __at(byte *addr) noexcept
  {
    return reinterpret_cast<free_block *>(addr);
  }

  static inline i32
  __ctz(mask_t m) noexcept
  {
    if constexpr( sizeof(mask_t) == 8 )
      return __bits::__ctz64(m);
    else
      return __bits::__ctz32(m);
  }

  static inline i32
  __fls_mask(mask_t m) noexcept
  {
    if constexpr( sizeof(mask_t) == 8 )
      return 63 - __builtin_clzll(static_cast<unsigned long long>(m));
    else
      return 31 - __builtin_clz(static_cast<unsigned int>(m));
  }

  inline usize
  tag_index(const byte *addr) const noexcept
  {
    return static_cast<usize>(addr - base) >> __log2_min;
  }

  inline u8
  tag_at(usize off) const noexcept
  {
    return block_tags[off >> __log2_min];
  }

  inline bool
  is_sheet_at(usize off) const noexcept
  {
    return (block_tags[off >> __log2_min] & __tag_kind) == __tag_sheet;
  }

  inline void
  mark_sheet(const byte *p) noexcept
  {
    u8 &t = block_tags[tag_index(p)];
    t = static_cast<u8>(t | __tag_sheet);
  }

  inline void
  unmark_sheet(const byte *p) noexcept
  {
    u8 &t = block_tags[tag_index(p)];
    t = static_cast<u8>(t & (__tag_alloc | __tag_order));
  }

  inline bool
  __in_range(const void *p) const noexcept
  {
    return static_cast<usize>(reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(base)) < total;
  }

  inline bool
  __link_valid(const free_block *n) const noexcept
  {
    const usize d = static_cast<usize>(reinterpret_cast<uintptr_t>(n) - reinterpret_cast<uintptr_t>(base));
    return n == nullptr || ((d < total) & ((d & (Min - 1)) == 0));
  }

  inline i32
  find_free_order(i32 o) const noexcept
  {
    const mask_t m = free_mask >> o;
    return m == 0 ? max_order : o + __ctz(m);
  }

  __bb_hot void
  __link_front(byte *addr, i32 o) noexcept
  {
    free_block *nb = __at(addr);
    free_block *head = free_lists[o];
    nb->next = head;
    nb->prev = nullptr;
    free_block *t = head != nullptr ? head : &__scratch;
    t->prev = nb;
    free_lists[o] = nb;
    block_tags[tag_index(addr)] = static_cast<u8>(__tag_free | o);
  }

  __bb_hot void
  __push_free(byte *addr, i32 o) noexcept
  {
    __link_front(addr, o);
    free_mask |= __bit(o);
  }

  __bb_hot bool
  __unlink_check(const free_block *node, i32 o) const noexcept
  {
    if constexpr( __check_links ) {
      const free_block *next = node->next;
      const bool is_head = free_lists[o] == node;
      const free_block *prev = is_head ? nullptr : node->prev;
      if( !(__link_valid(next) & __link_valid(prev)) ) [[unlikely]]
        return false;
      const free_block *pv = prev != nullptr ? prev : &__scratch;
      const free_block *nv = next != nullptr ? next : &__scratch;
      const bool prev_ok = is_head | ((prev != nullptr) & (pv->next == node));
      const bool next_ok = (next == nullptr) | (nv->prev == node);
      return prev_ok & next_ok;
    } else {
      (void)node;
      (void)o;
      return true;
    }
  }

  __bb_hot void
  __unlink_do(free_block *node, i32 o) noexcept
  {
    free_block *next = node->next;
    const bool is_head = free_lists[o] == node;
    free_block *prev = is_head ? nullptr : node->prev;
    free_block *pv = prev != nullptr ? prev : &__scratch;
    free_block **slot = is_head ? &free_lists[o] : &pv->next;
    *slot = next;
    free_block *t = ((next != nullptr) & !is_head) ? next : &__scratch;
    t->prev = prev;
    free_mask &= ~(__bit(o) & __all_if(free_lists[o] == nullptr));
  }

  __bb_hot bool
  __unlink(byte *addr, i32 o) noexcept
  {
    free_block *node = __at(addr);
    if( !__unlink_check(node, o) ) [[unlikely]]
      return false;
    __unlink_do(node, o);
    return true;
  }

  __bb_hot free_block *
  __take_and_split(i32 from, i32 target) noexcept
  {
    free_block *blk = free_lists[from];
    if( blk == nullptr ) [[unlikely]] {
      free_mask &= ~__bit(from);
      return nullptr;
    }
    free_block *next = blk->next;
    if constexpr( __check_links ) {
      if( !__link_valid(next) ) [[unlikely]]
        next = nullptr;
    }
    free_lists[from] = next;
    mask_t m = free_mask & ~(__bit(from) & __all_if(next == nullptr));
    usize split = order_size(from);
    byte *b = reinterpret_cast<byte *>(blk);
    for( i32 o = from; o > target; ) {
      --o;
      split >>= 1;
      __link_front(b + split, o);
    }
    m |= (__bit(from) - 1) & ~(__bit(target) - 1);
    free_mask = m;
    return blk;
  }

  __bb_hot void
  __merge_and_free(byte *addr, i32 o) noexcept
  {
    byte *const b = base;
    u8 *const tags = block_tags;
    const usize tot = total;
    const i32 top = max_order - 1;
    usize off = static_cast<usize>(addr - b);
    while( o < top ) {
      const usize sz = order_size(o);
      const usize buddy_off = off ^ sz;
      if( buddy_off >= tot )
        break;
      if( tags[buddy_off >> __log2_min] != static_cast<u8>(__tag_free | o) )
        break;
      if( !__unlink(b + buddy_off, o) ) [[unlikely]]
        break;
      tags[(off | sz) >> __log2_min] = __tag_none;
      off &= ~sz;
      ++o;
    }
    __push_free(b + off, o);
  }

  inline void
  __zero_arrays() noexcept
  {
    free_mask = 0;
    __scratch.next = nullptr;
    __scratch.prev = nullptr;
    for( i32 i = 0; i < Mx; ++i )
      free_lists[i] = nullptr;
    if constexpr( __cache_cap > 0 ) {
      for( i32 i = 0; i < Mx; ++i ) {
        cache.head[i] = nullptr;
        cache.count[i] = 0;
      }
    }
  }

  inline void
  __seed() noexcept
  {
    const usize units = tag_count;
#if defined(BB_SEED_TOP_ONLY)
    __push_free(base, max_order - 1);
    (void)units;
#else
    usize off = 0;
    for( i32 o = max_order - 1; o >= 0; --o ) {
      if( units & (static_cast<usize>(1) << o) ) {
        __push_free(base + off, o);
        off += order_size(o);
      }
    }
#endif
  }

  bool
  __setup(bool zeroed) noexcept
  {
    base = nullptr;
    total = 0;
    max_order = 0;
    allocated_bytes = 0;
    base_align = 0;
    block_tags = nullptr;
    tag_count = 0;
    __zero_arrays();
    if( raw_mem == nullptr || raw_len < 4 * Min )
      return false;

    usize align = want_align < Min ? Min : want_align;
    if( !__bits::__is_pow2(align) )
      align = static_cast<usize>(1) << __bits::__fls(align);
    if( align < Min )
      align = Min;
    const uintptr_t end = reinterpret_cast<uintptr_t>(raw_mem) + raw_len;
    usize tag_area = 0;
    u8 *tags_at = ext_tags;
    if( ext_tags == nullptr ) {
      tag_area = (raw_len + Min) / (Min + 1);
      tags_at = raw_mem;
    }
    const uintptr_t data = reinterpret_cast<uintptr_t>(raw_mem) + tag_area;
    uintptr_t b = 0;
    for( ;; ) {
      b = __bits::__align_up_ptr(data, align);
      const usize need = align > 2 * Min ? align : 2 * Min;
      if( b >= data && b <= end && static_cast<usize>(end - b) >= need )
        break;
      if( align <= Min )
        return false;
      align >>= 1;
    }
    base = reinterpret_cast<byte *>(b);
    base_align = align;
    usize t = static_cast<usize>(end - b);
    if constexpr( Mx < static_cast<i32>(sizeof(usize) * 8) ) {
      const usize cap = ((static_cast<usize>(1) << Mx) - 1) * Min;
      if( t > cap )
        t = cap;
    }
    if( t > __max_total )
      t = __max_total;
    t = (t / Min) * Min;
    usize count = t / Min;
    if( ext_tags != nullptr && count > ext_cap ) {
      count = ext_cap;
      t = count * Min;
    }
    if( count == 0 )
      return false;
    total = t;
    tag_count = count;
    block_tags = tags_at;
    max_order = __bits::__fls(count) + 1;
    if( !zeroed )
      __builtin_memset(block_tags, __tag_none, tag_count);
    __seed();
    return true;
  }

  bool
  init(byte *mem, usize len, u8 *tags, usize tags_cap, usize align, bool zeroed = false) noexcept
  {
    raw_mem = mem;
    raw_len = len;
    ext_tags = tags;
    ext_cap = tags_cap;
    want_align = align;
    return __setup(zeroed);
  }

  bool
  reset() noexcept
  {
    return __setup(false);
  }

  void
  flush_cache() noexcept
  {
    if constexpr( __cache_cap > 0 ) {
      for( i32 o = 0; o < max_order; ++o ) {
        while( cache.head[o] != nullptr ) {
          free_block *blk = cache.head[o];
          cache.head[o] = __link_valid(blk->next) ? blk->next : nullptr;
          __merge_and_free(reinterpret_cast<byte *>(blk), o);
        }
        cache.count[o] = 0;
      }
    }
  }

  __bb_hot chunk_t
  __grant(free_block *blk, i32 o) noexcept
  {
    byte *addr = reinterpret_cast<byte *>(blk);
    block_tags[tag_index(addr)] = static_cast<u8>(__tag_alloc | o);
    const usize sz = order_size(o);
    allocated_bytes += sz;
    return { addr, sz };
  }

  __bb_hot chunk_t
  __allocate_order(i32 o) noexcept
  {
    if( o >= max_order ) [[unlikely]]
      return { nullptr, 0 };
    if constexpr( __cache_cap > 0 ) {
      if( o <= __cache_max_order && cache.head[o] != nullptr ) {
        free_block *blk = cache.head[o];
        free_block *nxt = __link_valid(blk->next) ? blk->next : nullptr;
        cache.head[o] = nxt;
        cache.count[o] = nxt == nullptr ? 0 : static_cast<u8>(cache.count[o] - 1);
        return __grant(blk, o);
      }
    }
    i32 i = find_free_order(o);
    if( i >= max_order ) [[unlikely]] {
      if constexpr( __cache_cap > 0 ) {
        flush_cache();
        i = find_free_order(o);
        if( i >= max_order )
          return { nullptr, 0 };
      } else {
        return { nullptr, 0 };
      }
    }
    free_block *blk = __take_and_split(i, o);
    if( blk == nullptr ) [[unlikely]]
      return { nullptr, 0 };
    return __grant(blk, o);
  }

  __bb_hot chunk_t
  allocate(usize n) noexcept
  {
    if( n == 0 ) [[unlikely]]
      return { nullptr, 0 };
    return __allocate_order(order_for_size(n));
  }

  chunk_t
  allocate_aligned(usize n, usize alignment) noexcept
  {
    if( n == 0 || !__bits::__is_pow2(alignment) )
      return { nullptr, 0 };
    if( alignment > base_align )
      return { nullptr, 0 };
    i32 o = order_for_size(n);
    const i32 oa = order_for_size(alignment);
    if( oa > o )
      o = oa;
    return __allocate_order(o);
  }

  bool
  contains(const byte *p) const noexcept
  {
    return __in_range(p);
  }

  __bb_hot i32
  __order_at(const byte *p) const noexcept
  {
    const usize off = static_cast<usize>(reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(base));
    if( (off >= total) | ((off & (Min - 1)) != 0) ) [[unlikely]]
      return -1;
    const u8 t = block_tags[off >> __log2_min];
    const i32 o = t & __tag_order;
    return (((t & __tag_alloc) != 0) & (o < max_order)) ? o : -1;
  }

  bool
  is_allocated(const byte *p) const noexcept
  {
    return __order_at(p) >= 0;
  }

  usize
  block_size(const byte *p) const noexcept
  {
    const i32 o = __order_at(p);
    return o < 0 ? 0 : order_size(o);
  }

  inline bool
  servable(usize n) const noexcept
  {
    return base != nullptr && n != 0 && order_for_size(n) < max_order;
  }

  inline bool
  servable_aligned(usize n, usize alignment) const noexcept
  {
    if( base == nullptr || n == 0 || !__bits::__is_pow2(alignment) || alignment > base_align )
      return false;
    i32 o = order_for_size(n);
    const i32 oa = order_for_size(alignment);
    if( oa > o )
      o = oa;
    return o < max_order;
  }

  __bb_hot void
  deallocate_order(byte *p, i32 o) noexcept
  {
    allocated_bytes -= order_size(o);
    if constexpr( __cache_cap > 0 ) {
      if( o <= __cache_max_order && cache.count[o] < __cache_cap ) {
        free_block *blk = __at(p);
        blk->next = cache.head[o];
        blk->prev = nullptr;
        cache.head[o] = blk;
        ++cache.count[o];
        block_tags[tag_index(p)] = __tag_none;
        return;
      }
    }
    __merge_and_free(p, o);
  }

  __bb_hot ret_flag
  deallocate(byte *p) noexcept
  {
    const i32 o = __order_at(p);
    if( o < 0 ) [[unlikely]]
      return ret_flag::invalid;
    deallocate_order(p, o);
    return ret_flag::ok;
  }

  usize
  grow(byte *p, usize n) noexcept
  {
    const i32 o = __order_at(p);
    if( o < 0 || n == 0 )
      return 0;
    const i32 want = order_for_size(n);
    if( want <= o )
      return order_size(o);
    if( want >= max_order )
      return 0;
    const usize off = static_cast<usize>(p - base);
    if( (off & (order_size(want) - 1)) != 0 )
      return 0;
    for( i32 j = o; j < want; ++j ) {
      const usize boff = off + order_size(j);
      if( boff >= total || block_tags[boff >> __log2_min] != static_cast<u8>(__tag_free | j) )
        return 0;
      if( !__unlink_check(__at(base + boff), j) )
        return 0;
    }
    for( i32 j = o; j < want; ++j ) {
      const usize boff = off + order_size(j);
      __unlink_do(__at(base + boff), j);
      block_tags[boff >> __log2_min] = __tag_none;
    }
    block_tags[off >> __log2_min] = static_cast<u8>(__tag_alloc | want);
    allocated_bytes += order_size(want) - order_size(o);
    return order_size(want);
  }

  usize
  available() const noexcept
  {
    return base ? total - allocated_bytes : 0;
  }

  usize
  used() const noexcept
  {
    return allocated_bytes;
  }

  usize
  __total() const noexcept
  {
    return total;
  }

  usize
  largest_free() noexcept
  {
    flush_cache();
    if( free_mask == 0 )
      return 0;
    return order_size(__fls_mask(free_mask));
  }

  usize
  largest_free_quiet() const noexcept
  {
    if( free_mask == 0 )
      return 0;
    return order_size(__fls_mask(free_mask));
  }

  u32
  free_list_count() const noexcept
  {
    u32 c = 0;
    for( i32 o = 0; o < max_order; ++o )
      for( const free_block *n = free_lists[o]; n != nullptr && c <= tag_count; n = n->next )
        ++c;
    return c;
  }
};

};
