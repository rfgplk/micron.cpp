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
#include "../../../control.hpp"
#include "../../../numerics.hpp"
#include "../../../type_traits.hpp"
#include "../../../types.hpp"
#include "../kmemory.hpp"

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

// must be here

template <typename P>
void *
operator new(usize size, P *ptr)
{
  (void)size;
  return ptr;
}

namespace abc
{

class __arena : private cache
{
  template <typename T> struct alignas(16) node {
    T *nd;
    node<T> *nxt;
  };

  alloc_predictor __predict;
  sheet<__class_arena_internal> _arena_memory;
  node<sheet<__class_precise>> _cache_buffer;
  node<sheet<__class_arena_internal>> _arena_buffer;
  node<sheet<__class_arena_internal>> *_tail_arena_buffer;
  node<sheet<__class_small>> _small_buckets;
  node<sheet<__class_small>> *_tail_small_buckets;
  node<sheet<__class_medium>> _medium_buckets;
  node<sheet<__class_medium>> *_tail_medium_buckets;
  node<sheet<__class_large>> _large_buckets;
  node<sheet<__class_large>> *_tail_large_buckets;
  node<sheet<__class_huge>> _huge_buckets;
  node<sheet<__class_huge>> *_tail_huge_buckets;

  void
  __reload_arena_buf(void)
  {
    // if arena buf is full, double it's capacity (inefficient and naive but it works)
    __expand_bucket_arena<__class_arena_internal>(&_arena_buffer, _tail_arena_buffer, _tail_arena_buffer->nd->allocated() * 2);
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_bucket_arena(F *nd, G *&tail, const usize sz)
  {
    while ( nd->nxt != nullptr ) {
      nd = nd->nxt;
    }
    __debug_print("__expand_bucket_arena() with req. space: ", sz);
    micron::__chunk<byte> buf_nd = _arena_memory.try_mark(sizeof(node<sheet<Sz>>));
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    // _arena_memory is full and unable to slot in more memory
    if ( buf_nd.failed_allocation() or buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__expand_bucket_arena() failed to reallocate arena with size: ", sz);
      abort_state();
    }
    nd->nxt = new (buf_nd.ptr) node<sheet<Sz>>();
    nd->nxt->nd = new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    nd->nxt->nxt = nullptr;
    tail = nd->nxt;
  }

  template <u64 Sz, typename F>
  inline __attribute__((always_inline)) void
  __expand_cache(F *nd, const usize sz)
  {
    __debug_print("__expand_cache() with req. space: ", sz);
    while ( nd->nxt != nullptr ) {
      nd = nd->nxt;
    }
  retry_memory_e:
    micron::__chunk<byte> buf_nd = _tail_arena_buffer->nd->try_mark(sizeof(node<sheet<Sz>>));
    micron::__chunk<byte> buf = _tail_arena_buffer->nd->try_mark(sizeof(sheet<Sz>));
    // _arena_memory is full and unable to slot in more memory
    if ( buf_nd.failed_allocation() or buf.failed_allocation() ) [[unlikely]] {
      __reload_arena_buf();
      goto retry_memory_e;
    }
    nd->nxt = new (buf_nd.ptr) node<sheet<Sz>>();
    nd->nxt->nd = new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    nd->nxt->nxt = nullptr;
  }

  template <u64 Sz, typename F, typename G>
  inline __attribute__((always_inline)) void
  __expand_bucket(F *nd, G *&tail, const usize sz)
  {
    __debug_print("__expand_bucket() with req. space: ", sz);
    while ( nd->nxt != nullptr ) {
      nd = nd->nxt;
    }
  retry_memory_e:
    micron::__chunk<byte> buf_nd = _tail_arena_buffer->nd->try_mark(sizeof(node<sheet<Sz>>));
    micron::__chunk<byte> buf = _tail_arena_buffer->nd->try_mark(sizeof(sheet<Sz>));
    // _arena_memory is full and unable to slot in more memory
    if ( buf_nd.failed_allocation() or buf.failed_allocation() ) [[unlikely]] {
      __debug_print("__expand_bucket(): allocation failed (memory was full most likely)", 0);
      __reload_arena_buf();
      goto retry_memory_e;
    }
    nd->nxt = new (buf_nd.ptr) node<sheet<Sz>>();
    if constexpr ( __default_insert_guard_pages ) {
      // plus an additional for the guard page
      auto chnk = __get_kernel_chunk<micron::__chunk<byte>>(sz + __system_pagesize);
      __make_guard(chnk);
      nd->nxt->nd = new (buf.ptr) sheet<Sz>(chnk, __system_pagesize);
    } else {
      nd->nxt->nd = new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(sz));
    }
    nd->nxt->nxt = nullptr;
    if ( tail )
      tail = nd->nxt;
  }

