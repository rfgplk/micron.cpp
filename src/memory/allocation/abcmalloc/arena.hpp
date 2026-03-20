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
    if ( frac % __system_pagesize != 0 )
      frac += __system_pagesize - (frac % __system_pagesize);
    return frac;
  } else {
    u64 scaled = total / __class_weight_denom;
    u64 rem = total % __class_weight_denom;
    u64 frac = scaled * w + (rem * w) / __class_weight_denom;
    if ( frac % __system_pagesize != 0 )
      frac += __system_pagesize - (frac % __system_pagesize);
    return frac;
  }
}

static inline __attribute__((always_inline)) usize
__page_round(usize sz)
{
  return (sz + __system_pagesize - 1) & ~(__system_pagesize - 1);
}

class __arena : private cache
{
  template <typename T> struct alignas(16) node {
    using sheet_type = T;

    T *nd;
    node<T> *nxt;
  };

  alloc_predictor __predict;
  sheet<__class_arena_internal> _arena_memory;

  // these now rely on the new tlsf backed allocator (buddy lists were too slow killing perf)
  node<tlsf_sheet<__class_precise>> _cache_buffer;
  node<tlsf_sheet<__class_precise>> *_tail_cache_buffer;
  node<tlsf_sheet<__class_small>> _small_buckets;
  node<tlsf_sheet<__class_small>> *_tail_small_buckets;

  // regular buddies
  node<sheet<__class_arena_internal>> _arena_buffer;
  node<sheet<__class_arena_internal>> *_tail_arena_buffer;
  node<sheet<__class_medium>> _medium_buckets;
  node<sheet<__class_medium>> *_tail_medium_buckets;
  node<sheet<__class_large>> _large_buckets;
  node<sheet<__class_large>> *_tail_large_buckets;
  node<sheet<__class_huge>> _huge_buckets;
  node<sheet<__class_huge>> *_tail_huge_buckets;

  // __dispatch is kept for read-only / freeze paths only
  template <typename Fn>
  inline __attribute__((always_inline)) bool
  __dispatch(Fn &&fn)
  {
    return fn(&_cache_buffer) or fn(&_small_buckets) or fn(&_medium_buckets) or fn(&_large_buckets) or fn(&_huge_buckets);
  }

  template <typename Fn>
  inline __attribute__((always_inline)) bool
  __dispatch(Fn &&fn) const
  {
    return fn(&_cache_buffer) or fn(&_small_buckets) or fn(&_medium_buckets) or fn(&_large_buckets) or fn(&_huge_buckets);
  }

  void
  __reload_arena_buf(void)
  {
    __debug_print("__reload_arena_buf(): current arena buf allocated: ", _tail_arena_buffer->nd->allocated());
    __debug_print("__reload_arena_buf(): arena metadata buf remaining: ", __available_buffer());
    __expand_bucket_arena<__class_arena_internal>(&_arena_buffer, _tail_arena_buffer, _tail_arena_buffer->nd->allocated() * 2);
    __debug_print("__reload_arena_buf(): arena buf expanded, new allocated: ", _tail_arena_buffer->nd->allocated());
  }

