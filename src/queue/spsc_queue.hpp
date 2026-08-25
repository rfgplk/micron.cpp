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

template<typename T, usize N, class Alloc = micron::allocator_serial<>>
  requires micron::is_move_constructible_v<T> and (micron::is_move_assignable_v<T> or micron::is_copy_assignable_v<T>)
           and micron::is_destructible_v<T> and (N > 0 and N <= (static_cast<usize>(-1) / 2 + 1))
class spsc_queue: public __mutable_memory_resource_move_only<T, Alloc>
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
    if constexpr ( sizeof(usize) > 4 ) {
      constexpr usize __hi_shift = sizeof(usize) * 8u / 2u;
      n |= n >> __hi_shift;
    }
    return n + 1;
  }

  using __mem = __mutable_memory_resource_move_only<T, Alloc>;

  static constexpr usize __spsc_capacity = __next_pow2(N);
  static constexpr usize __mask = __spsc_capacity - 1;
  static_assert(__spsc_capacity <= static_cast<usize>(-1) / sizeof(T), "spsc_queue: allocation size is not representable");

  struct alignas(__cache_line) __producer_state {
    micron::atomic_token<usize> tail;
    usize cached_head;
  } __producer;

  struct alignas(__cache_line) __consumer_state {
    micron::atomic_token<usize> head;
    usize cached_tail;
  } __consumer;

  static constexpr bool __assign_nothrow
      = micron::is_move_assignable_v<T> ? micron::is_nothrow_move_assignable_v<T> : micron::is_nothrow_copy_assignable_v<T>;

  [[gnu::always_inline]] static inline void
  __assign(T &out, T &value) noexcept(__assign_nothrow)
  {
    if constexpr ( micron::is_move_assignable_v<T> )
      out = micron::move(value);
    else
      out = value;
  }

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

  spsc_queue() : __mem(__spsc_capacity), __producer{ 0, 0 }, __consumer{ 0, 0 } { }

  spsc_queue(const std::initializer_list<T> &lst) : spsc_queue()
  {
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
#endif
      for ( const T &value : lst ) push(value);
#if !defined(__micron_freestanding) || defined(__micron_eh)
    } catch ( ... ) {
      clear();
      throw;
    }
