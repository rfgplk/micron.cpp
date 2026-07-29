//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__backoff.hpp"
#include "../concepts.hpp"
#include "../memory/cache.hpp"
#include "../new.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// chase_lev
//
// single owner work stealing deque;
// (LIFO for the owner, FIFO for thieves)
//
// used primarily by our coroutine scheduler; now optimized (mostly)

namespace micron
{

namespace __cl_impl
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

constexpr u64
__log2_pow2(u64 n) noexcept
{
  u64 s = 0;
  while ( (1ULL << s) < n ) ++s;
  return s;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// index width
//
// finicky depending on arch
// on armv7-a relaxed loads are ldrexd (exclusive);
// on i386 loads go to a fildll fistpll

using __idx = usize;
using __sidx = micron::make_signed_t<usize>;

[[gnu::always_inline]] constexpr __sidx
__sdiff(__idx __a, __idx __b) noexcept
{
  return static_cast<__sidx>(__a - __b);
}

[[gnu::always_inline]] inline __idx
__cl_dec_bottom_load_top(micron::atomic_token<__idx> &__bottom, micron::atomic_token<__idx> &__top, __idx __b) noexcept
{
#if defined(__micron_arch_arm64)
  __bottom.store(__b, memory_order_seq_cst);      // stlr
  return __top.get(memory_order_seq_cst);         // ldar
#else
  __bottom.store(__b, memory_order_relaxed);
  micron::atom::thread_fence(atomic_seq_cst);
  return __top.get(memory_order_relaxed);
#endif
}

};      // namespace __cl_impl

using __cl_impl::__idx;
using __cl_impl::__sdiff;
using __cl_impl::__sidx;

enum class steal_status : u8 { empty = 0, lost = 1, got = 2 };

template<is_atomic_type T> struct steal_result {
  T __v;
  steal_status __st;
  bool __more;
};

template<is_atomic_type T, usize N>
  requires(N > 0)
class chase_lev
{
  static constexpr u64 __cache_line = cache_line_size();
  static constexpr usize __cap = __cl_impl::__next_pow2(N);
  static constexpr __idx __mask = static_cast<__idx>(__cap) - 1;

  using __slot = micron::atomic_token<T>;

  // cache line 0
  alignas(__cache_line) micron::atomic_token<__idx> __bottom;
  __slot *__slots = nullptr;
  __idx __top_cache = 0;
  // no explicit padding

  // cache line 1, touched on last elem
  alignas(__cache_line) micron::atomic_token<__idx> __top;
  [[no_unique_address]] __cache_pad<__cache_line - sizeof(__idx)> __pad1;      // keep a neighbour off this line

  // must always be inlined; runs significantly worse without (even when taking into account inflated inst pressure)
  [[gnu::always_inline]] inline T
  __pop_cold(__idx __b, __idx __t) noexcept
  {
    T __x = __empty;
    if ( __t == __b ) {
      __x = __slots[__b & __mask].get(memory_order_relaxed);
      __idx __e = __t;
      if ( !__top.compare_exchange_strong(__e, __t + 1, memory_order_seq_cst, memory_order_relaxed) ) __x = __empty;      // a thief took it
    }
    __bottom.store(__b + 1, memory_order_relaxed);
    return __x;
  }

  // cold tail
  [[gnu::noinline, gnu::cold]] bool
  __push_full(__idx __b) noexcept
  {
    __top_cache = __top.get(memory_order_relaxed);
    return __sdiff(__b, __top_cache) <= static_cast<__sidx>(__mask);
  }

  [[gnu::noinline]] steal_result<T>
  __take_top(__idx __t, __idx __b) noexcept
  {
    const T __x = __slots[__t & __mask].get(memory_order_relaxed);
    __idx __e = __t;
    // weak is better, keep it
    if ( !__top.compare_exchange_weak(__e, __t + 1, memory_order_seq_cst, memory_order_relaxed) ) [[unlikely]]
      return { __empty, steal_status::lost, true };
    return { __x, steal_status::got, __sdiff(__b, __t) > 1 };
  }

public:
  typedef T value_type;
  static constexpr T __empty = T{};

  chase_lev() : __bottom(0), __top(0)
  {
    __slots = static_cast<__slot *>(::operator new(sizeof(__slot) * __cap, static_cast<std::align_val_t>(__cache_line)));
    for ( usize i = 0; i < __cap; ++i ) new (&__slots[i]) __slot(T{});
  }

