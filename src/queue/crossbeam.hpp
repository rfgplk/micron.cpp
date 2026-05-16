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
#include "../bits/__pause.hpp"
#include "../concepts.hpp"
#include "../memory/allocation/resources.hpp"
#include "../new.hpp"
#include "../types.hpp"

#include "../memory/cache.hpp"

namespace micron
{

namespace __crossbeam_detail
{

// portable cacheline padding
template<usize N> struct cache_pad {
  char __[N];
};

template<> struct cache_pad<0> {
};

// exponential backoff for CAS fail paths
__attribute__((always_inline)) inline unsigned
spin_backoff(unsigned b) noexcept
{
  for ( unsigned i = 0; i < b; ++i ) __cpu_pause();
  return (b < 64u) ? (b << 1u) : 64u;
}

};      // namespace __crossbeam_detail

// crossbeam
//
// bounded multi-producer multi-consumer queue using Vyukov's cell-tag protocol
// each cell carries a sequence tag that producers and consumers CAS-advance
template<is_movable_object T, usize N, class Alloc = micron::allocator_serial<>>
  requires(N > 0)
class crossbeam: public __mutable_memory_resource_move_only<T, Alloc>
{
  // WARNING: the final n |= n >> 32 is UB! when sizeof(usize) == 4 (arm32),
  // since shifting a 32-bit value by 32 bits is UB per C++
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

  // cache-line aligned to eliminate false sharing between neighboring cells under MPMC contention
  struct alignas(__cache_line) padded_seq {
    micron::atomic_token<usize> v;
  };

  static constexpr usize __seq_size = sizeof(micron::atomic_token<usize>);
  static constexpr usize __seq_pad = (__cache_line > __seq_size) ? (__cache_line - __seq_size) : 0u;

  padded_seq *__seqs = nullptr;

  alignas(__cache_line) micron::atomic_token<usize> __tail;
  [[no_unique_address]] __crossbeam_detail::cache_pad<__seq_pad> __pad1;

  alignas(__cache_line) micron::atomic_token<usize> __head;
  [[no_unique_address]] __crossbeam_detail::cache_pad<__seq_pad> __pad2;

public:
  using category_type = buffer_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef const T &const_reference;
  typedef T *pointer;
  typedef const T *const_pointer;

  crossbeam() : __mem(__capacity), __tail(0), __head(0)
  {
    __seqs = static_cast<padded_seq *>(::operator new(sizeof(padded_seq) * __capacity, static_cast<std::align_val_t>(alignof(padded_seq))));
    for ( usize i = 0; i < __capacity; ++i ) {
      new (&__seqs[i]) padded_seq{};
      __seqs[i].v.store(i, memory_order_relaxed);
    }
  }

