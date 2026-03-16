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

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//  __tlsf_list — O(1) two level segregated fit
// our buddy list greatly underperformed on lots of small tiny allocations
// this has been implemented to alleviate that pressure and speed up performance for general purpose code
// all ops are o(1)
//
//  block layout (within __hdr_offset of 32b, reused for compat across systems):
//    offset 0:  total block size in bytes (incl header)
//    offset 4:  __block_alloc / __block_free / __block_tombstone
//    offset 8:  previous physical block
//    offset 16: free-list link
//    offset 24: free-list link
//    offset 32: meta

template <typename T, i64 Min, i32 Mx = 64>
  requires(micron::is_trivially_constructible_v<T> and micron::is_trivially_destructible_v<T> and (bool)((Min & (Min - 1)) == 0))
struct __tlsf_list {

  static constexpr usize __block_align = __hdr_offset;        // 32
  static constexpr usize __min_block = __block_align * 2;     // 64
  static constexpr i32 __sl_log2 = 4;
  static constexpr i32 __sl_count = 1 << __sl_log2;                // 16
  static constexpr i32 __fl_shift = 6;                             // log2(64)
  static constexpr i32 __fl_count = 26;                            // indices 0..25
  static constexpr i32 __list_count = __fl_count * __sl_count;     // 416

  static_assert(__block_align >= 32, "header must fit in __hdr_offset");
  static_assert((1ULL << __fl_shift) == __min_block, "__fl_shift must equal log2(__min_block)");
  static_assert(__fl_count <= 32, "__fl_count must fit in u32 bitmap");
  static_assert(Mx <= 64, "Mx must be <= 64");

  struct tlsf_hdr {
    u32 bsize;
    i32 flags;
    tlsf_hdr *prev_phys;
    tlsf_hdr *next_free;
    tlsf_hdr *prev_free;
  };

  static_assert(sizeof(tlsf_hdr) <= __hdr_offset, "tlsf_hdr must fit in __hdr_offset bytes");

  byte *base;       // pool start (= start sentinel address)
  usize total;      // data-region size between sentinels
  i32 fl_count;     // runtime FL levels used
  usize allocated_bytes;
  usize tombstoned_bytes;
  u32 fl_bitmap;                               // first-level bitmap
  u32 sl_bitmap[__fl_count];                   // second-level bitmaps
  tlsf_hdr *heads[__list_count];               // free-list heads  [fi * __sl_count + si]
  tlsf_hdr *temporal_active[__list_count];     // one active temporal block per class

  __attribute__((always_inline)) static inline usize
  align_up(usize v, usize a) noexcept
  {
    return (v + a - 1) & ~(a - 1);
  }

  // floor(log2(v)),  v must be > 0
  __attribute__((always_inline)) static inline i32
  fls64(usize v) noexcept
  {
    return 63 - __builtin_clzll(v);
  }

  __attribute__((always_inline)) static inline i32
  idx(i32 fi, i32 si) noexcept
  {
    return fi * __sl_count + si;
  }

  __attribute__((always_inline)) static inline void
  mapping_insert(usize size, i32 &fi, i32 &si) noexcept
  {
    i32 fl = fls64(size);
    fi = fl - __fl_shift;
    i32 shift = fl - __sl_log2;
    si = (i32)((size >> shift) & (__sl_count - 1));
  }

  __attribute__((always_inline)) static inline void
  mapping_search(usize size, i32 &fi, i32 &si) noexcept
  {

    i32 fl = fls64(size);
    i32 shift = fl - __sl_log2;
    size += ((usize)1 << shift) - 1;
    fl = fls64(size);
    fi = fl - __fl_shift;
    shift = fl - __sl_log2;
    si = (i32)((size >> shift) & (__sl_count - 1));
  }

  __attribute__((always_inline)) inline tlsf_hdr *
  next_phys(tlsf_hdr *b) const noexcept
  {
    return reinterpret_cast<tlsf_hdr *>(reinterpret_cast<byte *>(b) + (usize)b->bsize);
  }

