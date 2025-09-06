//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../mem.hpp"
#include "../../memory/cmemory.hpp"
#include "../../memory/pointers/sentinel.hpp"
#include "../../simd/types.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"

namespace abc
{

using ret_flag = micron::sentinel_pointer;
constexpr static const uintptr_t __flag_invalid = 0;
constexpr static const uintptr_t __flag_failure = -1;
constexpr static const uintptr_t __flag_out_of_space = -2;

template <typename T, i64 Min, i32 Mx = 64>
  requires(micron::is_trivially_constructible_v<T> and micron::is_trivially_destructible_v<T>
           and (bool)((Min & (Min - 1)) == 0))
struct __buddy_list {
  struct free_block {
    free_block *next;
  };
  constexpr static size_t __hdr_offset = sizeof(micron::simd::i256);
  // size_t min_block;
  byte *base;
  size_t total;
  i64 max_order;
  size_t allocated_bytes;
  free_block *free_lists[Mx];     // on the stack

  void
  __impl_init_memory(byte *_ptr, size_t _len)
  {
    uintptr_t ptr = (uintptr_t)_ptr;
    uintptr_t a = alignof(void *);
    uintptr_t r = (ptr + (a - 1)) & ~(a - 1);
    base = (byte *)r;
    size_t adjust = r - ptr;
    if ( _len <= adjust )
      return;
    size_t usable = _len - adjust;

    usable = (usable / Min) * Min;
    if ( usable < Min )
      return;
    total = usable;

    int m = 0;
    size_t blk = Min;
    while ( blk <= total && m < Mx ) {
      ++m;
      blk <<= 1;
    }
    max_order = m;

    for ( i64 i = 0; i < max_order; ++i )
      free_lists[i] = nullptr;
    free_lists[max_order - 1] = (free_block *)base;
    free_lists[max_order - 1]->next = nullptr;
  }
  ~__buddy_list() noexcept = default;
  __buddy_list(void) = delete;
  //__buddy_list(void *mem, size_t bytes) noexcept : base(nullptr), total(0), max_order(0)
  __buddy_list(const T &mem) noexcept : base(nullptr), total(0), max_order(0), allocated_bytes(0)
  {
    if ( mem.zero() or mem.len < Min )
      std::abort();
    __impl_init_memory(mem.ptr, mem.len);
  }
  __buddy_list(const __buddy_list &) = delete;
  __buddy_list(__buddy_list &&o)
      : base(o.base), total(o.total), max_order(o.max_order), allocated_bytes(o.allocated_bytes)
  {
    o.base = nullptr;
    o.total = 0;
    o.max_order = 0;
    o.allocated_bytes = 0;

    for ( i64 i = 0; i < max_order; ++i ) {
      free_lists[i] = o.free_lists[i];
      o.free_lists[i] = nullptr;
    }
  }
  __buddy_list &operator=(const __buddy_list &) = delete;
  __buddy_list &
  operator=(__buddy_list &&o)
  {
    base = o.base;
    total = o.total;
    max_order = o.max_order;
    allocated_bytes = o.allocated_bytes;

    o.base = nullptr;
    o.total = 0;
    o.max_order = 0;
    o.allocated_bytes = 0;
    for ( i64 i = 0; i < max_order; ++i ) {
      free_lists[i] = o.free_lists[i];
      o.free_lists[i] = nullptr;
    }
    return *this;
  }
  static inline int
  ceil_log2_u64(uint64_t v) noexcept
  {
    if ( v <= 1 )
      return 0;
    return 64 - __builtin_clzll(v - 1);
  }
  inline int
  order_for_size(size_t n) const noexcept
  {
    size_t units = (n + Min - 1) / Min;
    return ceil_log2_u64(units);
  }
  inline size_t
  order_size(i64 o) const noexcept
  {
    return Min << o;
  }

  T
  allocate(size_t n) noexcept
  {
    n += sizeof(micron::simd::i256);
    if ( n == 0 )
      n = 1;
    if ( !base )
      return { nullptr, 0 };
    size_t needed = (n + Min - 1) & ~(Min - 1);
    needed += sizeof(i64);
    i64 o = order_for_size(needed);
    if ( o >= max_order )
      return { nullptr, 0 };
    i64 i = o;
    while ( i < max_order && free_lists[i] == nullptr )
      ++i;

    // max order has been hit, send it back and notify
    if ( i == max_order ) {
      return { nullptr, 0 };
    }     //  return { nullptr, 0 };
    free_block *blk = free_lists[i];
    free_lists[i] = blk->next;
    while ( i > o ) {
      --i;
      size_t len = order_size(i);
      byte *right = (byte *)blk + len;
      free_block *buddy = (free_block *)right;
      buddy->next = free_lists[i];
      free_lists[i] = buddy;
    }
    i64 *hdr = reinterpret_cast<i64 *>(blk);
    *hdr = static_cast<i64>(i);
    allocated_bytes += order_size(i);
    // NOTE: this is here due to alignment, plus we might add on later other info to the metadata, so we get a little
    // leeway. the offset is 256-bit to account properly for AVX2.
    return { ((byte *)blk + __hdr_offset), order_size(i) - __hdr_offset };
  }

