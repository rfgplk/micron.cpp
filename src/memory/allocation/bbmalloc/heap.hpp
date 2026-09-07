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
#include "harden.hpp"
#include "metadata.hpp"
#include "pool.hpp"
#include "region.hpp"
#include "stats.hpp"

#include "../../../memory/allocation/kmemory.hpp"
#include "../../../types.hpp"

namespace bb
{

enum class tier : u8 { none = 0, tlsf = 1, buddy = 2 };

struct __located {
  __region *r;
  usize len;
  u32 off;
  tier t;
};

struct __heap {
  using chunk_t = micron::__chunk<byte>;
  using buddy_t = __region::buddy_t;

  __region regions[__max_regions];
  u32 count;
  bool inited;

  __bb_cold bool
  __clear_of_all(const void *lo, usize len) const noexcept
  {
    for( u32 i = 0; i < count; ++i )
      if( regions[i].overlaps(lo, len) )
        return false;
    return true;
  }

  __bb_cold bool
  __attach(byte *mem, usize len, u8 *tags, usize tag_cap, bool zeroed) noexcept
  {
    if( count >= __max_regions || mem == nullptr || len == 0 )
      return false;
    const uintptr_t a = reinterpret_cast<uintptr_t>(mem);
    if( a + len < a )
      return false;
    if( !__clear_of_all(mem, len) )
      return false;
    if( tags != nullptr ) {
      if( tag_cap == 0 )
        return false;
      const uintptr_t t = reinterpret_cast<uintptr_t>(tags);
      if( t + tag_cap < t )
        return false;
      if( t < a + len && a < t + tag_cap )
        return false;
      if( !__clear_of_all(tags, tag_cap) )
        return false;
    }
    if( !regions[count].attach(mem, len, tags, tag_cap, zeroed) )
      return false;
    ++count;
    inited = true;
    return true;
  }

  bool
  attach(byte *mem, usize len, u8 *tags, usize tag_cap) noexcept
  {
    return __attach(mem, len, tags, tag_cap, false);
  }

  __bb_cold bool
  init() noexcept
  {
    if( inited )
      return count > 0;
    inited = true;
    const __pool_desc d = __configured_pool();
    if( d.mem != nullptr )
      __attach(d.mem, d.len, d.tags, d.tag_cap, d.zeroed);
    return count > 0;
  }

  inline bool
  __ensure() noexcept
  {
    return inited ? count > 0 : init();
  }

  void
  reset() noexcept
  {
    (void)__ensure();
    for( u32 i = 0; i < count; ++i )
      regions[i].reset();
  }

  __bb_slow __region *
  __region_slow(const void *p) noexcept
  {
    if( !__ensure() )
      return nullptr;
    for( u32 i = 0; i < count; ++i )
      if( regions[i].contains(p) )
        return &regions[i];
    return nullptr;
  }

  __bb_hot __region *
  region_of(const void *p) noexcept
  {
    if( regions[0].contains(p) ) [[likely]]
      return &regions[0];
    return __region_slow(p);
  }

  __bb_hot __located
  locate(const void *p) noexcept
  {
    __region *r = region_of(p);
    if( r == nullptr ) [[unlikely]]
      return { nullptr, 0, __null_off, tier::none };
    const u32 off = r->off_of(p);
    if( r->buddy.is_sheet_at(off & ~static_cast<u32>(__region::__sheet - 1)) ) {
      const auto o = r->tlsf.owns_len(p);
      if( o.hdr == __null_off ) [[unlikely]]
        return { r, 0, __null_off, tier::none };
      return { r, o.payload, o.hdr, tier::tlsf };
    }
    const i32 o = r->buddy.__order_at(static_cast<const byte *>(p));
    if( o < 0 ) [[unlikely]]
      return { r, 0, __null_off, tier::none };
    return { r, buddy_t::order_size(o), static_cast<u32>(o), tier::buddy };
  }

  __bb_cold bool
  __grow(usize bytes) noexcept
  {
    if constexpr( !__growable ) {
      (void)bytes;
      return false;
    } else {
      if( count >= __max_regions )
        return false;
      usize want = __bits::__next_pow2(bytes);
      if( want <= __bits::__usize_max / 2 )
        want *= 2;
      want = __bits::__round_up_sat(want, __default_tlsf_sheet);
      const __pool_desc d = __grow_pool(want);
      if( d.mem == nullptr )
        return false;
      return __attach(d.mem, d.len, d.tags, d.tag_cap, d.zeroed);
    }
  }

  __bb_slow chunk_t
  __alloc_scan(usize n) noexcept
  {
    chunk_t c{ nullptr, 0 };
    if( n <= __class_small ) {
      c = regions[0].small_alloc_slow(n);
      if constexpr( __max_regions > 1 ) {
        for( u32 i = 1; i < count && c.ptr == nullptr; ++i )
          c = regions[i].small_alloc(n);
      }
      if( c.ptr != nullptr )
        return c;
      collect_stats<stat_type::fallback>();
      c = regions[0].large_alloc(n);
      if constexpr( __max_regions > 1 ) {
        for( u32 i = 1; i < count && c.ptr == nullptr; ++i )
          c = regions[i].large_alloc(n);
      }
      return c;
    }
    c = regions[0].large_alloc_slow(n);
    if constexpr( __max_regions > 1 ) {
      for( u32 i = 1; i < count && c.ptr == nullptr; ++i )
        c = regions[i].large_alloc(n);
    }
    return c;
  }