#endif
  }

  spsc_queue(const spsc_queue &) = delete;
  spsc_queue(spsc_queue &&) = delete;
  spsc_queue &operator=(const spsc_queue &) = delete;
  spsc_queue &operator=(spsc_queue &&) = delete;

  inline void
  clear()
  {
    const usize h = __consumer.head.get(memory_order_relaxed);
    const usize t = __producer.tail.get(memory_order_relaxed);

    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      for ( usize i = h; i != t; ++i ) {
        (__mem::memory)[i & __mask].~T();
      }
    }

    __consumer.head.store(0, memory_order_relaxed);
    __producer.tail.store(0, memory_order_relaxed);
    __producer.cached_head = 0;
    __consumer.cached_tail = 0;
  }

  inline bool
  empty() const
  {
    const usize h = __consumer.head.get(memory_order_acquire);
    return h == __producer.tail.get(memory_order_acquire);
  }

  inline usize
  size() const
  {
    const usize h = __consumer.head.get(memory_order_acquire);
    const usize t = __producer.tail.get(memory_order_acquire);
    const usize n = t - h;
    return n < __spsc_capacity ? n : __spsc_capacity;
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
    const usize t = __producer.tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (t - __producer.cached_head);

    if ( avail == 0 ) [[unlikely]] {
      __producer.cached_head = __consumer.head.get(memory_order_acquire);
      avail = __spsc_capacity - (t - __producer.cached_head);

      if ( avail == 0 ) return false;
    }

    const usize idx = t & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[idx])) T{};
    } else {
      __mem::memory[idx] = T{};
    }

    __producer.tail.store(__next, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  push(T &&val)
  {
    const usize t = __producer.tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (t - __producer.cached_head);

    if ( avail == 0 ) [[unlikely]] {
      __producer.cached_head = __consumer.head.get(memory_order_acquire);
      avail = __spsc_capacity - (t - __producer.cached_head);

      if ( avail == 0 ) return false;
    }

    const usize idx = t & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[idx])) T{ micron::move(val) };
    } else {
      __mem::memory[idx] = micron::move(val);
    }

    __producer.tail.store(__next, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  push(const T &val)
  {
    const usize t = __producer.tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (t - __producer.cached_head);

    if ( avail == 0 ) [[unlikely]] {
      __producer.cached_head = __consumer.head.get(memory_order_acquire);
      avail = __spsc_capacity - (t - __producer.cached_head);

      if ( avail == 0 ) return false;
    }

    const usize idx = t & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[idx])) T{ val };
    } else {
      __mem::memory[idx] = val;
    }

    __producer.tail.store(__next, memory_order_release);
    return true;
  }

  template<typename... Args>
  __attribute__((always_inline)) inline bool
  emplace(Args &&...args)
  {
    const usize t = __producer.tail.get(memory_order_relaxed);
    const usize __next = t + 1;

    usize avail = __spsc_capacity - (t - __producer.cached_head);

    if ( avail == 0 ) [[unlikely]] {
      __producer.cached_head = __consumer.head.get(memory_order_acquire);
      avail = __spsc_capacity - (t - __producer.cached_head);

      if ( avail == 0 ) return false;
    }

    const usize idx = t & __mask;

    new (micron::addr(__mem::memory[idx])) T{ micron::forward<Args>(args)... };

    __producer.tail.store(__next, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  pop(T &out)
  {
    const usize h = __consumer.head.get(memory_order_relaxed);

    usize avail = __consumer.cached_tail - h;

    if ( avail == 0 ) [[unlikely]] {
      __consumer.cached_tail = __producer.tail.get(memory_order_acquire);
      avail = __consumer.cached_tail - h;

      if ( avail == 0 ) return false;
    }

    const usize idx = h & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      __assign(out, __mem::memory[idx]);
      __mem::memory[idx].~T();
    } else {
      out = __mem::memory[idx];
    }

    __consumer.head.store(h + 1, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  pop(void)
  {
    const usize h = __consumer.head.get(memory_order_relaxed);

    usize avail = __consumer.cached_tail - h;

    if ( avail == 0 ) [[unlikely]] {
      __consumer.cached_tail = __producer.tail.get(memory_order_acquire);
      avail = __consumer.cached_tail - h;

      if ( avail == 0 ) return false;
    }

    const usize idx = h & __mask;

    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_destructible_v<T> ) {
      __mem::memory[idx].~T();
    }

    __consumer.head.store(h + 1, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  peek(T &out) const
  {
    const usize h = __consumer.head.get(memory_order_relaxed);
    const usize t = __producer.tail.get(memory_order_acquire);

    const bool has_data = (h != t);
    if ( has_data ) [[likely]] {
      out = __mem::memory[h & __mask];
    }
    return has_data;
  }

  __attribute__((always_inline)) inline T &
  front()
  {
    return __mem::memory[__consumer.head.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  front() const
  {
    return __mem::memory[__consumer.head.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline T &
  last()
  {
    return __mem::memory[(__producer.tail.get(memory_order_relaxed) - 1) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  last() const
  {
    return __mem::memory[(__producer.tail.get(memory_order_relaxed) - 1) & __mask];
  }

  __attribute__((always_inline)) inline usize
  push_batch(const T *items, usize count)
  {
    const usize t = __producer.tail.get(memory_order_relaxed);
    usize avail = __spsc_capacity - (t - __producer.cached_head);

    if ( avail < count ) [[unlikely]] {
      __producer.cached_head = __consumer.head.get(memory_order_acquire);
      avail = __spsc_capacity - (t - __producer.cached_head);
    }

    const usize to_push = (avail < count) ? avail : count;
    if ( to_push == 0 ) return 0;
    const usize idx = t & __mask;
    const usize first = to_push < (__spsc_capacity - idx) ? to_push : (__spsc_capacity - idx);

    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::memcpy(micron::addressof(__mem::memory[idx]), items, first);
      if ( first != to_push ) micron::memcpy(__mem::memory, items + first, to_push - first);
    } else {
      usize made = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
#endif
        for ( ; made < to_push; ++made ) new (micron::addr(__mem::memory[(t + made) & __mask])) T(items[made]);
#if !defined(__micron_freestanding) || defined(__micron_eh)
      } catch ( ... ) {
        for ( usize i = 0; i < made; ++i ) __mem::memory[(t + i) & __mask].~T();
        throw;
      }
#endif
    }

    __producer.tail.store(t + to_push, memory_order_release);
    return to_push;
  }

  __attribute__((always_inline)) inline usize
  pop_batch(T *items, usize count)
  {
    const usize h = __consumer.head.get(memory_order_relaxed);
    usize avail = __consumer.cached_tail - h;

    if ( avail < count ) [[unlikely]] {
      __consumer.cached_tail = __producer.tail.get(memory_order_acquire);
      avail = __consumer.cached_tail - h;
    }

    const usize to_pop = (avail < count) ? avail : count;
    if ( to_pop == 0 ) return 0;
    const usize idx = h & __mask;
    const usize first = to_pop < (__spsc_capacity - idx) ? to_pop : (__spsc_capacity - idx);

    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::memcpy(items, micron::addressof(__mem::memory[idx]), first);
      if ( first != to_pop ) micron::memcpy(items + first, __mem::memory, to_pop - first);
      __consumer.head.store(h + to_pop, memory_order_release);
    } else if constexpr ( __assign_nothrow ) {
      for ( usize i = 0; i < to_pop; ++i ) {
        T &v = __mem::memory[(h + i) & __mask];
        __assign(items[i], v);
        v.~T();
      }
      __consumer.head.store(h + to_pop, memory_order_release);
    } else {
      for ( usize i = 0; i < to_pop; ++i ) {
        T &v = __mem::memory[(h + i) & __mask];
        __assign(items[i], v);
        v.~T();
        __consumer.head.store(h + i + 1, memory_order_release);
      }
    }
    return to_pop;
  }
};

};      // namespace micron
