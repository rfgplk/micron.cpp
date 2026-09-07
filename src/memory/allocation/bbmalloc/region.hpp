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

#include "cache_list.hpp"
#include "config.hpp"
#include "free_list.hpp"
#include "metadata.hpp"
#include "stats.hpp"

#include "../../../memory/allocation/kmemory.hpp"
#include "../../../types.hpp"

namespace bb
{

struct alignas(64) __region {
  using buddy_t = __buddy_list<__default_min_block, __max_orders>;
  using tlsf_t = __tlsf_list<__default_tlsf_sheet, __sl_log2, __max_tlsf_sheets, __class_small>;
  using chunk_t = micron::__chunk<byte>;

  static constexpr usize __sheet = __default_tlsf_sheet;

  static_assert(__class_small + header_size + (__default_redzone ? __default_redzone_size : 0) <= tlsf_t::__data_bytes,
                "bb: the small class must fit inside one sheet");

  buddy_t buddy;
  tlsf_t tlsf;
  u32 spare;
  bool live;

  bool
  attach(byte *mem, usize len, u8 *tags, usize tag_cap, bool zeroed = false) noexcept
  {
    live = buddy.init(mem, len, tags, tag_cap, __default_max_alignment, zeroed);
    spare = __null_off;
    if( live )
      tlsf.init(buddy.base);
    return live;
  }

  void
  reset() noexcept
  {
    if( !live )
      return;
    buddy.reset();
    tlsf.init(buddy.base);
    spare = __null_off;
  }

  __bb_slow void
  flush_spare() noexcept
  {
    if( spare == __null_off )
      return;
    const u32 s = spare;
    spare = __null_off;
    if( buddy.is_sheet_at(s) && tlsf.sheet_empty(s) )
      release_sheet(s);
  }

  __bb_hot bool
  contains(const void *p) const noexcept
  {
    return buddy.__in_range(p);
  }

  inline bool
  overlaps(const void *lo, usize len) const noexcept
  {
    if( !live )
      return false;
    const uintptr_t a = reinterpret_cast<uintptr_t>(lo);
    const uintptr_t b = a + len;
    const uintptr_t r = reinterpret_cast<uintptr_t>(buddy.raw_mem);
    const uintptr_t s = r + buddy.raw_len;
    return a < s && r < b;
  }

  inline u32
  off_of(const void *p) const noexcept
  {
    return static_cast<u32>(reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(buddy.base));
  }

  __bb_slow void
  release_sheet(u32 sheet) noexcept
  {
    if( !live || sheet == __null_off )
      return;
    byte *p = buddy.base + sheet;
    if( !buddy.__in_range(p) || !buddy.is_sheet_at(sheet) )
      return;
    if( !tlsf.remove_sheet(sheet) )
      return;
    buddy.unmark_sheet(p);
    if( buddy.deallocate(p) != ret_flag::ok ) {
      buddy.mark_sheet(p);
      (void)tlsf.add_sheet(sheet);
      return;
    }
    if( spare == sheet )
      spare = __null_off;
    collect_stats<stat_type::sheet_released>();
  }

  __bb_slow chunk_t
  small_alloc_slow(usize n) noexcept
  {
    if( tlsf.count >= __max_tlsf_sheets )
      return { nullptr, 0 };
    chunk_t s = buddy.allocate(__sheet);
    if( s.ptr == nullptr )
      return { nullptr, 0 };
    const u32 fresh = off_of(s.ptr);
    if( !tlsf.add_sheet(fresh) ) {
      (void)buddy.deallocate(s.ptr);
      return { nullptr, 0 };
    }
    buddy.mark_sheet(s.ptr);
    collect_stats<stat_type::sheet_acquired>();
    chunk_t c = tlsf.allocate(n);
    if( c.ptr == nullptr ) {
      release_sheet(fresh);
      return { nullptr, 0 };
    }
    return c;
  }

  __bb_hot chunk_t
  small_alloc(usize n) noexcept
  {
    const chunk_t c = tlsf.allocate(n);
    if( c.ptr == nullptr ) [[unlikely]]
      return small_alloc_slow(n);
    return c;
  }

  __bb_slow void
  __sheet_emptied(u32 sheet) noexcept
  {
    if( spare == __null_off || !tlsf.sheet_empty(spare) ) {
      spare = sheet;
      return;
    }
    release_sheet(sheet);
  }

  __bb_hot void
  small_free(u32 hdr_off) noexcept
  {
    const u32 s = tlsf.deallocate(hdr_off);
    if( s != __null_off && s != spare )
      __sheet_emptied(s);
  }

  __bb_slow chunk_t
  large_alloc_slow(usize n) noexcept
  {
    if( spare == __null_off || !buddy.servable(n) )
      return { nullptr, 0 };
    flush_spare();
    return buddy.allocate(n);
  }

  __bb_hot chunk_t
  large_alloc(usize n) noexcept
  {
    const chunk_t c = buddy.allocate(n);
    if( c.ptr == nullptr ) [[unlikely]]
      return large_alloc_slow(n);
    return c;
  }

  chunk_t
  large_alloc_aligned(usize n, usize alignment) noexcept
  {
    chunk_t c = buddy.allocate_aligned(n, alignment);
    if( c.ptr == nullptr && spare != __null_off && buddy.servable_aligned(n, alignment) ) {
      flush_spare();
      c = buddy.allocate_aligned(n, alignment);
    }
    return c;
  }

  usize
  available() const noexcept
  {
    if( !live )
      return 0;
    const usize t = buddy.__total();
    const usize u = used();
    return u >= t ? 0 : t - u;
  }

  usize
  used() const noexcept
  {
    const usize b = buddy.used();
    usize credit = static_cast<usize>(tlsf.count) * tlsf_t::__data_bytes;
    if( spare != __null_off && buddy.is_sheet_at(spare) && tlsf.sheet_empty(spare) )
      credit += __sheet - tlsf_t::__data_bytes;
    return (b < credit ? 0 : b - credit) + tlsf.allocated_bytes;
  }

  usize
  largest_free() noexcept
  {
    flush_spare();
    const usize b = buddy.largest_free();
    usize t = tlsf.largest_free();
    if( t > __class_small )
      t = __class_small;
    return b > t ? b : t;
  }
};

};
