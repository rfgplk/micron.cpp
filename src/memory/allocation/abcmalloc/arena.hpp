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
#include "oom.hpp"
#include "stats.hpp"

#include "printing.hpp"

template <typename P>
void *
operator new(usize size, P *ptr)
{
  (void)size;
  return ptr;
}

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

template <u64 Class>
constexpr static u64
__weight_for_class()
{
  constexpr u64 shift = __builtin_ctzll(Class);
  return __compute_weight(shift);
}

template <u64 Class>
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

static inline __attribute__((always_inline)) usize
__page_round(usize sz)
{
  return (sz + __system_pagesize - 1) & ~(__system_pagesize - 1);
}

static inline micron::__chunk<byte>
__get_guarded_kernel_chunk(usize sz)
{
  auto chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz + __system_pagesize);
  if ( chnk.zero() or chnk.ptr == (byte *)-1 ) [[unlikely]] {
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
  return chnk.ptr != nullptr and chnk.ptr != (byte *)-1 and chnk.len != 0;
}

class __arena : private cache
{
  template <typename T> struct alignas(16) node {
    using sheet_type = T;

    T *nd;
    node<T> *prev;
    node<T> *nxt;
  };

  template <typename sheet_type> struct alignas(64) __tier {
    static constexpr u32 __max_sheets = 64;
    static constexpr u32 __no_hit = __max_sheets;

    struct __range {
      addr_t *lo;
      addr_t *hi;
      node<sheet_type> *nd;
    };

    node<sheet_type> head;
    node<sheet_type> *tail;

    __range __idx[__max_sheets];
    u32 __count;
    u64 __space_mask;
    u32 __last_hit;
    u32 __dealloc_count;

    void
    init(void)
    {
      head.nd = nullptr;
      head.prev = nullptr;
      head.nxt = nullptr;
      tail = nullptr;
      __count = 0;
      __space_mask = 0;
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
        return __max_sheets;     // sentinel: tier full, caller must handle

      addr_t *lo = nd->nd->addr();
      addr_t *hi = nd->nd->addr_end();

      u32 pos = 0;
      while ( pos < __count and __idx[pos].lo < lo ) ++pos;

      // shift entries right
      for ( u32 i = __count; i > pos; --i ) __idx[i] = __idx[i - 1];

      __idx[pos] = { lo, hi, nd };

      // shift space_mask bits >= pos up by one, set new bit
      u64 lo_mask = __space_mask & ((1ULL << pos) - 1);
      u64 hi_mask = (__space_mask >> pos) << (pos + 1);
      __space_mask = lo_mask | hi_mask | (1ULL << pos);

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

      // collapse space_mask: remove bit at pos, shift upper bits down
      u64 lo_mask = __space_mask & ((1ULL << pos) - 1);
      u64 hi_mask = (__space_mask >> (pos + 1)) << pos;
      __space_mask = lo_mask | hi_mask;

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
      __space_mask &= ~(1ULL << pos);
    }

    inline __attribute__((always_inline)) void
    mark_available(u32 pos)
    {
      __space_mask |= (1ULL << pos);
    }

    bool
    has_space(void) const
    {
      return __space_mask != 0;
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

    template <typename Fn>
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

    template <typename Fn>
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

  // tlsf-backed tiers (precise + small)
  __tier<tlsf_sheet<__class_precise>> _precise;
  __tier<tlsf_sheet<__class_small>> _small;

  // buddy-backed tiers
  __tier<sheet<__class_arena_internal>> _arena_tier;     // internal metadata
  __tier<sheet<__class_medium>> _medium;
  __tier<sheet<__class_large>> _large;
  __tier<sheet<__class_huge>> _huge;

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
      _arena_tier.head.nd = new (buf.ptr) sheet<__class_arena_internal>(chnk, __system_pagesize);
    } else {
      _arena_tier.head.nd = new (buf.ptr) sheet<__class_arena_internal>(__get_kernel_chunk<micron::__chunk<byte>>(n));
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
      nd->nd = new (p) Sh(chnk, __system_pagesize);
    } else {
      auto chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz);
      if ( !__kernel_chunk_valid(chnk) ) [[unlikely]] {
        __debug_print("__expand_arena_tier()!!!: mmap failed for arena tier expansion, req: ", sz);
        abort_state();
      }
      nd->nd = new (p) Sh(chnk);
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

  template <u64 Sz>
  void
  __init_tlsf(__tier<tlsf_sheet<Sz>> &tier, usize n)
  {
    n = __page_round(n);
    __debug_print("__init_tlsf(): class size: ", Sz);
    __debug_print("__init_tlsf(): backing region size: ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(tlsf_sheet<Sz>));
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__init_tlsf()!!!: no arena metadata for tlsf header, class: ", Sz);
      abort_state();
    }
    tier.head.nd = new (buf.ptr) tlsf_sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(n));
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

  template <u64 Sz>
  inline __attribute__((always_inline)) bool
  __expand_tlsf(__tier<tlsf_sheet<Sz>> &tier, usize sz)
  {
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
    nd->nd = new (p) Sh(chnk);
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

  template <u64 Sz>
  void
  __init_buddy(__tier<sheet<Sz>> &tier, usize n = __default_magic_size)
  {
    if ( n == __default_magic_size ) {
      if ( Sz >= __class_medium )
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
    tier.head.nd = new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(n));
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

  template <u64 Sz>
  inline __attribute__((always_inline)) bool
  __expand_buddy(__tier<sheet<Sz>> &tier, usize sz)
  {
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
      nd->nd = new (p) Sh(chnk, __system_pagesize);
    } else {
      nd->nd = new (p) Sh(chnk);
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
          return true;     // __init aborts on failure, reaching here means success
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

  template <typename sheet_type>
  inline __attribute__((always_inline)) micron::__chunk<byte>
  __bucket_insert(__tier<sheet_type> &tier, const usize sz)
  {
    // mru cache opt
    u32 lh = tier.__last_hit;
    if ( lh < tier.__count and (tier.__space_mask & (1ULL << lh)) ) {
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

    // bitmap scan fallback
    u64 mask = tier.__space_mask;
    while ( mask ) {
      u32 bit = __builtin_ctzll(mask);
      auto &sh = *tier.__idx[bit].nd->nd;
      micron::__chunk<byte> mem;
      if constexpr ( __default_launder ) {
        mem = sh.temporal_mark(sz);
      } else {
        mem = sh.mark(sz);
      }
      if ( !mem.zero() ) {
        tier.__last_hit = bit;
        return mem;
      }
      // sheet exhausted for this size, clear bit
      tier.mark_exhausted(bit);
      mask &= mask - 1;     // clear lowest set bit
    }
    return { nullptr, 0 };
  }

  template <typename sheet_type>
  inline __attribute__((always_inline)) micron::__chunk<byte>
  __bucket_insert_temporal(__tier<sheet_type> &tier, const usize sz)
  {
    // mru cache opt
    u32 lh = tier.__last_hit;
    if ( lh < tier.__count and (tier.__space_mask & (1ULL << lh)) ) {
      micron::__chunk<byte> mem = tier.__idx[lh].nd->nd->temporal_mark(sz);
      if ( !mem.zero() ) return mem;
      tier.mark_exhausted(lh);
    }

    u64 mask = tier.__space_mask;
    while ( mask ) {
      u32 bit = __builtin_ctzll(mask);
      auto &sh = *tier.__idx[bit].nd->nd;
      micron::__chunk<byte> mem = sh.temporal_mark(sz);
      if ( !mem.zero() ) {
        tier.__last_hit = bit;
        return mem;
      }
      tier.mark_exhausted(bit);
      mask &= mask - 1;
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

  hot_fn(micron::__chunk<byte>) __vmap_alloc(const usize sz)
  {
    if ( sz <= __class_small ) {
      __debug_print("__vmap_alloc(): tier=precise, sz: ", sz);
      return __bucket_insert(_precise, sz);
    }
    if ( sz < __class_medium ) {
      __debug_print("__vmap_alloc(): tier=small, sz: ", sz);
      return __bucket_insert(_small, sz);
    }
    if ( sz <= __class_large ) {
      __debug_print("__vmap_alloc(): tier=medium, sz: ", sz);
      return __bucket_insert(_medium, sz);
    }
    if ( sz <= __class_huge ) {
      __debug_print("__vmap_alloc(): tier=large, sz: ", sz);
      return __bucket_insert(_large, sz);
    }
    __debug_print("__vmap_alloc(): tier=huge, sz: ", sz);
    return __bucket_insert(_huge, sz);
  }

  micron::__chunk<byte>
  __vmap_launder(const usize sz)
  {
    if ( sz < __class_medium ) return __bucket_insert_temporal(_small, sz);
    if ( sz <= __class_large ) return __bucket_insert_temporal(_medium, sz);
    if ( sz <= __class_huge ) return __bucket_insert_temporal(_large, sz);
    return __bucket_insert_temporal(_huge, sz);
  }

  template <typename Fn>
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

  template <typename Fn>
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

  template <typename sheet_type>
  void
  __sweep_tier_tombstones(__tier<sheet_type> &tier)
  {
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
        __debug_print("__sweep_tier_tombstones(): reclaiming sheet at idx: ", (usize)i);
        sh.reset();
        tier.unlink_node(nd);
        tier.unregister(static_cast<u32>(i));
        __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<sheet_type>) + sizeof(sheet_type));
      }
    }
  }

  template <typename sheet_type>
  inline __attribute__((always_inline)) void
  __try_reclaim_empty(__tier<sheet_type> &tier, i32 range_idx, node<sheet_type> *nd)
  {
    if ( nd->nd->used() == 0 and nd != &tier.head ) {
      __debug_print("__try_reclaim_empty(): sheet fully drained, unlinking and resetting", 0);
      nd->nd->reset();
      tier.unlink_node(nd);
      tier.unregister(range_idx);
      __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<sheet_type>) + sizeof(sheet_type));
    }
  }

  template <typename sheet_type>
  inline __attribute__((always_inline)) void
  __tombstone_accounting(__tier<sheet_type> &tier, i32 range_idx, node<sheet_type> *nd)
  {
    if constexpr ( __default_tombstone_sweep_interval == 0 ) {
      auto &sh = *nd->nd;
      usize ts = sh.tombstoned();
      usize ft = sh.ftotal();
      __debug_print("__tombstone_accounting(): tombstoned: ", ts);
      __debug_print("__tombstone_accounting(): ftotal: ", ft);
      if ( (ts > (ft >> 1)) and sh.used() == 0 and nd != &tier.head ) {
        __debug_print("__tombstone_accounting(): threshold crossed, compacting sheet", 0);
        sh.reset();
        tier.unlink_node(nd);
        tier.unregister(range_idx);
        __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<sheet_type>) + sizeof(sheet_type));
      }
    } else {
      // immediate reclaim: if this specific sheet is fully drained (used == 0)
      // AND tombstoned bytes occupy >= 50% of the sheet's total capacity,
      // reclaim now rather than waiting for the batch sweep. prevents sheet
      // accumulation under monotonically-growing size patterns while preserving
      // mostly-pristine sheets that still have reusable free space.
      // uses the same condition as the batch sweep and the interval==0 path.
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
          __unmark_from_arena(reinterpret_cast<byte *>(nd), sizeof(node<sheet_type>) + sizeof(sheet_type));
          return;
        }
      }
      if ( tier.bump_dealloc() ) __sweep_tier_tombstones(tier);
    }
  }

  template <typename sheet_type>
  bool
  __tier_remove(__tier<sheet_type> &tier, i32 range_idx, const micron::__chunk<byte> &memory)
  {
    auto *nd = tier.__idx[range_idx].nd;
    auto &sh = *nd->nd;
    __debug_print_addr("__tier_remove(): found in sheet at addr: ", memory.ptr);

    if constexpr ( !__default_tombstone ) {
      if ( sh.try_unmark(memory) ) {
        tier.mark_available(range_idx);
        __debug_print("__tier_remove(): unmark succeeded, sheet used: ", sh.used());
        __try_reclaim_empty(tier, range_idx, nd);
        return true;
      }
      __debug_print_addr("__tier_remove(): try_unmark failed (possible double free): ", memory.ptr);
      return handle_double_free(memory.ptr);
    } else {
      if ( sh.try_tombstone(memory) ) {
        tier.mark_available(range_idx);
        __debug_print("__tier_remove(): tombstone set", 0);
        __tombstone_accounting(tier, range_idx, nd);
        return true;
      }
      __debug_print_addr("__tier_remove(): try_tombstone failed (possible double free): ", memory.ptr);
      return handle_double_free(memory.ptr);
    }
  }

  template <typename sheet_type>
  bool
  __tier_remove_at(__tier<sheet_type> &tier, i32 range_idx, byte *addr)
  {
    auto *nd = tier.__idx[range_idx].nd;
    auto &sh = *nd->nd;
    __debug_print_addr("__tier_remove_at(): found in sheet at addr: ", addr);

    if constexpr ( !__default_tombstone ) {
      if ( sh.try_unmark_no_size(addr) ) {
        tier.mark_available(range_idx);
        __debug_print("__tier_remove_at(): unmark succeeded, sheet used: ", sh.used());
        __try_reclaim_empty(tier, range_idx, nd);
        return true;
      }
      __debug_print_addr("__tier_remove_at(): try_unmark_no_size failed (possible double free): ", addr);
      return handle_double_free(addr);
    } else {
      if ( sh.try_tombstone_no_size(addr) ) {
        tier.mark_available(range_idx);
        __debug_print("__tier_remove_at(): tombstone set", 0);
        __tombstone_accounting(tier, range_idx, nd);
        return true;
      }
      __debug_print_addr("__tier_remove_at(): try_tombstone_no_size failed (possible double free): ", addr);
      return handle_double_free(addr);
    }
  }

  template <typename sheet_type>
  bool
  __tier_tombstone_at(__tier<sheet_type> &tier, i32 range_idx, byte *addr)
  {
    auto *nd = tier.__idx[range_idx].nd;
    auto &sh = *nd->nd;
    __debug_print_addr("__tier_tombstone_at(): found in sheet at addr: ", addr);

    sh.try_tombstone_no_size(addr);
    tier.mark_available(range_idx);
    __debug_print("__tier_tombstone_at(): tombstone set", 0);
    __tombstone_accounting(tier, range_idx, nd);
    return true;
  }

  template <typename sheet_type>
  bool
  __tier_tombstone(__tier<sheet_type> &tier, i32 range_idx, const micron::__chunk<byte> &memory)
  {
    auto *nd = tier.__idx[range_idx].nd;
    auto &sh = *nd->nd;
    __debug_print_addr("__tier_tombstone(): found in sheet at addr: ", memory.ptr);

    sh.try_tombstone(memory);
    tier.mark_available(range_idx);
    __debug_print("__tier_tombstone(): tombstone set", 0);
    __tombstone_accounting(tier, range_idx, nd);
    return true;
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
        return __tier_remove(_precise, idx, adj);
      }
      if ( (idx = _small.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) {
        if ( !verify_redzone(m.ptr, m.len) ) [[unlikely]] {
          __debug_print_addr("__vmap_remove(): redzone corruption detected at: ", m.ptr);
          return fail_state();
        }
        micron::__chunk<byte> adj = { m.ptr - __default_redzone_size, m.len + 2 * __default_redzone_size };
        return __tier_remove(_small, idx, adj);
      }
      if ( (idx = _medium.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __tier_remove(_medium, idx, m);
      if ( (idx = _large.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __tier_remove(_large, idx, m);
      if ( (idx = _huge.find_range(reinterpret_cast<addr_t *>(m.ptr))) >= 0 ) return __tier_remove(_huge, idx, m);
      __debug_print_addr("__vmap_remove(): WARNING address not found in any tier: ", m.ptr);
      return false;
    }
    bool ok = __dispatch_addr(reinterpret_cast<addr_t *>(m.ptr), [&](auto &tier, i32 idx) { return __tier_remove(tier, idx, m); });
    if ( !ok ) [[unlikely]]
      __debug_print_addr("__vmap_remove(): WARNING address not found in any tier: ", m.ptr);
    return ok;
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
    bool ok = __dispatch_addr(reinterpret_cast<addr_t *>(addr), [&](auto &tier, i32 idx) { return __tier_remove_at(tier, idx, addr); });
    if ( !ok ) [[unlikely]]
      __debug_print_addr("__vmap_remove_at(): WARNING address not found in any tier: ", addr);
    return ok;
  }

  bool
  __vmap_tombstone(const micron::__chunk<byte> &m)
  {
    return __vmap_remove(m);
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
  __vmap_locate_at(addr_t *addr) const
  {
    if constexpr ( !__default_tombstone ) return false;
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
    return __dispatch_addr(reinterpret_cast<addr_t *>(memory.ptr), [&](auto &tier, i32 idx) {
      __debug_print_addr("__vmap_freeze(): freezing sheet containing addr: ", memory.ptr);
      bool ok = tier.__idx[idx].nd->nd->freeze();
      __debug_print("__vmap_freeze(): freeze result: ", (usize)ok);
      return ok;
    });
  }

  bool
  __vmap_freeze_at(byte *addr)
  {
    return __dispatch_addr(reinterpret_cast<addr_t *>(addr), [&](auto &tier, i32 idx) {
      __debug_print_addr("__vmap_freeze_at(): freezing sheet containing: ", addr);
      bool ok = tier.__idx[idx].nd->nd->freeze();
      __debug_print("__vmap_freeze_at(): freeze result: ", (usize)ok);
      return ok;
    });
  }

  template <typename sheet_type>
  void
  __release_tier(__tier<sheet_type> &tier)
  {
    auto *nd = &tier.head;
    while ( nd != nullptr ) {
      if ( nd->nd ) nd->nd->release();
      nd = nd->nxt;
    }
  }

public:
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
      : _arena_memory(__default_guard_arena_metadata
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

    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      if ( memory = __vmap_alloc(alloc_sz); !memory.zero() ) [[likely]] {
        if ( rz_active ) __rz_apply(memory, sz);
        __debug_print("push(): allocated bytes: ", memory.len);
        zero_on_alloc(memory.ptr, memory.len);
        sanitize_on_alloc(memory.ptr, memory.len);
        collect_stats<stat_type::total_memory_throughput>(memory.len);
        return memory;
      }

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
        if ( alloc_sz >= __class_medium && alloc_sz < __class_1mb ) {
          usize __next_sz = __calculate_space_medium(alloc_sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): medium path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
        } else if ( alloc_sz >= __class_1mb && alloc_sz < __class_gb ) {
          usize __next_sz = __calculate_space_huge(alloc_sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): 1mb-gb path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
        } else if ( alloc_sz >= __class_gb ) {
          usize bulk = __calculate_space_bulk(alloc_sz);
          __debug_print("push(): bulk (>=gb) path, bulk_sz: ", bulk);
          expanded = __buf_expand_exact(alloc_sz, bulk);
        } else {
          usize __next_sz = __calculate_space_small(alloc_sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): fallback small path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          expanded = __buf_expand_exact(alloc_sz, predicted);
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
    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      if ( memory = __vmap_launder(alloc_sz); !memory.zero() ) {
        if ( rz_active ) __rz_apply(memory, sz);
        __debug_print("launder(): allocated bytes: ", memory.len);
        zero_on_alloc(memory.ptr, memory.len);
        sanitize_on_alloc(memory.ptr, memory.len);
        collect_stats<stat_type::total_memory_throughput>(memory.len);
        return memory;
      }
      __debug_print("launder(): launder failed, retry: ", i);
      bool expanded = false;
      if ( alloc_sz >= __class_medium and alloc_sz < __class_gb ) {
        usize next = __calculate_space_huge(alloc_sz) * __default_overcommit;
        __debug_print("launder(): medium-gb path, expanding: ", next);
        expanded = __buf_expand_exact(alloc_sz, next);
      } else if ( alloc_sz >= __class_gb ) {
        usize bulk = __calculate_space_bulk(alloc_sz);
        __debug_print("launder(): bulk path, expanding: ", bulk);
        expanded = __buf_expand_exact(alloc_sz, bulk);
      } else {
        usize next = __calculate_space_small(alloc_sz) * __default_overcommit;
        __debug_print("launder(): small path, expanding: ", next);
        expanded = __buf_expand_exact(alloc_sz, next);
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) ) {
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
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
  pop(byte *mem, usize len)
  {
    if ( !mem ) return false;
    if ( !check_chunk_valid(mem, len) ) [[unlikely]]
      return fail_state();
    if ( !check_alignment(mem) ) [[unlikely]]
      return fail_state();
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) ) {
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) ) {
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
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
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
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

  template <u64 Sz>
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
    __debug_print_addr("reset_page(): resetting sheet containing: ", ptr);
    // binary search each tier for the sheet whose range covers the address
    addr_t *addr = reinterpret_cast<addr_t *>(ptr);
    auto do_reset = [&](auto &tier) {
      i32 idx = tier.find_range(addr);
      if ( idx >= 0 ) tier.__idx[idx].nd->nd->reset();
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

  usize
  __size_of_alloc(addr_t *addr) const
  {
    i32 idx;

    // tlsf classes: block header at ptr - __hdr_offset, first u32 is bsize
    // with redzones: user ptr is shifted by __default_redzone_size from the
    // TLSF return pointer, so the header is at ptr - rz_size - __hdr_offset
    if ( (idx = _precise.find_range(addr)) >= 0 or (idx = _small.find_range(addr)) >= 0 ) {
      if constexpr ( __default_redzone ) {
        u32 bsz = *reinterpret_cast<u32 *>(reinterpret_cast<byte *>(addr) - static_cast<usize>(__default_redzone_size) - __hdr_offset);
        usize recovered = (usize)bsz - __hdr_offset - 2 * __default_redzone_size;
        __debug_print("__size_of_alloc(): tlsf block size (rz-adjusted): ", recovered);
        return recovered;
      } else {
        u32 bsz = *reinterpret_cast<u32 *>(reinterpret_cast<byte *>(addr) - __hdr_offset);
        __debug_print("__size_of_alloc(): tlsf raw bsize: ", (usize)bsz);
        __debug_print("__size_of_alloc(): tlsf user size: ", (usize)(bsz - __hdr_offset));
        return (usize)(bsz - __hdr_offset);
      }
    }

    // buddy classes: order stored at metadata addr
    // NOTE: buddy header lives at the TAIL of the block (block_start + order_size - __hdr_offset)
    // so the user-visible size is order_size - __hdr_offset, same adjustment as TLSF
    if ( (idx = _medium.find_range(addr)) >= 0 ) {
      i64 order_class = static_cast<i64>(*get_metadata_addr(addr));
      usize recovered = (__class_medium << order_class) - __hdr_offset;
      __debug_print("__size_of_alloc(): medium buddy order: ", order_class);
      __debug_print("__size_of_alloc(): recovered user size: ", recovered);
      return recovered;
    }
    if ( (idx = _large.find_range(addr)) >= 0 ) {
      i64 order_class = static_cast<i64>(*get_metadata_addr(addr));
      usize recovered = (__class_large << order_class) - __hdr_offset;
      __debug_print("__size_of_alloc(): large buddy order: ", order_class);
      __debug_print("__size_of_alloc(): recovered user size: ", recovered);
      return recovered;
    }
    if ( (idx = _huge.find_range(addr)) >= 0 ) {
      i64 order_class = static_cast<i64>(*get_metadata_addr(addr));
      usize recovered = (__class_huge << order_class) - __hdr_offset;
      __debug_print("__size_of_alloc(): huge buddy order: ", order_class);
      __debug_print("__size_of_alloc(): recovered user size: ", recovered);
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
};     // namespace abc
