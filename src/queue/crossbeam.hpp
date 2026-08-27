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
#include "../bits/__backoff.hpp"
#include "../bits/__container.hpp"
#include "../concepts.hpp"
#include "../memory/allocation/resources.hpp"
#include "../new.hpp"
#include "../types.hpp"

#include "../memory/cache.hpp"

namespace micron
{

// crossbeam
//
// bounded multi-producer multi-consumer queue using Vyukov's cell-tag protocol
// each cell carries a sequence tag that producers and consumers CAS-advance
template<typename T, usize N, class Alloc = micron::allocator_serial<>>
  requires micron::is_move_constructible_v<T> and (micron::is_move_assignable_v<T> or micron::is_copy_assignable_v<T>)
           and micron::is_destructible_v<T> and (N > 0 and N <= (static_cast<usize>(-1) / 2 + 1))
class crossbeam
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

  static constexpr u64 __cache_line = cache_line_size();
  static constexpr usize __capacity = __next_pow2(N);
  static constexpr usize __mask = __capacity - 1u;
  static constexpr usize __cell_align = alignof(T) > __cache_line ? alignof(T) : __cache_line;
  using __diff = make_signed_t<usize>;

  enum __cell_state : u8 { __free = 0, __ready = 1, __cancelled = 2, __running = 3 };

  struct alignas(__cell_align) __cell {
    micron::atomic_token<usize> seq;
    micron::atomic_token<u8> state;
    alignas(T) byte storage[sizeof(T)];
  };

  static_assert(sizeof(__diff) == sizeof(usize), "crossbeam: tag difference must match counter width");
  static_assert(__capacity <= (static_cast<usize>(-1) - (__cell_align - 1)) / sizeof(__cell),
                "crossbeam: cell allocation size is not representable");

  chunk<byte> __block{ nullptr, 0 };
  __cell *__cells = nullptr;

  alignas(__cache_line) micron::atomic_token<usize> __tail;

  alignas(__cache_line) micron::atomic_token<usize> __head;

  [[gnu::always_inline]] static inline T *
  __value(__cell &cell) noexcept
  {
    return reinterpret_cast<T *>(cell.storage);
  }

  [[gnu::always_inline]] static inline const T *
  __value(const __cell &cell) noexcept
  {
    return reinterpret_cast<const T *>(cell.storage);
  }

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