  T
  allocate_exact(size_t n) noexcept
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
    i64 *hdr = reinterpret_cast<i64 *>(blk);
    *hdr = static_cast<int>(o);
    allocated_bytes += order_size(o);
    // NOTE: this is here due to alignment, plus we might add on later other info to the metadata, so we get a little
    // leeway. the offset is 256-bit to account properly for AVX2.
    return { ((byte *)blk + __hdr_offset), order_size(o) - __hdr_offset };
  }

  ret_flag
  deallocate(byte *ptr) noexcept
  {
    byte *addr = ptr - __hdr_offset;     // start of block including header
    i64 *hdr = reinterpret_cast<i64 *>(addr);
    i64 o = static_cast<i64>(*hdr);
    if ( o < 0 || o >= max_order )
      return { __flag_invalid };
    allocated_bytes -= order_size(o);
    if ( !base || !addr || o == 0 )
      return __flag_failure;
    if ( o >= max_order )
      o = max_order - 1;
    while ( true ) {
      size_t blk_sz = order_size(o);
      size_t off = (size_t)(addr - base);
      size_t buddy_off = off ^ blk_sz;
      byte *buddy = base + buddy_off;
      free_block *prev = nullptr;
      free_block *cur = free_lists[o];
      while ( cur ) {
        if ( (byte *)cur == buddy ) {
          if ( prev )
            prev->next = cur->next;
          else
            free_lists[o] = cur->next;
          if ( buddy < addr )
            addr = buddy;
          ++o;
          if ( o >= max_order ) {
            o = max_order - 1;
            break;
          }
          goto merged;
        }
        prev = cur;
        cur = cur->next;
      }
      {
        free_block *nb = (free_block *)addr;
        nb->next = free_lists[o];
        free_lists[o] = nb;
        return addr;
      }
    merged:
      continue;
    }
    return __flag_out_of_space;
  }

  ret_flag
  deallocate(T &node) noexcept
  {
    if ( !node.ptr or node.len == 0 )
      return __flag_invalid;
    return deallocate(node.ptr);
  }
  T
  reallocate(T node, size_t new_size) noexcept
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

    size_t to_copy = (node.len < nnode.len) ? node.len : nnode.len;
    micron::memcpy(nnode.ptr, node.ptr, to_copy);
    deallocate(node);
    return nnode;
  }

  size_t
  available() const noexcept
  {
    if ( !base )
      return 0;
    return total - allocated_bytes;
  }
  size_t
  used() const noexcept
  {
    return allocated_bytes;
  }
  size_t
  block_size(byte *ptr) const noexcept
  {
    if ( !base || !ptr )
      return 0;
    if ( ptr < base || ptr >= base + total )
      return 0;
    for ( i64 o = 0; o < max_order; ++o ) {
      size_t len = order_size(o);
      size_t off = (size_t)(ptr - base);
      if ( (off & (len - 1)) == 0 )
        return len;
    }
    return 0;
  }
  bool
  is_allocated(byte *ptr) const noexcept
  {
    if ( !ptr || !base || ptr < base || ptr >= base + total )
      return false;

    // check all free lists
    for ( i64 i = 0; i < max_order; ++i ) {
      free_block *cur = free_lists[i];
      while ( cur ) {
        if ( reinterpret_cast<byte *>(cur) == ptr - sizeof(i64) )
          return false;
        cur = cur->next;
      }
    }
    return true;
  }

  bool
  allocated_size(byte *ptr) const noexcept
  {
    if ( !base || !ptr )
      return false;
    if ( ptr < base || ptr >= base + total )
      return false;
    for ( i64 i = 0; i < max_order; ++i ) {
      free_block *cur = free_lists[i];
      while ( cur ) {
        if ( (byte *)cur == ptr )
          return false;
        cur = cur->next;
      }
    }
    return true;
  }
};
};
