//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/new.hpp"
#include "../../types.hpp"
#include "../../vector/svector.hpp"
#include "../linux/allocate_map.hpp"
#include "book.hpp"
#include "config.hpp"

namespace abc
{

inline u64
__calculate_desired_space(u64 pg, u64 pg_mul = __default_page_mul)
{
  u32 mul = micron::math::ceil((float)pg / (float)pg_mul);
  if ( mul > 1 ) {
    return (mul + 1) * pg_mul;
  }
  return mul * pg_mul;
}

void *
__get_kernel_memory(u64 sz)
{
  return micron::std_allocator<byte>::alloc(sz);
}

class __arena
{
  template <typename T> struct alignas(16) node {
    T *nd;
    node<T> *nxt;
  };
  sheet<__class_arena_internal, __default_arena_page_buf> _arena_memory;     // 1 MiB to store above nodes
  // 2.0 GiB in stack alloc'd fast access cache.
  // micron::svector<sheet<__class_small>, __default_cached_opt> _small_buckets;
  // rest is dynamic, bootstapped heap alloc'd
  node<sheet<__class_small>> _small_buckets;
  node<sheet<__class_medium>> _medium_buckets;
  node<sheet<__class_large>> _large_buckets;
  node<sheet<__class_huge>> _huge_buckets;
  void
  __buf_expand(const size_t sz = __default_arena_page_buf)
  {
    if ( sz < __class_medium ) {
      auto *nd = &_small_buckets;
      while ( nd != nullptr )
        nd = nd->nxt;
      micron::__chunk<byte> buf_nd = _arena_memory.try_mark(sizeof(node<sheet<__class_small>>));
      micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<__class_small>));
      nd = reinterpret_cast<node<sheet<__class_small>> *>(buf_nd.ptr);
      nd->nd = new (buf.ptr)
          sheet<__class_small>(__get_kernel_memory(__calculate_desired_space(sz)), __calculate_desired_space(sz));
      nd->nxt = nullptr;
    } else if ( sz <= __class_large and sz >= __class_medium ) {
      auto *nd = &_medium_buckets;
      while ( nd != nullptr )
        nd = nd->nxt;
      micron::__chunk<byte> buf_nd = _arena_memory.try_mark(sizeof(node<sheet<__class_medium>>));
      micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<__class_medium>));
      nd = reinterpret_cast<node<sheet<__class_medium>> *>(buf_nd.ptr);
      nd->nd = new (buf.ptr)
          sheet<__class_medium>(__get_kernel_memory(__calculate_desired_space(sz)), __calculate_desired_space(sz));
      nd->nxt = nullptr;
    } else if ( sz <= __class_huge and sz > __class_large ) {
      auto *nd = &_large_buckets;
      while ( nd != nullptr )
        nd = nd->nxt;
      micron::__chunk<byte> buf_nd = _arena_memory.try_mark(sizeof(node<sheet<__class_large>>));
      micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<__class_large>));
      nd = reinterpret_cast<node<sheet<__class_large>> *>(buf_nd.ptr);
      nd->nd = new (buf.ptr)
          sheet<__class_large>(__get_kernel_memory(__calculate_desired_space(sz)), __calculate_desired_space(sz));
      nd->nxt = nullptr;
    } else if ( sz > __class_huge ) {
      auto *nd = &_huge_buckets;
      while ( nd != nullptr )
        nd = nd->nxt;
      micron::__chunk<byte> buf_nd = _arena_memory.try_mark(sizeof(node<sheet<__class_huge>>));
      micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<__class_huge>));
      nd = reinterpret_cast<node<sheet<__class_huge>> *>(buf_nd.ptr);
      nd->nd = new (buf.ptr)
          sheet<__class_huge>(__get_kernel_memory(__calculate_desired_space(sz)), __calculate_desired_space(sz));
      nd->nxt = nullptr;
    }
  }
  micron::__chunk<byte>
  __heap_append(const size_t sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    if ( sz < __class_small ) {
      auto *nd = &_small_buckets;
      while ( nd != nullptr ) {
        auto &sh = *nd->nd;

        if ( memory = sh.try_mark(sz); !memory.zero() )
          return memory;
        nd = nd->nxt;
      }
    } else if ( sz <= __class_large and sz >= __class_medium ) {
      auto *nd = &_medium_buckets;
      while ( nd != nullptr ) {
        auto &sh = *nd->nd;

        if ( memory = sh.try_mark(sz); !memory.zero() )
          return memory;
        nd = nd->nxt;
      }
    } else if ( sz <= __class_huge and sz > __class_large ) {
      auto *nd = &_large_buckets;
      while ( nd != nullptr ) {
        auto &sh = *nd->nd;

        if ( memory = sh.try_mark(sz); !memory.zero() )
          return memory;
        nd = nd->nxt;
      }
    } else if ( sz > __class_huge ) {
      auto *nd = &_huge_buckets;
      while ( nd != nullptr ) {
        auto &sh = *nd->nd;

        if ( memory = sh.try_mark(sz); !memory.zero() )
          return memory;
        nd = nd->nxt;
      }
    }
    return memory;
  }
  // TODO: add popping
public:
  ~__arena(void) {}     //_small_buckets.~svector<sheet<__class_small>, __default_cached_opt>(); }
  __arena(void) : _arena_memory()
  {
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<__class_small>));
    _small_buckets = { new (buf.ptr) sheet<__class_small>(), nullptr };
    // init medium
    buf = _arena_memory.try_mark(sizeof(sheet<__class_medium>));
    _medium_buckets = { new (buf.ptr) sheet<__class_medium>(), nullptr };
    // init large
    buf = _arena_memory.try_mark(sizeof(sheet<__class_large>));
    _large_buckets = { new (buf.ptr) sheet<__class_large>(), nullptr };
    // init huge
    buf = _arena_memory.try_mark(sizeof(sheet<__class_huge>));
    _huge_buckets = { new (buf.ptr) sheet<__class_huge>(), nullptr };
  }
  __arena(const __arena &) = delete;
  __arena(__arena &&) = delete;
  __arena &operator=(const __arena &) = delete;
  __arena &operator=(__arena &&) = delete;
  micron::__chunk<byte>
  append(const size_t sz)
  {
    micron::__chunk<byte> memory;
    /*
    if ( sz <= __default_cache_limit and sz <= __class_large ) {
      // try to slot memory in cached region first
      if ( _small_buckets.size() )
        for ( auto &n : _small_buckets )
          if ( memory = n.try_mark(sz); !memory.zero() )
            return memory;
      // append if failed or empty
      _small_buckets.push_back(__get_kernel_memory(sz), sz);
      if ( memory = _small_buckets.back().try_mark(sz); !memory.zero() )
        return memory;
    }*/
    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      if ( memory = __heap_append(sz); !memory.zero() )
        return memory;
      __buf_expand(sz);
    }
    return { nullptr, 0 };
  }
};

};