  template<typename... Args>
  [[gnu::always_inline]] inline bool
  __emplace(Args &&...args)
  {
    usize pos = __tail.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      __cell &cell = __cells[pos & __mask];
      const usize seq = cell.seq.get(memory_order_acquire);
      const __diff dif = static_cast<__diff>(seq - pos);
      if ( dif == 0 ) {
        if constexpr ( __capacity == 1 ) {
          if ( cell.state.get(memory_order_acquire) != __free ) return false;
        }
        if ( __tail.compare_exchange_weak(pos, pos + 1u, memory_order_relaxed, memory_order_relaxed) ) {
#if !defined(__micron_freestanding) || defined(__micron_eh)
          try {
#endif
            new (micron::addr(*__value(cell))) T(micron::forward<Args>(args)...);
#if !defined(__micron_freestanding) || defined(__micron_eh)
          } catch ( ... ) {
            cell.state.store(__cancelled, memory_order_relaxed);
            cell.seq.store(pos + 1u, memory_order_release);
            throw;
          }
#endif
          cell.state.store(__ready, memory_order_relaxed);
          cell.seq.store(pos + 1u, memory_order_release);
          return true;
        }
        backoff = __spin_backoff(backoff);
      } else if ( dif < 0 ) {
        return false;
      } else {
        backoff = __spin_backoff(backoff);
        pos = __tail.get(memory_order_relaxed);
      }
    }
  }

  [[gnu::always_inline]] inline bool
  __discard()
  {
    usize pos = __head.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      __cell &cell = __cells[pos & __mask];
      const usize seq = cell.seq.get(memory_order_acquire);
      const __diff dif = static_cast<__diff>(seq - (pos + 1u));
      if ( dif == 0 ) {
        if ( __head.compare_exchange_weak(pos, pos + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          if ( cell.state.get(memory_order_relaxed) == __ready ) __value(cell)->~T();
          cell.state.store(__free, memory_order_release);
          cell.seq.store(pos + __capacity, memory_order_release);
          return true;
        }
        backoff = __spin_backoff(backoff);
      } else if ( dif < 0 ) {
        return false;
      } else {
        backoff = __spin_backoff(backoff);
        pos = __head.get(memory_order_relaxed);
      }
    }
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

  crossbeam() : __tail(0), __head(0)
  {
    __block = __allocator_create<Alloc, __cell_align>(allocation_multiply_or_throw(sizeof(__cell), __capacity));
    __cells = reinterpret_cast<__cell *>(__block.ptr);
    for ( usize i = 0; i < __capacity; ++i ) {
      new (&__cells[i]) __cell;
      __cells[i].state.store(__free, memory_order_relaxed);
      __cells[i].seq.store(i, memory_order_relaxed);
    }
  }

  // WARNING: no concurrent producers or consumers may be running
  ~crossbeam()
  {
    if ( __cells ) {
      const usize h = __head.get(memory_order_acquire);
      const usize t = __tail.get(memory_order_acquire);
      for ( usize s = h; s != t; ++s ) {
        const usize idx = s & __mask;
        __cell &cell = __cells[idx];
        if ( cell.seq.get(memory_order_acquire) == s + 1u && cell.state.get(memory_order_relaxed) == __ready ) __value(cell)->~T();
      }
      for ( usize i = 0; i < __capacity; ++i ) __cells[i].~__cell();
      __allocator_destroy<Alloc, __cell_align>(__block);
      __block = { nullptr, 0 };
      __cells = nullptr;
    }
  }

  crossbeam(const crossbeam &) = delete;
  crossbeam(crossbeam &&) = delete;
  crossbeam &operator=(const crossbeam &) = delete;
  crossbeam &operator=(crossbeam &&) = delete;

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
    const usize h = __head.get(memory_order_acquire);
    const usize t = __tail.get(memory_order_acquire);
    const usize n = t - h;
    return n < __capacity ? n : __capacity;
  }

  inline bool
  empty() const noexcept
  {
    const usize h = __head.get(memory_order_acquire);
    return h == __tail.get(memory_order_acquire);
  }

  __attribute__((always_inline)) inline bool
  push(const T &val)
  {
    return __emplace(val);
  }

  __attribute__((always_inline)) inline bool
  push(T &&val)
  {
    return __emplace(micron::move(val));
  }

  // WARNING: only clear if no concurrent writes are occuring
  inline void
  clear()
  {
    while ( __discard() );
  }

  template<typename... Args>
  __attribute__((always_inline)) inline bool
  emplace(Args &&...args)
  {
    return __emplace(micron::forward<Args>(args)...);
  }

  __attribute__((always_inline)) inline bool
  pop(T &out)
  {
    usize pos = __head.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      __cell &cell = __cells[pos & __mask];
      const usize seq = cell.seq.get(memory_order_acquire);
      const __diff dif = static_cast<__diff>(seq - (pos + 1u));
      if ( dif == 0 ) {
        if ( __head.compare_exchange_weak(pos, pos + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          if ( cell.state.get(memory_order_relaxed) == __cancelled ) {
            cell.state.store(__free, memory_order_release);
            cell.seq.store(pos + __capacity, memory_order_release);
            pos = __head.get(memory_order_relaxed);
            backoff = 1u;
            continue;
          }
          cell.state.store(__running, memory_order_relaxed);
          T *value = __value(cell);
#if !defined(__micron_freestanding) || defined(__micron_eh)
          try {
#endif
            __assign(out, *value);
#if !defined(__micron_freestanding) || defined(__micron_eh)
          } catch ( ... ) {
            value->~T();
            cell.state.store(__free, memory_order_release);
            cell.seq.store(pos + __capacity, memory_order_release);
            throw;
          }
#endif
          value->~T();
          cell.state.store(__free, memory_order_release);
          cell.seq.store(pos + __capacity, memory_order_release);
          return true;
        }
        backoff = __spin_backoff(backoff);
      } else if ( dif < 0 ) {
        return false;      // empty
      } else {
        backoff = __spin_backoff(backoff);
        pos = __head.get(memory_order_relaxed);
      }
    }
  }
};

};      // namespace micron