  // WARNING: no concurrent producers or consumers may be running
  ~crossbeam()
  {
    if ( __seqs ) {
      const usize h = __head.get(memory_order_acquire);
      const usize t = __tail.get(memory_order_acquire);
      for ( usize s = h; s != t; ++s ) {
        const usize idx = s & __mask;
        if ( __seqs[idx].v.get(memory_order_acquire) == s + 1u ) (__mem::memory)[idx].~T();
      }
      for ( usize i = 0; i < __capacity; ++i ) __seqs[i].~padded_seq();
      ::operator delete(__seqs, static_cast<std::align_val_t>(alignof(padded_seq)));
      __seqs = nullptr;
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
    // temporal approximation only, impossible to get exact value while writers are running
    return __tail.get(memory_order_acquire) - __head.get(memory_order_acquire);
  }

  inline bool
  empty() const noexcept
  {
    return __tail.get(memory_order_acquire) == __head.get(memory_order_acquire);
  }

  __attribute__((always_inline)) inline bool
  push(const T &val)
  {
    usize tail = __tail.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      micron::atomic_token<usize> &cell_seq = __seqs[tail & __mask].v;
      const usize seq = cell_seq.get(memory_order_acquire);
      const long long dif = static_cast<long long>(seq) - static_cast<long long>(tail);
      if ( dif == 0 ) {
        if ( __tail.compare_exchange_weak(tail, tail + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          const usize idx = tail & __mask;
          new (micron::addr(__mem::memory[idx])) T{ val };
          cell_seq.store(tail + 1u, memory_order_release);
          return true;
        }
        backoff = __crossbeam_detail::spin_backoff(backoff);
        tail = __tail.get(memory_order_relaxed);
      } else if ( dif < 0 ) {
        return false;      // full
      } else {
        backoff = __crossbeam_detail::spin_backoff(backoff);
        tail = __tail.get(memory_order_relaxed);
      }
    }
  }

  __attribute__((always_inline)) inline bool
  push(T &&val)
  {
    usize tail = __tail.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      micron::atomic_token<usize> &cell_seq = __seqs[tail & __mask].v;
      const usize seq = cell_seq.get(memory_order_acquire);
      const long long dif = static_cast<long long>(seq) - static_cast<long long>(tail);
      if ( dif == 0 ) {
        if ( __tail.compare_exchange_weak(tail, tail + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          const usize idx = tail & __mask;
          new (micron::addr(__mem::memory[idx])) T{ micron::move(val) };
          cell_seq.store(tail + 1u, memory_order_release);
          return true;
        }
        backoff = __crossbeam_detail::spin_backoff(backoff);
        tail = __tail.get(memory_order_relaxed);
      } else if ( dif < 0 ) {
        return false;
      } else {
        backoff = __crossbeam_detail::spin_backoff(backoff);
        tail = __tail.get(memory_order_relaxed);
      }
    }
  }

  // WARNING: only clear if no concurrent writes are occuring
  inline void
  clear() noexcept
  {
    T tmp;
    while ( pop(tmp) );
  }

  template<typename... Args>
  __attribute__((always_inline)) inline bool
  emplace(Args &&...args)
  {
    usize tail = __tail.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      micron::atomic_token<usize> &cell_seq = __seqs[tail & __mask].v;
      const usize seq = cell_seq.get(memory_order_acquire);
      const long long dif = static_cast<long long>(seq) - static_cast<long long>(tail);
      if ( dif == 0 ) {
        if ( __tail.compare_exchange_weak(tail, tail + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          const usize idx = tail & __mask;
          new (micron::addr(__mem::memory[idx])) T{ micron::forward<Args>(args)... };
          cell_seq.store(tail + 1u, memory_order_release);
          return true;
        }
        backoff = __crossbeam_detail::spin_backoff(backoff);
        tail = __tail.get(memory_order_relaxed);
      } else if ( dif < 0 ) {
        return false;
      } else {
        backoff = __crossbeam_detail::spin_backoff(backoff);
        tail = __tail.get(memory_order_relaxed);
      }
    }
  }

  __attribute__((always_inline)) inline bool
  pop(T &out)
  {
    usize head = __head.get(memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      micron::atomic_token<usize> &cell_seq = __seqs[head & __mask].v;
      const usize seq = cell_seq.get(memory_order_acquire);
      const long long dif = static_cast<long long>(seq) - static_cast<long long>(head + 1u);
      if ( dif == 0 ) {
        if ( __head.compare_exchange_weak(head, head + 1u, memory_order_relaxed, memory_order_relaxed) ) {
          const usize idx = head & __mask;
          out = micron::move(__mem::memory[idx]);
          __mem::memory[idx].~T();
          cell_seq.store(head + __capacity, memory_order_release);
          return true;
        }
        backoff = __crossbeam_detail::spin_backoff(backoff);
        head = __head.get(memory_order_relaxed);
      } else if ( dif < 0 ) {
        return false;      // empty
      } else {
        backoff = __crossbeam_detail::spin_backoff(backoff);
        head = __head.get(memory_order_relaxed);
      }
    }
  }
};

};      // namespace micron
