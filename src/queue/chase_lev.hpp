//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__pause.hpp"
#include "../concepts.hpp"
#include "../memory/cache.hpp"
#include "../new.hpp"
#include "../types.hpp"

// TODO: WIP/INDEV FOR COROUTINES OPTIMIZE THIS LATER

namespace micron
{

namespace __chase_lev_detail
{

constexpr usize
__next_pow2(usize n) noexcept
{
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  if constexpr ( sizeof(usize) > 4 ) {
    constexpr usize __hi = sizeof(usize) * 8u / 2u;
    n |= n >> __hi;
  }
  return n + 1;
}

template<usize N> struct cache_pad {
  char __[N];
};

template<> struct cache_pad<0> {
};

}      // namespace __chase_lev_detail

enum class steal_status : u8 { empty = 0, lost = 1, got = 2 };

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// chase_lev
//
// single-owner work-stealing deque; LIFO, near lock free

template<is_atomic_type T, usize N>
  requires(N > 0)
class chase_lev
{
  static constexpr u64 __cache_line = cache_line_size();
  static constexpr usize __cap = __chase_lev_detail::__next_pow2(N);
  static constexpr i64 __mask = static_cast<i64>(__cap) - 1;

  static constexpr usize __idx_size = sizeof(micron::atomic_token<i64>);
  static constexpr usize __idx_pad = (__cache_line > __idx_size) ? (__cache_line - __idx_size) : 0u;

  micron::atomic_token<T> *__slots = nullptr;

  alignas(__cache_line) micron::atomic_token<i64> __bottom;      // owner write index
  [[no_unique_address]] __chase_lev_detail::cache_pad<__idx_pad> __pad1;

  alignas(__cache_line) micron::atomic_token<i64> __top;      // thief steal index
  [[no_unique_address]] __chase_lev_detail::cache_pad<__idx_pad> __pad2;

public:
  typedef T value_type;
  static constexpr T __empty = T{};

  chase_lev() : __bottom(0), __top(0)
  {
    __slots = static_cast<micron::atomic_token<T> *>(
        ::operator new(sizeof(micron::atomic_token<T>) * __cap, static_cast<std::align_val_t>(alignof(micron::atomic_token<T>))));
    for ( usize i = 0; i < __cap; ++i ) {
      new (&__slots[i]) micron::atomic_token<T>(T{});
    }
  }

  ~chase_lev()
  {
    if ( __slots ) {
      for ( usize i = 0; i < __cap; ++i ) __slots[i].~atomic_token<T>();
      ::operator delete(__slots, static_cast<std::align_val_t>(alignof(micron::atomic_token<T>)));
      __slots = nullptr;
    }
  }

  chase_lev(const chase_lev &) = delete;
  chase_lev(chase_lev &&) = delete;
  chase_lev &operator=(const chase_lev &) = delete;
  chase_lev &operator=(chase_lev &&) = delete;

  static constexpr usize
  capacity() noexcept
  {
    return __cap;
  }

  inline usize
  size() const noexcept
  {
    const i64 b = __bottom.get(memory_order_acquire);
    const i64 t = __top.get(memory_order_acquire);
    return (b > t) ? static_cast<usize>(b - t) : 0u;
  }

  inline bool
  empty() const noexcept
  {
    return __bottom.get(memory_order_acquire) <= __top.get(memory_order_acquire);
  }

  // OWNER ONLY
  [[gnu::always_inline]] inline bool
  push_bottom(T x) noexcept
  {
    const i64 b = __bottom.get(memory_order_relaxed);
    const i64 t = __top.get(memory_order_acquire);
    if ( b - t > static_cast<i64>(__cap) - 1 ) return false;      // full
    __slots[b & __mask].store(x, memory_order_relaxed);
    micron::atom::thread_fence(atomic_release);      // publish the slot before bottom advances
    __bottom.store(b + 1, memory_order_relaxed);
    return true;
  }