  inline __attribute__((always_inline)) void
  __make_guard(micron::__chunk<byte> &mem)
  {
    __debug_print_addr("__make_guard() @", mem.ptr + (mem.len - __system_pagesize));

    if ( long int r = micron::mprotect(mem.ptr + (mem.len - __system_pagesize), __system_pagesize, __default_guard_page_perms); r != 0 ) {
      __debug_print("Failed to make guard page", r);
      abort_state();
    }
  }

  void
  __buf_expand_exact(const usize class_sz, const usize exact_sz)
  {
    if ( class_sz <= __class_small ) {
      __expand_cache<__class_precise>(&_cache_buffer, exact_sz);
      return;
    }

    if ( class_sz < __class_medium ) [[likely]] {
      if constexpr ( __default_lazy_construct ) {
        if ( _small_buckets.nd == nullptr ) [[unlikely]]
          __init_bucket<__class_small>(_small_buckets, _tail_small_buckets);
        else
          __expand_bucket<__class_small>(&_small_buckets, _tail_small_buckets, exact_sz);
      } else {
        __expand_bucket<__class_small>(&_small_buckets, _tail_small_buckets, exact_sz);
      }
      return;
    }

    if ( class_sz <= __class_large ) {
      if constexpr ( __default_lazy_construct ) {
        if ( _medium_buckets.nd == nullptr ) [[unlikely]]
          __init_bucket<__class_medium>(_medium_buckets, _tail_medium_buckets);
        else
          __expand_bucket<__class_medium>(&_medium_buckets, _tail_medium_buckets, exact_sz);
      } else {
        __expand_bucket<__class_medium>(&_medium_buckets, _tail_medium_buckets, exact_sz);
      }
      return;
    }

    if ( class_sz <= __class_huge ) {
      if ( _large_buckets.nd == nullptr ) [[unlikely]]
        __init_bucket<__class_large>(_large_buckets, _tail_large_buckets);
      else
        __expand_bucket<__class_large>(&_large_buckets, _tail_large_buckets, exact_sz);
      return;
    }

    // class_sz > __class_huge
    if ( _huge_buckets.nd == nullptr ) [[unlikely]]
      __init_bucket<__class_huge>(_huge_buckets, _tail_huge_buckets);
    else
      __expand_bucket<__class_huge>(&_huge_buckets, _tail_huge_buckets, exact_sz);
  }

  template <typename F>
  bool
  __within(F *__node, addr_t *memory) const
  {
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;
      if ( sh.is_at(memory) ) [[unlikely]]
        return true;
      __node = __node->nxt;
    }
    return false;
  }

