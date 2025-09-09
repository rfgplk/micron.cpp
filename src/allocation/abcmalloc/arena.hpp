//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../closures.hpp"
#include "../../concepts.hpp"
#include "../../control.hpp"
#include "../../numerics.hpp"
#include "../../type_traits.hpp"
#include "../../types.hpp"
#include "../linux/kmemory.hpp"
#include "book.hpp"
#include "config.hpp"
#include "harden.hpp"
#include "hooks.hpp"
#include "oom.hpp"
#include "stats.hpp"

#ifndef __OPTIMIZE__
#include <iostream>
#endif

namespace abc
{

inline __attribute__((always_inline)) void
__debug_print(const char *str, const size_t n)
{
#ifndef __OPTIMIZE__
  // NOTE: we're using iostream since our io library depends on malloc()
  if constexpr ( __default_debug_notices ) {
    std::cout << "\033[34mabcmalloc[] debug: \033[0m " << str << " with size: " << n << std::endl;
  }
#endif
}

template <typename T>
inline __attribute__((always_inline)) void
__debug_print(const char *str, T& t)
{
#ifndef __OPTIMIZE__
  // NOTE: we're using iostream since our io library depends on malloc()
  if constexpr ( __default_debug_notices ) {
    std::cout << "\033[34mabcmalloc[] debug: \033[0m " << str << " at addr: " << static_cast<const void*>(t) << std::endl;
  }
#endif
}
class __arena
{
  template <typename T> struct alignas(16) node {
    T *nd;
    node<T> *nxt;
  };

  sheet<__class_arena_internal> _arena_memory;     // 2 MiB metadata buffer
  node<sheet<__class_arena_internal>> _arena_buffer;
  node<sheet<__class_arena_internal>> *_tail_arena_buffer;
  node<sheet<__class_small>> _small_buckets;
  node<sheet<__class_small>> *_tail_small_buckets;
  node<sheet<__class_medium>> _medium_buckets;
  node<sheet<__class_medium>> *_tail_medium_buckets;
  node<sheet<__class_large>> _large_buckets;     // only init if needed
  node<sheet<__class_large>> *_tail_large_buckets;
  node<sheet<__class_huge>> _huge_buckets;     // only init if needed
  node<sheet<__class_huge>> *_tail_huge_buckets;
  inline void
  __reload_arena_buf(void)
  {
    // if arena buf is full, double it's capacity (inefficient and naive but it works)
    __expand_bucket<__class_arena_internal>(&_arena_buffer, _tail_arena_buffer, _tail_arena_buffer->nd->allocated() * 2);
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_bucket(F *nd, G *&tail, const size_t sz)
  {
    while ( nd->nxt != nullptr ) {
      nd = nd->nxt;
    }
    micron::__chunk<byte> buf_nd = _arena_memory.try_mark(sizeof(node<sheet<Sz>>));
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    // _arena_memory is full and unable to slot in more memory
    if ( buf_nd.failed_allocation() or buf.failed_allocation() ) [[unlikely]] {
      // TODO: add error
      micron::abort();
    }
    nd->nxt = new (buf_nd.ptr) node<sheet<Sz>>();
    nd->nxt->nd = new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    nd->nxt->nxt = nullptr;
    tail = nd->nxt;
  }
  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_bucket(F *nd, G *&tail)
  {
    size_t sz = __calculate_desired_space(Sz);     //(__default_page_mul * __system_pagesize);
    while ( nd->nxt != nullptr ) {
      nd = nd->nxt;
    }
  retry_memory:
    micron::__chunk<byte> buf_nd = _tail_arena_buffer->nd->try_mark(sizeof(node<sheet<Sz>>));
    micron::__chunk<byte> buf = _tail_arena_buffer->nd->try_mark(sizeof(sheet<Sz>));
    // _arena_memory is full and unable to slot in more memory
    if ( buf_nd.failed_allocation() or buf.failed_allocation() ) [[unlikely]] {
      __reload_arena_buf();
      goto retry_memory;
    }
    nd->nxt = new (buf_nd.ptr) node<sheet<Sz>>();
    nd->nxt->nd = new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    nd->nxt->nxt = nullptr;
    tail = nd->nxt;
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __insert_guard(F *nd, G *&tail)
  {
    if constexpr ( __default_insert_guard_pages ) {
      __expand_bucket<Sz, F, G>(nd, tail, 4096);
      nd->nxt->nd->freeze(__default_guard_page_perms);
    }
  }
  void
  __buf_expand(const size_t hint_sz)
  {
    if ( hint_sz < __class_medium ) {
      __expand_bucket<__class_small>(&_small_buckets, _tail_small_buckets);
      __insert_guard<__class_small>(&_small_buckets, _tail_small_buckets);
    } else if ( hint_sz <= __class_large and hint_sz >= __class_medium ) {
      __expand_bucket<__class_medium>(&_medium_buckets, _tail_medium_buckets);
    } else if ( hint_sz <= __class_huge and hint_sz > __class_large ) {
      if ( _large_buckets.nd == nullptr ) [[unlikely]] {
        __init_bucket<__class_large>(_large_buckets, _tail_large_buckets);
      } else
        __expand_bucket<__class_large>(&_large_buckets, _tail_large_buckets);
    } else if ( hint_sz > __class_huge ) {
      if ( _huge_buckets.nd == nullptr ) [[unlikely]] {
        __init_bucket<__class_huge>(_huge_buckets, _tail_huge_buckets);
      } else
        __expand_bucket<__class_huge>(&_huge_buckets, _tail_huge_buckets);
    }
  }
  template <typename F>
  micron::__chunk<byte>
  __append_bucket(F *nd, const size_t sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    while ( nd != nullptr ) {
      auto &sh = *nd->nd;

      if ( memory = sh.mark(sz); !memory.zero() )
        return memory;
      nd = nd->nxt;
    }
    return memory;
  }
  template <typename F>
  bool
  __find_and_remove(F *__node, const micron::__chunk<byte> &memory)
  {
    F *__init_nd = __node;
    F *__p_head = __node;
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;

      if ( sh.is_at(memory.ptr) ) [[unlikely]] {
        if ( sh.try_unmark(memory) ) {
          if ( sh.used() == 0 and __node != __init_nd )     // always keep one sheet around
          {
            sh.reset();
            __p_head->nxt = __node->nxt;     // relink
          }
          return true;
        }
      }
      __p_head = __node;
      __node = __node->nxt;
    }
    return false;
  }
  template <typename F>
  bool
  __find_and_remove(F *__node, byte *addr)
  {
    F *__init_nd = __node;
    F *__p_head = __node;
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;     // safe

      if ( sh.is_at(addr) ) [[unlikely]] {
        if ( sh.try_unmark_no_size(addr) )
          if ( sh.used() == 0 and __node != __init_nd )     // always keep one sheet around
          {
            sh.reset();
            __p_head->nxt = __node->nxt;     // relink
          }
        return true;
      }
      __p_head = __node;
      __node = __node->nxt;
    }
    return false;
  }