  // OWNER ONLY
  [[gnu::always_inline]] inline T
  pop_bottom() noexcept
  {
    const i64 b = __bottom.get(memory_order_relaxed) - 1;
    __bottom.store(b, memory_order_relaxed);
    micron::atom::thread_fence(atomic_seq_cst);      // order bottom-- vs the top load (the key barrier)
    i64 t = __top.get(memory_order_relaxed);
    T x = __empty;
    if ( t <= b ) {
      x = __slots[b & __mask].get(memory_order_relaxed);
      if ( t == b ) {
        // last element: race with thieves on the seq_cst CAS
        i64 expected = t;
        if ( !__top.compare_exchange_strong(expected, t + 1, memory_order_seq_cst, memory_order_relaxed) )
          x = __empty;      // a thief took it
        __bottom.store(b + 1, memory_order_relaxed);
      }
      // t < b : uncontended, no CAS
    } else {
      __bottom.store(b + 1, memory_order_relaxed);      // empty; restore bottom
    }
    return x;
  }

  // ANY NON OWNER
  [[gnu::always_inline]] inline T
  steal_top() noexcept
  {
    i64 t = __top.get(memory_order_acquire);
    micron::atom::thread_fence(atomic_seq_cst);      // order the top load before the bottom load
    const i64 b = __bottom.get(memory_order_acquire);
    if ( t < b ) {
      T x = __slots[t & __mask].get(memory_order_relaxed);
      i64 expected = t;
      if ( !__top.compare_exchange_strong(expected, t + 1, memory_order_seq_cst, memory_order_relaxed) )
        return __empty;      // lost the race
      return x;
    }
    return __empty;
  }

  // ANY NON OWNER
  [[gnu::always_inline]] inline T
  steal_top(steal_status &__st) noexcept
  {
    i64 t = __top.get(memory_order_acquire);
    micron::atom::thread_fence(atomic_seq_cst);      // order the top load before the bottom load
    const i64 b = __bottom.get(memory_order_acquire);
    if ( t < b ) {
      T x = __slots[t & __mask].get(memory_order_relaxed);
      i64 expected = t;
      if ( !__top.compare_exchange_strong(expected, t + 1, memory_order_seq_cst, memory_order_relaxed) ) {
        __st = steal_status::lost;
        return __empty;
      }
      __st = steal_status::got;
      return x;
    }
    __st = steal_status::empty;
    return __empty;
  }
};

template<is_atomic_type T, usize InitN>
  requires(InitN > 0)
class chase_lev_grow
{
  static constexpr u64 __cache_line = cache_line_size();
  static constexpr usize __idx_size = sizeof(micron::atomic_token<i64>);
  static constexpr usize __idx_pad = (__cache_line > __idx_size) ? (__cache_line - __idx_size) : 0u;

  struct __seg {
    i64 __cap;
    i64 __mask;
    __seg *__prev;                         // retain chain (freed at destruction)
    micron::atomic_token<T> *__slots;      // __cap atomic slots
  };

  static __seg *
  __make_seg(i64 __cap, __seg *__prev)
  {
    __seg *__s = static_cast<__seg *>(::operator new(sizeof(__seg)));
    __s->__cap = __cap;
    __s->__mask = __cap - 1;
    __s->__prev = __prev;
    __s->__slots = static_cast<micron::atomic_token<T> *>(::operator new(sizeof(micron::atomic_token<T>) * static_cast<usize>(__cap),
                                                                         static_cast<std::align_val_t>(alignof(micron::atomic_token<T>))));
    for ( i64 __i = 0; __i < __cap; ++__i ) new (&__s->__slots[__i]) micron::atomic_token<T>(T{});
    return __s;
  }

  static void
  __free_seg(__seg *__s) noexcept
  {
    for ( i64 __i = 0; __i < __s->__cap; ++__i ) __s->__slots[__i].~atomic_token<T>();
    ::operator delete(__s->__slots, static_cast<std::align_val_t>(alignof(micron::atomic_token<T>)));
    ::operator delete(__s);
  }

  micron::atomic_token<__seg *> __seg_ptr;      // current backing array (owner stores release; thieves load acquire)
  alignas(__cache_line) micron::atomic_token<i64> __bottom;
  [[no_unique_address]] __chase_lev_detail::cache_pad<__idx_pad> __pad1;
  alignas(__cache_line) micron::atomic_token<i64> __top;
  [[no_unique_address]] __chase_lev_detail::cache_pad<__idx_pad> __pad2;

public:
  typedef T value_type;
  static constexpr T __empty = T{};

  chase_lev_grow() : __seg_ptr(__make_seg(static_cast<i64>(__chase_lev_detail::__next_pow2(InitN)), nullptr)), __bottom(0), __top(0) { }

  ~chase_lev_grow()
  {
    __seg *__s = __seg_ptr.get(memory_order_relaxed);
    while ( __s != nullptr ) {
      __seg *__p = __s->__prev;
      __free_seg(__s);
      __s = __p;
    }
  }

