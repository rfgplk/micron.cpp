
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
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../memory/cache.hpp"

namespace micron
{

inline constexpr u64 __cache_line = cache_line_size();

template <is_movable_object T, usize N, class Alloc = micron::allocator_serial<>>
class spsc_queue : public __mutable_memory_resource<T, Alloc>
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

  using __mem = __mutable_memory_resource<T, Alloc>;

  static constexpr usize __spsc_capacity = __next_pow2(N);
  static constexpr usize __mask = __spsc_capacity - 1;

  alignas(__cache_line) micron::atomic_token<usize> head;
  char __pad1[__cache_line - sizeof(micron::atomic_token<usize>)];

  alignas(__cache_line) micron::atomic_token<usize> tail;
  char __pad2[__cache_line - sizeof(micron::atomic_token<usize>)];

  alignas(__cache_line) usize cached_head;
  char __pad3[__cache_line - sizeof(usize)];

  alignas(__cache_line) usize cached_tail;
  char __pad4[__cache_line - sizeof(usize)];

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef T *pointer;
  typedef const T *const_pointer;

  ~spsc_queue() { clear(); }

  spsc_queue() : __mem(__spsc_capacity), head(0), tail(0), cached_head(0), cached_tail(0) {}

  spsc_queue(const std::initializer_list<T> &lst) : spsc_queue()
  {
    for ( const T &value : lst ) {
      push(value);
    }
  }

  spsc_queue(const spsc_queue &) = delete;
  spsc_queue(spsc_queue &&) = delete;
  spsc_queue &operator=(const spsc_queue &) = delete;
  spsc_queue &operator=(spsc_queue &&) = delete;

  inline void
  clear()
  {
    const usize h = head.get(memory_order_relaxed);
    const usize t = tail.get(memory_order_relaxed);

    if constexpr ( micron::is_class_v<T> ) {
      for ( usize i = h; i != t; ++i ) {
        (__mem::memory)[i & __mask].~T();
      }
    }

    micron::zero((byte *)micron::voidify(&(__mem::memory)[0]), __spsc_capacity * sizeof(T));
    head.store(0, memory_order_relaxed);
    tail.store(0, memory_order_relaxed);
    cached_head = 0;
    cached_tail = 0;
  }

  inline bool
  empty() const
  {
    return head.get(memory_order_acquire) == tail.get(memory_order_acquire);
  }

  inline usize
  size() const
  {
    return tail.get(memory_order_acquire) - head.get(memory_order_acquire);
  }

  inline usize
  capacity() const
  {
    return __spsc_capacity;
  }

  inline usize
  max_size() const
  {
    return __spsc_capacity;
  }

  __attribute__((always_inline)) inline bool
  push(void)
  {
    const usize t = tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (__next - cached_head);

    if ( avail == 0 ) [[unlikely]] {
      cached_head = head.get(memory_order_acquire);
      avail = __spsc_capacity - (__next - cached_head);

      if ( avail == 0 )
        return false;
    }

    const usize idx = t & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (&__mem::memory[idx]) T{};
    } else {
      __mem::memory[idx] = T{};
    }

