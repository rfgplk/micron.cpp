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

// Vyukov's cell-tag protocol, matching microns src/queue/crossbeam.hpp
// we don't pull in the raw crossbeam headers because it internally calls abcmalloc itself (circular includes + deadlocks)

#pragma once

#include "../../../atomic/atomic.hpp"
#include "../../../bits/__pause.hpp"
#include "../../../memory/cache.hpp"
#include "../../../types.hpp"

namespace abc
{

struct __mpsc_free_payload {
  byte *ptr;
  usize size;
};

template<usize N>
  requires(N > 0 and (N & (N - 1)) == 0)
class __mpsc_free_queue
{

  static constexpr u64 __cache_line = micron::cache_line_size();
  static constexpr usize __capacity = N;
  static constexpr usize __mask = N - 1;

  struct alignas(__cache_line) __cell {
    micron::atomic_token<usize> seq;
    __mpsc_free_payload payload;
  };

  alignas(__cache_line) __cell __cells[__capacity];
  alignas(__cache_line) micron::atomic_token<usize> __tail;
  alignas(__cache_line) usize __head;

  [[gnu::always_inline]] static inline unsigned
  __spin_backoff(unsigned b) noexcept
  {
    for ( unsigned i = 0; i < b; ++i ) __cpu_pause();
    return (b < 64u) ? (b << 1u) : 64u;
  }

public:
  __mpsc_free_queue() noexcept : __tail(0), __head(0)
  {
    for ( usize i = 0; i < __capacity; ++i ) {
      __cells[i].seq.store(i, micron::memory_order_relaxed);
    }
  }

  __mpsc_free_queue(const __mpsc_free_queue &) = delete;
  __mpsc_free_queue(__mpsc_free_queue &&) = delete;
  __mpsc_free_queue &operator=(const __mpsc_free_queue &) = delete;
  __mpsc_free_queue &operator=(__mpsc_free_queue &&) = delete;

  static constexpr usize
  capacity() noexcept
  {
    return __capacity;
  }

  [[gnu::always_inline]] inline bool
  maybe_nonempty() const noexcept
  {
    return __tail.get(micron::memory_order_relaxed) != __head;
  }

  [[gnu::always_inline]] inline bool
  push(byte *p, usize sz) noexcept
  {
    usize tail = __tail.get(micron::memory_order_relaxed);
    unsigned backoff = 1u;
    for ( ;; ) {
      __cell &c = __cells[tail & __mask];
      const usize seq = c.seq.get(micron::memory_order_acquire);
      const long long diff = static_cast<long long>(seq) - static_cast<long long>(tail);
      if ( diff == 0 ) {
        if ( __tail.compare_exchange_weak(tail, tail + 1u, micron::memory_order_relaxed, micron::memory_order_relaxed) ) {
          c.payload.ptr = p;
          c.payload.size = sz;
          c.seq.store(tail + 1u, micron::memory_order_release);
          return true;
        }
        backoff = __spin_backoff(backoff);
        tail = __tail.get(micron::memory_order_relaxed);
      } else if ( diff < 0 ) {
        return false;
      } else {
        backoff = __spin_backoff(backoff);
        tail = __tail.get(micron::memory_order_relaxed);
      }
    }
  }

  [[gnu::always_inline]] inline bool
  pop(__mpsc_free_payload &out) noexcept
  {
    const usize head = __head;
    __cell &c = __cells[head & __mask];
    const usize seq = c.seq.get(micron::memory_order_acquire);
    const long long diff = static_cast<long long>(seq) - static_cast<long long>(head + 1u);
    if ( diff != 0 ) return false;
    out = c.payload;
    c.seq.store(head + __capacity, micron::memory_order_release);
    __head = head + 1u;
    return true;
  }

  template<typename Fn>
  [[gnu::always_inline]] inline u32
  drain(Fn &&fn) noexcept
  {
    u32 n = 0;
    __mpsc_free_payload p;
    while ( pop(p) ) {
      fn(p.ptr, p.size);
      ++n;
    }
    return n;
  }
};

};      // namespace abc
