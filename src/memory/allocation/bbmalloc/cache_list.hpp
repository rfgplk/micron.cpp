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
#include "../../../types.hpp"

namespace bb
{

template <usize Sheet, i32 SlLog2, u32 MaxSheets, usize SmallMax = __class_small> struct __tlsf_list {
  static_assert((Sheet & (Sheet - 1)) == 0 && Sheet >= 1024, "bb: sheet must be a power of two of at least 1 KiB");
  static_assert(SlLog2 >= 1 && SlLog2 <= 5, "bb: SlLog2 out of range");
  static_assert(MaxSheets >= 1 && MaxSheets <= 64, "bb: MaxSheets out of range");

  using chunk_t = micron::__chunk<byte>;

  static constexpr usize __block_align = 16;
  static constexpr usize __min_block = 32;
  static constexpr i32 __fl_shift = 5;
  static constexpr i32 __sl_count = 1 << SlLog2;
  static constexpr usize __bitmap_bytes = ((Sheet / 128) + 15) & ~static_cast<usize>(15);
  static constexpr usize __data_off = header_size + __bitmap_bytes;
  static constexpr usize __data_bytes = Sheet - __data_off - header_size;
  static constexpr i32 __fl_count = __bits::__log2(__data_bytes) - __fl_shift + 1;
  static constexpr i32 __list_count = __fl_count * __sl_count;
  static constexpr usize __max_payload = __data_bytes - header_size;
  static constexpr usize __table_max = ((SmallMax + 31) & ~static_cast<usize>(15)) + 16;
  static constexpr usize __table_n = (__table_max >> 4) + 1;

  static_assert(__fl_count >= 1 && __fl_count <= 27, "bb: derived first-level count out of range");
  static_assert(__bits::__log2(__data_bytes) - __fl_shift < __fl_count,
                "bb: first-level count cannot index the largest block");
  static_assert(__data_bytes > header_size && __data_bytes % __block_align == 0, "bb: sheet data span invalid");
  static_assert(SmallMax % 16 == 0 && __table_max * 2 <= __data_bytes, "bb: small class does not fit the sheet");

  struct __search_table {
    u8 fl[__table_n];
    u8 sl[__table_n];
  };

  struct __hit {
    u32 off;
    u32 idx;
  };

  struct __own {
    u32 hdr;
    u32 payload;
  };

  static constexpr __search_table
  __make_table() noexcept
  {
    __search_table t{};
    for( usize i = 0; i < __table_n; ++i ) {
      const u32 size = static_cast<u32>(i << 4);
      if( size < __min_block ) {
        t.fl[i] = 0;
        t.sl[i] = 0;
        continue;
      }
      const i32 f = __bits::__log2(size);
      const u32 r = size + (1u << (f - SlLog2)) - 1;
      const i32 f2 = __bits::__log2(r);
      t.fl[i] = static_cast<u8>(f2 - __fl_shift);
      t.sl[i] = static_cast<u8>((r >> (f2 - SlLog2)) & static_cast<u32>(__sl_count - 1));
    }
    return t;
  }

  static constexpr __search_table __search = __make_table();

  byte *rbase;
  u32 fl_bitmap;
  u32 sl_bitmap[__fl_count];
  usize allocated_bytes;
  u32 count;
  tlsf_hdr __scratch;
  u32 heads[__list_count];

  static inline tlsf_hdr *
  hdr_at(byte *rb, u32 off) noexcept
  {
    return reinterpret_cast<tlsf_hdr *>(rb + off);
  }

  inline tlsf_hdr *
  hdr(u32 off) const noexcept
  {
    return reinterpret_cast<tlsf_hdr *>(rbase + off);
  }

  static inline u32
  bsize(const tlsf_hdr *h) noexcept
  {
    return h->bsize_flags & ~__tlsf_flags;
  }

  static inline bool
  is_free(const tlsf_hdr *h) noexcept
  {
    return (h->bsize_flags & (__tlsf_alloc | __tlsf_sentinel)) == 0;
  }

  static inline u32
  sheet_of(u32 off) noexcept
  {
    return off & ~static_cast<u32>(Sheet - 1);
  }

  inline u8 *
  bitmap(u32 sheet) const noexcept
  {
    return reinterpret_cast<u8 *>(rbase + sheet + header_size);
  }

  void
  reset() noexcept
  {
    fl_bitmap = 0;
    for( i32 i = 0; i < __fl_count; ++i )
      sl_bitmap[i] = 0;
    for( i32 i = 0; i < __list_count; ++i )
      heads[i] = __null_off;
    count = 0;
    allocated_bytes = 0;
    __scratch.bsize_flags = 0;
    __scratch.prev_phys = __null_off;
    __scratch.next_free = __null_off;
    __scratch.prev_free = __null_off;
  }

  void
  init(byte *base) noexcept
  {
    rbase = base;
    reset();
  }

  static inline void
  mapping_insert(u32 size, i32 &fl, i32 &sl) noexcept
  {
    fl = __bits::__fls32(size) - __fl_shift;
    sl = static_cast<i32>((size >> (fl + __fl_shift - SlLog2)) & static_cast<u32>(__sl_count - 1));
  }

  static inline void
  mapping_search(u32 size, i32 &fl, i32 &sl) noexcept
  {
    const i32 f = __bits::__fls32(size);
    const u32 r = size + (1u << (f - SlLog2)) - 1;
    const i32 f2 = f + static_cast<i32>(r >> (f + 1));
    fl = f2 - __fl_shift;
    sl = static_cast<i32>((r >> (f2 - SlLog2)) & static_cast<u32>(__sl_count - 1));
  }

  __hit
  __find_exact(u32 size) const noexcept
  {
    i32 fl = 0;
    i32 sl = 0;
    mapping_insert(size, fl, sl);
    if( fl >= __fl_count )
      return { __null_off, 0 };
    const u32 idx = static_cast<u32>(fl) * __sl_count + static_cast<u32>(sl);
    const u32 off = heads[idx];
    if( off != __null_off && bsize(hdr(off)) >= size )
      return { off, idx };
    return { __null_off, 0 };
  }

  __bb_hot __hit
  find_free(u32 size) const noexcept
  {
    i32 fl = 0;
    i32 sl = 0;
    if( size <= __table_max ) [[likely]] {
      fl = __search.fl[size >> 4];
      sl = __search.sl[size >> 4];
    } else {
      mapping_search(size, fl, sl);
      if( fl >= __fl_count ) [[unlikely]]
        return __find_exact(size);
    }
    const u32 m = sl_bitmap[fl] & (~0u << sl);
    if( m != 0 ) [[likely]] {
      const u32 idx = static_cast<u32>(fl) * __sl_count + static_cast<u32>(__bits::__ctz32(m));
      return { heads[idx], idx };
    }
    const u32 fm = fl_bitmap & (~0u << (fl + 1));
    if( fm != 0 ) {
      const i32 hf = __bits::__ctz32(fm);
      const u32 idx = static_cast<u32>(hf) * __sl_count + static_cast<u32>(__bits::__ctz32(sl_bitmap[hf]));
      return { heads[idx], idx };
    }
    return __find_exact(size);
  }

  __bb_hot void
  __clear_if_empty(u32 idx, u32 head_now) noexcept
  {
    const i32 fl = static_cast<i32>(idx >> SlLog2);
    const i32 sl = static_cast<i32>(idx & static_cast<u32>(__sl_count - 1));
    const u32 drop = head_now == __null_off ? (1u << sl) : 0u;
    const u32 s = sl_bitmap[fl] & ~drop;
    sl_bitmap[fl] = s;
    const u32 fdrop = s == 0 ? (1u << fl) : 0u;
    fl_bitmap &= ~fdrop;
  }

  __bb_hot void
  insert_free(u32 off, u32 bs) noexcept
  {
    byte *const rb = rbase;
    i32 fl = 0;
    i32 sl = 0;
    mapping_insert(bs, fl, sl);
    const u32 idx = static_cast<u32>(fl) * __sl_count + static_cast<u32>(sl);
    tlsf_hdr *h = hdr_at(rb, off);
    const u32 head = heads[idx];
    h->next_free = head;
    const u32 hs = head != __null_off ? head : 0;
    tlsf_hdr *t = head != __null_off ? hdr_at(rb, hs) : &__scratch;
    t->prev_free = off;
    heads[idx] = off;
    sl_bitmap[fl] |= 1u << sl;
    fl_bitmap |= 1u << fl;
  }

  __bb_hot void
  remove_free(u32 off, u32 bs) noexcept
  {
    byte *const rb = rbase;
    i32 fl = 0;
    i32 sl = 0;
    mapping_insert(bs, fl, sl);
    const u32 idx = static_cast<u32>(fl) * __sl_count + static_cast<u32>(sl);
    tlsf_hdr *h = hdr_at(rb, off);
    const u32 nx = h->next_free;
    const u32 pv = h->prev_free;
    const u32 head = heads[idx];
    const bool is_head = head == off;
    const u32 pvs = is_head ? 0 : pv;
    u32 *slot = is_head ? &heads[idx] : &hdr_at(rb, pvs)->next_free;
    *slot = nx;
    const bool relink = (nx != __null_off) & !is_head;
    const u32 nxs = relink ? nx : 0;
    tlsf_hdr *t = relink ? hdr_at(rb, nxs) : &__scratch;
    t->prev_free = pv;
    __clear_if_empty(idx, is_head ? nx : head);
  }

  static inline usize
  adjusted(usize n) noexcept
  {
    if( n - 1 >= __data_bytes ) [[unlikely]]
      return 0;
    return (n + (header_size + __block_align - 1)) & ~(__block_align - 1);
  }

  __bb_hot u8 *
  __mark_byte(u32 off, u32 &bit) const noexcept
  {
    const u32 rel = off & static_cast<u32>(Sheet - 1);
    const u32 bi = (rel >> 4) + 1;
    bit = 1u << (bi & 7);
    return rbase + (off - rel) + header_size + (bi >> 3);
  }

  __bb_hot void
  __mark(u32 off) noexcept
  {
    u32 bit = 0;
    u8 *b = __mark_byte(off, bit);
    *b = static_cast<u8>(*b | bit);
  }

  __bb_hot void
  __unmark(u32 off) noexcept
  {
    u32 bit = 0;
    u8 *b = __mark_byte(off, bit);
    *b = static_cast<u8>(*b & ~bit);
  }

  inline bool
  __marked(u32 sheet, u32 rel) const noexcept
  {
    const u32 bi = rel >> 4;
    return (bitmap(sheet)[bi >> 3] >> (bi & 7)) & 1u;
  }

  __bb_hot chunk_t
  allocate(usize n) noexcept
  {
    const usize adj = adjusted(n);
    if( adj == 0 ) [[unlikely]]
      return { nullptr, 0 };
    byte *const rb = rbase;
    const __hit f = find_free(static_cast<u32>(adj));
    if( f.off == __null_off ) [[unlikely]]
      return { nullptr, 0 };
    tlsf_hdr *h = hdr_at(rb, f.off);
    const u32 bs = bsize(h);
    const u32 nx = h->next_free;
    heads[f.idx] = nx;
    __clear_if_empty(f.idx, nx);
    u32 take = static_cast<u32>(adj);
    if( bs - take >= __min_block ) {
      const u32 rem = f.off + take;
      tlsf_hdr *rh = hdr_at(rb, rem);
      rh->bsize_flags = bs - take;
      rh->prev_phys = f.off;
      hdr_at(rb, f.off + bs)->prev_phys = rem;
      insert_free(rem, bs - take);
    } else {
      take = bs;
    }
    h->bsize_flags = take | __tlsf_alloc;
    __mark(f.off);
    allocated_bytes += take;
    return { rb + f.off + header_size, take - header_size };
  }

  __bb_hot __own
  owns_len(const void *p) const noexcept
  {
    const u32 up = static_cast<u32>(reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(rbase));
    const u32 sheet = sheet_of(up);
    const u32 rel = up - sheet;
    constexpr u32 lo = static_cast<u32>(__data_off + header_size);
    constexpr u32 span = static_cast<u32>(Sheet - header_size) - lo;
    if( ((rel - lo) >= span) | ((rel & (__block_align - 1)) != 0) ) [[unlikely]]
      return { __null_off, 0 };
    const tlsf_hdr *h = hdr(up - static_cast<u32>(header_size));
    const u32 f = h->bsize_flags;
    const u32 bs = f & ~__tlsf_flags;
    const bool ok = __marked(sheet, rel) & ((f & (__tlsf_alloc | __tlsf_sentinel)) == __tlsf_alloc) & (bs >= __min_block)
                    & (rel - static_cast<u32>(header_size) + bs <= Sheet - header_size);
    if( !ok ) [[unlikely]]
      return { __null_off, 0 };
    return { up - static_cast<u32>(header_size), bs - static_cast<u32>(header_size) };
  }

  inline u32
  owns(const void *p) const noexcept
  {
    return owns_len(p).hdr;
  }

  inline usize
  payload(u32 off) const noexcept
  {
    return bsize(hdr(off)) - header_size;
  }

  __bb_hot u32
  deallocate(u32 off) noexcept
  {
    byte *const rb = rbase;
    tlsf_hdr *h = hdr_at(rb, off);
    u32 bs = bsize(h);
    __unmark(off);
    allocated_bytes -= bs;
    const u32 next = off + bs;
    tlsf_hdr *nh = hdr_at(rb, next);
    if( is_free(nh) ) {
      const u32 nb = bsize(nh);
      remove_free(next, nb);
      bs += nb;
    }
    const u32 prev = h->prev_phys;
    tlsf_hdr *ph = hdr_at(rb, prev);
    if( is_free(ph) ) {
      const u32 pb = bsize(ph);
      remove_free(prev, pb);
      bs += pb;
      off = prev;
      h = ph;
    }
    h->bsize_flags = bs;
    hdr_at(rb, off + bs)->prev_phys = off;
    insert_free(off, bs);
    return bs == static_cast<u32>(__data_bytes) ? sheet_of(off) : __null_off;
  }

  bool
  shrink(u32 off, usize n) noexcept
  {
    const usize adj = adjusted(n);
    if( adj == 0 )
      return false;
    byte *const rb = rbase;
    tlsf_hdr *h = hdr_at(rb, off);
    const u32 bs = bsize(h);
    if( adj > bs )
      return false;
    if( bs - adj < __min_block )
      return true;
    const u32 take = static_cast<u32>(adj);
    const u32 rem = off + take;
    tlsf_hdr *rh = hdr_at(rb, rem);
    u32 rest = bs - take;
    rh->prev_phys = off;
    h->bsize_flags = take | __tlsf_alloc;
    allocated_bytes -= rest;
    const u32 next = off + bs;
    tlsf_hdr *nh = hdr_at(rb, next);
    if( is_free(nh) ) {
      const u32 nb = bsize(nh);
      remove_free(next, nb);
      rest += nb;
    }
    rh->bsize_flags = rest;
    hdr_at(rb, rem + rest)->prev_phys = rem;
    insert_free(rem, rest);
    return true;
  }

  bool
  grow(u32 off, usize n) noexcept
  {
    const usize adj = adjusted(n);
    if( adj == 0 )
      return false;
    byte *const rb = rbase;
    tlsf_hdr *h = hdr_at(rb, off);
    const u32 bs = bsize(h);
    if( adj <= bs )
      return true;
    const u32 next = off + bs;
    tlsf_hdr *nh = hdr_at(rb, next);
    if( !is_free(nh) )
      return false;
    const u32 nb = bsize(nh);
    const u32 span = bs + nb;
    if( span < adj )
      return false;
    remove_free(next, nb);
    const u32 take = static_cast<u32>(adj);
    const u32 rest = span - take;
    if( rest >= __min_block ) {
      const u32 rem = off + take;
      tlsf_hdr *rh = hdr_at(rb, rem);
      rh->bsize_flags = rest;
      rh->prev_phys = off;
      hdr_at(rb, off + span)->prev_phys = rem;
      h->bsize_flags = take | __tlsf_alloc;
      allocated_bytes += take - bs;
      insert_free(rem, rest);
    } else {
      hdr_at(rb, off + span)->prev_phys = off;
      h->bsize_flags = span | __tlsf_alloc;
      allocated_bytes += span - bs;
    }
    return true;
  }

  bool
  sheet_empty(u32 sheet) const noexcept
  {
    const tlsf_hdr *h = hdr(sheet + static_cast<u32>(__data_off));
    return is_free(h) && bsize(h) == static_cast<u32>(__data_bytes);
  }

  bool
  add_sheet(u32 sheet) noexcept
  {
    if( count >= MaxSheets || rbase == nullptr )
      return false;
    tlsf_hdr *start = hdr(sheet);
    start->bsize_flags = static_cast<u32>(__data_off) | __tlsf_alloc | __tlsf_sentinel;
    start->prev_phys = __null_off;
    start->next_free = 0;
    start->prev_free = 0;
    __builtin_memset(bitmap(sheet), 0, __bitmap_bytes);
    tlsf_hdr *end = hdr(sheet + static_cast<u32>(Sheet - header_size));
    end->bsize_flags = static_cast<u32>(header_size) | __tlsf_alloc | __tlsf_sentinel;
    end->prev_phys = sheet + static_cast<u32>(__data_off);
    end->next_free = __null_off;
    end->prev_free = __null_off;
    tlsf_hdr *data = hdr(sheet + static_cast<u32>(__data_off));
    data->bsize_flags = static_cast<u32>(__data_bytes);
    data->prev_phys = sheet;
    ++count;
    insert_free(sheet + static_cast<u32>(__data_off), static_cast<u32>(__data_bytes));
    return true;
  }

  bool
  remove_sheet(u32 sheet) noexcept
  {
    if( count == 0 || !sheet_empty(sheet) )
      return false;
    remove_free(sheet + static_cast<u32>(__data_off), static_cast<u32>(__data_bytes));
    --count;
    return true;
  }

  usize
  largest_free() const noexcept
  {
    if( fl_bitmap == 0 )
      return 0;
    const i32 fl = __bits::__fls32(fl_bitmap);
    if( fl >= __fl_count || sl_bitmap[fl] == 0 )
      return 0;
    const i32 sl = __bits::__fls32(sl_bitmap[fl]);
    const u32 off = heads[fl * __sl_count + sl];
    if( off == __null_off )
      return 0;
    const u32 bs = bsize(hdr(off));
    return bs <= header_size ? 0 : bs - header_size;
  }

  u32
  free_block_count() const noexcept
  {
    u32 c = 0;
    for( i32 i = 0; i < __list_count; ++i )
      for( u32 o = heads[i]; o != __null_off && c <= 0xFFFFFF; o = hdr(o)->next_free )
        ++c;
    return c;
  }
};

};
