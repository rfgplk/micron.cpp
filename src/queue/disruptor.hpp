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
template<typename T, usize N, class Alloc = micron::allocator_serial<>>
  requires micron::is_move_constructible_v<T> and (micron::is_move_assignable_v<T> or micron::is_copy_assignable_v<T>)
           and micron::is_destructible_v<T> and (N > 0 and N <= (static_cast<usize>(-1) / 2 + 1))
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
    if constexpr ( sizeof(usize) > 4 ) {
      constexpr usize __hi_shift = sizeof(usize) * 8u / 2u;
      n |= n >> __hi_shift;
    }
    return n + 1;
  }

  using __mem = __mutable_memory_resource_move_only<T, Alloc>;

  static constexpr u64 __cache_line = cache_line_size();
  static constexpr usize __capacity = __next_pow2(N);
  static constexpr usize __mask = __capacity - 1u;
  static_assert(__capacity <= static_cast<usize>(-1) / sizeof(T), "disruptor: allocation size is not representable");

  struct alignas(__cache_line) __producer_state {
    micron::atomic_token<usize> published;
    usize cached_consumer;
  } __producer;

  struct alignas(__cache_line) __consumer_state {
    micron::atomic_token<usize> consumed;
    usize cached_published;
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
  typedef const T &const_reference;
  typedef T *pointer;
  typedef const T *const_pointer;

  ~disruptor() { clear(); }

  disruptor() : __mem(__capacity), __producer{ 0, 0 }, __consumer{ 0, 0 } { }

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
    const usize c = __consumer.consumed.get(memory_order_acquire);
    const usize p = __producer.published.get(memory_order_acquire);
    const usize n = p - c;
    return n < __capacity ? n : __capacity;
  }

  inline bool
  empty() const noexcept
  {
    const usize c = __consumer.consumed.get(memory_order_acquire);
    return c == __producer.published.get(memory_order_acquire);
  }

  inline void
  clear()
  {
    const usize p = __producer.published.get(memory_order_relaxed);
    const usize c = __consumer.consumed.get(memory_order_relaxed);
    if constexpr ( !micron::is_trivially_destructible_v<T> ) {
      for ( usize s = c; s != p; ++s ) (__mem::memory)[s & __mask].~T();
    }
    __producer.published.store(0, memory_order_relaxed);
    __consumer.consumed.store(0, memory_order_relaxed);
    __producer.cached_consumer = 0;
    __consumer.cached_published = 0;
  }

  __attribute__((always_inline)) inline bool
  publish(const T &val)
  {
    const usize p = __producer.published.get(memory_order_relaxed);
    usize avail = __capacity - (p - __producer.cached_consumer);
    if ( avail == 0 ) [[unlikely]] {
      __producer.cached_consumer = __consumer.consumed.get(memory_order_acquire);
      avail = __capacity - (p - __producer.cached_consumer);
      if ( avail == 0 ) return false;
    }
    const usize idx = p & __mask;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[idx])) T{ val };
    } else {
      __mem::memory[idx] = val;
    }
    __producer.published.store(p + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline bool
  publish(T &&val)
  {
    const usize p = __producer.published.get(memory_order_relaxed);
    usize avail = __capacity - (p - __producer.cached_consumer);
    if ( avail == 0 ) [[unlikely]] {
      __producer.cached_consumer = __consumer.consumed.get(memory_order_acquire);
      avail = __capacity - (p - __producer.cached_consumer);
      if ( avail == 0 ) return false;
    }
    const usize idx = p & __mask;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_constructible_v<T> ) {
      new (micron::addr(__mem::memory[idx])) T{ micron::move(val) };
    } else {
      __mem::memory[idx] = micron::move(val);
    }
    __producer.published.store(p + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline usize
  try_publish_batch(const T *items, usize count)
  {
    const usize p = __producer.published.get(memory_order_relaxed);
    usize avail = __capacity - (p - __producer.cached_consumer);
    if ( avail < count ) [[unlikely]] {
      __producer.cached_consumer = __consumer.consumed.get(memory_order_acquire);
      avail = __capacity - (p - __producer.cached_consumer);
    }
    const usize n = avail < count ? avail : count;
    if ( n == 0 ) return 0;
    const usize idx = p & __mask;
    const usize first = n < (__capacity - idx) ? n : (__capacity - idx);
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::memcpy(micron::addressof(__mem::memory[idx]), items, first);
      if ( first != n ) micron::memcpy(__mem::memory, items + first, n - first);
    } else {
      usize made = 0;
#if !defined(__micron_freestanding) || defined(__micron_eh)
      try {
#endif
        for ( ; made < n; ++made ) new (micron::addr(__mem::memory[(p + made) & __mask])) T(items[made]);
#if !defined(__micron_freestanding) || defined(__micron_eh)
      } catch ( ... ) {
        for ( usize i = 0; i < made; ++i ) __mem::memory[(p + i) & __mask].~T();
        throw;
      }
#endif
    }
    __producer.published.store(p + n, memory_order_release);
    return n;
  }

  __attribute__((always_inline)) inline bool
  consume(T &out)
  {
    const usize c = __consumer.consumed.get(memory_order_relaxed);
    if ( c == __consumer.cached_published ) {
      __consumer.cached_published = __producer.published.get(memory_order_acquire);
      if ( c == __consumer.cached_published ) return false;
    }
    const usize idx = c & __mask;
    if constexpr ( micron::is_class_v<T> or !micron::is_trivially_copyable_v<T> ) {
      __assign(out, __mem::memory[idx]);
      __mem::memory[idx].~T();
    } else {
      out = __mem::memory[idx];
    }
    __consumer.consumed.store(c + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline usize
  try_consume_batch(T *items, usize count)
  {
    const usize c = __consumer.consumed.get(memory_order_relaxed);
    usize avail = __consumer.cached_published - c;
    if ( avail < count ) {
      __consumer.cached_published = __producer.published.get(memory_order_acquire);
      avail = __consumer.cached_published - c;
    }
    const usize n = avail < count ? avail : count;
    if ( n == 0 ) return 0;
    const usize idx = c & __mask;
    const usize first = n < (__capacity - idx) ? n : (__capacity - idx);
    if constexpr ( micron::is_trivially_copyable_v<T> ) {
      micron::memcpy(items, micron::addressof(__mem::memory[idx]), first);
      if ( first != n ) micron::memcpy(items + first, __mem::memory, n - first);
      __consumer.consumed.store(c + n, memory_order_release);
    } else if constexpr ( __assign_nothrow ) {
      for ( usize i = 0; i < n; ++i ) {
        T &v = __mem::memory[(c + i) & __mask];
        __assign(items[i], v);
        v.~T();
      }
      __consumer.consumed.store(c + n, memory_order_release);
    } else {
      for ( usize i = 0; i < n; ++i ) {
        T &v = __mem::memory[(c + i) & __mask];
        __assign(items[i], v);
        v.~T();
        __consumer.consumed.store(c + i + 1, memory_order_release);
      }
    }
    return n;
  }

  __attribute__((always_inline)) inline bool
  peek(T &out) const
  {
    const usize c = __consumer.consumed.get(memory_order_relaxed);
    const usize p = __producer.published.get(memory_order_acquire);
    if ( c == p ) return false;
    out = __mem::memory[c & __mask];
    return true;
  }

  inline usize
  pub_cursor() const noexcept
  {
    return __producer.published.get(memory_order_acquire);
  }

  inline usize
  cons_cursor() const noexcept
  {
    return __consumer.consumed.get(memory_order_acquire);
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
    const usize p = __producer.published.get(memory_order_relaxed);
    usize avail = __capacity - (p - __producer.cached_consumer);
    if ( avail == 0 ) [[unlikely]] {
      __producer.cached_consumer = __consumer.consumed.get(memory_order_acquire);
      avail = __capacity - (p - __producer.cached_consumer);
      if ( avail == 0 ) return false;
    }
    const usize idx = p & __mask;
    new (micron::addr(__mem::memory[idx])) T{ micron::forward<Args>(args)... };
    __producer.published.store(p + 1u, memory_order_release);
    return true;
  }

  __attribute__((always_inline)) inline T &
  front()
  {
    return __mem::memory[__consumer.consumed.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  front() const
  {
    return __mem::memory[__consumer.consumed.get(memory_order_relaxed) & __mask];
  }

  __attribute__((always_inline)) inline T &
  last()
  {
    return __mem::memory[(__producer.published.get(memory_order_relaxed) - 1u) & __mask];
  }

  __attribute__((always_inline)) inline const T &
  last() const
  {
    return __mem::memory[(__producer.published.get(memory_order_relaxed) - 1u) & __mask];
  }
};

};      // namespace micron