    tail.store(__next, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  push(T &&val)
  {
    const usize t = tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (__next - cached_head);

    if ( avail == 0 ) [[unlikely]] {
      cached_head = head.get(memory_order_acquire);
      avail = __spsc_capacity - (__next - cached_head);

      if ( avail == 0 )
        return false;
    }

    const usize idx = t & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (&__mem::memory[idx]) T{ micron::move(val) };
    } else {
      __mem::memory[idx] = micron::move(val);
    }

    tail.store(__next, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  push(const T &val)
  {
    const usize t = tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (__next - cached_head);

    if ( avail == 0 ) [[unlikely]] {
      cached_head = head.get(memory_order_acquire);
      avail = __spsc_capacity - (__next - cached_head);

      if ( avail == 0 )
        return false;
    }

    const usize idx = t & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (&__mem::memory[idx]) T{ val };
    } else {
      __mem::memory[idx] = val;
    }

    tail.store(__next, memory_order_release);
    return true;
  }

  template <typename... Args>
  __attribute__((always_inline)) inline bool
  emplace(Args &&...args)
  {
    const usize t = tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (__next - cached_head);

    if ( avail == 0 ) [[unlikely]] {
      cached_head = head.get(memory_order_acquire);
      avail = __spsc_capacity - (__next - cached_head);

      if ( avail == 0 )
        return false;
    }

    const usize idx = t & __mask;

    new (&__mem::memory[idx]) T{ micron::forward<Args>(args)... };

    tail.store(__next, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  pop(T &out)
  {
    const usize h = head.get(memory_order_relaxed);

    usize avail = cached_tail - h;

    if ( avail == 0 ) [[unlikely]] {
      cached_tail = tail.get(memory_order_acquire);
      avail = cached_tail - h;

      if ( avail == 0 )
        return false;
    }

    const usize idx = h & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      out = micron::move(__mem::memory[idx]);
      __mem::memory[idx].~T();
    } else {
      out = __mem::memory[idx];
    }

    head.store(h + 1, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  pop(void)
  {
    const usize h = head.get(memory_order_relaxed);

    usize avail = cached_tail - h;

    if ( avail == 0 ) [[unlikely]] {
      cached_tail = tail.get(memory_order_acquire);
      avail = cached_tail - h;

      if ( avail == 0 )
        return false;
    }

    const usize idx = h & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_destructible_v<T> ) {
      __mem::memory[idx].~T();
    } else {
      __mem::memory[idx] = T{};
    }

    head.store(h + 1, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  peek(T &out) const
  {
    const usize h = head.get(memory_order_relaxed);
    const usize t = tail.get(memory_order_acquire);

    const bool has_data = (h != t);
    if ( has_data ) [[likely]] {
      out = __mem::memory[h & __mask];
    }
    return has_data;
  }

  __attribute__((always_inline)) inline T &
  front()
  {
    return __mem::memory[head.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  front() const
  {
    return __mem::memory[head.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline T &
  last()
  {
    return __mem::memory[(tail.get(memory_order_relaxed) - 1) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  last() const
  {
    return __mem::memory[(tail.get(memory_order_relaxed) - 1) & __mask];
  }

  __attribute__((always_inline)) inline usize
  push_batch(const T *items, usize count)
  {
    const usize t = tail.get(memory_order_relaxed);
    usize avail = __spsc_capacity - (t - cached_head);

    if ( avail < count ) [[unlikely]] {
      cached_head = head.get(memory_order_acquire);
      avail = __spsc_capacity - (t - cached_head);
    }

    const usize to_push = (avail < count) ? avail : count;

    for ( usize i = 0; i < to_push; ++i ) {
      const usize idx = (t + i) & __mask;
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
        new (&__mem::memory[idx]) T{ items[i] };
      } else {
        __mem::memory[idx] = items[i];
      }
    }

    tail.store(t + to_push, memory_order_release);
    return to_push;
  }

  __attribute__((always_inline)) inline usize
  pop_batch(T *items, usize count)
  {
    const usize h = head.get(memory_order_relaxed);
    usize avail = cached_tail - h;

    if ( avail < count ) [[unlikely]] {
      cached_tail = tail.get(memory_order_acquire);
      avail = cached_tail - h;
    }

    const usize to_pop = (avail < count) ? avail : count;

    for ( usize i = 0; i < to_pop; ++i ) {
      const usize idx = (h + i) & __mask;
      if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
        items[i] = micron::move(__mem::memory[idx]);
        __mem::memory[idx].~T();
      } else {
        items[i] = __mem::memory[idx];
      }
    }

    head.store(h + to_pop, memory_order_release);
    return to_pop;
  }
};

};     // namespace micron