  void
  fl_insert(tlsf_hdr *block) noexcept
  {
    i32 fi, si;
    mapping_insert((usize)block->bsize, fi, si);
    i32 i = idx(fi, si);

    tlsf_hdr *head = heads[i];
    block->next_free = head;
    block->prev_free = nullptr;
    if ( head )
      head->prev_free = block;
    heads[i] = block;

    block->flags = __block_free;
    fl_bitmap |= (1u << fi);
    sl_bitmap[fi] |= (1u << si);
  }

  void
  fl_remove(tlsf_hdr *block) noexcept
  {
    i32 fi, si;
    mapping_insert((usize)block->bsize, fi, si);
    i32 i = idx(fi, si);

    if ( block->prev_free )
      block->prev_free->next_free = block->next_free;
    else
      heads[i] = block->next_free;

    if ( block->next_free )
      block->next_free->prev_free = block->prev_free;

    if ( !heads[i] ) {
      sl_bitmap[fi] &= ~(1u << si);
      if ( !sl_bitmap[fi] )
        fl_bitmap &= ~(1u << fi);
    }
  }

  tlsf_hdr *
  find_free(usize needed) noexcept
  {
    i32 fi, si;
    mapping_search(needed, fi, si);
    if ( fi >= fl_count )
      return nullptr;

    u32 sl_map = sl_bitmap[fi] & (~0u << si);
    if ( !sl_map ) {

      u32 fl_map = fl_bitmap & (~0u << (fi + 1));
      if ( !fl_map )
        return nullptr;
      fi = __builtin_ctz(fl_map);
      sl_map = sl_bitmap[fi];
    }
    si = __builtin_ctz(sl_map);

    tlsf_hdr *block = heads[idx(fi, si)];
    fl_remove(block);
    return block;
  }

  void
  try_split(tlsf_hdr *block, usize needed) noexcept
  {
    usize remain = (usize)block->bsize - needed;
    if ( remain < __min_block )
      return;

    block->bsize = (u32)needed;

    tlsf_hdr *rem = reinterpret_cast<tlsf_hdr *>(reinterpret_cast<byte *>(block) + needed);
    rem->bsize = (u32)remain;
    rem->flags = __block_free;
    rem->prev_phys = block;

    tlsf_hdr *after = next_phys(rem);
    after->prev_phys = rem;

    fl_insert(rem);
  }

  void
  coalesce_and_free(tlsf_hdr *block) noexcept
  {
    // forward merge
    tlsf_hdr *next = next_phys(block);
    if ( next->flags == __block_free ) {
      fl_remove(next);
      block->bsize = (u32)((usize)block->bsize + (usize)next->bsize);
      tlsf_hdr *after = next_phys(block);
      after->prev_phys = block;
    }

    // backward merge
    tlsf_hdr *prev = block->prev_phys;
    if ( prev && prev->flags == __block_free ) {
      fl_remove(prev);
      prev->bsize = (u32)((usize)prev->bsize + (usize)block->bsize);
      tlsf_hdr *after = next_phys(prev);
      after->prev_phys = prev;
      block = prev;
    }

    fl_insert(block);
  }

  void
  __impl_zero_arrays() noexcept
  {
    fl_bitmap = 0;
    for ( i32 i = 0; i < __fl_count; ++i )
      sl_bitmap[i] = 0;
    for ( i32 i = 0; i < __list_count; ++i ) {
      heads[i] = nullptr;
      temporal_active[i] = nullptr;
    }
  }

  void
  __impl_init_memory(byte *_ptr, usize _len)
  {
    uintptr_t p = (uintptr_t)_ptr;
    uintptr_t r = align_up(p, __block_align);
    usize adjust = r - p;
    if ( _len <= adjust )
      goto fail;
    {
      byte *aligned = (byte *)r;
      usize usable = _len - adjust;
      usable = (usable / __block_align) * __block_align;

      // need start-sentinel + at least __min_block + end-sentinel
      if ( usable < 2 * __block_align + __min_block )
        goto fail;

      // u32 bsize guard — reject pools > 4 GiB
      usize data = usable - 2 * __block_align;
      if ( data > (usize)0xFFFFFFFFu )
        goto fail;

      base = aligned;
      total = data;

      i32 fl = fls64(total);
      fl_count = fl - __fl_shift + 1;
      if ( fl_count > __fl_count )
        fl_count = __fl_count;
      if ( fl_count < 1 )
        fl_count = 1;

      __impl_zero_arrays();

      tlsf_hdr *ss = reinterpret_cast<tlsf_hdr *>(base);
      ss->bsize = (u32)__block_align;
      ss->flags = __block_alloc;
      ss->prev_phys = nullptr;
      ss->next_free = nullptr;
      ss->prev_free = nullptr;

      tlsf_hdr *es = reinterpret_cast<tlsf_hdr *>(base + __block_align + total);
      es->bsize = (u32)__block_align;
      es->flags = __block_alloc;
      es->prev_phys = nullptr;
      es->next_free = nullptr;
      es->prev_free = nullptr;

      tlsf_hdr *blk = reinterpret_cast<tlsf_hdr *>(base + __block_align);
      blk->bsize = (u32)total;
      blk->flags = __block_free;
      blk->prev_phys = ss;

      es->prev_phys = blk;

      fl_insert(blk);
      return;
    }
  fail:
    base = nullptr;
    total = 0;
    fl_count = 0;
  }

