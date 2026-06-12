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

#include "../../../closures.hpp"
#include "../../../concepts.hpp"
#include "../../../memory/allocation/kmemory.hpp"
#include "../../../numerics.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"

#include "../../../linux/sys/sysinfo.hpp"

#include "prediction.hpp"

#include "book.hpp"
#include "cache.hpp"
#include "config.hpp"
#include "harden.hpp"
#include "hooks.hpp"
#include "mpsc_free.hpp"
#include "oom.hpp"
#include "stats.hpp"
#include "tcache.hpp"

#include "printing.hpp"

#include "../../placement_new.hpp"      // global templated placement operator new (shared, breaks the abcmalloc<->arrays cycle)

namespace abc
{

// precomputed weights
// weight(shift) = round(2^((63 - shift) * alpha)),  alpha = 0.33
constexpr static f64 __prealloc_alpha = 0.33;

constexpr static u64
__compute_weight(u64 shift)
{
  // powerf is constexpr
  return static_cast<u64>(micron::math::powerf(2.0, (63.0 - (f64)shift) * __prealloc_alpha) + 0.5);
}

constexpr static u64 __class_weight_precise = __compute_weight(__class_precise_shift);
constexpr static u64 __class_weight_small = __compute_weight(__class_small_shift);
constexpr static u64 __class_weight_medium = __compute_weight(__class_medium_shift);
constexpr static u64 __class_weight_large = __compute_weight(__class_large_shift);
constexpr static u64 __class_weight_huge = __compute_weight(__class_huge_shift);
constexpr static u64 __class_weight_denom
    = __class_weight_precise + __class_weight_small + __class_weight_medium + __class_weight_large + __class_weight_huge;

template<u64 Class>
constexpr static u64
__weight_for_class()
{
  constexpr u64 shift = __builtin_ctzll(Class);
  return __compute_weight(shift);
}

template<u64 Class>
static inline u64
__prealloc_share(u64 total)
{
  if ( total == 0 ) [[unlikely]]
    return __default_magic_size;

  constexpr u64 w = __weight_for_class<Class>();

  if constexpr ( __wordsize == 64 ) {
    u64 frac = static_cast<u64>((static_cast<u128>(total) * w) / __class_weight_denom);
    if ( frac % __system_pagesize != 0 ) frac += __system_pagesize - (frac % __system_pagesize);
    return frac;
  } else {
    u64 scaled = total / __class_weight_denom;
    u64 rem = total % __class_weight_denom;
    u64 frac = scaled * w + (rem * w) / __class_weight_denom;
    if ( frac % __system_pagesize != 0 ) frac += __system_pagesize - (frac % __system_pagesize);
    return frac;
  }
}

template<u64 Sz>
consteval bool
tomb_for(void) noexcept
{
  if constexpr ( __default_persistent_mode )
    return true;      // persistent mode tombstones every class
  else if constexpr ( Sz <= __class_precise )
    return __tombstone_precise;
  else if constexpr ( Sz <= __class_small )
    return __tombstone_small;
  else if constexpr ( Sz <= __class_medium )
    return __tombstone_medium;
  else if constexpr ( Sz <= __class_large )
    return __tombstone_large;
  else
    return __tombstone_huge;
}

static inline __attribute__((always_inline)) usize
__page_round(usize sz)
{
  return (sz + __system_pagesize - 1) & ~(__system_pagesize - 1);
}

static inline micron::__chunk<byte>
__get_guarded_kernel_chunk(usize sz)
{
  auto chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz + __system_pagesize);
  if ( chnk.zero() or micron::mmap_failed(chnk.ptr) ) [[unlikely]] {
    __debug_print("__get_guarded_kernel_chunk()!!!: mmap failed for size: ", sz + __system_pagesize);
    micron::abort();
  }
  if ( micron::mprotect(chnk.ptr + (chnk.len - __system_pagesize), __system_pagesize, __default_guard_page_perms) != 0 ) {
    __debug_print("__get_guarded_kernel_chunk()!!!: mprotect failed for guard page", 0);
    micron::abort();
  }
  __debug_print("__get_guarded_kernel_chunk(): guard page at offset: ", chnk.len - __system_pagesize);
  return chnk;
}

// validate a kernel chunk before constructing a sheet from it
static inline bool
__kernel_chunk_valid(const micron::__chunk<byte> &chnk)
{
  return chnk.ptr != nullptr and not micron::mmap_failed(chnk.ptr) and chnk.len != 0;
}

class __arena: private cache
{
  template<typename T> struct alignas(16) node {
    using sheet_type = T;

    T *nd;
    node<T> *prev;
    node<T> *nxt;
  };

  template<typename sheet_type, u32 MaxSheets = 64, u32 CacheSlots = 0, typename Cache = __tier_tcache<CacheSlots>>
  struct alignas(64) __tier {
    using sheet_t = sheet_type;
    static constexpr u32 __max_sheets = MaxSheets;
    static constexpr u32 __no_hit = __max_sheets;
    static constexpr u32 __detail_words = (MaxSheets + 63) / 64;
    static constexpr u32 __cache_slots = Cache::__cache_slots;

    static_assert(MaxSheets > 0, "__tier MaxSheets must be > 0");

    struct __range {
      addr_t *lo;
      addr_t *hi;
      node<sheet_type> *nd;
    };

    node<sheet_type> head;
    node<sheet_type> *tail;

    // alloc-hot block
    __range __idx[__max_sheets];
    u32 __count;
    u64 __space_mask[__detail_words];      // bit pos set iff __idx[pos] has space
    u32 __last_hit;

    alignas(64) u32 __dealloc_count;

    Cache __cache;

    // multi-word bitmap helpers
    inline __attribute__((always_inline)) bool
    __mask_get(u32 pos) const noexcept
    {
      return (__space_mask[pos >> 6] >> (pos & 63)) & 1ULL;
    }

    inline __attribute__((always_inline)) void
    __mask_set(u32 pos) noexcept
    {
      __space_mask[pos >> 6] |= (1ULL << (pos & 63));
    }

    inline __attribute__((always_inline)) void
    __mask_clear(u32 pos) noexcept
    {
      __space_mask[pos >> 6] &= ~(1ULL << (pos & 63));
    }

    inline __attribute__((always_inline)) void
    __mask_insert(u32 pos, bool value) noexcept
    {
      const u32 word = pos >> 6;
      const u32 bit = pos & 63;
      const u64 w = __space_mask[word];
      const u64 carry = (w >> 63) & 1ULL;      // bit 63 carries into next-higher word

      const u64 lo_mask = (bit == 0) ? 0ULL : ((1ULL << bit) - 1);
      const u64 lo = w & lo_mask;
      const u64 hi_keep_mask = (bit == 63) ? 0ULL : ~((1ULL << (bit + 1)) - 1);      // covers [bit+1..63]
      const u64 hi = (w << 1) & hi_keep_mask;
      __space_mask[word] = lo | hi | ((value ? 1ULL : 0ULL) << bit);

      u64 c = carry;
      for ( u32 i = word + 1; i < __detail_words; ++i ) {
        const u64 nc = (__space_mask[i] >> 63) & 1ULL;
        __space_mask[i] = (__space_mask[i] << 1) | c;
        c = nc;
      }
    }

    inline __attribute__((always_inline)) void
    __mask_remove(u32 pos) noexcept
    {
      const u32 word = pos >> 6;
      const u32 bit = pos & 63;

      const u64 carry_in = (word + 1 < __detail_words) ? (__space_mask[word + 1] & 1ULL) : 0ULL;
      const u64 w = __space_mask[word];

      const u64 lo_mask = (bit == 0) ? 0ULL : ((1ULL << bit) - 1);
      const u64 lo = w & lo_mask;
      const u64 hi_keep_mask = (bit == 0) ? ((1ULL << 63) - 1) : (((1ULL << 63) - 1) & ~((1ULL << bit) - 1));
      const u64 hi = (w >> 1) & hi_keep_mask;
      __space_mask[word] = lo | hi | (carry_in << 63);

      for ( u32 i = word + 1; i < __detail_words; ++i ) {
        const u64 next_carry = (i + 1 < __detail_words) ? (__space_mask[i + 1] & 1ULL) : 0ULL;
        __space_mask[i] = (__space_mask[i] >> 1) | (next_carry << 63);
      }
    }

    void
    init(void)
    {
      head.nd = nullptr;
      head.prev = nullptr;
      head.nxt = nullptr;
      tail = nullptr;
      __count = 0;
      for ( u32 i = 0; i < __detail_words; ++i ) __space_mask[i] = 0;
      __last_hit = __no_hit;
      __dealloc_count = 0;
    }

    inline __attribute__((always_inline)) void
    link_at_tail(node<sheet_type> *nd)
    {
      nd->prev = tail;
      nd->nxt = nullptr;
      if ( tail ) tail->nxt = nd;
      tail = nd;
    }

    inline __attribute__((always_inline)) void
    unlink_node(node<sheet_type> *nd)
    {
      if ( nd->prev ) nd->prev->nxt = nd->nxt;
      if ( nd->nxt ) nd->nxt->prev = nd->prev;
      if ( nd == tail ) tail = nd->prev;
    }

    u32
    register_sheet(node<sheet_type> *nd)
    {
      if ( __count >= __max_sheets ) [[unlikely]]
        return __max_sheets;      // sentinel: tier full, caller must handle

      addr_t *lo = nd->nd->addr();
      addr_t *hi = nd->nd->addr_end();

      u32 pos = 0;
      while ( pos < __count and __idx[pos].lo < lo ) ++pos;

      // shift entries right
      for ( u32 i = __count; i > pos; --i ) __idx[i] = __idx[i - 1];

      __idx[pos] = { lo, hi, nd };

      // insert availability bit at pos, shifting bits >= pos up by one
      __mask_insert(pos, true);

      if ( __last_hit != __no_hit and pos <= __last_hit and __last_hit + 1 < __max_sheets ) ++__last_hit;

      ++__count;
      return pos;
    }

    void
    unregister(u32 pos)
    {
      if ( pos >= __count ) return;

      // shift entries left
      for ( u32 i = pos; i + 1 < __count; ++i ) __idx[i] = __idx[i + 1];

      // remove bit at pos, shift upper bits down
      __mask_remove(pos);

      if ( __last_hit != __no_hit ) {
        if ( pos == __last_hit )
          __last_hit = __no_hit;
        else if ( pos < __last_hit )
          --__last_hit;
      }

      --__count;
    }

