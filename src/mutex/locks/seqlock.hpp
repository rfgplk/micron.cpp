//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../bits/__backoff.hpp"
#include "../../type_traits.hpp"

#include "../mutex.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// seqlock
// readers never write, never block a writer, and never block each other

template<typename T, typename Lock = fast_mutex>
  requires micron::is_trivially_copyable_v<T>
class seqlock
{
  atomic_token<u32> __seq;
  Lock __w;
  T __v;

public:
  using value_type = T;

  ~seqlock() = default;

  seqlock() noexcept : __seq(0), __v() { }

  explicit seqlock(const T &init) noexcept : __seq(0), __v(init) { }

  seqlock(const seqlock &) = delete;
  seqlock(seqlock &&) = delete;
  seqlock &operator=(const seqlock &) = delete;

  [[nodiscard]] T
  load() const noexcept
  {
    T out;
    // spin only
    __lock_backoff<spin_only> bo;
    for ( ;; ) {
      const u32 s0 = __seq.get(memory_order::acquire);
      if ( s0 & 1u ) {      // a writer holds it
        bo.relax();
        continue;
      }
      __builtin_memcpy(__builtin_addressof(out), __builtin_addressof(__v), sizeof(T));
      // the payload read must not sink past the second sequence read, or the check proves nothing
      atom::thread_fence(atomic_acquire);
      if ( __seq.get(memory_order::relaxed) == s0 ) return out;
      bo.relax();
    }
  }

  bool
  try_load(T &out) const noexcept
  {
    const u32 s0 = __seq.get(memory_order::acquire);
    if ( s0 & 1u ) return false;
    __builtin_memcpy(__builtin_addressof(out), __builtin_addressof(__v), sizeof(T));
    atom::thread_fence(atomic_acquire);
    return __seq.get(memory_order::relaxed) == s0;
  }

  void
  store(const T &n) noexcept
  {
    __w.lock();
    const u32 s = __seq.get(memory_order::relaxed);
    __seq.store(s + 1u, memory_order::relaxed);
    atom::thread_fence(atomic_release);
    __builtin_memcpy(__builtin_addressof(__v), __builtin_addressof(n), sizeof(T));
    __seq.store(s + 2u, memory_order::release);
    __w.unlock();
  }

  // inplace mutation
  template<typename Fn>
  void
  write(Fn &&f) noexcept
  {
    __w.lock();
    const u32 s = __seq.get(memory_order::relaxed);
    __seq.store(s + 1u, memory_order::relaxed);
    atom::thread_fence(atomic_release);
    f(__v);
    atom::thread_fence(atomic_release);
    __seq.store(s + 2u, memory_order::release);
    __w.unlock();
  }

  [[nodiscard]] u32
  sequence() const noexcept
  {
    return __seq.get(memory_order::acquire);
  }

  [[nodiscard]] bool
  writing() const noexcept
  {
    return (__seq.get(memory_order::acquire) & 1u) != 0u;
  }
};

};      // namespace micron