  template <typename F>
  bool
  __find_and_freeze(F *__node, const micron::__chunk<byte> &memory)
  {
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;

      if ( sh.is_at(memory.ptr) ) [[unlikely]] {
        if ( sh.freeze() ) {
          return true;
        } else
          return false;
      }
      __node = __node->nxt;
    }
    return false;
  }

  template <typename F>
  bool
  __find_and_freeze(F *__node, byte *addr)
  {
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;     // safe

      if ( sh.is_at(addr) ) [[unlikely]] {
        return sh.freeze();
      }
      __node = __node->nxt;
    }
    return false;
  }
  bool
  __vmap_freeze(const micron::__chunk<byte> &memory)
  {
    if ( __find_and_freeze(&_small_buckets, memory) )
      return true;
    if ( __find_and_freeze(&_medium_buckets, memory) )
      return true;
    if ( __find_and_freeze(&_large_buckets, memory) )
      return true;
    if ( __find_and_freeze(&_huge_buckets, memory) )
      return true;
    return false;
  }
  bool
  __vmap_freeze_at(byte *addr)
  {
    if ( __find_and_freeze(&_small_buckets, addr) )
      return true;
    if ( __find_and_freeze(&_medium_buckets, addr) )
      return true;
    if ( __find_and_freeze(&_large_buckets, addr) )
      return true;
    if ( __find_and_freeze(&_huge_buckets, addr) )
      return true;
    return false;
  }

  bool
  __vmap_remove(const micron::__chunk<byte> &memory)
  {
    // TODO: this must match the offsets of the headers in free_list
    // clean this up

    if ( __find_and_remove(&_small_buckets, memory) )
      return true;
    if ( __find_and_remove(&_medium_buckets, memory) )
      return true;
    if ( __find_and_remove(&_large_buckets, memory) )
      return true;
    if ( __find_and_remove(&_huge_buckets, memory) )
      return true;
    /*if ( (memory.len + sizeof(micron::simd::i256)) < __class_medium ) {
      if ( __find_and_remove(&_small_buckets, memory) )
        return true;
    } else if ( ((memory.len + sizeof(micron::simd::i256)) <= __class_large)
                and (memory.len + sizeof(micron::simd::i256)) >= __class_medium ) {
      if ( __find_and_remove(&_medium_buckets, memory) )
        return true;
    } else if ( (memory.len + sizeof(micron::simd::i256)) <= __class_huge
                and (memory.len + sizeof(micron::simd::i256)) > __class_large ) {
      if ( __find_and_remove(&_large_buckets, memory) )
        return true;
    } else if ( (memory.len + sizeof(micron::simd::i256)) > __class_huge ) {
      if ( __find_and_remove(&_huge_buckets, memory) )
        return true;
    }*/
    return false;
  }
  bool
  __vmap_remove_at(byte *addr)
  {
    if ( __find_and_remove(&_small_buckets, addr) )
      return true;
    if ( __find_and_remove(&_medium_buckets, addr) )
      return true;
    if ( __find_and_remove(&_large_buckets, addr) )
      return true;
    if ( __find_and_remove(&_huge_buckets, addr) )
      return true;
    return false;
  }
  micron::__chunk<byte>
  __heap_grow(const size_t sz)
  {
    return { reinterpret_cast<byte *>(micron::sbrk(sz)), sz };
  }
  void
  __heap_shrink(const ssize_t sz)
  {
    micron::sbrk((-1) * sz);
  }
  micron::__chunk<byte>
  __vmap_append_tail(const size_t sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    if ( sz < __class_medium ) {
      memory = __append_bucket(_tail_small_buckets, sz);
    } else if ( sz <= __class_large and sz >= __class_medium ) {
      memory = __append_bucket(_tail_medium_buckets, sz);
    } else if ( sz <= __class_huge and sz > __class_large ) {
      memory = __append_bucket(_tail_large_buckets, sz);
    } else if ( sz > __class_huge ) {
      memory = __append_bucket(_tail_huge_buckets, sz);
    }
    // could be nullptr here
    return memory;
  }
  micron::__chunk<byte>
  __vmap_append(const size_t sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    if ( sz < __class_medium ) {
      memory = __append_bucket(&_small_buckets, sz);
    } else if ( sz <= __class_large and sz >= __class_medium ) {
      memory = __append_bucket(&_medium_buckets, sz);
    } else if ( sz <= __class_huge and sz > __class_large ) {
      memory = __append_bucket(&_large_buckets, sz);
    } else if ( sz > __class_huge ) {
      memory = __append_bucket(&_huge_buckets, sz);
    }
    // could be nullptr here
    return memory;
  }
  // TODO: add popping
  template <u64 Sz, typename F, typename G>
  void
  __init_bucket(F &bucket, G *&tail, size_t n = __default_page_mul * __system_pagesize)
  {
    if ( n == __default_page_mul * __system_pagesize ) {
      // override default size depending on size of bucket provided
      // larger classes will inherently store more data, therefore should be init'd with far more memory
      // __calculate_desired_space is in hooks.hpp
      n = __calculate_desired_space(Sz);
    }
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    bucket = { new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(n)), nullptr };
    tail = &bucket;
    if ( bucket.nd->empty() )
      micron::abort();
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
      if ( nd->nd ) {
        fn(nd, args...);
      }
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
      if ( nd->nd ) {
        _ret += fn(nd->nd, args...);
      }
      nd = nd->nxt;
    } while ( nd != nullptr );
    return _ret;
  }