  template <typename F>
  bool
  __locate_at(F *__node, addr_t *memory) const
  {
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;

      if ( sh.find(reinterpret_cast<byte *>(memory)) ) [[unlikely]] {
        return true;
      }
      __node = __node->nxt;
    }
    return false;
  }

  template <typename F>
  bool
  __locate(F *__node, const micron::__chunk<byte> &memory) const
  {
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;

      if ( sh.find(memory.ptr) ) [[unlikely]] {
        return true;
      }
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
      auto &sh = *nd->nd;
      if constexpr ( __default_launder ) {
        if ( memory = sh.temporal_mark(sz); !memory.zero() )
          return memory;
      } else if constexpr ( __default_launder == false ) {

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
      auto &sh = *nd->nd;
      if ( memory = sh.temporal_mark(sz); !memory.zero() )
        return memory;
      nd = nd->nxt;
    }
    return memory;
  }

  template <typename F>
  bool
  __find_and_tombstone(F *__node, const micron::__chunk<byte> &memory)
  {
    // F *__init_nd = __node;
    F *__p_head = __node;
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;

      if ( sh.is_at(reinterpret_cast<addr_t *>(memory.ptr)) ) [[unlikely]] {
        if ( sh.try_tombstone(memory) ) {
          // work with tombstoning properly
          // if tombstoned fraction is more than 50% available mem
          if ( sh.tombstoned() > (sh.ftotal() >> 1) and sh.used() == 0 )     // ignores root sheet req
          {
            sh.reset();
            __p_head->nxt = __node->nxt;     // relink
          }
          return true;
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
  __find_and_tombstone(F *__node, byte *addr)
  {
    // F *__init_nd = __node;
    F *__p_head = __node;
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;     // safe

      if ( sh.is_at(reinterpret_cast<addr_t *>(addr)) ) [[unlikely]] {
        if ( sh.try_tombstone_no_size(addr) )
          // if tombstoned fraction is more than 50% available mem
          if ( sh.tombstoned() > (sh.ftotal() >> 1) and sh.used() == 0 )     // ignores root sheet req
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
  __find_and_remove(F *__node, const micron::__chunk<byte> &memory)
  {
    F *__init_nd = __node;
    F *__p_head = __node;
    __debug_print_addr("__find_and_remove(): searching for ", memory.ptr);
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;

      if ( sh.is_at(reinterpret_cast<addr_t *>(memory.ptr)) ) [[unlikely]] {
        __debug_print_addr("found ", memory.ptr);
        if constexpr ( !__default_tombstone ) {
          if ( sh.try_unmark(memory) ) {
            if ( sh.used() == 0 and __node != __init_nd )     // always keep one sheet around
            {
              __debug_print_addr("__find_and_remove(): wiping sheet @", __node);
              sh.reset();
              __p_head->nxt = __node->nxt;     // relink
            }
            return true;
          }
        } else {
          if ( sh.try_tombstone(memory) ) {
            // if tombstoned fraction is more than 50% available mem
            if ( sh.tombstoned() > (sh.ftotal() >> 1) and sh.used() == 0 ) {
              __debug_print_addr("__find_and_remove(): wiping sheet @", __node);
              sh.reset();
              __p_head->nxt = __node->nxt;     // relink
            }
            return true;
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
    __debug_print_addr("__find_and_remove(): searching for ", addr);
    while ( __node != nullptr ) {
      if ( __node->nd == nullptr ) {
        __p_head = __node;
        __node = __node->nxt;
        continue;
      }
      auto &sh = *__node->nd;     // safe

      if ( sh.is_at(reinterpret_cast<addr_t *>(addr)) ) [[unlikely]] {
        __debug_print_addr("found ", addr);
        if constexpr ( !__default_tombstone ) {
          if ( sh.try_unmark_no_size(addr) ) {
            __debug_print("usage of sheet = ", sh.used());
            if ( sh.used() == 0 and __node != __init_nd )     // always keep one sheet around
            {
              __debug_print_addr("__find_and_remove(): wiping sheet @", __node);
              sh.reset();
              __p_head->nxt = __node->nxt;     // relink
            } else
              __debug_print_addr("__find_and_remove(): sheet is root @", __node);
            return true;
          }
        } else {
          if ( sh.try_tombstone_no_size(addr) ) {
            __debug_print("usage of sheet = ", sh.ftotal());
            // if tombstoned fraction is more than 50% available mem
            if ( sh.tombstoned() > (sh.ftotal() >> 1) and sh.used() == 0 ) {
              __debug_print_addr("__find_and_remove(): wiping sheet @", __node);
              sh.reset();
              __p_head->nxt = __node->nxt;     // relink
            } else
              __debug_print_addr("__find_and_remove(): sheet is root @", __node);
            return true;
          }
        }
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

      if ( sh.is_at(reinterpret_cast<addr_t *>(memory.ptr)) ) [[unlikely]] {
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

      if ( sh.is_at(reinterpret_cast<addr_t *>(addr)) ) [[unlikely]] {
        return sh.freeze();
      }
      __node = __node->nxt;
    }
    return false;
  }

  bool
  __vmap_freeze(const micron::__chunk<byte> &memory)
  {
    if ( __find_and_freeze(&_cache_buffer, memory) )
      return true;
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
    if ( __find_and_freeze(&_cache_buffer, addr) )
      return true;
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
  __vmap_tombstone(const micron::__chunk<byte> &memory)
  {
    if ( __find_and_remove(&_cache_buffer, memory) )
      return true;
    if ( __find_and_remove(&_small_buckets, memory) )
      return true;
    if ( __find_and_remove(&_medium_buckets, memory) )
      return true;
    if ( __find_and_remove(&_large_buckets, memory) )
      return true;
    if ( __find_and_remove(&_huge_buckets, memory) )
      return true;
    return false;
  }

  bool
  __vmap_tombstone_at(byte *addr)
  {
    if ( __find_and_tombstone(&_cache_buffer, addr) )
      return true;
    if ( __find_and_tombstone(&_small_buckets, addr) )
      return true;
    if ( __find_and_tombstone(&_medium_buckets, addr) )
      return true;
    if ( __find_and_tombstone(&_large_buckets, addr) )
      return true;
    if ( __find_and_tombstone(&_huge_buckets, addr) )
      return true;
    return false;
  }

  bool
  __vmap_within(addr_t *addr) const
  {
    if ( __within(&_cache_buffer, addr) )
      return true;
    if ( __within(&_small_buckets, addr) )
      return true;
    if ( __within(&_medium_buckets, addr) )
      return true;
    if ( __within(&_large_buckets, addr) )
      return true;
    if ( __within(&_huge_buckets, addr) )
      return true;
    return false;
  }

  bool
  __vmap_locate_at(addr_t *addr) const
  {
    if constexpr ( !__default_tombstone )
      return false;
    if ( __locate_at(&_cache_buffer, addr) )
      return true;
    if ( __locate_at(&_small_buckets, addr) )
      return true;
    if ( __locate_at(&_medium_buckets, addr) )
      return true;
    if ( __locate_at(&_large_buckets, addr) )
      return true;
    if ( __locate_at(&_huge_buckets, addr) )
      return true;
    return false;
  }

  bool
  __vmap_remove(const micron::__chunk<byte> &memory)
  {
    if ( __find_and_remove(&_cache_buffer, memory) )
      return true;
    if ( __find_and_remove(&_small_buckets, memory) )
      return true;
    if ( __find_and_remove(&_medium_buckets, memory) )
      return true;
    if ( __find_and_remove(&_large_buckets, memory) )
      return true;
    if ( __find_and_remove(&_huge_buckets, memory) )
      return true;
    return false;
  }

  bool
  __vmap_remove_at(byte *addr)
  {
    if ( __find_and_remove(&_cache_buffer, addr) )
      return true;
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

  hot_fn(micron::__chunk<byte>) __append_cache(const usize sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    abc::__arena::node<abc::sheet<256>> *nd = &_cache_buffer;
    while ( nd != nullptr ) {
      auto &sh = *nd->nd;
      if constexpr ( __default_launder ) {
        if ( memory = sh.temporal_mark(sz); !memory.zero() )
          return memory;
      } else if constexpr ( __default_launder == false ) {

        if ( memory = sh.mark(sz); !memory.zero() )
          return memory;
      }
      nd = nd->nxt;
    }
    return memory;
  }

  micron::__chunk<byte>
  __vmap_append_tail(const usize sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    if ( sz <= __class_small ) {
      memory = __append_cache(sz);
    } else if ( sz < __class_medium ) {
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
  __vmap_append(const usize sz)
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

  micron::__chunk<byte>
  __vmap_launder(const usize sz)
  {
    micron::__chunk<byte> memory = { nullptr, 0 };
    if ( sz < __class_medium ) {
      memory = __append_bucket_launder(&_small_buckets, sz);
    } else if ( sz <= __class_large and sz >= __class_medium ) {
      memory = __append_bucket_launder(&_medium_buckets, sz);
    } else if ( sz <= __class_huge and sz > __class_large ) {
      memory = __append_bucket_launder(&_large_buckets, sz);
    } else if ( sz > __class_huge ) {
      memory = __append_bucket_launder(&_huge_buckets, sz);
    }
    // could be nullptr here
    return memory;
  }

  template <u64 Sz, typename F>
  void
  __init_cache(F &bucket, usize n = __default_cache_size_factor * Sz)
  {
    __debug_print("__init_bucket(): ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    bucket = { new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(n)), nullptr };
    if ( bucket.nd->empty() )
      abort_state();
  }

  template <u64 Sz, typename F, typename G>
  void
  __init_bucket(F &bucket, G *&tail, usize n = __default_magic_size)
  {
    if ( n == __default_magic_size ) {
      // override default size depending on size of bucket provided
      // larger classes will inherently store more data, therefore should be init'd with far more memory
      // __calculate_space_x is in hooks.hpp
      if ( Sz >= __class_medium ) {
        n = __calculate_space_huge(Sz);
      } else {     // for all other allocs, grow the most aggressive
        n = __calculate_space_small(Sz);
      }
    }
    __debug_print("__init_bucket(): ", n);
    micron::__chunk<byte> buf = _arena_memory.try_mark(sizeof(sheet<Sz>));
    bucket = { new (buf.ptr) sheet<Sz>(__get_kernel_chunk<micron::__chunk<byte>>(n)), nullptr };
    tail = &bucket;
    if ( bucket.nd->empty() )
      abort_state();
  }

  template <typename F>
  void
  __release(F &bucket)
  {
    auto *nd = &bucket;
    do {
      if ( nd->nd ) {
        nd->nd->release();
      }
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
  ~__arena(void)
  {
    // WARNING: this destructor is meant to trigger iff you seek to recycle the entire memory arena, .ie you're looking
    // to reload the entire allocator. if abcmalloc is compiled in stm, on global scope destruction THIS FUNCTION WILL BE
    // DESTROYED IN REVERSE ORDER OF CONSTRUCTION, if any global scope objects registered memory with the allocator ALL
    // SUBSEQUENT CALLS WILL FAIL. this will ALWAYS happen if using automatic/default allocation, since all objects
    // invoke the allocator WITHIN their own constructors if compiled in global_mode THIS CODE WILL NEVER TRIGGER (kernel
    // reclaims on ret)

    // NOTE: ONCE THESE CALLS FIRE ALL MEMORY IS UNMAPPED AND LOST IRREVOCABLY.
    __release(_cache_buffer);
    __release(_small_buckets);
    __release(_medium_buckets);
    __release(_large_buckets);
    __release(_huge_buckets);
    __release(_arena_buffer);
    _arena_memory.release();

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
        _cache_buffer{ nullptr, nullptr }, _arena_buffer{ nullptr, nullptr }, _tail_arena_buffer{ nullptr },
        _small_buckets{ nullptr, nullptr }, _tail_small_buckets{ nullptr }, _medium_buckets{ nullptr, nullptr },
        _tail_medium_buckets{ nullptr }, _large_buckets{ nullptr, nullptr }, _tail_large_buckets{ nullptr },
        _huge_buckets{ nullptr, nullptr }, _tail_huge_buckets{ nullptr }
  {
    micron::sysinfo info;
    auto __share = []<u64 Class>(u64 total) -> u64 {
      if ( total == 0 ) [[unlikely]]
        return __default_magic_size;
      constexpr f64 alpha_factor = 0.33f;
      if constexpr ( __wordsize == 64 ) {
        // 2 << s  = 2^(s+1)
        u32 shift = __builtin_ctzll(Class);     // val = 1 << shift

        // 1/val  → 2^(K - shift), K = 63
        f64 w = 63.0 - f64(shift);
        u64 weight = static_cast<u64>(micron::math::pow(2.0, w * alpha_factor) + 0.5);

        const u32 shifts[5] = { __class_precise_shift, __class_small_shift, __class_medium_shift, __class_large_shift, __class_huge_shift };

        f64 __denom = 0.0;
        for ( u32 i = 0; i < 5; ++i ) {
          f64 wi = 63.0 - f64(shifts[i]);
          __denom += micron::math::pow(2.0, wi * alpha_factor);
        }
        u64 denom = static_cast<u64>(__denom + 0.5);

        u128 num = static_cast<u128>(total) * weight;
        u64 frac = num / denom;
        if ( frac % __system_pagesize != 0 )
          frac += __system_pagesize - (frac % __system_pagesize);
        return frac;
      } else if constexpr ( __wordsize == 32 ) {
        const u64 shifts[5] = { __class_precise_shift, __class_small_shift, __class_medium_shift, __class_large_shift, __class_huge_shift };

        u64 shift = 63u - __builtin_clzll(Class);

        f64 __denom = 0.0;
        for ( unsigned i = 0; i < 5; ++i ) {
          f64 w = 63.0 - f64(shifts[i]);
          __denom += micron::math::pow(2.0, w * alpha_factor);
        }
        u64 denom = static_cast<u64>(__denom + 0.5);

        f64 w = (63.0 - f64(shift));
        f64 weight_fp = micron::math::pow(2.0, w * alpha_factor);
        u64 weight = static_cast<u64>(weight_fp + 0.5);     // round to nearest integer

        u64 scaled = total / denom;
        u64 remainder = total % denom;

        u64 frac = scaled * weight;
        frac += (remainder * weight) / denom;
        if ( frac % __system_pagesize != 0 )
          frac += __system_pagesize - (frac % __system_pagesize);
        return frac;
      }
      return __default_magic_size;
    };
    //
    u64 prealloc_size = micron::math::floor<u64>(static_cast<f32>(info.totalram) * __default_prealloc_factor);
    __debug_print("Total memory: ", info.totalram);
    __debug_print("Memory to preallocate: ", prealloc_size);
    __init_bucket<__class_arena_internal>(_arena_buffer, _tail_arena_buffer, (__default_arena_page_buf * __system_pagesize));
    __init_cache<__class_precise>(_cache_buffer);     // by config inits with a 1MB buffer
    if constexpr ( !__default_lazy_construct ) {
      __init_bucket<__class_small>(_small_buckets, _tail_small_buckets,
                                   __share.template operator()<__class_small>(prealloc_size));     // 1.6MB
      __init_bucket<__class_medium>(_medium_buckets, _tail_medium_buckets,
                                    __share.template operator()<__class_medium>(prealloc_size));     // 283KB
      if constexpr ( __default_init_large_pages and !__is_constrained ) {
        __init_bucket<__class_large>(_large_buckets, _tail_large_buckets,
                                     __share.template operator()<__class_large>(prealloc_size));     // 3.54MB
        __init_bucket<__class_huge>(_huge_buckets, _tail_huge_buckets,
                                    __share.template operator()<__class_huge>(prealloc_size));     // 40.8MB
      }
    }
  }

  __arena(const __arena &) = delete;
  __arena(__arena &&) = delete;
  __arena &operator=(const __arena &) = delete;
  __arena &operator=(__arena &&) = delete;

  // main method for allocating/mallocing memory
  hot_fn(micron::__chunk<byte>) push(const usize sz)
  {
    __debug_print("push(): ", sz);
    collect_stats<stat_type::alloc>();
    collect_stats<stat_type::total_memory_req>(sz);

    if ( check_constraint(sz) ) [[unlikely]]
      abort_state();
    if ( check_oom() ) [[unlikely]]
      abort_state();

    micron::__chunk<byte> memory;

    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      if ( memory = __vmap_append_tail(sz); !memory.zero() ) [[likely]] {
        __debug_print("__vmap_append() alloc'd: ", memory.len);
        zero_on_alloc(memory.ptr, memory.len);
        sanitize_on_alloc(memory.ptr, memory.len);
        collect_stats<stat_type::total_memory_throughput>(memory.len);
        return memory;
      }

      __debug_print("failed to __vmap_append(): ", sz);

      if ( sz <= __class_small ) {
        usize __next_sz = __calculate_space_cache(__default_cache_step);
        __debug_print("growing (cache) container with size: ", __next_sz);
        __buf_expand_exact(sz, __next_sz);
        continue;
      }

      if constexpr ( __is_constrained ) {
        if ( sz >= __class_medium ) {
          usize __next_sz = __calculate_space_medium(sz) * __default_overcommit;
          __predict += __next_sz;
          __debug_print("growing (large) container with size: ", __predict.predict_size(__next_sz));
          __buf_expand_exact(sz, __predict.predict_size(__next_sz));
          continue;
        }
      }

      if constexpr ( !__is_constrained ) {
        if ( sz >= __class_medium && sz < __class_1mb ) {
          usize __next_sz = __calculate_space_medium(sz) * __default_overcommit;
          __predict += __next_sz;
          __debug_print("growing (large) container with size: ", __predict.predict_size(__next_sz));
          __buf_expand_exact(sz, __predict.predict_size(__next_sz));
          continue;
        }

        if ( sz >= __class_1mb && sz < __class_gb ) {
          usize __next_sz = __calculate_space_huge(sz) * __default_overcommit;
          __predict += __next_sz;
          __debug_print("growing (1mb+) container with size: ", __predict.predict_size(__next_sz));
          __buf_expand_exact(sz, __predict.predict_size(__next_sz));
          continue;
        }

        if ( sz >= __class_gb ) {
          usize __next_sz = __calculate_space_bulk(sz);
          __debug_print("growing (gb+) container with size: ", __next_sz);
          __buf_expand_exact(sz, __next_sz);
          continue;
        }
      }

      // smallest allocations grow the faster
      {
        usize __next_sz = __calculate_space_small(sz) * __default_overcommit;
        __predict += __next_sz;
        __debug_print("growing (small) container with size: ", __predict.predict_size(__next_sz));
        __buf_expand_exact(sz, __predict.predict_size(__next_sz));
      }
    }

    return { (byte *)-1, micron::numeric_limits<usize>::max() };
  }

  micron::__chunk<byte>
  launder(const usize sz)
  {
    __debug_print("launder(): ", sz);
    collect_stats<stat_type::alloc>();
    collect_stats<stat_type::total_memory_req>(sz);
    if ( check_constraint(sz) ) [[unlikely]]
      abort_state();
    if ( check_oom() ) [[unlikely]]
      abort_state();
    micron::__chunk<byte> memory;
    for ( u64 i = 0; i < __default_max_retries; ++i ) {
      {
        if ( memory = __vmap_launder(sz); !memory.zero() ) {
          __debug_print("__vmap_append() alloc'd: ", memory.len);
          zero_on_alloc(memory.ptr, memory.len);
          sanitize_on_alloc(memory.ptr, memory.len);
          collect_stats<stat_type::total_memory_throughput>(memory.len);
          return memory;
        }
        __debug_print("failed to __vmap_append(): ", sz);
        if ( sz >= __class_medium and sz < __class_gb ) {
          __debug_print("growing container with size: ", __calculate_space_huge(sz) * __default_overcommit);
          __buf_expand_exact(sz, __calculate_space_huge(sz) * __default_overcommit);
        } else if ( sz >= __class_gb ) {     // for if someone requests allocations exceeding 1gb
          __debug_print("growing container with size: ", __calculate_space_bulk(sz));
          __buf_expand_exact(sz, __calculate_space_bulk(sz));
        } else {     // for all other allocs, grow the most aggressive
          __debug_print("growing container with size: ", __calculate_space_small(sz) * __default_overcommit);
          __buf_expand_exact(sz, __calculate_space_small(sz) * __default_overcommit);
        }
      }
    }
    return { (byte *)-1, micron::numeric_limits<usize>::max() };
  }

  // main methods for freeing memory
  bool
  pop(const micron::__chunk<byte> &mem)
  {
    __debug_print_addr("pop() address: ", mem.ptr);
    __debug_print("pop() size: ", mem.len);
    if ( mem.zero() )
      return true;
    // NOTE: according to the standard freeing null should succeed
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) )
        return fail_state();
    }
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
    __debug_print_addr("pop() address: ", mem);
    if ( mem == nullptr )
      return true;
    // NOTE: according to the standard freeing null should succeed
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) )
        return fail_state();
    }
    collect_stats<stat_type::dealloc>();
    full_on_free(mem);
    zero_on_free(mem);
    return __vmap_remove_at(mem);
  }

  bool
  present(addr_t *mem) const
  {
    if ( mem == nullptr )
      return false;
    return __vmap_locate_at(mem);
  }

  bool
  has_provenance(addr_t *mem) const
  {
    if ( mem == nullptr )
      return false;
    return __vmap_within(mem);
  }

  bool
  pop(byte *mem, usize len)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    __debug_print_addr("pop() address: ", mem);
    if ( mem == nullptr )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) )
        return fail_state();
    }
    collect_stats<stat_type::total_memory_freed>(len);
    return __vmap_remove({ mem, len });
  }

  // tombstone pop
  bool
  ts_pop(const micron::__chunk<byte> &mem)
  {
    __debug_print_addr("pop() address: ", mem.ptr);
    __debug_print("pop() size: ", mem.len);
    if ( mem.zero() )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) )
        return fail_state();
    }
    collect_stats<stat_type::dealloc>();
    collect_stats<stat_type::total_memory_freed>(mem.len);
    full_on_free(mem.ptr, mem.len);
    zero_on_free(mem.ptr, mem.len);
    return __vmap_tombstone(mem);
  }

  bool
  ts_pop(byte *mem)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    __debug_print_addr("pop() address: ", mem);
    // TODO: add this
    // collect_stats<stat_type::total_memory_freed>(mem.len);
    if ( mem == nullptr )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) )
        return fail_state();
    }
    collect_stats<stat_type::dealloc>();
    full_on_free(mem);
    zero_on_free(mem);
    return __vmap_tombstone_at(mem);
  }

  bool
  ts_pop(byte *mem, usize len)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    if ( mem == nullptr )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) )
        return fail_state();
    }
    collect_stats<stat_type::total_memory_freed>(len);
    return __vmap_tombstone({ mem, len });
  }

  bool
  freeze(const micron::__chunk<byte> &mem)
  {
    if ( mem.zero() )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem.ptr)) )
        return fail_state();
    }
    return __vmap_freeze(mem);
  }

  bool
  freeze(byte *mem)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    if ( mem == nullptr )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) )
        return fail_state();
    }
    return __vmap_freeze_at(mem);
  }

  bool
  freeze(byte *mem, usize len)     // need to support popping memory by addr only, will be slower due to req. lookup
  {
    if ( mem == nullptr )
      return false;
    if constexpr ( __default_enforce_provenance ) {
      if ( !has_provenance(reinterpret_cast<addr_t *>(mem)) )
        return fail_state();
    }
    return __vmap_freeze({ mem, len });
  }

  usize
  total_usage(void) const
  {
    usize total = 0;
    total += __for_each_bckt(_cache_buffer, [](sheet<__class_precise> *const v) -> usize { return v->allocated(); });
    total += __for_each_bckt(_small_buckets, [](sheet<__class_small> *const v) -> usize { return v->allocated(); });
    total += __for_each_bckt(_medium_buckets, [](sheet<__class_medium> *const v) -> usize { return v->allocated(); });
    total += __for_each_bckt(_large_buckets, [](sheet<__class_large> *const v) -> usize { return v->allocated(); });
    total += __for_each_bckt(_huge_buckets, [](sheet<__class_huge> *const v) -> usize { return v->allocated(); });
    return total;
  }

  template <u64 Sz>
  usize
  total_usage_of_class(void) const
  {
    if constexpr ( Sz == __class_precise ) {
      return __for_each_bckt(_cache_buffer, [](sheet<__class_precise> *const v) -> usize { return v->allocated(); });
    } else if constexpr ( Sz == __class_small ) {
      return __for_each_bckt(_small_buckets, [](sheet<__class_small> *const v) -> usize { return v->allocated(); });
    } else if constexpr ( Sz == __class_medium ) {
      return __for_each_bckt(_medium_buckets, [](sheet<__class_medium> *const v) -> usize { return v->allocated(); });
    } else if constexpr ( Sz == __class_large ) {
      return __for_each_bckt(_large_buckets, [](sheet<__class_large> *const v) -> usize { return v->allocated(); });
    } else if constexpr ( Sz == __class_huge ) {
      return __for_each_bckt(_huge_buckets, [](sheet<__class_huge> *const v) -> usize { return v->allocated(); });
    }
    return 0;
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

        if ( sh.is_at(reinterpret_cast<addr_t *>(__ptr)) ) [[unlikely]]
          sh.reset();
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
    if ( __within(&_cache_buffer, addr) ) {
      addr_t *ptr = get_metadata_addr(addr);
      i64 order_class = static_cast<i64>(*ptr);
      return __class_precise << order_class;
    }
    if ( __within(&_small_buckets, addr) ) {
      addr_t *ptr = get_metadata_addr(addr);
      i64 order_class = static_cast<i64>(*ptr);
      return __class_small << order_class;
    }
    if ( __within(&_medium_buckets, addr) ) {
      addr_t *ptr = get_metadata_addr(addr);
      i64 order_class = static_cast<i64>(*ptr);
      return __class_medium << order_class;
    }
    if ( __within(&_large_buckets, addr) ) {
      addr_t *ptr = get_metadata_addr(addr);
      i64 order_class = static_cast<i64>(*ptr);
      return __class_large << order_class;
    }
    if ( __within(&_huge_buckets, addr) ) {
      addr_t *ptr = get_metadata_addr(addr);
      i64 order_class = static_cast<i64>(*ptr);
      return __class_huge << order_class;
    }
    return 0;
  }

  bool
  zeroed(void) const
  {
    if ( _cache_buffer.nd == nullptr and _cache_buffer.nxt == nullptr )
      return true;
    return false;
  }
};
};     // namespace abc
