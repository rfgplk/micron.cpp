//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../atomic/atomic.hpp"
#include "../../sync/futex.hpp"

#include "../backoff.hpp"
#include "../locks.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reader-writer lock for THREADS

template<spin_policy P = spin_yield> class basic_shared_mutex
{
  static constexpr u32 __wr = 0x80000000u;
  static constexpr u32 __rmask = 0x7FFFFFFFu;

  atomic_token<u32> __s;       // __wr | reader-count
  atomic_token<u32> __ww;      // writers queued (held or waiting)
  [[no_unique_address]] __lock_stats st;

  void
  __wake_all() noexcept
  {
    micron::wake_futex(__s.ptr(), 0x7FFFFFFF);
  }

  void
  __park_on(u32 observed) noexcept
  {
    micron::__futex(__s.ptr(), futex_wait | futex_private_flag, observed, nullptr, nullptr, 0);
  }

  void
  reset() noexcept
  {
    unlock();
  }

public:
  ~basic_shared_mutex() = default;

  constexpr basic_shared_mutex() noexcept : __s(0), __ww(0) { }

  basic_shared_mutex(const basic_shared_mutex &) = delete;
  basic_shared_mutex(basic_shared_mutex &&) = delete;
  basic_shared_mutex &operator=(const basic_shared_mutex &) = delete;

  // exclusive

  auto
  operator()() noexcept
  {
    __ww.fetch_add(1, memory_order::acq_rel);

    __counted_backoff<P> bo(st);
    for ( ;; ) {
      u32 s = __s.get(memory_order::acquire);
      if ( s == 0u ) {
        if ( __s.compare_exchange_weak(s, __wr, memory_order::acquire, memory_order::relaxed) ) break;
        continue;
      }
      if ( bo.next() == spin_step::park ) __park_on(s);
    }

    __ww.sub_fetch(1, memory_order::acq_rel);
    st.note_acquire();
    return &basic_shared_mutex::reset;
  }

  auto
  lock() noexcept
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    u32 e = 0u;
    if ( !__s.compare_exchange_strong(e, __wr, memory_order::acquire, memory_order::relaxed) ) return false;
    st.note_acquire();
    return true;
  }

  void
  unlock() noexcept
  {
    __s.store(0u, memory_order::release);
    __wake_all();
  }

  //   shared

  void
  lock_shared() noexcept
  {
    __counted_backoff<P> bo(st);
    for ( ;; ) {
      if ( __ww.get(memory_order::acquire) != 0u ) {
        bo.relax();
        continue;
      }
      u32 s = __s.get(memory_order::acquire);
      if ( s & __wr ) {
        if ( bo.next() == spin_step::park ) __park_on(s);
        continue;
      }
      if ( __s.compare_exchange_weak(s, s + 1u, memory_order::acquire, memory_order::relaxed) ) {
        st.note_acquire();
        return;
      }
    }
  }

  bool
  try_lock_shared() noexcept
  {
    if ( __ww.get(memory_order::acquire) != 0u ) return false;
    u32 s = __s.get(memory_order::acquire);
    if ( s & __wr ) return false;
    if ( !__s.compare_exchange_strong(s, s + 1u, memory_order::acquire, memory_order::relaxed) ) return false;
    st.note_acquire();
    return true;
  }

  void
  unlock_shared() noexcept
  {
    const u32 n = __s.sub_fetch(1, memory_order::acq_rel);
    if ( (n & __rmask) == __rmask ) [[unlikely]] {
      __s.fetch_add(1, memory_order::acq_rel);
      __wake_all();
      return;
    }
    if ( (n & __rmask) == 0u ) __wake_all();
  }

  // observation

  auto
  retrieve() noexcept
  {
    return &basic_shared_mutex::reset;
  }

  bool
  operator!() const noexcept
  {
    return !is_locked();
  }

  bool
  is_locked() const noexcept
  {
    return __s.get(memory_order::relaxed) != 0u;
  }

  [[nodiscard]] bool
  is_writer_held() const noexcept
  {
    return (__s.get(memory_order::relaxed) & __wr) != 0u;
  }

  [[nodiscard]] u32
  readers() const noexcept
  {
    return __s.get(memory_order::relaxed) & __rmask;
  }

  [[nodiscard]] u32
  writers_queued() const noexcept
  {
    return __ww.get(memory_order::relaxed);
  }

  [[nodiscard]] const __lock_stats &
  stats() const noexcept
  {
    return st;
  }

  template<typename... T> friend void unlock(T &...);
};

using shared_mutex = basic_shared_mutex<spin_yield>;

template<typename M = shared_mutex> class shared_lock
{
  M *mtx;
  bool held;

public:
  ~shared_lock()
  {
    if ( held and mtx ) mtx->unlock_shared();
  }

  shared_lock() noexcept : mtx(nullptr), held(false) { }

  explicit shared_lock(M &m) : mtx(&m), held(false)
  {
    m.lock_shared();
    held = true;
  }

  shared_lock(M &m, defer_lock_t) noexcept : mtx(&m), held(false) { }

  shared_lock(M &m, adopt_lock_t) noexcept : mtx(&m), held(true) { }

  shared_lock(M &m, try_to_lock_t) noexcept : mtx(&m), held(m.try_lock_shared()) { }

  shared_lock(const shared_lock &) = delete;
  shared_lock &operator=(const shared_lock &) = delete;

  shared_lock(shared_lock &&o) noexcept : mtx(o.mtx), held(o.held)
  {
    o.mtx = nullptr;
    o.held = false;
  }

  shared_lock &
  operator=(shared_lock &&o) noexcept
  {
    if ( this == &o ) return *this;
    if ( held and mtx ) mtx->unlock_shared();
    mtx = o.mtx;
    held = o.held;
    o.mtx = nullptr;
    o.held = false;
    return *this;
  }

  void
  lock()
  {
    if ( held or !mtx ) return;
    mtx->lock_shared();
    held = true;
  }

  bool
  try_lock() noexcept
  {
    if ( held or !mtx ) return false;
    held = mtx->try_lock_shared();
    return held;
  }

  void
  unlock() noexcept
  {
    if ( held and mtx ) {
      mtx->unlock_shared();
      held = false;
    }
  }

  M *
  release() noexcept
  {
    M *m = mtx;
    mtx = nullptr;
    held = false;
    return m;
  }

  [[nodiscard]] bool
  owns_lock() const noexcept
  {
    return held;
  }

  explicit
  operator bool() const noexcept
  {
    return held;
  }
};

};      // namespace micron