  __bb_slow chunk_t
  __alloc_slow(usize n) noexcept
  {
    if( n == 0 || (!inited && !init()) )
      return { nullptr, 0 };
    chunk_t c = __alloc_scan(n);
    if constexpr( __growable ) {
      if( c.ptr == nullptr && __grow(n) )
        c = __alloc_scan(n);
    }
    return c;
  }

  __bb_hot chunk_t
  alloc(usize n) noexcept
  {
    if( count == 0 ) [[unlikely]]
      return __alloc_slow(n);
    if( n <= __class_small ) {
      const chunk_t c = regions[0].tlsf.allocate(n);
      if( c.ptr != nullptr ) [[likely]]
        return c;
      return __alloc_slow(n);
    }
    const chunk_t c = regions[0].buddy.allocate(n);
    if( c.ptr != nullptr ) [[likely]]
      return c;
    return __alloc_slow(n);
  }

  __bb_slow chunk_t
  __alloc_aligned_scan(usize n, usize alignment) noexcept
  {
    chunk_t c{ nullptr, 0 };
    for( u32 i = 0; i < count && c.ptr == nullptr; ++i )
      c = regions[i].large_alloc_aligned(n, alignment);
    return c;
  }

  __bb_slow chunk_t
  __alloc_aligned_slow(usize n, usize alignment) noexcept
  {
    if( !inited && !init() )
      return { nullptr, 0 };
    chunk_t c = __alloc_aligned_scan(n, alignment);
    if constexpr( __growable ) {
      if( c.ptr == nullptr && __grow(n > alignment ? n + alignment : alignment * 2) )
        c = __alloc_aligned_scan(n, alignment);
    }
    return c;
  }

  __bb_hot chunk_t
  alloc_aligned(usize n, usize alignment) noexcept
  {
    if( n == 0 || !__bits::__is_pow2(alignment) ) [[unlikely]]
      return { nullptr, 0 };
    if( alignment <= native_alignment )
      return alloc(n);
    if( count == 0 ) [[unlikely]]
      return __alloc_aligned_slow(n, alignment);
    const chunk_t c = regions[0].buddy.allocate_aligned(n, alignment);
    if( c.ptr != nullptr ) [[likely]]
      return c;
    return __alloc_aligned_slow(n, alignment);
  }

  __bb_hot bool
  release(const __located &l, void *p) noexcept
  {
    if( l.t == tier::tlsf ) {
      l.r->small_free(l.off);
      return true;
    }
    if( l.t == tier::buddy ) {
      l.r->buddy.deallocate_order(static_cast<byte *>(p), static_cast<i32>(l.off));
      return true;
    }
    return false;
  }

  bool
  dealloc(void *p) noexcept
  {
    return release(locate(p), p);
  }

  usize
  size_of(const void *p) noexcept
  {
    return locate(p).len;
  }

  bool
  present(const void *p) noexcept
  {
    return locate(p).t != tier::none;
  }

  bool
  within(const void *p) noexcept
  {
    return region_of(p) != nullptr;
  }

  usize
  shrink_at(const __located &l, usize n) noexcept
  {
    if( l.t == tier::tlsf )
      return l.r->tlsf.shrink(l.off, n) ? l.r->tlsf.payload(l.off) : 0;
    if( l.t == tier::buddy )
      return n <= l.len ? l.len : 0;
    return 0;
  }

  usize
  grow_at(const __located &l, void *p, usize n) noexcept
  {
    if( l.t == tier::tlsf ) {
      if( n > __class_small )
        return 0;
      return l.r->tlsf.grow(l.off, n) ? l.r->tlsf.payload(l.off) : 0;
    }
    if( l.t == tier::buddy )
      return l.r->buddy.grow(static_cast<byte *>(p), n);
    return 0;
  }

  bool
  shrink_in_place(const void *p, usize n) noexcept
  {
    return shrink_at(locate(p), n) != 0;
  }

  usize
  available() noexcept
  {
    (void)__ensure();
    usize s = 0;
    for( u32 i = 0; i < count; ++i )
      s += regions[i].available();
    return s;
  }

  usize
  used() noexcept
  {
    (void)__ensure();
    usize s = 0;
    for( u32 i = 0; i < count; ++i )
      s += regions[i].used();
    return s;
  }

  usize
  largest_free() noexcept
  {
    (void)__ensure();
    usize m = 0;
    for( u32 i = 0; i < count; ++i ) {
      const usize v = regions[i].largest_free();
      if( v > m )
        m = v;
    }
    return m;
  }

  usize
  total() noexcept
  {
    (void)__ensure();
    usize s = 0;
    for( u32 i = 0; i < count; ++i )
      s += regions[i].buddy.__total();
    return s;
  }

  u32
  region_count() noexcept
  {
    (void)__ensure();
    return count;
  }
};

template <usize Id> struct __heap_store {
  static inline __heap value = {};
};

inline __heap &__the_heap = __heap_store<__abi_id>::value;

};