    i32
    find_range(addr_t *addr) const
    {
      i32 lo = 0, hi = static_cast<i32>(__count) - 1;
      while ( lo <= hi ) {
        i32 mid = lo + ((hi - lo) >> 1);
        if ( addr < __idx[mid].lo )
          hi = mid - 1;
        else if ( addr >= __idx[mid].hi )
          lo = mid + 1;
        else
          return mid;
      }
      return -1;
    }

    inline __attribute__((always_inline)) void
    mark_exhausted(u32 pos)
    {
      __mask_clear(pos);
    }

    inline __attribute__((always_inline)) void
    mark_available(u32 pos)
    {
      __mask_set(pos);
    }

    bool
    has_space(void) const
    {
      for ( u32 i = 0; i < __detail_words; ++i ) {
        if ( __space_mask[i] != 0 ) return true;
      }
      return false;
    }

    inline __attribute__((always_inline)) bool
    bump_dealloc(void)
    {
      return ++__dealloc_count >= __default_tombstone_sweep_interval;
    }

    bool
    empty(void) const
    {
      return head.nd == nullptr;
    }

    bool
    zeroed(void) const
    {
      return head.nd == nullptr and head.nxt == nullptr;
    }

    template<typename Fn>
    auto
    for_each(Fn fn) const -> micron::lambda_return_t<decltype(fn)>
    {
      using Rt = micron::lambda_return_t<decltype(fn)>;
      Rt ret{};
      for ( u32 i = 0; i < __count; ++i ) {
        if ( __idx[i].nd and __idx[i].nd->nd ) ret += fn(__idx[i].nd->nd);
      }
      return ret;
    }

    template<typename Fn>
    void
    for_each_void(Fn fn) const
    {
      for ( u32 i = 0; i < __count; ++i ) {
        if ( __idx[i].nd and __idx[i].nd->nd ) fn(__idx[i].nd->nd);
      }
    }
  };

  alloc_predictor __predict;
  sheet<__class_arena_internal> _arena_memory;

  // tlsf-backed tiers; they use the linear-scan LIFO __tier_tcache
  __tier<tlsf_sheet<__class_precise>, __max_sheets_precise, __cache_slots_precise> _precise;
  __tier<tlsf_sheet<__class_small>, __max_sheets_small, __cache_slots_small> _small;

  // buddy-backed tiers
  __tier<sheet<__class_arena_internal>, __max_sheets_arena_internal> _arena_tier;      // internal metadata
  __tier<sheet<__class_medium>, __max_sheets_medium, __cache_slots_medium> _medium;
  __tier<sheet<__class_large>, __max_sheets_large, __cache_slots_large> _large;
  __tier<sheet<__class_huge>, __max_sheets_huge, __cache_slots_huge> _huge;

  // 64 slots * 64 B  = ~4 KiB per arena
  __mpsc_free_queue<64> __remote_free;

  // remote-free: the ring above is bounded and only drained when the owner thread touches its arena;
  // compute/poll-bound owner that never allocates starves it, and a freeing thread that spins on a full ring livelocks
  struct __remote_ovf_node {
    __remote_ovf_node *next;
    usize sz;      // 0 == size-unknown (route through __vmap_remove_at)
  };

  micron::atomic_token<__remote_ovf_node *> __remote_ovf{ nullptr };

  micron::atomic_flag __struct_mtx{};

  void
  __reload_arena_buf(void)
  {
    __debug_print("__reload_arena_buf(): current arena buf allocated: ", _arena_tier.tail->nd->allocated());
    __debug_print("__reload_arena_buf(): arena metadata buf remaining: ", __available_buffer());
    __expand_arena_tier(_arena_tier.tail->nd->allocated() * 2);
    __debug_print("__reload_arena_buf(): arena buf expanded, new allocated: ", _arena_tier.tail->nd->allocated());
  }