  ~__tlsf_list() noexcept = default;
  __tlsf_list(void) = delete;

  __tlsf_list(const T &mem) noexcept : base(nullptr), total(0), fl_count(0), allocated_bytes(0), tombstoned_bytes(0), fl_bitmap(0)
  {
    __impl_zero_arrays();
    if ( mem.zero() or mem.len < (usize)Min )
      micron::abort();
    __impl_init_memory(mem.ptr, mem.len);
  }

  __tlsf_list(const T &mem, u8 *) noexcept : __tlsf_list(mem) {}

  __tlsf_list(const __tlsf_list &) = delete;

  __tlsf_list(__tlsf_list &&o)
      : base(o.base), total(o.total), fl_count(o.fl_count), allocated_bytes(o.allocated_bytes), tombstoned_bytes(o.tombstoned_bytes),
        fl_bitmap(o.fl_bitmap)
  {
    o.base = nullptr;
    o.total = 0;
    o.fl_count = 0;
    o.allocated_bytes = 0;
    o.tombstoned_bytes = 0;
    o.fl_bitmap = 0;

    for ( i32 i = 0; i < __fl_count; ++i ) {
      sl_bitmap[i] = o.sl_bitmap[i];
      o.sl_bitmap[i] = 0;
    }
    for ( i32 i = 0; i < __list_count; ++i ) {
      heads[i] = o.heads[i];
      temporal_active[i] = o.temporal_active[i];
      o.heads[i] = nullptr;
      o.temporal_active[i] = nullptr;
    }
  }

  __tlsf_list &operator=(const __tlsf_list &) = delete;

  __tlsf_list &
  operator=(__tlsf_list &&o)
  {
    base = o.base;
    total = o.total;
    fl_count = o.fl_count;
    allocated_bytes = o.allocated_bytes;
    tombstoned_bytes = o.tombstoned_bytes;
    fl_bitmap = o.fl_bitmap;

    o.base = nullptr;
    o.total = 0;
    o.fl_count = 0;
    o.allocated_bytes = 0;
    o.tombstoned_bytes = 0;
    o.fl_bitmap = 0;

    for ( i32 i = 0; i < __fl_count; ++i ) {
      sl_bitmap[i] = o.sl_bitmap[i];
      o.sl_bitmap[i] = 0;
    }
    for ( i32 i = 0; i < __list_count; ++i ) {
      heads[i] = o.heads[i];
      temporal_active[i] = o.temporal_active[i];
      o.heads[i] = nullptr;
      o.temporal_active[i] = nullptr;
    }
    return *this;
  }

  __attribute__((always_inline)) static inline usize
  adjusted_block_size(usize user_n) noexcept
  {
    usize needed = align_up(user_n + __hdr_offset, __block_align);
    return needed < __min_block ? __min_block : needed;
  }

  T
  allocate(usize n) noexcept
  {
    n += sizeof(micron::simd::i256);
    if ( n == 0 )
      n = 1;
    if ( !base )
      return { nullptr, 0 };

    usize needed = adjusted_block_size(n);

    tlsf_hdr *block = find_free(needed);
    if ( !block )
      return { nullptr, 0 };

    try_split(block, needed);
    block->flags = __block_alloc;
    allocated_bytes += (usize)block->bsize;

    return { reinterpret_cast<byte *>(block) + __hdr_offset, (usize)block->bsize - __hdr_offset };
  }