  ~chase_lev()
  {
    if ( __slots ) {
      for ( usize i = 0; i < __cap; ++i ) __slots[i].~__slot();
      ::operator delete(__slots, static_cast<std::align_val_t>(__cache_line));
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
    const __idx b = __bottom.get(memory_order_acquire);
    const __idx t = __top.get(memory_order_acquire);
    const __sidx d = __sdiff(b, t);
    return d > 0 ? static_cast<usize>(d) : 0u;
  }

  inline bool
  empty() const noexcept
  {
    return __sdiff(__bottom.get(memory_order_acquire), __top.get(memory_order_acquire)) <= 0;
  }

  // OWNER ONLY
  [[gnu::always_inline]] inline bool
  push_bottom(T x) noexcept
  {
    const __idx b = __bottom.get(memory_order_relaxed);
    if ( __sdiff(b, __top_cache) > static_cast<__sidx>(__mask) ) [[unlikely]] {
      if ( !__push_full(b) ) return false;      // genuinely full
    }
    __slots[b & __mask].store(x, memory_order_relaxed);
    // release store, not a release fence
    __bottom.store(b + 1, memory_order_release);
    return true;
  }

  // OWNER ONLY (no CAS nor restore)
  [[gnu::always_inline]] inline T
  pop_bottom() noexcept
  {
    const __idx b = __bottom.get(memory_order_relaxed) - 1;
    const __idx t = __cl_impl::__cl_dec_bottom_load_top(__bottom, __top, b);
    if ( __sdiff(b, t) > 0 ) [[likely]]
      return __slots[b & __mask].get(memory_order_relaxed);
    return __pop_cold(b, t);
  }

  [[gnu::always_inline]] inline steal_result<T>
  try_steal() noexcept
  {
    // don't use a fence
    const __idx t = __top.get(memory_order_acquire);
    const __idx b = __bottom.get(memory_order_seq_cst);      // seq_cst, not acquire
    if ( __sdiff(b, t) <= 0 ) [[likely]]
      return { __empty, steal_status::empty, false };
    return __take_top(t, b);
  }

  // ANY NON OWNER
  [[gnu::always_inline]] inline T
  steal_top() noexcept
  {
    return try_steal().__v;
  }

  // ANY NON OWNER
  [[gnu::always_inline]] inline T
  steal_top(steal_status &__st) noexcept
  {
    const steal_result<T> r = try_steal();
    __st = r.__st;
    return r.__v;
  }
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// chase_lev_grow
//
// doubles instead of failing; default

template<is_atomic_type T, usize InitN>
  requires(InitN > 0)
class chase_lev_grow
{
  static constexpr u64 __cache_line = cache_line_size();
  static constexpr u64 __tag_mask = 63ULL;      // low 6 bits of a 64b aligned pointer

  using __slot = micron::atomic_token<T>;

  struct __hdr {
    i64 __cap;
    __hdr *__prev;
  };

  [[gnu::always_inline]] static __hdr *
  __hdr_of(u64 __g) noexcept
  {
    return reinterpret_cast<__hdr *>(__g & ~__tag_mask);
  }

  [[gnu::always_inline]] static __slot *
  __slots_of(u64 __g) noexcept
  {
    return reinterpret_cast<__slot *>((__g & ~__tag_mask) + __cache_line);
  }

  [[gnu::always_inline]] static __idx
  __mask_of(u64 __g) noexcept
  {
    return (static_cast<__idx>(1) << (__g & __tag_mask)) - 1;
  }

  // returns the tagged base word
  static u64
  __make_seg(u64 __shift, __hdr *__prev)
  {
    const i64 __cap = static_cast<i64>(1ULL << __shift);
    void *__raw = ::operator new(__cache_line + sizeof(__slot) * static_cast<usize>(__cap), static_cast<std::align_val_t>(__cache_line));
    __hdr *__h = static_cast<__hdr *>(__raw);
    __h->__cap = __cap;
    __h->__prev = __prev;
    __slot *__s = reinterpret_cast<__slot *>(reinterpret_cast<byte *>(__raw) + __cache_line);
    for ( i64 __i = 0; __i < __cap; ++__i ) new (&__s[__i]) __slot(T{});
    return reinterpret_cast<u64>(__raw) | __shift;
  }

  static void
  __free_seg(__hdr *__h) noexcept
  {
    __slot *__s = reinterpret_cast<__slot *>(reinterpret_cast<byte *>(__h) + __cache_line);
    for ( i64 __i = 0; __i < __h->__cap; ++__i ) __s[__i].~__slot();
    ::operator delete(static_cast<void *>(__h), static_cast<std::align_val_t>(__cache_line));
  }

  alignas(__cache_line) micron::atomic_token<__idx> __bottom;
  micron::atomic_token<u64> __base;
  __idx __top_cache = 0;
  // no padding

  alignas(__cache_line) micron::atomic_token<__idx> __top;
  [[no_unique_address]] __cache_pad<__cache_line - sizeof(__idx)> __pad1;      // keep a neighbour off this line

  [[gnu::noinline, gnu::cold]] u64
  __grow(__idx __b, __idx __t) noexcept
  {
    const u64 __g = __base.get(memory_order_relaxed);
    __hdr *__oh = __hdr_of(__g);
    const u64 __ns = static_cast<u64>(__cl_impl::__log2_pow2(static_cast<u64>(__oh->__cap) * 2ULL));
    const u64 __ng = __make_seg(__ns, __oh);
    __slot *__new = __slots_of(__ng);
    __slot *__old = __slots_of(__g);
    const __idx __nm = __mask_of(__ng);
    const __idx __om = __mask_of(__g);
    for ( __idx __i = __t; __sdiff(__b, __i) > 0; ++__i )
      __new[__i & __nm].store(__old[__i & __om].get(memory_order_relaxed), memory_order_relaxed);
    __base.store(__ng, memory_order_release);      // pairs with the thief's acquire load
    return __ng;
  }

  [[gnu::noinline, gnu::cold]] u64
  __push_full(__idx __b, u64 __g) noexcept
  {
    __top_cache = __top.get(memory_order_relaxed);
    if ( __sdiff(__b, __top_cache) <= static_cast<__sidx>(__mask_of(__g)) ) return __g;      // the cache was just stale
    return __grow(__b, __top_cache);
  }

  [[gnu::always_inline]] inline T
  __pop_cold(__idx __b, __idx __t, u64 __g) noexcept
  {
    T __x = __empty;
    if ( __t == __b ) {
      __x = __slots_of(__g)[__b & __mask_of(__g)].get(memory_order_relaxed);
      __idx __e = __t;      // STRONG: see the fixed variant
      if ( !__top.compare_exchange_strong(__e, __t + 1, memory_order_seq_cst, memory_order_relaxed) ) __x = __empty;
    }
    __bottom.store(__b + 1, memory_order_relaxed);
    return __x;
  }

  [[gnu::noinline]] steal_result<T>
  __take_top(__idx __t, __idx __b) noexcept
  {
    const u64 __g = __base.get(memory_order_acquire);      // pairs with __grow's release
    const T __x = __slots_of(__g)[__t & __mask_of(__g)].get(memory_order_relaxed);
    __idx __e = __t;
    if ( !__top.compare_exchange_weak(__e, __t + 1, memory_order_seq_cst, memory_order_relaxed) ) [[unlikely]]
      return { __empty, steal_status::lost, true };
    return { __x, steal_status::got, __sdiff(__b, __t) > 1 };
  }

public:
  typedef T value_type;
  static constexpr T __empty = T{};

  chase_lev_grow() : __bottom(0), __base(__make_seg(__cl_impl::__log2_pow2(__cl_impl::__next_pow2(InitN)), nullptr)), __top(0) { }

  ~chase_lev_grow()
  {
    __hdr *__h = __hdr_of(__base.get(memory_order_relaxed));
    while ( __h != nullptr ) {
      __hdr *__p = __h->__prev;
      __free_seg(__h);
      __h = __p;
    }
  }

  chase_lev_grow(const chase_lev_grow &) = delete;
  chase_lev_grow(chase_lev_grow &&) = delete;
  chase_lev_grow &operator=(const chase_lev_grow &) = delete;
  chase_lev_grow &operator=(chase_lev_grow &&) = delete;

  inline usize
  capacity() const noexcept
  {
    return static_cast<usize>(__mask_of(__base.get(memory_order_acquire))) + 1u;
  }

  inline usize
  size() const noexcept
  {
    const __idx b = __bottom.get(memory_order_acquire);
    const __idx t = __top.get(memory_order_acquire);
    const __sidx d = __sdiff(b, t);
    return d > 0 ? static_cast<usize>(d) : 0u;
  }

  inline bool
  empty() const noexcept
  {
    return __sdiff(__bottom.get(memory_order_acquire), __top.get(memory_order_acquire)) <= 0;
  }

  [[gnu::always_inline]] inline bool
  push_bottom(T x) noexcept
  {
    const __idx b = __bottom.get(memory_order_relaxed);
    u64 g = __base.get(memory_order_relaxed);      // same line as __bottom
    if ( __sdiff(b, __top_cache) > static_cast<__sidx>(__mask_of(g)) ) [[unlikely]]
      g = __push_full(b, g);
    __slots_of(g)[b & __mask_of(g)].store(x, memory_order_relaxed);
    __bottom.store(b + 1, memory_order_release);      // also publishes __grow's copy above
    return true;
  }

  // OWNER ONLY
  [[gnu::always_inline]] inline T
  pop_bottom() noexcept
  {
    const __idx b = __bottom.get(memory_order_relaxed) - 1;
    const u64 g = __base.get(memory_order_relaxed);      // ahead of the barrier: latency hides there
    const __idx t = __cl_impl::__cl_dec_bottom_load_top(__bottom, __top, b);
    if ( __sdiff(b, t) > 0 ) [[likely]]
      return __slots_of(g)[b & __mask_of(g)].get(memory_order_relaxed);
    return __pop_cold(b, t, g);
  }

  // ANY NON OWNER
  [[gnu::always_inline]] inline steal_result<T>
  try_steal() noexcept
  {
    const __idx t = __top.get(memory_order_acquire);
    const __idx b = __bottom.get(memory_order_seq_cst);
    if ( __sdiff(b, t) <= 0 ) [[likely]]
      return { __empty, steal_status::empty, false };
    return __take_top(t, b);
  }

  [[gnu::always_inline]] inline T
  steal_top() noexcept
  {
    return try_steal().__v;
  }

  [[gnu::always_inline]] inline T
  steal_top(steal_status &__st) noexcept
  {
    const steal_result<T> r = try_steal();
    __st = r.__st;
    return r.__v;
  }
};

};      // namespace micron