  inline __attribute__((always_inline)) micron::__chunk<byte>
  __mark_arena(usize sz)
  {
  retry_mark:
    micron::__chunk<byte> buf = _arena_tier.tail->nd->try_mark(sz);
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__mark_arena(): arena buf exhausted, triggering reload", 0);
      __reload_arena_buf();
      goto retry_mark;
    }
    return buf;
  }

  void
  __unmark_from_arena(byte *addr, usize sz)
  {
    __debug_print_addr("__unmark_from_arena(): searching for ", addr);
    i32 idx = _arena_tier.find_range(reinterpret_cast<addr_t *>(addr));
    if ( idx < 0 ) [[unlikely]] {
      __debug_print_addr("__unmark_from_arena(): WARNING address not found: ", addr);
      return;
    }
    auto &sh = *_arena_tier.__idx[idx].nd->nd;
    if ( sh.try_unmark(micron::__chunk<byte>(addr, sz)) ) {
      _arena_tier.mark_available(idx);
      __debug_print("__unmark_from_arena(): unmark succeeded, sheet used: ", sh.used());
    } else {
      __debug_print_addr("__unmark_from_arena(): WARNING try_unmark failed for: ", addr);
    }
  }

  void
  __init_arena_tier(usize n)
  {
    __debug_print("__init_arena_tier(): backing region size: ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<__class_arena_internal>));
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__init_arena_tier()!!!: no arena metadata for buddy header", 0);
      abort_state();
    }
    if constexpr ( __default_guard_arena_metadata ) {
      __debug_print("__init_arena_tier(): inserting guard page for arena tier", 0);
      auto chnk = __get_guarded_kernel_chunk(n);
      _arena_tier.head.nd = new (buf.ptr) sheet<__class_arena_internal>(this, chnk, __system_pagesize);
    } else {
      _arena_tier.head.nd = new (buf.ptr) sheet<__class_arena_internal>(this, __get_kernel_chunk<micron::__chunk<byte>>(n));
    }
    _arena_tier.head.prev = nullptr;
    _arena_tier.head.nxt = nullptr;
    _arena_tier.tail = &_arena_tier.head;
    if ( _arena_tier.head.nd->empty() ) [[unlikely]] {
      __debug_print("__init_arena_tier()!!!: kernel chunk returned empty", 0);
      abort_state();
    }
    _arena_tier.register_sheet(&_arena_tier.head);
    __debug_print("__init_arena_tier(): initialised successfully", 0);
  }

  void
  __expand_arena_tier(usize sz)
  {
    auto __g = __struct_guard();
    __debug_print("__expand_arena_tier(): requested expansion size: ", sz);
    using Nd = node<sheet<__class_arena_internal>>;
    using Sh = sheet<__class_arena_internal>;
    usize pair_sz = sizeof(Nd) + sizeof(Sh);
    micron::__chunk<byte> buf = _arena_memory.try_mark(pair_sz);
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__expand_arena_tier()!!!: arena metadata exhausted, req: ", sz);
      __debug_print("__expand_arena_tier(): arena metadata buf remaining: ", _arena_memory.available());
      abort_state();
    }
    byte *p = buf.ptr;
    auto *nd = new (p) Nd();
    p += sizeof(Nd);
    if constexpr ( __default_guard_arena_metadata ) {
      __debug_print("__expand_arena_tier(): inserting guard page for arena tier expansion", 0);
      auto chnk = __get_guarded_kernel_chunk(sz);
      nd->nd = new (p) Sh(this, chnk, __system_pagesize);
    } else {
      auto chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz);
      if ( !__kernel_chunk_valid(chnk) ) [[unlikely]] {
        __debug_print("__expand_arena_tier()!!!: mmap failed for arena tier expansion, req: ", sz);
        abort_state();
      }
      nd->nd = new (p) Sh(this, chnk);
    }
    if ( nd->nd->empty() ) [[unlikely]] {
      __debug_print("__expand_arena_tier()!!!: sheet construction failed, kernel chunk invalid", 0);
      abort_state();
    }
    nd->nxt = nullptr;
    _arena_tier.link_at_tail(nd);
    if ( _arena_tier.register_sheet(nd) == _arena_tier.__max_sheets ) [[unlikely]] {
      __debug_print("__expand_arena_tier()!!!: arena tier index full", 0);
      abort_state();
    }
    __debug_print("__expand_arena_tier(): new arena node allocated, size: ", sz);
  }

  template<u64 Sz, typename TierT>
  void
  __init_tlsf(TierT &tier, usize n)
  {
    if ( n == __default_magic_size ) n = __calculate_space_small(Sz);
    n = __page_round(n);
    __debug_print("__init_tlsf(): class size: ", Sz);
    __debug_print("__init_tlsf(): backing region size: ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(tlsf_sheet<Sz>));
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__init_tlsf()!!!: no arena metadata for tlsf header, class: ", Sz);
      abort_state();
    }
    tier.head.nd = new (buf.ptr) tlsf_sheet<Sz>(this, __get_kernel_chunk<micron::__chunk<byte>>(n));
    tier.head.prev = nullptr;
    tier.head.nxt = nullptr;
    tier.tail = &tier.head;
    if ( tier.head.nd->empty() ) [[unlikely]] {
      __debug_print("__init_tlsf()!!!: kernel chunk returned empty for class: ", Sz);
      abort_state();
    }
    tier.register_sheet(&tier.head);
    __debug_print("__init_tlsf(): initialised successfully for class: ", Sz);
  }

  template<u64 Sz, typename TierT>
  inline __attribute__((always_inline)) bool
  __expand_tlsf(TierT &tier, usize sz)
  {
    auto __g = __struct_guard();
    __debug_print("__expand_tlsf(): class size: ", Sz);
    __debug_print("__expand_tlsf(): requested backing region: ", sz);
    using Nd = node<tlsf_sheet<Sz>>;
    using Sh = tlsf_sheet<Sz>;
    usize pair_sz = sizeof(Nd) + sizeof(Sh);
    micron::__chunk<byte> buf = __mark_arena(pair_sz);
    byte *p = buf.ptr;
    usize aligned_sz = __page_round(sz);
    auto chnk = __get_kernel_chunk<micron::__chunk<byte>>(aligned_sz);
    if ( !__kernel_chunk_valid(chnk) ) [[unlikely]] {
      __debug_print("__expand_tlsf(): mmap failed for tlsf expansion, class: ", Sz);
      __debug_print("__expand_tlsf(): requested size: ", aligned_sz);
      __unmark_from_arena(buf.ptr, pair_sz);
      return false;
    }
    auto *nd = new (p) Nd();
    p += sizeof(Nd);
    nd->nd = new (p) Sh(this, chnk);
    if ( nd->nd->empty() ) [[unlikely]] {
      __debug_print("__expand_tlsf(): sheet construction failed, class: ", Sz);
      nd->nd->release();
      __unmark_from_arena(buf.ptr, pair_sz);
      return false;
    }
    nd->nxt = nullptr;
    tier.link_at_tail(nd);
    u32 pos = tier.register_sheet(nd);
    if ( pos == tier.__max_sheets ) [[unlikely]] {
      __debug_print("__expand_tlsf(): tier full (__max_sheets reached), class: ", Sz);
      tier.unlink_node(nd);
      nd->nd->release();
      __unmark_from_arena(buf.ptr, pair_sz);
      return false;
    }
    __debug_print("__expand_tlsf(): new tlsf node ready, backing size: ", aligned_sz);
    return true;
  }

  template<u64 Sz, typename TierT>
  void
  __init_buddy(TierT &tier, usize n = __default_magic_size)
  {
    if ( n == __default_magic_size ) {
      // pick the tier's own curve when the caller can't supply a sysinfo-derived share
      if constexpr ( Sz == __class_medium )
        n = __calculate_space_medium(Sz);
      else if constexpr ( Sz == __class_large )
        n = __calculate_space_large(Sz);
      else if constexpr ( Sz == __class_huge )
        n = __calculate_space_huge(Sz);
      else
        n = __calculate_space_small(Sz);
    }
    __debug_print("__init_buddy(): class size: ", Sz);
    __debug_print("__init_buddy(): backing region size: ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__init_buddy()!!!: no arena metadata for buddy header, class: ", Sz);
      abort_state();
    }
    tier.head.nd = new (buf.ptr) sheet<Sz>(this, __get_kernel_chunk<micron::__chunk<byte>>(n));
    tier.head.prev = nullptr;
    tier.head.nxt = nullptr;
    tier.tail = &tier.head;
    if ( tier.head.nd->empty() ) [[unlikely]] {
      __debug_print("__init_buddy()!!!: kernel chunk returned empty for class: ", Sz);
      abort_state();
    }
    tier.register_sheet(&tier.head);
    __debug_print("__init_buddy(): initialised successfully for class: ", Sz);
  }

  template<u64 Sz, typename TierT>
  inline __attribute__((always_inline)) bool
  __expand_buddy(TierT &tier, usize sz)
  {
    auto __g = __struct_guard();
    __debug_print("__expand_buddy(): class size: ", Sz);
    __debug_print("__expand_buddy(): requested expansion size: ", sz);
    using Nd = node<sheet<Sz>>;
    using Sh = sheet<Sz>;
    usize pair_sz = sizeof(Nd) + sizeof(Sh);
    micron::__chunk<byte> buf = __mark_arena(pair_sz);
    byte *p = buf.ptr;

    micron::__chunk<byte> chnk;
    if constexpr ( __default_insert_guard_pages ) {
      __debug_print("__expand_buddy(): inserting guard page for class: ", Sz);
      chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz + __system_pagesize);
      if ( !__kernel_chunk_valid(chnk) ) [[unlikely]] {
        __debug_print("__expand_buddy(): mmap failed for buddy expansion, class: ", Sz);
        __unmark_from_arena(buf.ptr, pair_sz);
        return false;
      }
      __make_guard(chnk);
    } else {
      chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz);
      if ( !__kernel_chunk_valid(chnk) ) [[unlikely]] {
        __debug_print("__expand_buddy(): mmap failed for buddy expansion, class: ", Sz);
        __unmark_from_arena(buf.ptr, pair_sz);
        return false;
      }
    }

    auto *nd = new (p) Nd();
    p += sizeof(Nd);
    if constexpr ( __default_insert_guard_pages ) {
      nd->nd = new (p) Sh(this, chnk, __system_pagesize);
    } else {
      nd->nd = new (p) Sh(this, chnk);
    }
    if ( nd->nd->empty() ) [[unlikely]] {
      __debug_print("__expand_buddy(): sheet construction failed (empty), class: ", Sz);
      nd->nd->release();
      __unmark_from_arena(buf.ptr, pair_sz);
      return false;
    }
    nd->nxt = nullptr;
    tier.link_at_tail(nd);
    u32 pos = tier.register_sheet(nd);
    if ( pos == tier.__max_sheets ) [[unlikely]] {
      __debug_print("__expand_buddy(): tier full (__max_sheets reached), class: ", Sz);
      tier.unlink_node(nd);
      nd->nd->release();
      __unmark_from_arena(buf.ptr, pair_sz);
      return false;
    }
    __debug_print("__expand_buddy(): new buddy node ready for class: ", Sz);
    return true;
  }

  inline __attribute__((always_inline)) void
  __make_guard(micron::__chunk<byte> &mem)
  {
    __debug_print_addr("__make_guard() @", mem.ptr + (mem.len - __system_pagesize));
    if ( long int r = micron::mprotect(mem.ptr + (mem.len - __system_pagesize), __system_pagesize, __default_guard_page_perms); r != 0 ) {
      __debug_print("__make_guard()!!!: mprotect failed, errno: ", r);
      abort_state();
    }
    __debug_print("__make_guard(): guard page installed at offset: ", mem.len - __system_pagesize);
  }

  bool
  __buf_expand_exact(const usize class_sz, const usize exact_sz)
  {
    __debug_print("__buf_expand_exact(): routing class_sz: ", class_sz);
    __debug_print("__buf_expand_exact(): target expansion exact_sz: ", exact_sz);

    if ( class_sz <= __class_small ) {
      __debug_print("__buf_expand_exact(): routed to precise/tlsf tier", 0);
      return __expand_tlsf<__class_precise>(_precise, exact_sz);
    }

    if ( class_sz < __class_medium ) [[likely]] {
      __debug_print("__buf_expand_exact(): routed to small/tlsf tier", 0);
      if constexpr ( __default_lazy_construct and !__default_eager_hot_tiers ) {
        if ( _small.empty() ) [[unlikely]] {
          __debug_print("__buf_expand_exact(): lazy-constructing small bucket", 0);
          __init_tlsf<__class_small>(_small, __calculate_space_small(__class_small));
          return true;      // __init aborts on failure, reaching here means success
        } else {
          return __expand_tlsf<__class_small>(_small, exact_sz);
        }
      } else {
        return __expand_tlsf<__class_small>(_small, exact_sz);
      }
    }

    if ( class_sz <= __class_large ) {
      __debug_print("__buf_expand_exact(): routed to medium/buddy tier", 0);
      if constexpr ( __default_lazy_construct and !__default_eager_hot_tiers ) {
        if ( _medium.empty() ) [[unlikely]] {
          __debug_print("__buf_expand_exact(): lazy-constructing medium bucket", 0);
          __init_buddy<__class_medium>(_medium);
          return true;
        } else {
          return __expand_buddy<__class_medium>(_medium, exact_sz);
        }
      } else {
        return __expand_buddy<__class_medium>(_medium, exact_sz);
      }
    }

    if ( class_sz <= __class_huge ) {
      __debug_print("__buf_expand_exact(): routed to large/buddy tier", 0);
      if ( _large.empty() ) [[unlikely]] {
        __debug_print("__buf_expand_exact(): lazy-constructing large bucket", 0);
        __init_buddy<__class_large>(_large, exact_sz);
        return true;
      } else {
        return __expand_buddy<__class_large>(_large, exact_sz);
      }
    }

    __debug_print("__buf_expand_exact(): routed to huge/buddy tier", 0);
    if ( _huge.empty() ) [[unlikely]] {
      __debug_print("__buf_expand_exact(): lazy-constructing huge bucket", 0);
      __init_buddy<__class_huge>(_huge, exact_sz);
      return true;
    } else {
      return __expand_buddy<__class_huge>(_huge, exact_sz);
    }
  }

  template<typename TierT>
  inline __attribute__((always_inline)) micron::__chunk<byte>
  __bucket_insert(TierT &tier, const usize sz)
  {
    // mru cache opt
    u32 lh = tier.__last_hit;
    if ( lh < tier.__count and tier.__mask_get(lh) ) {
      auto &sh = *tier.__idx[lh].nd->nd;
      micron::__chunk<byte> mem;
      if constexpr ( __default_launder ) {
        mem = sh.temporal_mark(sz);
      } else {
        mem = sh.mark(sz);
      }
      if ( !mem.zero() ) return mem;
      tier.mark_exhausted(lh);
    }

    // bitmap scan fallback: walk each detail word, pop bits via ctz
    for ( u32 w = 0; w < TierT::__detail_words; ++w ) {
      u64 mask = tier.__space_mask[w];
      while ( mask ) {
        const u32 bit = __builtin_ctzll(mask);
        const u32 pos = (w << 6) | bit;
        if ( pos >= tier.__count ) break;      // shouldn't trip
        auto &sh = *tier.__idx[pos].nd->nd;
        micron::__chunk<byte> mem;
        if constexpr ( __default_launder ) {
          mem = sh.temporal_mark(sz);
        } else {
          mem = sh.mark(sz);
        }
        if ( !mem.zero() ) {
          tier.__last_hit = pos;
          return mem;
        }
        // sheet exhausted for this size, clear bit
        tier.mark_exhausted(pos);
        mask &= mask - 1;      // clear lowest set bit in the working copy
      }
    }
    return { nullptr, 0 };
  }

  template<typename TierT>
  inline __attribute__((always_inline)) micron::__chunk<byte>
  __bucket_insert_temporal(TierT &tier, const usize sz)
  {
    // mru cache opt
    u32 lh = tier.__last_hit;
    if ( lh < tier.__count and tier.__mask_get(lh) ) {
      micron::__chunk<byte> mem = tier.__idx[lh].nd->nd->temporal_mark(sz);
      if ( !mem.zero() ) return mem;
      tier.mark_exhausted(lh);
    }

    for ( u32 w = 0; w < TierT::__detail_words; ++w ) {
      u64 mask = tier.__space_mask[w];
      while ( mask ) {
        const u32 bit = __builtin_ctzll(mask);
        const u32 pos = (w << 6) | bit;
        if ( pos >= tier.__count ) break;
        auto &sh = *tier.__idx[pos].nd->nd;
        micron::__chunk<byte> mem = sh.temporal_mark(sz);
        if ( !mem.zero() ) {
          tier.__last_hit = pos;
          return mem;
        }
        tier.mark_exhausted(pos);
        mask &= mask - 1;
      }
    }
    return { nullptr, 0 };
  }

  // inflate allocation size for redzones; only if inflated stays in TLSF territory
  static inline __attribute__((always_inline)) usize
  __rz_inflate(usize sz)
  {
    if constexpr ( __default_redzone ) {
      usize inflated = sz + 2 * __default_redzone_size;
      if ( inflated < __class_medium ) return inflated;
    }
    return sz;
  }

  // apply redzones to a freshly-allocated TLSF chunk, shift pointer to user region
  static inline __attribute__((always_inline)) void
  __rz_apply(micron::__chunk<byte> &memory, usize user_sz)
  {
    write_redzone(memory.ptr + __default_redzone_size, user_sz);
    memory.ptr += __default_redzone_size;
    memory.len = user_sz;
  }

  template<typename TierT>
  inline __attribute__((always_inline)) micron::__chunk<byte>
  __cache_pop_or_insert(TierT &tier, const usize sz)
  {
    // if caching is disabled behavior is identical to without it, comped out
    if constexpr ( __default_per_class_free_cache && TierT::__cache_slots > 0 && !__default_launder ) {
      i32 hit;
      if constexpr ( __default_redzone ) {
        hit = tier.__cache.probe(static_cast<u32>(sz));
      } else {
        hit = tier.__cache.probe_ge(static_cast<u32>(sz));
      }
      if ( hit >= 0 ) [[likely]] {
        __tcache_chunk c = tier.__cache.pop_at(static_cast<u32>(hit));
        return { c.ptr, static_cast<usize>(c.size) };
      }
    }
    return __bucket_insert(tier, sz);
  }

  hot_fn(micron::__chunk<byte>) __vmap_alloc(const usize sz)
  {
    if ( sz <= __class_small ) {
      __debug_print("__vmap_alloc(): tier=precise, sz: ", sz);
      return __cache_pop_or_insert(_precise, sz);
    }
    if ( sz < __class_medium ) {
      __debug_print("__vmap_alloc(): tier=small, sz: ", sz);
      return __cache_pop_or_insert(_small, sz);
    }
    if ( sz <= __class_large ) {
      __debug_print("__vmap_alloc(): tier=medium, sz: ", sz);
      return __cache_pop_or_insert(_medium, sz);
    }
    if ( sz <= __class_huge ) {
      __debug_print("__vmap_alloc(): tier=large, sz: ", sz);
      return __cache_pop_or_insert(_large, sz);
    }
    __debug_print("__vmap_alloc(): tier=huge, sz: ", sz);
    return __cache_pop_or_insert(_huge, sz);
  }

  micron::__chunk<byte>
  __vmap_launder(const usize sz)
  {
    if ( sz < __class_medium ) return __bucket_insert_temporal(_small, sz);
    if ( sz <= __class_large ) return __bucket_insert_temporal(_medium, sz);
    if ( sz <= __class_huge ) return __bucket_insert_temporal(_large, sz);
    return __bucket_insert_temporal(_huge, sz);
  }

  template<typename Fn>
  inline __attribute__((always_inline)) bool
  __dispatch_addr(addr_t *addr, Fn &&fn)
  {
    i32 idx;
    if ( (idx = _precise.find_range(addr)) >= 0 ) return fn(_precise, idx);
    if ( (idx = _small.find_range(addr)) >= 0 ) return fn(_small, idx);
    if ( (idx = _medium.find_range(addr)) >= 0 ) return fn(_medium, idx);
    if ( (idx = _large.find_range(addr)) >= 0 ) return fn(_large, idx);
    if ( (idx = _huge.find_range(addr)) >= 0 ) return fn(_huge, idx);
    return false;
  }

  template<typename Fn>
  inline __attribute__((always_inline)) bool
  __dispatch_addr(addr_t *addr, Fn &&fn) const
  {
    i32 idx;
    if ( (idx = _precise.find_range(addr)) >= 0 ) return fn(_precise, idx);
    if ( (idx = _small.find_range(addr)) >= 0 ) return fn(_small, idx);
    if ( (idx = _medium.find_range(addr)) >= 0 ) return fn(_medium, idx);
    if ( (idx = _large.find_range(addr)) >= 0 ) return fn(_large, idx);
    if ( (idx = _huge.find_range(addr)) >= 0 ) return fn(_huge, idx);
    return false;
  }

  template<typename TierT>
  void
  __sweep_tier_tombstones(TierT &tier)
  {
    using sheet_t = typename TierT::sheet_t;
    auto __g = __struct_guard();
    __debug_print("__sweep_tier_tombstones(): sweeping tier, sheet count: ", tier.__count);
    tier.__dealloc_count = 0;
    for ( i32 i = static_cast<i32>(tier.__count) - 1; i >= 0; --i ) {
      auto *nd = tier.__idx[i].nd;
      if ( !nd or !nd->nd or nd == &tier.head ) continue;
      auto &sh = *nd->nd;
      if ( sh.used() != 0 ) continue;
      usize ts = sh.tombstoned();
      usize ft = sh.ftotal();
      __debug_print("__sweep_tier_tombstones(): sheet tombstoned: ", ts);
      __debug_print("__sweep_tier_tombstones(): sheet ftotal: ", ft);
      if ( ts > (ft >> 1) ) {
        if constexpr ( !__default_persistent_mode ) {
          __debug_print("__sweep_tier_tombstones(): reclaiming sheet at idx: ", (usize)i);
          sh.reset();
          tier.unlink_node(nd);
          tier.unregister(static_cast<u32>(i));
          __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<sheet_t>) + sizeof(sheet_t));
        }
      }
    }
  }

  template<typename TierT>
  inline __attribute__((always_inline)) void
  __try_reclaim_empty(TierT &tier, i32 range_idx, node<typename TierT::sheet_t> *nd)
  {
    using sheet_t = typename TierT::sheet_t;
    if ( nd->nd->used() == 0 and nd != &tier.head ) {
      if constexpr ( !__default_persistent_mode ) {
        auto __g = __struct_guard();
        __debug_print("__try_reclaim_empty(): sheet fully drained, unlinking and resetting", 0);
        nd->nd->reset();
        tier.unlink_node(nd);
        tier.unregister(range_idx);
        __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<sheet_t>) + sizeof(sheet_t));
      }
    }
  }

  template<typename TierT>
  inline __attribute__((always_inline)) void
  __tombstone_accounting(TierT &tier, i32 range_idx, node<typename TierT::sheet_t> *nd)
  {
    if constexpr ( __default_tombstone_sweep_interval == 0 ) {
      auto &sh = *nd->nd;
      usize ts = sh.tombstoned();
      usize ft = sh.ftotal();
      __debug_print("__tombstone_accounting(): tombstoned: ", ts);
      __debug_print("__tombstone_accounting(): ftotal: ", ft);
      if ( (ts > (ft >> 1)) and sh.used() == 0 and nd != &tier.head ) {
        if constexpr ( !__default_persistent_mode ) {
          __debug_print("__tombstone_accounting(): threshold crossed, compacting sheet", 0);
          sh.reset();
          tier.unlink_node(nd);
          tier.unregister(range_idx);
          __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<typename TierT::sheet_t>) + sizeof(typename TierT::sheet_t));
        }
      }
    } else {
      // immediate reclaim: if this specific sheet is fully drained (used == 0)
      // AND tombstoned bytes occupy >= 50% of the sheet's total capacity,
      // reclaim now rather than waiting for the batch sweep
      // prevents sheet accumulation under monotonically-growing size patterns while preserving
      // mostly-pristine sheets that still have reusable free space
      if constexpr ( !__default_persistent_mode ) {
        if ( nd != &tier.head and nd->nd->used() == 0 ) {
          usize ts = nd->nd->tombstoned();
          usize ft = nd->nd->ftotal();
          if ( ts > (ft >> 1) ) {
            __debug_print("__tombstone_accounting(): drained + ratio met, immediate reclaim", 0);
            __debug_print("__tombstone_accounting(): tombstoned: ", ts);
            __debug_print("__tombstone_accounting(): ftotal: ", ft);
            nd->nd->reset();
            tier.unlink_node(nd);
            tier.unregister(range_idx);
            __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<typename TierT::sheet_t>) + sizeof(typename TierT::sheet_t));
            return;
          }
        }
      }
      if ( tier.bump_dealloc() ) __sweep_tier_tombstones(tier);
    }
  }

  // unified __tier_remove
  template<bool HasSize, bool ForceTombstone, typename TierT>
  inline bool
  __tier_remove_impl(TierT &tier, i32 range_idx, byte *addr, const micron::__chunk<byte> &memory)
  {
    auto *nd = tier.__idx[range_idx].nd;
    auto &sh = *nd->nd;
    __debug_print_addr("__tier_remove_impl(): found in sheet at addr: ", addr);

    if constexpr ( ForceTombstone ) {
      if constexpr ( HasSize )
        sh.try_tombstone(memory);
      else
        sh.try_tombstone_no_size(addr);
      tier.mark_available(range_idx);
      __debug_print("__tier_remove_impl(): force tombstone set", 0);
      __tombstone_accounting(tier, range_idx, nd);
      return true;
    } else {
      // hot tiers default to unmark+reclaim
      // cold tiers default to tombstoning
      using sheet_t = typename TierT::sheet_t;
      constexpr bool tomb = tomb_for<sheet_t::__size_class>();
      bool ok;
      if constexpr ( !tomb ) {
        if constexpr ( HasSize )
          ok = sh.try_unmark(memory);
        else
          ok = sh.try_unmark_no_size(addr);
        if ( ok ) {
          tier.mark_available(range_idx);
          __debug_print("__tier_remove_impl(): unmark succeeded, sheet used: ", sh.used());
          __try_reclaim_empty(tier, range_idx, nd);
          return true;
        }
      } else {
        if constexpr ( HasSize )
          ok = sh.try_tombstone(memory);
        else
          ok = sh.try_tombstone_no_size(addr);
        if ( ok ) {
          tier.mark_available(range_idx);
          __debug_print("__tier_remove_impl(): tombstone set", 0);
          __tombstone_accounting(tier, range_idx, nd);
          return true;
        }
      }
      __debug_print_addr("__tier_remove_impl(): try failed (possible double free): ", addr);
      return handle_double_free(addr);
    }
  }

  template<typename TierT>
  inline bool
  __tier_remove(TierT &tier, i32 range_idx, const micron::__chunk<byte> &memory)
  {
    return __tier_remove_impl<true, false>(tier, range_idx, memory.ptr, memory);
  }

  template<typename TierT>
  inline bool
  __tier_remove_at(TierT &tier, i32 range_idx, byte *addr)
  {
    [[maybe_unused]] auto &sh = *tier.__idx[range_idx].nd->nd;
    // NOTE: block-validity guard on EVERY tier
    if constexpr ( !__default_redzone ) {
      if ( !sh.is_block_allocated(addr) ) [[unlikely]]
        return handle_double_free(addr);
    }
    // sizeless cache-push fast path
    if constexpr ( __default_per_class_free_cache && TierT::__cache_slots > 0 && !__default_launder && !__default_redzone ) {
      // already-cached double-free guard (block-validity handled above)
      if ( tier.__cache.contains(addr) ) [[unlikely]]
        return handle_double_free(addr);
      if ( !sh.is_temporal_block(addr) ) {
        const usize bsz = sh.block_size_of(addr);
        if ( bsz > __hdr_offset ) {
          if ( tier.__cache.push(addr, static_cast<u32>(bsz - __hdr_offset)) ) [[likely]]
            return true;
        }
      }
    }
    return __tier_remove_impl<false, false>(tier, range_idx, addr, {});
  }

  template<typename TierT>
  inline bool
  __tier_tombstone(TierT &tier, i32 range_idx, const micron::__chunk<byte> &memory)
  {
    return __tier_remove_impl<true, true>(tier, range_idx, memory.ptr, memory);
  }

  template<typename TierT>
  inline bool
  __tier_tombstone_at(TierT &tier, i32 range_idx, byte *addr)
  {
    return __tier_remove_impl<false, true>(tier, range_idx, addr, {});
  }

  template<typename TierT>
  inline __attribute__((always_inline)) bool
  __cache_push_or_remove(TierT &tier, i32 range_idx, const micron::__chunk<byte> &chunk)
  {
    if constexpr ( __default_per_class_free_cache && TierT::__cache_slots > 0 && !__default_launder ) {
      // double / bogus free guard
      auto &sh = *tier.__idx[range_idx].nd->nd;
      if ( !sh.is_block_allocated(chunk.ptr) || tier.__cache.contains(chunk.ptr) ) [[unlikely]]
        return handle_double_free(chunk.ptr);
      if ( tier.__cache.push(chunk.ptr, static_cast<u32>(chunk.len)) ) [[likely]]
        return true;
    }
    return __tier_remove(tier, range_idx, chunk);
  }

  bool
  __vmap_remove(const micron::__chunk<byte> &m)
  {
    if constexpr ( __default_redzone ) {
      i32 idx;
      if ( (idx = _precise.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) {
        if ( !verify_redzone(m.ptr, m.len) ) [[unlikely]] {
          __debug_print_addr("__vmap_remove(): redzone corruption detected at: ", m.ptr);
          return fail_state();
        }
        micron::__chunk<byte> adj = { m.ptr - __default_redzone_size, m.len + 2 * __default_redzone_size };
        return __cache_push_or_remove(_precise, idx, adj);
      }
      if ( (idx = _small.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) {
        if ( !verify_redzone(m.ptr, m.len) ) [[unlikely]] {
          __debug_print_addr("__vmap_remove(): redzone corruption detected at: ", m.ptr);
          return fail_state();
        }
        micron::__chunk<byte> adj = { m.ptr - __default_redzone_size, m.len + 2 * __default_redzone_size };
        return __cache_push_or_remove(_small, idx, adj);
      }
      if ( (idx = _medium.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __cache_push_or_remove(_medium, idx, m);
      if ( (idx = _large.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __cache_push_or_remove(_large, idx, m);
      if ( (idx = _huge.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __cache_push_or_remove(_huge, idx, m);
      __debug_print_addr("__vmap_remove(): WARNING address not found in any tier: ", m.ptr);
      return false;
    }
    addr_t *p = reinterpret_cast<addr_t *>(m.ptr);
    i32 idx;
    if ( (idx = _precise.find_range(p)) >= 0 ) [[likely]]
      return __cache_push_or_remove(_precise, idx, m);
    if ( (idx = _small.find_range(p)) >= 0 ) return __cache_push_or_remove(_small, idx, m);
    if ( (idx = _medium.find_range(p)) >= 0 ) return __cache_push_or_remove(_medium, idx, m);
    if ( (idx = _large.find_range(p)) >= 0 ) return __cache_push_or_remove(_large, idx, m);
    if ( (idx = _huge.find_range(p)) >= 0 ) return __cache_push_or_remove(_huge, idx, m);
    __debug_print_addr("__vmap_remove(): WARNING address not found in any tier: ", m.ptr);
    return false;
  }

  bool
  __vmap_remove_at(byte *addr)
  {
    if constexpr ( __default_redzone ) {
      i32 idx;
      if ( (idx = _precise.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) {
        if ( !verify_redzone_leading(addr) ) [[unlikely]] {
          __debug_print_addr("__vmap_remove_at(): leading redzone corrupted at: ", addr);
          return fail_state();
        }
        return __tier_remove_at(_precise, idx, addr - __default_redzone_size);
      }
      if ( (idx = _small.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) {
        if ( !verify_redzone_leading(addr) ) [[unlikely]] {
          __debug_print_addr("__vmap_remove_at(): leading redzone corrupted at: ", addr);
          return fail_state();
        }
        return __tier_remove_at(_small, idx, addr - __default_redzone_size);
      }
      if ( (idx = _medium.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) return __tier_remove_at(_medium, idx, addr);
      if ( (idx = _large.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) return __tier_remove_at(_large, idx, addr);
      if ( (idx = _huge.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) return __tier_remove_at(_huge, idx, addr);
      __debug_print_addr("__vmap_remove_at(): WARNING address not found in any tier: ", addr);
      return false;
    }
    addr_t *p = reinterpret_cast<addr_t *>(addr);
    i32 idx;
    if ( (idx = _precise.find_range(p)) >= 0 ) [[likely]]
      return __tier_remove_at(_precise, idx, addr);
    if ( (idx = _small.find_range(p)) >= 0 ) return __tier_remove_at(_small, idx, addr);
    if ( (idx = _medium.find_range(p)) >= 0 ) return __tier_remove_at(_medium, idx, addr);
    if ( (idx = _large.find_range(p)) >= 0 ) return __tier_remove_at(_large, idx, addr);
    if ( (idx = _huge.find_range(p)) >= 0 ) return __tier_remove_at(_huge, idx, addr);
    __debug_print_addr("__vmap_remove_at(): WARNING address not found in any tier: ", addr);
    return false;
  }

  bool
  __vmap_tombstone(const micron::__chunk<byte> &m)
  {
    // NOTE: always tombstones regardless of __default_tombstone
    if constexpr ( __default_redzone ) {
      i32 idx;
      if ( (idx = _precise.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) {
        if ( !verify_redzone(m.ptr, m.len) ) [[unlikely]] {
          __debug_print_addr("__vmap_tombstone(): redzone corruption detected at: ", m.ptr);
          return fail_state();
        }
        micron::__chunk<byte> adj = { m.ptr - __default_redzone_size, m.len + 2 * __default_redzone_size };
        return __tier_tombstone(_precise, idx, adj);
      }
      if ( (idx = _small.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) {
        if ( !verify_redzone(m.ptr, m.len) ) [[unlikely]] {
          __debug_print_addr("__vmap_tombstone(): redzone corruption detected at: ", m.ptr);
          return fail_state();
        }
        micron::__chunk<byte> adj = { m.ptr - __default_redzone_size, m.len + 2 * __default_redzone_size };
        return __tier_tombstone(_small, idx, adj);
      }
      if ( (idx = _medium.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __tier_tombstone(_medium, idx, m);
      if ( (idx = _large.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __tier_tombstone(_large, idx, m);
      if ( (idx = _huge.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __tier_tombstone(_huge, idx, m);
      __debug_print_addr("__vmap_tombstone(): WARNING address not found in any tier: ", m.ptr);
      return false;
    }
    addr_t *p = reinterpret_cast<addr_t *>(m.ptr);
    i32 idx;
    if ( (idx = _precise.find_range(p)) >= 0 ) [[likely]]
      return __tier_tombstone(_precise, idx, m);
    if ( (idx = _small.find_range(p)) >= 0 ) return __tier_tombstone(_small, idx, m);
    if ( (idx = _medium.find_range(p)) >= 0 ) return __tier_tombstone(_medium, idx, m);
    if ( (idx = _large.find_range(p)) >= 0 ) return __tier_tombstone(_large, idx, m);
    if ( (idx = _huge.find_range(p)) >= 0 ) return __tier_tombstone(_huge, idx, m);
    __debug_print_addr("__vmap_tombstone(): WARNING address not found in any tier: ", m.ptr);
    return false;
  }

  bool
  __vmap_tombstone_at(byte *addr)
  {
    // NOTE: always tombstones regardless of __default_tombstone
    if constexpr ( __default_redzone ) {
      i32 idx;
      if ( (idx = _precise.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) {
        if ( !verify_redzone_leading(addr) ) [[unlikely]] {
          __debug_print_addr("__vmap_tombstone_at(): leading redzone corrupted at: ", addr);
          return fail_state();
        }
        return __tier_tombstone_at(_precise, idx, addr - __default_redzone_size);
      }
      if ( (idx = _small.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) {
        if ( !verify_redzone_leading(addr) ) [[unlikely]] {
          __debug_print_addr("__vmap_tombstone_at(): leading redzone corrupted at: ", addr);
          return fail_state();
        }
        return __tier_tombstone_at(_small, idx, addr - __default_redzone_size);
      }
      if ( (idx = _medium.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) return __tier_tombstone_at(_medium, idx, addr);
      if ( (idx = _large.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) return __tier_tombstone_at(_large, idx, addr);
      if ( (idx = _huge.find_range(reinterpret_cast<addr_t *>(addr))) >= 0 ) return __tier_tombstone_at(_huge, idx, addr);
      __debug_print_addr("__vmap_tombstone_at(): WARNING address not found in any tier: ", addr);
      return false;
    }
    bool ok = __dispatch_addr(reinterpret_cast<addr_t *>(addr), [&](auto &tier, i32 idx) { return __tier_tombstone_at(tier, idx, addr); });
    if ( !ok ) [[unlikely]]
      __debug_print_addr("__vmap_tombstone_at(): WARNING address not found in any tier: ", addr);
    return ok;
  }

  bool
  __vmap_within(addr_t *addr) const
  {
    return __dispatch_addr(addr, [](const auto &, i32) { return true; });
  }

  bool
  __vmap_valid_block(addr_t *addr) const
  {
    // proper validity check
    if constexpr ( __default_redzone ) {
      i32 idx;
      if ( (idx = _precise.find_range(addr)) >= 0 )
        return _precise.__idx[idx].nd->nd->is_block_allocated(reinterpret_cast<byte *>(addr) - __default_redzone_size);
      if ( (idx = _small.find_range(addr)) >= 0 )
        return _small.__idx[idx].nd->nd->is_block_allocated(reinterpret_cast<byte *>(addr) - __default_redzone_size);
      if ( (idx = _medium.find_range(addr)) >= 0 ) return _medium.__idx[idx].nd->nd->is_block_allocated(reinterpret_cast<byte *>(addr));
      if ( (idx = _large.find_range(addr)) >= 0 ) return _large.__idx[idx].nd->nd->is_block_allocated(reinterpret_cast<byte *>(addr));
      if ( (idx = _huge.find_range(addr)) >= 0 ) return _huge.__idx[idx].nd->nd->is_block_allocated(reinterpret_cast<byte *>(addr));
      return false;
    }
    return __dispatch_addr(
        addr, [&](const auto &tier, i32 idx) { return tier.__idx[idx].nd->nd->is_block_allocated(reinterpret_cast<byte *>(addr)); });
  }

  bool
  __vmap_locate_at(addr_t *addr) const
  {
    if constexpr ( __default_redzone ) {
      i32 idx;
      // NOTE: for TLSF adjust pointer before calling find
      if ( (idx = _precise.find_range(addr)) >= 0 )
        return _precise.__idx[idx].nd->nd->find(reinterpret_cast<byte *>(addr) - __default_redzone_size);
      if ( (idx = _small.find_range(addr)) >= 0 )
        return _small.__idx[idx].nd->nd->find(reinterpret_cast<byte *>(addr) - __default_redzone_size);
      if ( (idx = _medium.find_range(addr)) >= 0 ) return _medium.__idx[idx].nd->nd->find(reinterpret_cast<byte *>(addr));
      if ( (idx = _large.find_range(addr)) >= 0 ) return _large.__idx[idx].nd->nd->find(reinterpret_cast<byte *>(addr));
      if ( (idx = _huge.find_range(addr)) >= 0 ) return _huge.__idx[idx].nd->nd->find(reinterpret_cast<byte *>(addr));
      return false;
    }
    return __dispatch_addr(addr, [&](const auto &tier, i32 idx) { return tier.__idx[idx].nd->nd->find(reinterpret_cast<byte *>(addr)); });
  }

  bool
  __vmap_freeze(const micron::__chunk<byte> &memory)
  {
    auto __g = __struct_guard();
    return __dispatch_addr(reinterpret_cast<addr_t *>(memory.ptr), [&](auto &tier, i32 idx) {
      __debug_print_addr("__vmap_freeze(): freezing sheet containing addr: ", memory.ptr);
      // drop any cached blocks belonging to this sheet
      tier.__cache.invalidate_range(reinterpret_cast<const byte *>(tier.__idx[idx].lo), reinterpret_cast<const byte *>(tier.__idx[idx].hi));
      bool ok = tier.__idx[idx].nd->nd->freeze();
      __debug_print("__vmap_freeze(): freeze result: ", (usize)ok);
      return ok;
    });
  }

  bool
  __vmap_freeze_at(byte *addr)
  {
    auto __g = __struct_guard();
    return __dispatch_addr(reinterpret_cast<addr_t *>(addr), [&](auto &tier, i32 idx) {
      __debug_print_addr("__vmap_freeze_at(): freezing sheet containing: ", addr);
      tier.__cache.invalidate_range(reinterpret_cast<const byte *>(tier.__idx[idx].lo), reinterpret_cast<const byte *>(tier.__idx[idx].hi));
      bool ok = tier.__idx[idx].nd->nd->freeze();
      __debug_print("__vmap_freeze_at(): freeze result: ", (usize)ok);
      return ok;
    });
  }

  template<typename TierT>
  void
  __release_tier(TierT &tier)
  {
    auto *nd = &tier.head;
    while ( nd != nullptr ) {
      if ( nd->nd ) nd->nd->release();
      nd = nd->nxt;
    }
  }

public:
  // MPSC multithreading code
  // the if constexprs will allow the compiler to instantly eliminate this for st workloads
  [[gnu::always_inline]] inline bool
  __remote_push(byte *p, usize sz) noexcept
  {
    if constexpr ( !__default_multithread_safe ) {
      (void)p;
      (void)sz;
      return true;
    } else {
      if ( __remote_free.push(p, sz) ) [[likely]]
        return true;
      // ring full (owner not draining): embed the node in the dead block and publish it on the
      // overflow LIFO -- never spin against an owner that may not be allocating (ABC-11)
      __remote_ovf_node *nd = reinterpret_cast<__remote_ovf_node *>(p);
      nd->sz = sz;
      __remote_ovf_node *head = __remote_ovf.get(micron::memory_order_relaxed);
      do {
        nd->next = head;
      } while ( !__remote_ovf.compare_exchange_weak(head, nd, micron::memory_order_release, micron::memory_order_relaxed) );
      return true;
    }
  }

  [[gnu::noinline]] u32
  __remote_drain(void) noexcept
  {
    if constexpr ( !__default_multithread_safe ) {
      return 0;
    } else {
      u32 n = __remote_free.drain([this](byte *p, usize sz) {
        if ( sz ) {
          (void)this->__vmap_remove({ p, sz });
        } else {
          (void)this->__vmap_remove_at(p);
        }
      });
      if ( __remote_ovf.get(micron::memory_order_relaxed) != nullptr ) {
        // take-all: producers only push, so swapping the head detaches a consistent list
        __remote_ovf_node *nd = __remote_ovf.swap(nullptr, micron::memory_order::acq_rel);
        while ( nd != nullptr ) {
          __remote_ovf_node *nx = nd->next;      // read the embedded node before the map free recycles it
          const usize sz = nd->sz;
          byte *p = reinterpret_cast<byte *>(nd);
          if ( sz ) {
            (void)this->__vmap_remove({ p, sz });
          } else {
            (void)this->__vmap_remove_at(p);
          }
          ++n;
          nd = nx;
        }
      }
      return n;
    }
  }

  [[gnu::always_inline]] inline void
  __maybe_drain(void) noexcept
  {
    if constexpr ( !__default_multithread_safe ) return;
    if ( __remote_free.maybe_nonempty() || __remote_ovf.get(micron::memory_order_relaxed) != nullptr ) [[unlikely]]
      (void)__remote_drain();
  }

  class __struct_guard_t
  {
    micron::atomic_flag *_flag;

  public:
    [[gnu::always_inline]] explicit __struct_guard_t(micron::atomic_flag *f) noexcept : _flag(f)
    {
      if constexpr ( __default_multithread_safe ) {
        while ( _flag->test_and_set(micron::memory_order::acquire) ) __cpu_pause();
      } else {
        (void)f;
      }
    }

    [[gnu::always_inline]] ~__struct_guard_t() noexcept
    {
      if constexpr ( __default_multithread_safe ) {
        _flag->clear(micron::memory_order::release);
      }
    }

    __struct_guard_t(const __struct_guard_t &) = delete;
    __struct_guard_t(__struct_guard_t &&) = delete;
    __struct_guard_t &operator=(const __struct_guard_t &) = delete;
    __struct_guard_t &operator=(__struct_guard_t &&) = delete;
  };

  [[gnu::always_inline]] inline __struct_guard_t
  __struct_guard(void) noexcept
  {
    return __struct_guard_t{ &__struct_mtx };
  }

  ~__arena(void)
  {
    // WARNING: this destructor is meant to trigger iff you seek to recycle the entire memory arena, .ie you're looking
    // to reload the entire allocator. if abcmalloc is compiled in stm, on global scope destruction THIS FUNCTION WILL BE
    // DESTROYED IN REVERSE ORDER OF CONSTRUCTION, if any global scope objects registered memory with the allocator ALL
    // SUBSEQUENT CALLS WILL FAIL. this will ALWAYS happen if using automatic/default allocation, since all objects
    // invoke the allocator WITHIN their own constructors if compiled in global_mode THIS CODE WILL NEVER TRIGGER (kernel
    // reclaims on ret)

    // NOTE: ONCE THESE CALLS FIRE ALL MEMORY IS UNMAPPED AND LOST IRREVOCABLY.
    if constexpr ( __default_self_cleanup ) {
      __debug_print("~__arena(): self-cleanup triggered, releasing all buckets", 0);
      __release_tier(_precise);
      __release_tier(_small);
      __release_tier(_medium);
      __release_tier(_large);
      __release_tier(_huge);
      __release_tier(_arena_tier);
      _arena_memory.release();
      __debug_print("~__arena(): all buckets released", 0);
    }
  }

  __arena(void)
      : _arena_memory(this,
                      __default_guard_arena_metadata
                          ? __get_guarded_kernel_chunk(__default_arena_page_buf * __system_pagesize)
                          : __get_kernel_chunk<micron::__chunk<byte>>(__default_arena_page_buf * __system_pagesize),
                      __default_guard_arena_metadata ? __system_pagesize : static_cast<usize>(0))
  {
    _precise.init();
    _small.init();
    _arena_tier.init();
    _medium.init();
    _large.init();
    _huge.init();

    micron::sysinfo info;
    u64 prealloc_size = micron::math::floor<u64>(static_cast<f32>(info.totalram) * __default_prealloc_factor);
    __debug_print("__arena(): total system RAM: ", info.totalram);
    __debug_print("__arena(): prealloc_factor target bytes: ", prealloc_size);
    __debug_print("__arena(): arena metadata buf size: ", __default_arena_page_buf * __system_pagesize);

    __init_arena_tier(__default_arena_page_buf * __system_pagesize);
    __init_tlsf<__class_precise>(_precise, __default_cache_size_factor * __class_precise);

    if constexpr ( __default_eager_hot_tiers or !__default_lazy_construct ) {
      u64 share_small = __prealloc_share<__class_small>(prealloc_size);
      u64 share_medium = __prealloc_share<__class_medium>(prealloc_size);
      __debug_print("__arena(): prealloc share small: ", share_small);
      __debug_print("__arena(): prealloc share medium: ", share_medium);
      __init_tlsf<__class_small>(_small, share_small);
      __init_buddy<__class_medium>(_medium, share_medium);
    }

    if constexpr ( !__default_lazy_construct ) {
      if constexpr ( __default_init_large_pages and !__is_constrained ) {
        u64 share_large = __prealloc_share<__class_large>(prealloc_size);
        u64 share_huge = __prealloc_share<__class_huge>(prealloc_size);
        __debug_print("__arena(): prealloc share large: ", share_large);
        __debug_print("__arena(): prealloc share huge: ", share_huge);
        __init_buddy<__class_large>(_large, share_large);
        __init_buddy<__class_huge>(_huge, share_huge);
      } else {
        __debug_print("__arena(): large/huge pages skipped (constrained or disabled)", 0);
      }
    } else if constexpr ( !__default_eager_hot_tiers ) {
      __debug_print("__arena(): lazy construction enabled, deferring small/medium/large/huge init", 0);
    } else {
      __debug_print("__arena(): lazy construction enabled, deferring large/huge init", 0);
    }

    __debug_print("__arena(): initialisation complete", 0);
  }

  __arena(const __arena &) = delete;
  __arena(__arena &&) = delete;
  __arena &operator=(const __arena &) = delete;
  __arena &operator=(__arena &&) = delete;

  hot_fn(micron::__chunk<byte>) push(const usize sz)
  {
    __debug_print("push(): requested size: ", sz);
    collect_stats<stat_type::alloc>();
    collect_stats<stat_type::total_memory_req>(sz);

    if ( check_constraint(sz) ) [[unlikely]] {
      __debug_print("push()!!!: size exceeds constraint limit: ", sz);
      abort_state();
    }
    if ( check_oom() ) [[unlikely]] {
      __debug_print("push()!!!: OOM check triggered at size: ", sz);
      abort_state();
    }

    usize alloc_sz = __rz_inflate(sz);
    bool rz_active = (alloc_sz != sz);

    micron::__chunk<byte> memory;

    // __default_max_retries + 1 not __default_max_retries
    // if __default_max_retries is == 0 or 1 this meant that this loop either never fired or did not reattempt to populate memory after the
    // first sheet expansion
    for ( u64 i = 0; i <= __default_max_retries; ++i ) {
      if ( memory = __vmap_alloc(alloc_sz); !memory.zero() ) [[likely]] {
        if ( rz_active ) __rz_apply(memory, sz);
        __debug_print("push(): allocated bytes: ", memory.len);
        zero_on_alloc(memory.ptr, memory.len);
        sanitize_on_alloc(memory.ptr, memory.len);
        collect_stats<stat_type::total_memory_throughput>(memory.len);
        return memory;
      }
      if ( i == __default_max_retries ) break;

      __debug_print("push(): alloc failed, retry: ", i);
      __debug_print("push(): expanding for alloc_sz: ", alloc_sz);

      bool expanded = false;

      if ( alloc_sz <= __class_small ) {
        usize __next_sz = __calculate_space_cache(__default_cache_step);
        __debug_print("push(): precise/small path, expanding by: ", __next_sz);
        expanded = __buf_expand_exact(alloc_sz, __next_sz);
      } else if constexpr ( __is_constrained ) {
        if ( alloc_sz >= __class_medium ) {
          usize __next_sz = __calculate_space_medium(alloc_sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push() constrained: medium+ path, next_sz: ", __next_sz);
          __debug_print("push() constrained: predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
        }
      } else if constexpr ( !__is_constrained ) {
        if ( alloc_sz < __class_medium ) {
          // small tier
          usize __next_sz = __calculate_space_small(alloc_sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): small path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
        } else if ( alloc_sz <= __class_large ) {
          // medium tier
          usize __next_sz = __calculate_space_medium(alloc_sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): medium path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
        } else if ( alloc_sz <= __class_huge ) {
          // large tier; gets 2x medium now
          usize __next_sz = __calculate_space_large(alloc_sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): large path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
        } else if ( alloc_sz < __class_gb ) {
          // huge tier; new growth fn, more aggressive low allocs with tapered high allocs
          // now multivariate, grows more rapidly the more sheets are in use
          usize __base = __calculate_space_huge(alloc_sz) * __default_overcommit;
          usize __mult = (alloc_sz >= __class_1mb) ? 1ULL : (1ULL + static_cast<usize>(_huge.__count));
          usize __next_sz = __base * __mult;
          // WARNING: off-by-one safeguard; our buddy requires that the chunk to strictly exceed 2 * alloc_sz
          // due to __hdr_offsets
          usize __min_huge = (alloc_sz << 1) + __class_huge;
          if ( __next_sz < __min_huge ) __next_sz = __min_huge;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): huge path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
        } else {
          // bulk; same off-by-one safeguard
          usize bulk = __calculate_space_bulk(alloc_sz);
          usize __min_bulk = (alloc_sz << 1) + __class_huge;
          if ( bulk < __min_bulk ) bulk = __min_bulk;
          __debug_print("push(): bulk (>=gb) path, bulk_sz: ", bulk);
          expanded = __buf_expand_exact(alloc_sz, bulk);
        }
      }

      if ( !expanded ) [[unlikely]] {
        __debug_print("push(): expansion failed (mmap OOM or tier full), giving up", 0);
        break;
      }
    }

    __debug_print("push()!!!: all retries exhausted for size: ", sz);
    return { (byte *)-1, micron::numeric_limits<usize>::max() };
  }

  micron::__chunk<byte>
  launder(const usize sz)
  {
    __debug_print("launder(): requested size: ", sz);
    collect_stats<stat_type::alloc>();
    collect_stats<stat_type::total_memory_req>(sz);
    if ( check_constraint(sz) ) [[unlikely]] {
      __debug_print("launder()!!!: size exceeds constraint: ", sz);
      abort_state();
    }
    if ( check_oom() ) [[unlikely]] {
      __debug_print("launder()!!!: OOM check triggered at size: ", sz);
      abort_state();
    }

    usize alloc_sz = __rz_inflate(sz);
    bool rz_active = (alloc_sz != sz);

    micron::__chunk<byte> memory;

    for ( u64 i = 0; i <= __default_max_retries; ++i ) {
      if ( memory = __vmap_launder(alloc_sz); !memory.zero() ) {
        if ( rz_active ) __rz_apply(memory, sz);
        __debug_print("launder(): allocated bytes: ", memory.len);
        zero_on_alloc(memory.ptr, memory.len);
        sanitize_on_alloc(memory.ptr, memory.len);
        collect_stats<stat_type::total_memory_throughput>(memory.len);
        return memory;
      }
      if ( i == __default_max_retries ) break;

      __debug_print("launder(): launder failed, retry: ", i);
      bool expanded = false;
      if ( alloc_sz < __class_medium ) {
        usize next = __calculate_space_small(alloc_sz) * __default_overcommit;
        __debug_print("launder(): small path, expanding: ", next);
        expanded = __buf_expand_exact(alloc_sz, next);
      } else if ( alloc_sz <= __class_large ) {
        usize next = __calculate_space_medium(alloc_sz) * __default_overcommit;
        __debug_print("launder(): medium path, expanding: ", next);
        expanded = __buf_expand_exact(alloc_sz, next);
      } else if ( alloc_sz <= __class_huge ) {
        usize next = __calculate_space_large(alloc_sz) * __default_overcommit;
        __debug_print("launder(): large path, expanding: ", next);
        expanded = __buf_expand_exact(alloc_sz, next);
      } else if ( alloc_sz < __class_gb ) {
        usize __base = __calculate_space_huge(alloc_sz) * __default_overcommit;
        usize __mult = (alloc_sz >= __class_1mb) ? 1ULL : (1ULL + static_cast<usize>(_huge.__count));
        usize next = __base * __mult;
        usize __min_huge = (alloc_sz << 1) + __class_huge;
        if ( next < __min_huge ) next = __min_huge;
        __debug_print("launder(): huge path, expanding: ", next);
        expanded = __buf_expand_exact(alloc_sz, next);
      } else {
        usize bulk = __calculate_space_bulk(alloc_sz);
        usize __min_bulk = (alloc_sz << 1) + __class_huge;
        if ( bulk < __min_bulk ) bulk = __min_bulk;
        __debug_print("launder(): bulk path, expanding: ", bulk);
        expanded = __buf_expand_exact(alloc_sz, bulk);
      }
      if ( !expanded ) [[unlikely]] {
        __debug_print("launder(): expansion failed (mmap OOM or tier full), giving up", 0);
        break;
      }
    }
    __debug_print("launder()!!!: all retries exhausted for size: ", sz);
    return { (byte *)-1, micron::numeric_limits<usize>::max() };
  }

  bool
  pop(const micron::__chunk<byte> &mem)
  {
    __debug_print_addr("pop() address: ", mem.ptr);
    if ( mem.zero() ) return true;
    if ( !check_ptr_valid(mem.ptr) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem.ptr) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem.ptr)) ) {
        __debug_print_addr("pop()!!!: provenance check failed for: ", mem.ptr);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(mem.len);
    poison_on_free(mem.ptr, mem.len);
    full_on_free(mem.ptr, mem.len);
    zero_on_free(mem.ptr, mem.len);
    bool ok = __vmap_remove(mem);
    __debug_print("pop(): removal result: ", (usize)ok);
    return ok;
  }

  bool
  pop(byte *mem)
  {
    __debug_print_addr("pop() address: ", mem);
    if ( mem == nullptr ) return true;
    if ( !check_ptr_valid(mem) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("pop()!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
    poison_on_free(mem);
    full_on_free(mem);
    zero_on_free(mem);
    bool ok = __vmap_remove_at(mem);
    __debug_print("pop() addr: removal result: ", (usize)ok);
    return ok;
  }

  bool
  present(addr_t *mem) const
  {
    return mem ? __vmap_locate_at(mem) : false;
  }

  bool
  has_provenance(addr_t *mem) const
  {
    return mem ? __vmap_within(mem) : false;
  }

  bool
  is_valid_block(addr_t *mem) const
  {
    return mem ? __vmap_valid_block(mem) : false;
  }

  bool
  __is_cached(byte *ptr)
  {
    bool cached = false;
    (void)__dispatch_addr(reinterpret_cast<addr_t *>(ptr), [&](auto &tier, i32) {
      cached = tier.__cache.contains(ptr);
      return true;
    });
    return cached;
  }

  bool
  pop(byte *mem, usize len)
  {
    if ( !mem ) return false;
    if ( !check_chunk_valid(mem, len) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("pop(len)!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::total_memory_freed>(len);
    poison_on_free(mem, len);
    full_on_free(mem, len);
    zero_on_free(mem, len);
    bool ok = __vmap_remove({ mem, len });
    __debug_print("pop(len): removal result: ", (usize)ok);
    return ok;
  }

  bool
  ts_pop(const micron::__chunk<byte> &mem)
  {
    if ( mem.zero() ) return false;
    if ( !check_ptr_valid(mem.ptr) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem.ptr) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem.ptr)) ) {
        __debug_print_addr("ts_pop()!!!: provenance check failed for: ", mem.ptr);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(mem.len);
    poison_on_free(mem.ptr, mem.len);
    full_on_free(mem.ptr, mem.len);
    zero_on_free(mem.ptr, mem.len);
    bool ok = __vmap_tombstone(mem);
    __debug_print("ts_pop(): tombstone result: ", (usize)ok);
    return ok;
  }

  bool
  ts_pop(byte *mem)
  {
    if ( !mem ) return false;
    if ( !check_ptr_valid(mem) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("ts_pop() addr!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
    poison_on_free(mem);
    full_on_free(mem);
    zero_on_free(mem);
    bool ok = __vmap_tombstone_at(mem);
    __debug_print("ts_pop() addr: tombstone result: ", (usize)ok);
    return ok;
  }

  bool
  ts_pop(byte *mem, usize len)
  {
    if ( !mem ) return false;
    if ( !check_chunk_valid(mem, len) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("ts_pop(len)!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::total_memory_freed>(len);
    poison_on_free(mem, len);
    full_on_free(mem, len);
    zero_on_free(mem, len);
    bool ok = __vmap_tombstone({ mem, len });
    __debug_print("ts_pop(len): tombstone result: ", (usize)ok);
    return ok;
  }

  bool
  freeze(const micron::__chunk<byte> &mem)
  {
    if ( mem.zero() ) return false;
    if ( !check_ptr_valid(mem.ptr) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem.ptr) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem.ptr)) ) {
        __debug_print_addr("freeze()!!!: provenance check failed for: ", mem.ptr);
        return fail_state();
      }
    }
    __debug_print_addr("freeze(): freezing region at: ", mem.ptr);
    return __vmap_freeze(mem);
  }

  bool
  freeze(byte *mem)
  {
    if ( !mem ) return false;
    if ( !check_ptr_valid(mem) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("freeze() addr!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    __debug_print_addr("freeze() addr: freezing region at: ", mem);
    return __vmap_freeze_at(mem);
  }

  bool
  freeze(byte *mem, usize len)
  {
    if ( !mem ) return false;
    if ( !check_chunk_valid(mem, len) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !is_valid_block(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("freeze(len)!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    __debug_print_addr("freeze(len): freezing region at: ", mem);
    __debug_print("freeze(len): region length: ", len);
    return __vmap_freeze({ mem, len });
  }

  usize
  total_usage(void) const
  {
    usize t = 0;
    t += _precise.for_each([](tlsf_sheet<__class_precise> *const v) -> usize { return v->allocated(); });
    t += _small.for_each([](tlsf_sheet<__class_small> *const v) -> usize { return v->allocated(); });
    t += _medium.for_each([](sheet<__class_medium> *const v) -> usize { return v->allocated(); });
    t += _large.for_each([](sheet<__class_large> *const v) -> usize { return v->allocated(); });
    t += _huge.for_each([](sheet<__class_huge> *const v) -> usize { return v->allocated(); });
    __debug_print("total_usage(): aggregate allocated bytes: ", t);
    return t;
  }

  template<u64 Sz>
  usize
  total_usage_of_class(void) const
  {
    if constexpr ( Sz == __class_precise )
      return _precise.for_each([](tlsf_sheet<__class_precise> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_small )
      return _small.for_each([](tlsf_sheet<__class_small> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_medium )
      return _medium.for_each([](sheet<__class_medium> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_large )
      return _large.for_each([](sheet<__class_large> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_huge )
      return _huge.for_each([](sheet<__class_huge> *const v) -> usize { return v->allocated(); });
    return 0;
  }

  void
  reset_page(byte *ptr)
  {
    if constexpr ( __default_persistent_mode ) {
      // explicit whole-sheet release violates the persistent guarantee
      __debug_print_addr("reset_page(): explicit sheet release forbidden in persistent mode: ", ptr);
      fail_state();
      return;
    }
    auto __g = __struct_guard();
    __debug_print_addr("reset_page(): resetting sheet containing: ", ptr);
    // binary search each tier for the sheet whose range covers the address
    addr_t *addr = reinterpret_cast<addr_t *>(ptr);
    auto do_reset = [&](auto &tier) {
      i32 idx = tier.find_range(addr);
      if ( idx >= 0 ) {
        // drop any cached blocks belonging to this sheet first
        tier.__cache.invalidate_range(reinterpret_cast<const byte *>(tier.__idx[idx].lo),
                                      reinterpret_cast<const byte *>(tier.__idx[idx].hi));
        tier.__idx[idx].nd->nd->reset();
      }
    };
    do_reset(_precise);
    do_reset(_small);
    do_reset(_medium);
    do_reset(_large);
    do_reset(_huge);
  }

  usize
  __available_buffer(void) const
  {
    return _arena_memory.available();
  }

  byte *
  resize(byte *ptr, usize new_sz)
  {
    if ( __is_cached(ptr) ) [[unlikely]]
      return nullptr;
    const usize old_size = __size_of_alloc(reinterpret_cast<addr_t *>(ptr));
    if ( old_size == 0 ) [[unlikely]]
      return nullptr;
    // in-place reuse
    if ( old_size >= new_sz and new_sz > (old_size >> 1) ) return ptr;

    micron::__chunk<byte> fresh = push(new_sz);
    if ( __is_sentinel_chunk(fresh) ) [[unlikely]]
      return nullptr;

    const usize copy_size = (old_size < new_sz) ? old_size : new_sz;
    micron::memcpy(fresh.ptr, ptr, copy_size);
    (void)pop(ptr);
    return fresh.ptr;
  }

  static inline bool
  __is_sentinel_chunk(const micron::__chunk<byte> &c) noexcept
  {
    return c.ptr == (byte *)-1 or c.ptr == nullptr;
  }

  usize
  __size_of_alloc(addr_t *addr) const
  {
    i32 idx;

    // tlsf classes: block header at ptr - __hdr_offset, first u32 is bsize
    // with redzones: user ptr is shifted by __default_redzone_size from the
    // TLSF return pointer, so the header is at ptr - rz_size - __hdr_offset
    {
      byte *tlsf_user = reinterpret_cast<byte *>(addr);
      usize tlsf_overhead = __hdr_offset;
      if constexpr ( __default_redzone ) {
        tlsf_user -= static_cast<usize>(__default_redzone_size);
        tlsf_overhead = __hdr_offset + 2 * static_cast<usize>(__default_redzone_size);
      }
      if ( (idx = _precise.find_range(addr)) >= 0 ) {
        const usize bs = _precise.__idx[idx].nd->nd->block_size_of(tlsf_user);
        const usize recovered = (bs > tlsf_overhead) ? bs - tlsf_overhead : 0;
        __debug_print("__size_of_alloc(): precise tlsf user size: ", recovered);
        return recovered;
      }
      if ( (idx = _small.find_range(addr)) >= 0 ) {
        const usize bs = _small.__idx[idx].nd->nd->block_size_of(tlsf_user);
        const usize recovered = (bs > tlsf_overhead) ? bs - tlsf_overhead : 0;
        __debug_print("__size_of_alloc(): small tlsf user size: ", recovered);
        return recovered;
      }
    }

    // buddy classes: the AUTHORITATIVE order lives in the buddy's block_tags
    // (one tag per min-block, rewritten on every (de)allocation, split and merge)
    if ( (idx = _medium.find_range(addr)) >= 0 ) {
      const usize bs = _medium.__idx[idx].nd->nd->block_size_of(reinterpret_cast<byte *>(addr));
      const usize recovered = (bs > __hdr_offset) ? bs - __hdr_offset : 0;
      __debug_print("__size_of_alloc(): medium buddy user size: ", recovered);
      return recovered;
    }
    if ( (idx = _large.find_range(addr)) >= 0 ) {
      const usize bs = _large.__idx[idx].nd->nd->block_size_of(reinterpret_cast<byte *>(addr));
      const usize recovered = (bs > __hdr_offset) ? bs - __hdr_offset : 0;
      __debug_print("__size_of_alloc(): large buddy user size: ", recovered);
      return recovered;
    }
    if ( (idx = _huge.find_range(addr)) >= 0 ) {
      const usize bs = _huge.__idx[idx].nd->nd->block_size_of(reinterpret_cast<byte *>(addr));
      const usize recovered = (bs > __hdr_offset) ? bs - __hdr_offset : 0;
      __debug_print("__size_of_alloc(): huge buddy user size: ", recovered);
      return recovered;
    }

    __debug_print_addr("__size_of_alloc(): WARNING addr not found in any bucket: ", addr);
    return 0;
  }

  bool
  zeroed(void) const
  {
    return _precise.zeroed();
  }
};
};      // namespace abc