  chase_lev_grow(const chase_lev_grow &) = delete;
  chase_lev_grow(chase_lev_grow &&) = delete;
  chase_lev_grow &operator=(const chase_lev_grow &) = delete;
  chase_lev_grow &operator=(chase_lev_grow &&) = delete;

  inline usize
  capacity() const noexcept
  {
    return static_cast<usize>(__seg_ptr.get(memory_order_acquire)->__cap);
  }

  inline usize
  size() const noexcept
  {
    const i64 b = __bottom.get(memory_order_acquire);
    const i64 t = __top.get(memory_order_acquire);
    return (b > t) ? static_cast<usize>(b - t) : 0u;
  }

  inline bool
  empty() const noexcept
  {
    return __bottom.get(memory_order_acquire) <= __top.get(memory_order_acquire);
  }

  // OWNER ONLY
  [[gnu::always_inline]] inline bool
  push_bottom(T x) noexcept
  {
    const i64 b = __bottom.get(memory_order_relaxed);
    const i64 t = __top.get(memory_order_acquire);
    __seg *__s = __seg_ptr.get(memory_order_relaxed);
    if ( b - t > __s->__cap - 1 ) [[unlikely]] {         // full -> grow
      __seg *__n = __make_seg(__s->__cap * 2, __s);      // link the old array onto the retain chain
      if ( __n == nullptr ) return false;
      for ( i64 __i = t; __i < b; ++__i )
        __n->__slots[__i & __n->__mask].store(__s->__slots[__i & __s->__mask].get(memory_order_relaxed), memory_order_relaxed);
      __seg_ptr.store(__n, memory_order_release);      // publish to thieves
      __s = __n;
    }
    __s->__slots[b & __s->__mask].store(x, memory_order_relaxed);
    micron::atom::thread_fence(atomic_release);
    __bottom.store(b + 1, memory_order_relaxed);
    return true;
  }

  // OWNER ONLY
  [[gnu::always_inline]] inline T
  pop_bottom() noexcept
  {
    const i64 b = __bottom.get(memory_order_relaxed) - 1;
    __seg *__s = __seg_ptr.get(memory_order_relaxed);
    __bottom.store(b, memory_order_relaxed);
    micron::atom::thread_fence(atomic_seq_cst);
    i64 t = __top.get(memory_order_relaxed);
    T x = __empty;
    if ( t <= b ) {
      x = __s->__slots[b & __s->__mask].get(memory_order_relaxed);
      if ( t == b ) {
        i64 expected = t;
        if ( !__top.compare_exchange_strong(expected, t + 1, memory_order_seq_cst, memory_order_relaxed) ) x = __empty;
        __bottom.store(b + 1, memory_order_relaxed);
      }
    } else {
      __bottom.store(b + 1, memory_order_relaxed);
    }
    return x;
  }

  // ANY NON OWNER
  [[gnu::always_inline]] inline T
  steal_top() noexcept
  {
    i64 t = __top.get(memory_order_acquire);
    micron::atom::thread_fence(atomic_seq_cst);
    const i64 b = __bottom.get(memory_order_acquire);
    if ( t < b ) {
      __seg *__s = __seg_ptr.get(memory_order_acquire);
      T x = __s->__slots[t & __s->__mask].get(memory_order_relaxed);
      i64 expected = t;
      if ( !__top.compare_exchange_strong(expected, t + 1, memory_order_seq_cst, memory_order_relaxed) ) return __empty;
      return x;
    }
    return __empty;
  }

  [[gnu::always_inline]] inline T
  steal_top(steal_status &__st) noexcept
  {
    i64 t = __top.get(memory_order_acquire);
    micron::atom::thread_fence(atomic_seq_cst);
    const i64 b = __bottom.get(memory_order_acquire);
    if ( t < b ) {
      __seg *__s = __seg_ptr.get(memory_order_acquire);
      T x = __s->__slots[t & __s->__mask].get(memory_order_relaxed);
      i64 expected = t;
      if ( !__top.compare_exchange_strong(expected, t + 1, memory_order_seq_cst, memory_order_relaxed) ) {
        __st = steal_status::lost;
        return __empty;
      }
      __st = steal_status::got;
      return x;
    }
    __st = steal_status::empty;
    return __empty;
  }
};

};      // namespace micron
