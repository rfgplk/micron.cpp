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
#include "../type_traits.hpp"
#include "../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// static_mpmc
//
// bounded multi-producer multi-consumer ring on Vyukov's cell-tag protocol;
// equivalent to crossbeam but with the storage inline and no alloc req

namespace micron
{

template<is_atomic_type T, usize N>
  requires(N > 0 and (N & (N - 1)) == 0)
class static_mpmc
{
  static constexpr usize __cache_line = cache_line_size();
  static constexpr usize __capacity = N;
  static constexpr usize __mask = N - 1u;

  using __diff = make_signed_t<usize>;
  static_assert(sizeof(__diff) == sizeof(usize), "static_mpmc: tag difference must match counter width");

  struct alignas(__cache_line) __cell {
    micron::atomic_token<usize> seq;
    T v;
  };

  alignas(__cache_line) __cell __cells[__capacity];
  alignas(__cache_line) micron::atomic_token<usize> __tail;      // push side
  alignas(__cache_line) micron::atomic_token<usize> __head;      // pop side

public:
  typedef T value_type;

  static_mpmc() noexcept : __tail(0), __head(0)
  {
    for ( usize i = 0; i < __capacity; ++i ) {
      __cells[i].seq.store(i, memory_order_relaxed);
      __cells[i].v = T{};
    }
  }

  static_mpmc(const static_mpmc &) = delete;
  static_mpmc(static_mpmc &&) = delete;
  static_mpmc &operator=(const static_mpmc &) = delete;
  static_mpmc &operator=(static_mpmc &&) = delete;

  static constexpr usize
  capacity() noexcept
  {
    return __capacity;
  }

  static constexpr usize
  max_size() noexcept
  {
    return __capacity;
  }

  // WARNING: temporal approximation only, exact while writers run is impossible
  inline usize
  size() const noexcept
  {
    return __tail.get(memory_order_acquire) - __head.get(memory_order_acquire);
  }

  // WARNING: as above
  inline bool
  empty() const noexcept
  {
    return __tail.get(memory_order_acquire) == __head.get(memory_order_acquire);
  }

  // relaxed probe, to skip a pop that is certain to fail
  [[gnu::always_inline]] inline bool
  maybe_nonempty() const noexcept
  {
    return __tail.get(memory_order_relaxed) != __head.get(memory_order_relaxed);
  }

  [[gnu::always_inline]] inline bool
  push(T val) noexcept
  {
    usize pos = __tail.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      __cell &c = __cells[pos & __mask];
      const usize seq = c.seq.get(memory_order_acquire);
      const __diff dif = static_cast<__diff>(seq - pos);
      if ( dif == 0 ) {
        // a failed weak CAS already refreshes pos, so the loop reloads nothing
        if ( __tail.compare_exchange_weak(pos, pos + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          c.v = val;
          c.seq.store(pos + 1u, memory_order_release);      // publishes c.v
          return true;
        }
        backoff = __spin_backoff(backoff);
      } else if ( dif < 0 ) {
        return false;      // full
      } else {
        backoff = __spin_backoff(backoff);
        pos = __tail.get(memory_order_relaxed);
      }
    }
  }

  [[gnu::always_inline]] inline bool
  pop(T &out) noexcept
  {
    usize pos = __head.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      __cell &c = __cells[pos & __mask];
      const usize seq = c.seq.get(memory_order_acquire);
      const __diff dif = static_cast<__diff>(seq - (pos + 1u));
      if ( dif == 0 ) {
        if ( __head.compare_exchange_weak(pos, pos + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          out = c.v;
          c.seq.store(pos + __capacity, memory_order_release);      // reopens the cell
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

  // WARNING: only usable when T{} is not a legal payload (the pointer case it exists for)
  [[nodiscard]] [[gnu::always_inline]] inline T
  pop() noexcept
  {
    T out{};
    return pop(out) ? out : T{};
  }

  template<typename Fn>
  inline u32
  drain(Fn &&fn) noexcept
  {
    u32 n = 0;
    T v{};
    while ( pop(v) ) {
      fn(v);
      ++n;
    }
    return n;
  }

  // WARNING: no concurrent producers or consumers may be running
  inline void
  clear() noexcept
  {
    T v{};
    while ( pop(v) );
  }
};

};      // namespace micron