public:
  // TODO: add cleanup
  ~__arena(void)
  {
    __release(_small_buckets);
    __release(_medium_buckets);
    __release(_large_buckets);
    __release(_huge_buckets);
    __release(_arena_buffer);
    _arena_memory.release();
  }
  __arena(void)
      : _arena_memory(__get_kernel_chunk<micron::__chunk<byte>>(__default_arena_page_buf * __system_pagesize)),
        _arena_buffer{ nullptr, nullptr }, _tail_arena_buffer{ nullptr }, _small_buckets{ nullptr, nullptr },
        _tail_small_buckets{ nullptr }, _medium_buckets{ nullptr, nullptr }, _tail_medium_buckets{ nullptr },
        _large_buckets{ nullptr, nullptr }, _tail_large_buckets{ nullptr }, _huge_buckets{ nullptr, nullptr },
        _tail_huge_buckets{ nullptr }
  {
    __init_bucket<__class_arena_internal>(_arena_buffer, _tail_arena_buffer,
                                          (__default_arena_page_buf * __system_pagesize));
    __init_bucket<__class_small>(_small_buckets, _tail_small_buckets);
    __init_bucket<__class_medium>(_medium_buckets, _tail_medium_buckets);
    if constexpr ( __default_init_large_pages ) {
      __init_bucket<__class_large>(_large_buckets, _tail_large_buckets);
      __init_bucket<__class_huge>(_huge_buckets, _tail_huge_buckets);
    }
  }
  __arena(const __arena &) = delete;
  __arena(__arena &&) = delete;
  __arena &operator=(const __arena &) = delete;
  __arena &operator=(__arena &&) = delete;

  micron::__chunk<byte>
  grow(const size_t sz)
  {
    collect_stats<stat_type::alloc>();
    collect_stats<stat_type::total_memory_req>(sz);
    if ( check_oom() ) [[unlikely]]
      micron::abort();

    micron::__chunk<byte> memory;
    if ( memory = __heap_grow(sz); !memory.zero() ) {
      zero_on_alloc(memory.ptr, memory.len);
      sanitize_on_alloc(memory.ptr, memory.len);
      collect_stats<stat_type::total_memory_throughput>(memory.len);
      return memory;
    }
    return { (byte *)-1, micron::numeric_limits<size_t>::max() };
  }

  void
  shrink(const ssize_t sz)
  {
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(sz);
    __heap_shrink(sz);
  }
  // main method for allocating/mallocing memory
  micron::__chunk<byte>
  push(const size_t sz)
  {
    __debug_print("push(): ", sz);
    collect_stats<stat_type::alloc>();
    collect_stats<stat_type::total_memory_req>(sz);
    if ( check_oom() ) [[unlikely]]
      micron::abort();
    micron::__chunk<byte> memory;
    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      {
        if ( memory = __vmap_append_tail(sz); !memory.zero() ) {
          __debug_print("__vmap_append() alloc'd: ", memory.len);
          zero_on_alloc(memory.ptr, memory.len);
          sanitize_on_alloc(memory.ptr, memory.len);
          collect_stats<stat_type::total_memory_throughput>(memory.len);
          return memory;
        }
        __debug_print("failed to __vmap_append(): ", sz);
        __buf_expand(sz);
      }
    }
    return { (byte *)-1, micron::numeric_limits<size_t>::max() };
  }
  // main methods for freeing memory
  bool
  pop(const micron::__chunk<byte> &mem)
  {
    __debug_print("pop() address: ", mem.ptr);
    __debug_print("pop() size: ", mem.len);
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(mem.len);
    full_on_free(mem.ptr, mem.len);
    zero_on_free(mem.ptr, mem.len);
    return __vmap_remove(mem);
  }
  bool
  pop(byte *mem)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    // TODO: add this
    // collect_stats<stat_type::total_memory_freed>(mem.len);
    collect_stats<stat_type::dealloc>();
    full_on_free(mem);
    zero_on_free(mem);
    return __vmap_remove_at(mem);
  }
  bool
  pop(byte *mem, size_t len)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    collect_stats<stat_type::total_memory_freed>(len);
    return __vmap_remove({ mem, len });
  }
  bool
  freeze(const micron::__chunk<byte> &mem)
  {
    return __vmap_freeze(mem);
  }
  bool
  freeze(byte *mem)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    return __vmap_freeze_at(mem);
  }
  bool
  freeze(byte *mem, size_t len)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    return __vmap_freeze({ mem, len });
  }

  size_t
  total_usage(void) const
  {
    size_t total = 0;
    total += __for_each_bckt(_small_buckets, [](sheet<__class_small> *const v) -> size_t { return v->allocated(); });
    total += __for_each_bckt(_medium_buckets, [](sheet<__class_medium> *const v) -> size_t { return v->allocated(); });
    total += __for_each_bckt(_large_buckets, [](sheet<__class_large> *const v) -> size_t { return v->allocated(); });
    total += __for_each_bckt(_huge_buckets, [](sheet<__class_huge> *const v) -> size_t { return v->allocated(); });
    return total;
  }
  void
  reset_page(byte *ptr)
  {
    auto check = [](auto nd, byte *__ptr) -> void {
      while ( nd != nullptr ) {
        if ( nd->nd == nullptr ) {
          nd = nd->nxt;
          continue;
        }
        auto &sh = *nd->nd;     // safe

        if ( sh.is_at(__ptr) ) [[unlikely]]
          sh.reset();
        nd = nd->nxt;
      }
    };

    __for_each_bckt_void(_small_buckets, check, ptr);
    __for_each_bckt_void(_medium_buckets, check, ptr);
    __for_each_bckt_void(_large_buckets, check, ptr);
    __for_each_bckt_void(_huge_buckets, check, ptr);
  }
  size_t
  __available_buffer(void) const
  {
    return _arena_memory.available();
  }
};
};
