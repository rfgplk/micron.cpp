//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/initializer_list"
#include "../type_traits.hpp"

#include "../algorithm/memory.hpp"
#include "../allocator.hpp"
#include "../atomic/atomic.hpp"
#include "../bits/__container.hpp"
#include "../concepts.hpp"
#include "../memory/allocation/resources.hpp"
#include "../new.hpp"
#include "../types.hpp"

#include "../memory/cache.hpp"

namespace micron
{

// LMAX-style single-producer, batched-consumer ring buffer
template<is_movable_object T, usize N, class Alloc = micron::allocator_serial<>>
class disruptor: public __mutable_memory_resource_move_only<T, Alloc>
{
  constexpr static usize
  __next_pow2(usize n)
  {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }

  using __mem = __mutable_memory_resource_move_only<T, Alloc>;

  static constexpr u64 __cache_line = cache_line_size();
  static constexpr usize __capacity = __next_pow2(N);
  static constexpr usize __mask = __capacity - 1u;

  alignas(__cache_line) micron::atomic_token<usize> __pub_seq;
  char __pad1[__cache_line - sizeof(micron::atomic_token<usize>)];

  alignas(__cache_line) micron::atomic_token<usize> __cons_seq;
  char __pad2[__cache_line - sizeof(micron::atomic_token<usize>)];

  alignas(__cache_line) usize __cached_cons;
  char __pad3[__cache_line - sizeof(usize)];

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef const T &const_reference;
  typedef T *pointer;
  typedef const T *const_pointer;

  ~disruptor() { clear(); }

  disruptor() : __mem(__capacity), __pub_seq(0), __cons_seq(0), __cached_cons(0) { }

  disruptor(const disruptor &) = delete;
  disruptor(disruptor &&) = delete;
  disruptor &operator=(const disruptor &) = delete;
  disruptor &operator=(disruptor &&) = delete;

  inline usize
  capacity() const noexcept
  {
    return __capacity;
  }

  inline usize
  max_size() const noexcept
  {
    return __capacity;
  }

  inline usize
  size() const noexcept
  {
    return __pub_seq.get(memory_order_acquire) - __cons_seq.get(memory_order_acquire);
  }

  inline bool
  empty() const noexcept
  {
    return __pub_seq.get(memory_order_acquire) == __cons_seq.get(memory_order_acquire);
  }

  inline void
  clear()
  {
    const usize p = __pub_seq.get(memory_order_relaxed);
    const usize c = __cons_seq.get(memory_order_relaxed);
    if constexpr ( micron::is_class_v<T> ) {
      for ( usize s = c; s != p; ++s ) (__mem::memory)[s & __mask].~T();
    }
    micron::zero((byte *)micron::voidify(&(__mem::memory)[0]), __capacity * sizeof(T));
    __pub_seq.store(0, memory_order_relaxed);
    __cons_seq.store(0, memory_order_relaxed);
    __cached_cons = 0;
  }

  __attribute__((always_inline)) inline bool
  publish(const T &val)
  {
    const usize p = __pub_seq.get(memory_order_relaxed);
    usize avail = __capacity - (p - __cached_cons);
    if ( avail == 0 ) [[unlikely]] {
      __cached_cons = __cons_seq.get(memory_order_acquire);
      avail = __capacity - (p - __cached_cons);
      if ( avail == 0 ) return false;
    }
    const usize idx = p & __mask;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[idx])) T{ val };
    } else {
      __mem::memory[idx] = val;
    }
    __pub_seq.store(p + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  publish(T &&val)
  {
    const usize p = __pub_seq.get(memory_order_relaxed);
    usize avail = __capacity - (p - __cached_cons);
    if ( avail == 0 ) [[unlikely]] {
      __cached_cons = __cons_seq.get(memory_order_acquire);
      avail = __capacity - (p - __cached_cons);
      if ( avail == 0 ) return false;
    }
    const usize idx = p & __mask;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[idx])) T{ micron::move(val) };
    } else {
      __mem::memory[idx] = micron::move(val);
    }
    __pub_seq.store(p + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline usize
  try_publish_batch(const T *items, usize count)
  {
    const usize p = __pub_seq.get(memory_order_relaxed);
    usize avail = __capacity - (p - __cached_cons);
    if ( avail < count ) [[unlikely]] {
      __cached_cons = __cons_seq.get(memory_order_acquire);
      avail = __capacity - (p - __cached_cons);
    }
    const usize n = avail < count ? avail : count;
    for ( usize i = 0; i < n; ++i ) {
      const usize idx = (p + i) & __mask;
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
        new (micron::addr(__mem::memory[idx])) T{ items[i] };
      } else {
        __mem::memory[idx] = items[i];
      }
    }
    __pub_seq.store(p + n, memory_order_release);
    return n;
  }

  __attribute__((always_inline)) inline bool
  consume(T &out)
  {
    const usize c = __cons_seq.get(memory_order_relaxed);
    const usize p = __pub_seq.get(memory_order_acquire);
    if ( c == p ) return false;
    const usize idx = c & __mask;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      out = micron::move(__mem::memory[idx]);
      __mem::memory[idx].~T();
    } else {
      out = __mem::memory[idx];
    }
    __cons_seq.store(c + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline usize
  try_consume_batch(T *items, usize count)
  {
    const usize c = __cons_seq.get(memory_order_relaxed);
    const usize p = __pub_seq.get(memory_order_acquire);
    const usize avail = p - c;
    const usize n = avail < count ? avail : count;
    for ( usize i = 0; i < n; ++i ) {
      const usize idx = (c + i) & __mask;
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
        items[i] = micron::move(__mem::memory[idx]);
        __mem::memory[idx].~T();
      } else {
        items[i] = __mem::memory[idx];
      }
    }
    __cons_seq.store(c + n, memory_order_release);
    return n;
  }

  __attribute__((always_inline)) inline bool
  peek(T &out) const
  {
    const usize c = __cons_seq.get(memory_order_relaxed);
    const usize p = __pub_seq.get(memory_order_acquire);
    if ( c == p ) return false;
    out = __mem::memory[c & __mask];
    return true;
  }

  inline usize
  pub_cursor() const noexcept
  {
    return __pub_seq.get(memory_order_acquire);
  }

  inline usize
  cons_cursor() const noexcept
  {
    return __cons_seq.get(memory_order_acquire);
  }

  __attribute__((always_inline)) inline bool
  push(const T &val)
  {
    return publish(val);
  }

  __attribute__((always_inline)) inline bool
  push(T &&val)
  {
    return publish(micron::move(val));
  }

  __attribute__((always_inline)) inline bool
  pop(T &out)
  {
    return consume(out);
  }

  template<typename... Args>
  __attribute__((always_inline)) inline bool
  emplace(Args &&...args)
  {
    const usize p = __pub_seq.get(memory_order_relaxed);
    usize avail = __capacity - (p - __cached_cons);
    if ( avail == 0 ) [[unlikely]] {
      __cached_cons = __cons_seq.get(memory_order_acquire);
      avail = __capacity - (p - __cached_cons);
      if ( avail == 0 ) return false;
    }
    const usize idx = p & __mask;
    new (micron::addr(__mem::memory[idx])) T{ micron::forward<Args>(args)... };
    __pub_seq.store(p + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline T &
  front()
  {
    return __mem::memory[__cons_seq.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  front() const
  {
    return __mem::memory[__cons_seq.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline T &
  last()
  {
    return __mem::memory[(__pub_seq.get(memory_order_relaxed) - 1u) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  last() const
  {
    return __mem::memory[(__pub_seq.get(memory_order_relaxed) - 1u) & __mask];
  }
};

};      // namespace micron