  T
  temporal_allocate(usize n) noexcept
  {
    n += sizeof(micron::simd::i256);
    if ( n == 0 )
      n = 1;
    if ( !base )
      return { nullptr, 0 };

    usize needed = adjusted_block_size(n);

    i32 fi, si;
    mapping_search(needed, fi, si);
    if ( fi >= 0 && fi < fl_count ) {
      i32 i = idx(fi, si);
      if ( temporal_active[i] ) {
        tlsf_hdr *blk = temporal_active[i];
        return { reinterpret_cast<byte *>(blk) + __hdr_offset, (usize)blk->bsize - __hdr_offset };
      }
    }

    tlsf_hdr *block = find_free(needed);
    if ( !block )
      return { nullptr, 0 };

    try_split(block, needed);
    block->flags = __block_alloc | __block_temporal;
    allocated_bytes += (usize)block->bsize;

    if ( fi >= 0 && fi < fl_count )
      temporal_active[idx(fi, si)] = block;

    return { reinterpret_cast<byte *>(block) + __hdr_offset, (usize)block->bsize - __hdr_offset };
  }

  T
  allocate_exact(usize n) noexcept
  {
    if ( !base )
      return { nullptr, 0 };
    if ( n < __min_block )
      return { nullptr, 0 };
    if ( (n & (n - 1)) != 0 )
      return { nullptr, 0 };

    i32 fi, si;
    mapping_insert(n, fi, si);
    if ( fi < 0 || fi >= fl_count )
      return { nullptr, 0 };

    i32 i = idx(fi, si);
    if ( !heads[i] )
      return { nullptr, 0 };

    tlsf_hdr *block = heads[i];
    fl_remove(block);
    block->flags = __block_alloc;
    allocated_bytes += (usize)block->bsize;

    return { reinterpret_cast<byte *>(block) + __hdr_offset, (usize)block->bsize - __hdr_offset };
  }

  ret_flag
  tombstone(byte *ptr) noexcept
  {
    tlsf_hdr *hdr = reinterpret_cast<tlsf_hdr *>(ptr - __hdr_offset);
    if ( !(hdr->flags & __block_alloc) )
      return { __flag_invalid };

    usize bsz = (usize)hdr->bsize;
    hdr->flags = __block_tombstone;
    allocated_bytes -= bsz;
    tombstoned_bytes += bsz;

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
    tlsf_hdr *hdr = reinterpret_cast<tlsf_hdr *>(ptr - __hdr_offset);
    return (hdr->flags & __block_tombstone) != 0;
  }

  ret_flag
  deallocate(byte *ptr) noexcept
  {
    if ( !ptr || !base )
      return __flag_failure;

    tlsf_hdr *block = reinterpret_cast<tlsf_hdr *>(ptr - __hdr_offset);
    usize bsz = (usize)block->bsize;

    if ( block->flags == __block_free )
      return { __flag_invalid };

    allocated_bytes -= bsz;
    if ( block->flags & __block_tombstone )
      tombstoned_bytes -= bsz;

    if ( block->flags & __block_temporal ) {
      i32 fi, si;
      mapping_insert(bsz, fi, si);
      if ( fi >= 0 && fi < fl_count ) {
        i32 i = idx(fi, si);
        if ( temporal_active[i] == block )
          temporal_active[i] = nullptr;
      }
    }

    coalesce_and_free(block);
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
    byte *data_lo = base + __block_align;
    byte *data_hi = base + __block_align + total;
    byte *blk = ptr - __hdr_offset;
    if ( blk < data_lo || blk >= data_hi )
      return 0;
    const tlsf_hdr *hdr = reinterpret_cast<const tlsf_hdr *>(blk);
    return (usize)hdr->bsize;
  }

  bool
  is_allocated(byte *ptr) const noexcept
  {
    if ( !ptr || !base )
      return false;
    byte *data_lo = base + __block_align;
    byte *data_hi = base + __block_align + total;
    byte *blk = ptr - __hdr_offset;
    if ( blk < data_lo || blk >= data_hi )
      return false;
    const tlsf_hdr *hdr = reinterpret_cast<const tlsf_hdr *>(blk);

    return hdr->flags != __block_free;
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