  inline __attribute__((always_inline)) micron::__chunk<byte>
  __mark_arena(usize sz)
  {
  retry_mark:
    micron::__chunk<byte> buf = _tail_arena_buffer->nd->try_mark(sz);
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__mark_arena(): arena buf exhausted, triggering reload", 0);
      __reload_arena_buf();
      goto retry_mark;
    }
    return buf;
  }

  // unmark arena
  // NOTE: type of node and sheet are incredibly important
  // different sizes WILL cause different/errneous frees and memory corruption
  template <typename Nd>
  void
  __unmark_arena(byte *addr)
  {
    using Sh = typename Nd::sheet_type;
    __find_and_remove_arena(&_arena_buffer, micron::__chunk<byte>(addr, sizeof(Nd) + sizeof(Sh)));
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_bucket_arena(F *nd, G *&tail, const usize sz)
  {
    while ( nd->nxt != nullptr )
      nd = nd->nxt;
    __debug_print("__expand_bucket_arena() with req. space: ", sz);
    usize pair_sz = sizeof(node<sheet<Sz>>) + sizeof(sheet<Sz>);
    micron::__chunk<byte> buf = _arena_memory.try_mark(pair_sz);
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__expand_bucket_arena()!!!: arena metadata exhausted, req: ", sz);
      __debug_print("__expand_bucket_arena(): arena metadata buf remaining: ", _arena_memory.available());
      abort_state();
    }
    byte *p = buf.ptr;
    nd->nxt = new (p) node<sheet<Sz>>();
    p += sizeof(node<sheet<Sz>>);
    nd->nxt->nd = new (p) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    nd->nxt->nxt = nullptr;
    tail = nd->nxt;
    __debug_print("__expand_bucket_arena(): new arena node allocated, size: ", sz);
  }

  template <u64 Sz, typename F, typename G>
  void
  __init_tlsf_bucket(F &bucket, G *&tail, usize n)
  {
    n = __page_round(n);
    __debug_print("__init_tlsf_bucket(): class size: ", Sz);
    __debug_print("__init_tlsf_bucket(): backing region size: ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(tlsf_sheet<Sz>));
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__init_tlsf_bucket()!!!: no arena metadata for tlsf header, class: ", Sz);
      abort_state();
    }
    bucket = { new (buf.ptr) tlsf_sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(n)), nullptr };
    tail = &bucket;
    if ( bucket.nd->empty() ) {
      __debug_print("__init_tlsf_bucket()!!!: kernel chunk returned empty for class: ", Sz);
      abort_state();
    }
    __debug_print("__init_tlsf_bucket(): initialised successfully for class: ", Sz);
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_tlsf(F *nd, G *&tail, const usize sz)
  {
    __debug_print("__expand_tlsf(): class size: ", Sz);
    __debug_print("__expand_tlsf(): requested backing region: ", sz);
    while ( nd->nxt != nullptr )
      nd = nd->nxt;
    micron::__chunk<byte> buf = __mark_arena(sizeof(node<tlsf_sheet<Sz>>) + sizeof(tlsf_sheet<Sz>));
    byte *p = buf.ptr;
    nd->nxt = new (p) node<tlsf_sheet<Sz>>();
    p += sizeof(node<tlsf_sheet<Sz>>);
    usize aligned_sz = __page_round(sz);
    nd->nxt->nd = new (p) tlsf_sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(aligned_sz));
    nd->nxt->nxt = nullptr;
    tail = nd->nxt;
    __debug_print("__expand_tlsf(): new tlsf node ready, backing size: ", aligned_sz);
  }

  template <u64 Sz, typename F, typename G>
  void
  __init_bucket(F &bucket, G *&tail, usize n = __default_magic_size)
  {
    if ( n == __default_magic_size ) {
      if ( Sz >= __class_medium )
        n = __calculate_space_huge(Sz);
      else
        n = __calculate_space_small(Sz);
    }
    __debug_print("__init_bucket(): class size: ", Sz);
    __debug_print("__init_bucket(): backing region size: ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    if ( buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__init_bucket()!!!: no arena metadata for buddy header, class: ", Sz);
      abort_state();
    }
    bucket = { new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(n)), nullptr };
    tail = &bucket;
    if ( bucket.nd->empty() ) {
      __debug_print("__init_bucket()!!!: kernel chunk returned empty for class: ", Sz);
      abort_state();
    }
    __debug_print("__init_bucket(): initialised successfully for class: ", Sz);
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_bucket(F *nd, G *&tail, const usize sz)
  {
    __debug_print("__expand_bucket(): class size: ", Sz);
    __debug_print("__expand_bucket(): requested expansion size: ", sz);
    while ( nd->nxt != nullptr )
      nd = nd->nxt;
    micron::__chunk<byte> buf = __mark_arena(sizeof(node<sheet<Sz>>) + sizeof(sheet<Sz>));
    byte *p = buf.ptr;
    nd->nxt = new (p) node<sheet<Sz>>();
    p += sizeof(node<sheet<Sz>>);
    if constexpr ( __default_insert_guard_pages ) {
      __debug_print("__expand_bucket(): inserting guard page for class: ", Sz);
      auto chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz + __system_pagesize);
      __make_guard(chnk);
      nd->nxt->nd = new (p) sheet<Sz>(chnk, __system_pagesize);
    } else {
      nd->nxt->nd = new (p) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    }
    nd->nxt->nxt = nullptr;
    if ( tail )
      tail = nd->nxt;
    __debug_print("__expand_bucket(): new buddy node ready for class: ", Sz);
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

  void
  __buf_expand_exact(const usize class_sz, const usize exact_sz)
  {
    __debug_print("__buf_expand_exact(): routing class_sz: ", class_sz);
    __debug_print("__buf_expand_exact(): target expansion exact_sz: ", exact_sz);

    if ( class_sz <= __class_small ) {
      __debug_print("__buf_expand_exact(): routed to precise/tlsf tier", 0);
      __expand_tlsf<__class_precise>(&_cache_buffer, _tail_cache_buffer, exact_sz);
      return;
    }

    if ( class_sz < __class_medium ) [[likely]] {
      __debug_print("__buf_expand_exact(): routed to small/tlsf tier", 0);
      if constexpr ( __default_lazy_construct ) {
        if ( _small_buckets.nd == nullptr ) [[unlikely]] {
          __debug_print("__buf_expand_exact(): lazy-constructing small bucket", 0);
          __init_tlsf_bucket<__class_small>(_small_buckets, _tail_small_buckets, __calculate_space_small(__class_small));
        } else {
          __expand_tlsf<__class_small>(&_small_buckets, _tail_small_buckets, exact_sz);
        }
      } else {
        __expand_tlsf<__class_small>(&_small_buckets, _tail_small_buckets, exact_sz);
      }
      return;
    }

    if ( class_sz <= __class_large ) {
      __debug_print("__buf_expand_exact(): routed to medium/buddy tier", 0);
      if constexpr ( __default_lazy_construct ) {
        if ( _medium_buckets.nd == nullptr ) [[unlikely]] {
          __debug_print("__buf_expand_exact(): lazy-constructing medium bucket", 0);
          __init_bucket<__class_medium>(_medium_buckets, _tail_medium_buckets);
        } else {
          __expand_bucket<__class_medium>(&_medium_buckets, _tail_medium_buckets, exact_sz);
        }
      } else {
        __expand_bucket<__class_medium>(&_medium_buckets, _tail_medium_buckets, exact_sz);
      }
      return;
    }

    if ( class_sz <= __class_huge ) {
      __debug_print("__buf_expand_exact(): routed to large/buddy tier", 0);
      if ( _large_buckets.nd == nullptr ) [[unlikely]] {
        __debug_print("__buf_expand_exact(): lazy-constructing large bucket", 0);
        __init_bucket<__class_large>(_large_buckets, _tail_large_buckets, exact_sz);
      } else {
        __expand_bucket<__class_large>(&_large_buckets, _tail_large_buckets, exact_sz);
      }
      return;
    }

    __debug_print("__buf_expand_exact(): routed to huge/buddy tier", 0);
    if ( _huge_buckets.nd == nullptr ) [[unlikely]] {
      __debug_print("__buf_expand_exact(): lazy-constructing huge bucket", 0);
      __init_bucket<__class_huge>(_huge_buckets, _tail_huge_buckets, exact_sz);
    } else {
      __expand_bucket<__class_huge>(&_huge_buckets, _tail_huge_buckets, exact_sz);
    }
  }

  template <typename F>
  bool
  __within(const F *__node, addr_t *memory) const
  {
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( __node->nd && __node->nd->is_at(memory) ) [[unlikely]]
        return true;
      __node = __node->nxt;
    }
    return false;
  }

  template <typename F>
  bool
  __locate_at(const F *__node, addr_t *memory) const
  {
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( __node->nd && __node->nd->find(reinterpret_cast<byte *>(memory)) ) [[unlikely]]
        return true;
      __node = __node->nxt;
    }
    return false;
  }

  template <typename F>
  micron::__chunk<byte>
  __append_bucket(F *nd, const usize sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    while ( nd != nullptr ) {
      __builtin_prefetch(nd->nxt, 0, 1);
      if ( !nd->nd ) {
        nd = nd->nxt;
        continue;
      }
      auto &sh = *nd->nd;
      if constexpr ( __default_launder ) {
        if ( memory = sh.temporal_mark(sz); !memory.zero() )
          return memory;
      } else {
        if ( memory = sh.mark(sz); !memory.zero() )
          return memory;
      }
      nd = nd->nxt;
    }
    return memory;
  }

  template <typename F>
  micron::__chunk<byte>
  __append_bucket_launder(F *nd, const usize sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    while ( nd != nullptr ) {
      __builtin_prefetch(nd->nxt, 0, 1);
      if ( memory = nd->nd->temporal_mark(sz); !memory.zero() )
        return memory;
      nd = nd->nxt;
    }
    return memory;
  }

  template <typename F>
  bool
  __find_and_tombstone(F *__node, F *&__tail, const micron::__chunk<byte> &memory)
  {
    F *__p_head = __node;
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( !__node->nd ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;
      if ( sh.is_at(reinterpret_cast<addr_t *>(memory.ptr)) ) [[unlikely]] {
        sh.try_tombstone(memory);
        usize ts = sh.tombstoned();
        usize ft = sh.ftotal();
        __debug_print("__find_and_tombstone(): tombstoned count: ", ts);
        __debug_print("__find_and_tombstone(): ftotal: ", ft);
        if ( (ts > (ft >> 1)) and sh.used() == 0 ) {
          __debug_print("__find_and_tombstone(): tombstone ratio exceeded threshold, resetting sheet", 0);
          sh.reset();
          __p_head->nxt = __node->nxt;
          // NOTE: MUST repair tail, dangling pointers otherwise
          if ( __node == __tail )
            __tail = __p_head;
          __unmark_arena<F>(reinterpret_cast<byte *>(__node));
        }
        return true;
      }
      __p_head = __node;
      __node = __node->nxt;
    }
    __debug_print_addr("__find_and_tombstone(): WARNING address not found in any sheet: ", memory.ptr);
    return false;
  }

  template <typename F>
  bool
  __find_and_tombstone(F *__node, F *&__tail, byte *addr)
  {
    F *__p_head = __node;
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( !__node->nd ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;
      if ( sh.is_at(reinterpret_cast<addr_t *>(addr)) ) [[unlikely]] {
        sh.try_tombstone_no_size(addr);
        usize ts = sh.tombstoned();
        usize ft = sh.ftotal();
        __debug_print("__find_and_tombstone() addr: tombstoned count: ", ts);
        __debug_print("__find_and_tombstone() addr: ftotal: ", ft);
        if ( (ts > (ft >> 1)) and sh.used() == 0 ) {
          __debug_print("__find_and_tombstone() addr: tombstone ratio exceeded threshold, resetting sheet", 0);
          sh.reset();
          __p_head->nxt = __node->nxt;
          if ( __node == __tail )
            __tail = __p_head;
          __unmark_arena<F>(reinterpret_cast<byte *>(__node));
        }
        return true;
      }
      __p_head = __node;
      __node = __node->nxt;
    }
    __debug_print_addr("__find_and_tombstone() addr: WARNING address not found in any sheet: ", addr);
    return false;
  }

  template <typename F>
  bool
  __find_and_remove_arena(F *__node, const micron::__chunk<byte> &memory)
  {
    __debug_print_addr("__find_and_remove_arena(): searching for ", memory.ptr);
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( !__node->nd ) {
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;
      if ( sh.is_at(reinterpret_cast<addr_t *>(memory.ptr)) ) [[unlikely]] {
        __debug_print_addr("__find_and_remove_arena(): found in sheet at addr: ", memory.ptr);
        if ( sh.try_unmark(memory) ) {
          __debug_print("__find_and_remove_arena(): unmark succeeded, sheet used: ", sh.used());
          return true;
        }
        __debug_print_addr("__find_and_remove_arena(): WARNING try_unmark failed for: ", memory.ptr);
      }
      __node = __node->nxt;
    }
    __debug_print_addr("__find_and_remove_arena(): WARNING address not found in any sheet: ", memory.ptr);
    // abort_state();
    // don't hard fail
    return false;
  }

  template <typename F>
  bool
  __find_and_remove(F *__node, F *&__tail, const micron::__chunk<byte> &memory)
  {
    F *__init_nd = __node;
    F *__p_head = __node;
    __debug_print_addr("__find_and_remove(): searching for ", memory.ptr);
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( !__node->nd ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;
      if ( sh.is_at(reinterpret_cast<addr_t *>(memory.ptr)) ) [[unlikely]] {
        __debug_print_addr("__find_and_remove(): found in sheet at addr: ", memory.ptr);
        if constexpr ( !__default_tombstone ) {
          if ( sh.try_unmark(memory) ) {
            __debug_print("__find_and_remove(): unmark succeeded, sheet used: ", sh.used());
            if ( sh.used() == 0 and __node != __init_nd ) {
              __debug_print("__find_and_remove(): sheet fully drained, unlinking and resetting", 0);
              sh.reset();
              __p_head->nxt = __node->nxt;
              // NOTE: must repair tail, dangling pointers otherwise
              if ( __node == __tail )
                __tail = __p_head;
              __unmark_arena<F>(reinterpret_cast<byte *>(__node));
            }
            return true;
          }
          __debug_print_addr("__find_and_remove(): WARNING try_unmark failed for: ", memory.ptr);
        } else {
          if ( sh.try_tombstone(memory) ) {
            usize ts = sh.tombstoned();
            usize ft = sh.ftotal();
            __debug_print("__find_and_remove(): tombstone set, tombstoned: ", ts);
            __debug_print("__find_and_remove(): ftotal: ", ft);
            if ( (ts > (ft >> 1)) and sh.used() == 0 ) {
              __debug_print("__find_and_remove(): tombstone threshold crossed, compacting sheet", 0);
              sh.reset();
              __p_head->nxt = __node->nxt;
              if ( __node == __tail )
                __tail = __p_head;
              __unmark_arena<F>(reinterpret_cast<byte *>(__node));
            }
            return true;
          }
        }
      }
      __p_head = __node;
      __node = __node->nxt;
    }
    __debug_print_addr("__find_and_remove(): WARNING address not found in any sheet: ", memory.ptr);
    return false;
  }

  template <typename F>
  bool
  __find_and_remove(F *__node, F *&__tail, byte *addr)
  {
    F *__init_nd = __node;
    F *__p_head = __node;
    __debug_print_addr("__find_and_remove(): searching for ", addr);
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( !__node->nd ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;
      if ( sh.is_at(reinterpret_cast<addr_t *>(addr)) ) [[unlikely]] {
        __debug_print_addr("__find_and_remove(): found in sheet at addr: ", addr);
        if constexpr ( !__default_tombstone ) {
          if ( sh.try_unmark_no_size(addr) ) {
            __debug_print("__find_and_remove() addr: unmark succeeded, sheet used: ", sh.used());
            if ( sh.used() == 0 and __node != __init_nd ) {
              __debug_print("__find_and_remove() addr: sheet fully drained, unlinking and resetting", 0);
              sh.reset();
              __p_head->nxt = __node->nxt;
              if ( __node == __tail )
                __tail = __p_head;
              __unmark_arena<F>(reinterpret_cast<byte *>(__node));
            }
            return true;
          }
          __debug_print_addr("__find_and_remove() addr: WARNING try_unmark_no_size failed for: ", addr);
        } else {
          if ( sh.try_tombstone_no_size(addr) ) {
            usize ts = sh.tombstoned();
            usize ft = sh.ftotal();
            __debug_print("__find_and_remove() addr: tombstone set, tombstoned: ", ts);
            __debug_print("__find_and_remove() addr: ftotal: ", ft);
            if ( ts > (ft >> 1) and sh.used() == 0 ) {
              __debug_print("__find_and_remove() addr: tombstone threshold crossed, compacting sheet", 0);
              sh.reset();
              __p_head->nxt = __node->nxt;
              if ( __node == __tail )
                __tail = __p_head;
              __unmark_arena<F>(reinterpret_cast<byte *>(__node));
            }
            return true;
          }
        }
      }
      __p_head = __node;
      __node = __node->nxt;
    }
    __debug_print_addr("__find_and_remove() addr: WARNING address not found in any sheet: ", addr);
    return false;
  }

  template <typename F>
  bool
  __find_and_freeze(F *__node, const micron::__chunk<byte> &memory)
  {
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( __node->nd && __node->nd->is_at(reinterpret_cast<addr_t *>(memory.ptr)) ) [[unlikely]] {
        __debug_print_addr("__find_and_freeze(): freezing sheet containing addr: ", memory.ptr);
        bool ok = __node->nd->freeze();
        __debug_print("__find_and_freeze(): freeze result: ", (usize)ok);
        return ok;
      }
      __node = __node->nxt;
    }
    __debug_print_addr("__find_and_freeze(): WARNING addr not found for freeze: ", memory.ptr);
    return false;
  }

  template <typename F>
  bool
  __find_and_freeze(F *__node, byte *addr)
  {
    while ( __node != nullptr ) {
      __builtin_prefetch(__node->nxt, 0, 1);
      if ( __node->nd && __node->nd->is_at(reinterpret_cast<addr_t *>(addr)) ) [[unlikely]] {
        __debug_print_addr("__find_and_freeze() addr: freezing sheet containing: ", addr);
        bool ok = __node->nd->freeze();
        __debug_print("__find_and_freeze() addr: freeze result: ", (usize)ok);
        return ok;
      }
      __node = __node->nxt;
    }
    __debug_print_addr("__find_and_freeze() addr: WARNING addr not found for freeze: ", addr);
    return false;
  }

  bool
  __vmap_freeze(const micron::__chunk<byte> &memory)
  {
    return __dispatch([&](auto *nd) { return __find_and_freeze(nd, memory); });
  }

  bool
  __vmap_freeze_at(byte *addr)
  {
    return __dispatch([&](auto *nd) { return __find_and_freeze(nd, addr); });
  }

  bool
  __vmap_tombstone(const micron::__chunk<byte> &m)
  {
    return __find_and_remove(&_cache_buffer, _tail_cache_buffer, m) or __find_and_remove(&_small_buckets, _tail_small_buckets, m)
           or __find_and_remove(&_medium_buckets, _tail_medium_buckets, m) or __find_and_remove(&_large_buckets, _tail_large_buckets, m)
           or __find_and_remove(&_huge_buckets, _tail_huge_buckets, m);
  }

  bool
  __vmap_tombstone_at(byte *addr)
  {
    return __find_and_tombstone(&_cache_buffer, _tail_cache_buffer, addr)
           or __find_and_tombstone(&_small_buckets, _tail_small_buckets, addr)
           or __find_and_tombstone(&_medium_buckets, _tail_medium_buckets, addr)
           or __find_and_tombstone(&_large_buckets, _tail_large_buckets, addr)
           or __find_and_tombstone(&_huge_buckets, _tail_huge_buckets, addr);
  }

  bool
  __vmap_within(addr_t *addr) const
  {
    return __dispatch([&](auto *nd) { return __within(nd, addr); });
  }

  bool
  __vmap_remove(const micron::__chunk<byte> &m)
  {
    return __find_and_remove(&_cache_buffer, _tail_cache_buffer, m) or __find_and_remove(&_small_buckets, _tail_small_buckets, m)
           or __find_and_remove(&_medium_buckets, _tail_medium_buckets, m) or __find_and_remove(&_large_buckets, _tail_large_buckets, m)
           or __find_and_remove(&_huge_buckets, _tail_huge_buckets, m);
  }

  bool
  __vmap_remove_at(byte *addr)
  {
    return __find_and_remove(&_cache_buffer, _tail_cache_buffer, addr) or __find_and_remove(&_small_buckets, _tail_small_buckets, addr)
           or __find_and_remove(&_medium_buckets, _tail_medium_buckets, addr)
           or __find_and_remove(&_large_buckets, _tail_large_buckets, addr) or __find_and_remove(&_huge_buckets, _tail_huge_buckets, addr);
  }

  bool
  __vmap_locate_at(addr_t *addr) const
  {
    if constexpr ( !__default_tombstone )
      return false;
    return __dispatch([&](auto *nd) { return __locate_at(nd, addr); });
  }

  hot_fn(micron::__chunk<byte>) __vmap_append_tail(const usize sz)
  {
    if ( sz <= __class_small ) {
      __debug_print("__vmap_append_tail(): tier=precise, sz: ", sz);
      return __append_bucket(_tail_cache_buffer, sz);
    }
    if ( sz < __class_medium ) {
      __debug_print("__vmap_append_tail(): tier=small, sz: ", sz);
      return __append_bucket(_tail_small_buckets, sz);
    }
    if ( sz <= __class_large ) {
      __debug_print("__vmap_append_tail(): tier=medium, sz: ", sz);
      return __append_bucket(_tail_medium_buckets, sz);
    }
    if ( sz <= __class_huge ) {
      __debug_print("__vmap_append_tail(): tier=large, sz: ", sz);
      return __append_bucket(_tail_large_buckets, sz);
    }
    __debug_print("__vmap_append_tail(): tier=huge, sz: ", sz);
    return __append_bucket(_tail_huge_buckets, sz);
  }

  micron::__chunk<byte>
  __vmap_append(const usize sz)
  {
    if ( sz < __class_medium )
      return __append_bucket(&_small_buckets, sz);
    if ( sz <= __class_large )
      return __append_bucket(&_medium_buckets, sz);
    if ( sz <= __class_huge )
      return __append_bucket(&_large_buckets, sz);
    return __append_bucket(&_huge_buckets, sz);
  }

  micron::__chunk<byte>
  __vmap_launder(const usize sz)
  {
    if ( sz < __class_medium )
      return __append_bucket_launder(&_small_buckets, sz);
    if ( sz <= __class_large )
      return __append_bucket_launder(&_medium_buckets, sz);
    if ( sz <= __class_huge )
      return __append_bucket_launder(&_large_buckets, sz);
    return __append_bucket_launder(&_huge_buckets, sz);
  }

  template <typename F>
  void
  __release(F &bucket)
  {
    auto *nd = &bucket;
    do {
      if ( nd->nd )
        nd->nd->release();
      nd = nd->nxt;
    } while ( nd != nullptr );
  }

  template <typename F, typename Fn, typename... Args>
  void
  __for_each_bckt_void(const F &bucket, Fn fn, Args &&...args) const
  {
    auto *nd = &bucket;
    do {
      if ( nd->nd )
        fn(nd, args...);
      nd = nd->nxt;
    } while ( nd != nullptr );
  }

  template <typename F, typename Fn, typename... Args>
  auto
  __for_each_bckt(const F &bucket, Fn fn, Args &&...args) const -> micron::lambda_return_t<decltype(fn)>
  {
    using Rt = micron::lambda_return_t<decltype(fn)>;
    Rt _ret{};
    auto *nd = &bucket;
    do {
      if ( nd->nd )
        _ret += fn(nd->nd, args...);
      nd = nd->nxt;
    } while ( nd != nullptr );
    return _ret;
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
      __release(_cache_buffer);
      __release(_small_buckets);
      __release(_medium_buckets);
      __release(_large_buckets);
      __release(_huge_buckets);
      __release(_arena_buffer);
      _arena_memory.release();
      __debug_print("~__arena(): all buckets released", 0);
    }
    /*
    Constructed objects (9.4) with static storage duration are destroyed and functions registered with std::atexit
are called as part of a call to std::exit (17.5). The call to std::exit is sequenced before the destructions
and the registered functions.
[Note 1 : Returning from main invokes std::exit (6.9.3.1). — end note]
 If the completion of the constructor or dynamic initialization of an object with static storage duration
strongly happens before that of another, the completion of the destructor of the second is sequenced before
the initiation of the destructor of the first. If the completion of the constructor or dynamic initialization of an
object with thread storage duration is sequenced before that of another, the completion of the destructor of
the second is sequenced before the initiation of the destructor of the first. If an object is initialized statically,
the object is destroyed in the same order as if the object was dynamically initialized. For an object of array or
class type, all subobjects of that object are destroyed before any block variable with static storage duration
initialized during the construction of the subobjects is destroyed. If the destruction of an object with static
or thread storage duration exits via an exception, the function std::terminate is called (14.6.2).
    * */
  }

  __arena(void)
      : _arena_memory(__get_kernel_chunk<micron::__chunk<byte>>(__default_arena_page_buf * __system_pagesize)),
        _cache_buffer{ nullptr, nullptr }, _tail_cache_buffer{ nullptr }, _small_buckets{ nullptr, nullptr },
        _tail_small_buckets{ nullptr }, _arena_buffer{ nullptr, nullptr }, _tail_arena_buffer{ nullptr },
        _medium_buckets{ nullptr, nullptr }, _tail_medium_buckets{ nullptr }, _large_buckets{ nullptr, nullptr },
        _tail_large_buckets{ nullptr }, _huge_buckets{ nullptr, nullptr }, _tail_huge_buckets{ nullptr }
  {
    micron::sysinfo info;
    u64 prealloc_size = micron::math::floor<u64>(static_cast<f32>(info.totalram) * __default_prealloc_factor);
    __debug_print("__arena(): total system RAM: ", info.totalram);
    __debug_print("__arena(): prealloc_factor target bytes: ", prealloc_size);
    __debug_print("__arena(): arena metadata buf size: ", __default_arena_page_buf * __system_pagesize);

    __init_bucket<__class_arena_internal>(_arena_buffer, _tail_arena_buffer, (__default_arena_page_buf * __system_pagesize));
    __init_tlsf_bucket<__class_precise>(_cache_buffer, _tail_cache_buffer, __default_cache_size_factor * __class_precise);

    if constexpr ( !__default_lazy_construct ) {
      u64 share_small = __prealloc_share<__class_small>(prealloc_size);
      u64 share_medium = __prealloc_share<__class_medium>(prealloc_size);
      __debug_print("__arena(): prealloc share small: ", share_small);
      __debug_print("__arena(): prealloc share medium: ", share_medium);
      __init_tlsf_bucket<__class_small>(_small_buckets, _tail_small_buckets, share_small);
      __init_bucket<__class_medium>(_medium_buckets, _tail_medium_buckets, share_medium);
      if constexpr ( __default_init_large_pages and !__is_constrained ) {
        u64 share_large = __prealloc_share<__class_large>(prealloc_size);
        u64 share_huge = __prealloc_share<__class_huge>(prealloc_size);
        __debug_print("__arena(): prealloc share large: ", share_large);
        __debug_print("__arena(): prealloc share huge: ", share_huge);
        __init_bucket<__class_large>(_large_buckets, _tail_large_buckets, share_large);
        __init_bucket<__class_huge>(_huge_buckets, _tail_huge_buckets, share_huge);
      } else {
        __debug_print("__arena(): large/huge pages skipped (constrained or disabled)", 0);
      }
    } else {
      __debug_print("__arena(): lazy construction enabled, deferring small/medium/large/huge init", 0);
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

    micron::__chunk<byte> memory;

    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      if ( memory = __vmap_append_tail(sz); !memory.zero() ) [[likely]] {
        __debug_print("push(): allocated bytes: ", memory.len);
        zero_on_alloc(memory.ptr, memory.len);
        sanitize_on_alloc(memory.ptr, memory.len);
        collect_stats<stat_type::total_memory_throughput>(memory.len);
        return memory;
      }

      __debug_print("push(): append failed, retry: ", i);
      __debug_print("push(): expanding for sz: ", sz);

      if ( sz <= __class_small ) {
        usize __next_sz = __calculate_space_cache(__default_cache_step);
        __debug_print("push(): precise/small path, expanding by: ", __next_sz);
        __buf_expand_exact(sz, __next_sz);
        continue;
      }

      if constexpr ( __is_constrained ) {
        if ( sz >= __class_medium ) {
          usize __next_sz = __calculate_space_medium(sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push() constrained: medium+ path, next_sz: ", __next_sz);
          __debug_print("push() constrained: predictor suggested: ", predicted);
          __buf_expand_exact(sz, predicted);
          continue;
        }
      }

      if constexpr ( !__is_constrained ) {
        if ( sz >= __class_medium && sz < __class_1mb ) {
          usize __next_sz = __calculate_space_medium(sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): medium path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          __buf_expand_exact(sz, predicted);
          continue;
        }
        if ( sz >= __class_1mb && sz < __class_gb ) {
          usize __next_sz = __calculate_space_huge(sz) * __default_overcommit;
          __predict += __next_sz;
          usize predicted = __predict.predict_size(__next_sz);
          __debug_print("push(): 1mb-gb path, next_sz: ", __next_sz);
          __debug_print("push(): predictor suggested: ", predicted);
          __buf_expand_exact(sz, predicted);
          continue;
        }
        if ( sz >= __class_gb ) {
          usize bulk = __calculate_space_bulk(sz);
          __debug_print("push(): bulk (>=gb) path, bulk_sz: ", bulk);
          __buf_expand_exact(sz, bulk);
          continue;
        }
      }

      {
        usize __next_sz = __calculate_space_small(sz) * __default_overcommit;
        __predict += __next_sz;
        usize predicted = __predict.predict_size(__next_sz);
        __debug_print("push(): fallback small path, next_sz: ", __next_sz);
        __debug_print("push(): predictor suggested: ", predicted);
        __buf_expand_exact(sz, predicted);
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

    micron::__chunk<byte> memory;
    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      if ( memory = __vmap_launder(sz); !memory.zero() ) {
        __debug_print("launder(): allocated bytes: ", memory.len);
        zero_on_alloc(memory.ptr, memory.len);
        sanitize_on_alloc(memory.ptr, memory.len);
        collect_stats<stat_type::total_memory_throughput>(memory.len);
        return memory;
      }
      __debug_print("launder(): launder failed, retry: ", i);
      if ( sz >= __class_medium and sz < __class_gb ) {
        usize next = __calculate_space_huge(sz) * __default_overcommit;
        __debug_print("launder(): medium-gb path, expanding: ", next);
        __buf_expand_exact(sz, next);
      } else if ( sz >= __class_gb ) {
        usize bulk = __calculate_space_bulk(sz);
        __debug_print("launder(): bulk path, expanding: ", bulk);
        __buf_expand_exact(sz, bulk);
      } else {
        usize next = __calculate_space_small(sz) * __default_overcommit;
        __debug_print("launder(): small path, expanding: ", next);
        __buf_expand_exact(sz, next);
      }
    }
    __debug_print("launder()!!!: all retries exhausted for size: ", sz);
    return { (byte *)-1, micron::numeric_limits<usize>::max() };
  }

  bool
  pop(const micron::__chunk<byte> &mem)
  {
    __debug_print_addr("pop() address: ", mem.ptr);
    if ( mem.zero() )
      return true;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) ) {
        __debug_print_addr("pop()!!!: provenance check failed for: ", mem.ptr);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(mem.len);
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
    if ( mem == nullptr )
      return true;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("pop()!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
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
    if ( !mem )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("pop(len)!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::total_memory_freed>(len);
    bool ok = __vmap_remove({ mem, len });
    __debug_print("pop(len): removal result: ", (usize)ok);
    return ok;
  }

  bool
  ts_pop(const micron::__chunk<byte> &mem)
  {
    if ( mem.zero() )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) ) {
        __debug_print_addr("ts_pop()!!!: provenance check failed for: ", mem.ptr);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(mem.len);
    full_on_free(mem.ptr, mem.len);
    zero_on_free(mem.ptr, mem.len);
    bool ok = __vmap_tombstone(mem);
    __debug_print("ts_pop(): tombstone result: ", (usize)ok);
    return ok;
  }

  bool
  ts_pop(byte *mem)
  {
    if ( !mem )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("ts_pop() addr!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::dealloc>();
    full_on_free(mem);
    zero_on_free(mem);
    bool ok = __vmap_tombstone_at(mem);
    __debug_print("ts_pop() addr: tombstone result: ", (usize)ok);
    return ok;
  }

  bool
  ts_pop(byte *mem, usize len)
  {
    if ( !mem )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) ) {
        __debug_print_addr("ts_pop(len)!!!: provenance check failed for: ", mem);
        return fail_state();
      }
    }
    collect_stats<stat_type::total_memory_freed>(len);
    bool ok = __vmap_tombstone({ mem, len });
    __debug_print("ts_pop(len): tombstone result: ", (usize)ok);
    return ok;
  }

  bool
  freeze(const micron::__chunk<byte> &mem)
  {
    if ( mem.zero() )
      return false;
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
    if ( !mem )
      return false;
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
    if ( !mem )
      return false;
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
    t += __for_each_bckt(_cache_buffer, [](tlsf_sheet<__class_precise> *const v) -> usize { return v->allocated(); });
    t += __for_each_bckt(_small_buckets, [](tlsf_sheet<__class_small> *const v) -> usize { return v->allocated(); });
    t += __for_each_bckt(_medium_buckets, [](sheet<__class_medium> *const v) -> usize { return v->allocated(); });
    t += __for_each_bckt(_large_buckets, [](sheet<__class_large> *const v) -> usize { return v->allocated(); });
    t += __for_each_bckt(_huge_buckets, [](sheet<__class_huge> *const v) -> usize { return v->allocated(); });
    __debug_print("total_usage(): aggregate allocated bytes: ", t);
    return t;
  }

  template <u64 Sz>
  usize
  total_usage_of_class(void) const
  {
    if constexpr ( Sz == __class_precise )
      return __for_each_bckt(_cache_buffer, [](tlsf_sheet<__class_precise> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_small )
      return __for_each_bckt(_small_buckets, [](tlsf_sheet<__class_small> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_medium )
      return __for_each_bckt(_medium_buckets, [](sheet<__class_medium> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_large )
      return __for_each_bckt(_large_buckets, [](sheet<__class_large> *const v) -> usize { return v->allocated(); });
    else if constexpr ( Sz == __class_huge )
      return __for_each_bckt(_huge_buckets, [](sheet<__class_huge> *const v) -> usize { return v->allocated(); });
    return 0;
  }

  void
  reset_page(byte *ptr)
  {
    __debug_print_addr("reset_page(): resetting all sheets containing: ", ptr);
    auto check = [](auto nd, byte *__ptr) -> void {
      while ( nd != nullptr ) {
        __builtin_prefetch(nd->nxt, 0, 1);
        if ( nd->nd && nd->nd->is_at(reinterpret_cast<addr_t *>(__ptr)) ) [[unlikely]]
          nd->nd->reset();
        nd = nd->nxt;
      }
    };
    __for_each_bckt_void(_cache_buffer, check, ptr);
    __for_each_bckt_void(_small_buckets, check, ptr);
    __for_each_bckt_void(_medium_buckets, check, ptr);
    __for_each_bckt_void(_large_buckets, check, ptr);
    __for_each_bckt_void(_huge_buckets, check, ptr);
  }

  usize
  __available_buffer(void) const
  {
    return _arena_memory.available();
  }

  usize
  __size_of_alloc(addr_t *addr) const
  {
    // tlsf classes: block header at ptr - __hdr_offset, first u32 is bsize
    if ( __within(&_cache_buffer, addr) or __within(&_small_buckets, addr) ) {
      u32 bsz = *reinterpret_cast<u32 *>(reinterpret_cast<byte *>(addr) - __hdr_offset);
      __debug_print("__size_of_alloc(): tlsf block size recovered: ", (usize)bsz);
      return (usize)bsz;
    }
    // buddy classes: order stored at metadata addr
    if ( __within(&_medium_buckets, addr) ) {
      i64 order_class = static_cast<i64>(*get_metadata_addr(addr));
      usize recovered = __class_medium << order_class;
      __debug_print("__size_of_alloc(): medium buddy order: ", order_class);
      __debug_print("__size_of_alloc(): recovered size: ", recovered);
      return recovered;
    }
    if ( __within(&_large_buckets, addr) ) {
      i64 order_class = static_cast<i64>(*get_metadata_addr(addr));
      usize recovered = __class_large << order_class;
      __debug_print("__size_of_alloc(): large buddy order: ", order_class);
      __debug_print("__size_of_alloc(): recovered size: ", recovered);
      return recovered;
    }
    if ( __within(&_huge_buckets, addr) ) {
      i64 order_class = static_cast<i64>(*get_metadata_addr(addr));
      usize recovered = __class_huge << order_class;
      __debug_print("__size_of_alloc(): huge buddy order: ", order_class);
      __debug_print("__size_of_alloc(): recovered size: ", recovered);
      return recovered;
    }
    __debug_print_addr("__size_of_alloc(): WARNING addr not found in any bucket: ", addr);
    return 0;
  }

  bool
  zeroed(void) const
  {
    return _cache_buffer.nd == nullptr and _cache_buffer.nxt == nullptr;
  }
};
};     // namespace abc
