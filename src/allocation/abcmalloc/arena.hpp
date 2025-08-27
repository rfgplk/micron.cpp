//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/new.hpp"
#include "../../numerics.hpp"
#include "../../types.hpp"
#include "../../vector/svector.hpp"
#include "../linux/allocate_map.hpp"
#include "book.hpp"
#include "config.hpp"
#include "hooks.hpp"

namespace abc
{

class __arena
{
  template <typename T> struct alignas(16) node {
    T *nd;
    node<T> *nxt;
  };

  sheet<__class_arena_internal> _arena_memory;     // 2 MiB metadata buffer
  node<sheet<__class_small>> _small_buckets;
  node<sheet<__class_small>> *_tail_small_buckets;
  node<sheet<__class_medium>> _medium_buckets;
  node<sheet<__class_medium>> *_tail_medium_buckets;
  node<sheet<__class_large>> _large_buckets;     // only init if needed
  node<sheet<__class_large>> *_tail_large_buckets;
  node<sheet<__class_huge>> _huge_buckets;     // only init if needed
  node<sheet<__class_huge>> *_tail_huge_buckets;
  void
  __reload_arena_buf(void)
  {
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_bucket(F *nd, G *&tail)
  {
    size_t sz = (__default_page_mul * __system_pagesize);
    while ( nd->nxt != nullptr ) {
      nd = nd->nxt;
    }
    micron::__chunk<byte> buf_nd = _arena_memory.try_mark(sizeof(node<sheet<Sz>>));
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    // _arena_memory is full and unable to slot in more memory
    if ( buf_nd.zero() or buf.zero() ) [[unlikely]] {
      micron::abort();
    }
    nd->nxt = new (buf_nd.ptr) node<sheet<Sz>>();
    // reinterpret_cast<node<sheet<__class_small>> *>(buf_nd.ptr);
    nd->nxt->nd = new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    nd->nxt->nxt = nullptr;
    tail = nd->nxt;
  }
  void
  __buf_expand(const size_t hint_sz)
  {
    //= hint_sz <= (__default_page_mul * __system_pagesize)
    //    ? (__default_page_mul * __system_pagesize)
    //  : ((size_t)(micron::math::ceil((float)hint_sz / (float)(__default_page_mul * __system_pagesize))) + 1)
    //      * (__default_page_mul * __system_pagesize);
    if ( hint_sz < __class_medium ) {
      __expand_bucket<__class_small>(&_small_buckets, _tail_small_buckets);
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
  micron::__chunk<byte>
  __heap_append_tail(const size_t sz)
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
  __heap_append(const size_t sz)
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
      __calculate_desired_space(n);
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

public:
  // TODO: add cleanup
  ~__arena(void)
  {
    __release(_small_buckets);
    __release(_medium_buckets);
    __release(_large_buckets);
    __release(_huge_buckets);
    _arena_memory.release();
  }
  __arena(void)
      : _arena_memory(__get_kernel_chunk<micron::__chunk<byte>>(__default_arena_page_buf * __system_pagesize)),
        _small_buckets{ nullptr, nullptr }, _tail_small_buckets{ nullptr }, _medium_buckets{ nullptr, nullptr },
        _tail_medium_buckets{ nullptr }, _large_buckets{ nullptr, nullptr }, _tail_large_buckets{ nullptr },
        _huge_buckets{ nullptr, nullptr }, _tail_huge_buckets{ nullptr }
  {
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
  append(const size_t sz)
  {
    micron::__chunk<byte> memory;
    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      if ( memory = __heap_append_tail(sz); !memory.zero() )
        return memory;
      __buf_expand(sz);
    }
    return { (byte *)-1, micron::numeric_limits<size_t>::max() };
  }
  size_t
  __available_buffer(void) const
  {
    return _arena_memory.available();
  }
};
};
