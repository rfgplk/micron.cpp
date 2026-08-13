//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../bits/__backoff.hpp"

namespace micron
{
// NOTE: a seq_cst test-and-set spin lock; hard correct,
// on x86 the unlock store compiles to a full barrier (xchg / mov+mfence)
// use only for _STRICT_ global ordering across unrelated atomics
class mutex
{
  atomic_token<bool> tk;

  // one release definition per lock
  void
  reset()
  {
    unlock();
  }

public:
  ~mutex() = default;
  mutex() = default;

  auto
  operator()()
  {
    default_backoff bo;
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) ) {
      do {
        bo.relax();
      } while ( tk.get(memory_order::relaxed) == ATOMIC_LOCKED );      // TTAS: relaxed-read spin, CAS only when observed OPEN
    };
    return &mutex::reset;
  }

  bool
  operator!()
  {
    return (tk.get() != ATOMIC_LOCKED);
  }

  auto
  lock()
  {
    default_backoff bo;
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) ) {
      do {
        bo.relax();
      } while ( tk.get(memory_order::relaxed) == ATOMIC_LOCKED );      // TTAS: relaxed-read spin, CAS only when observed OPEN
    };
    return &mutex::reset;
  }

  bool
  try_lock() noexcept
  {
    return tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED);
  }

  void
  unlock() noexcept
  {
    tk.store(ATOMIC_OPEN);
  }

  auto
  retrieve()
  {
    return &mutex::reset;
  }

  bool
  is_locked() const noexcept
  {
    return tk.get() == ATOMIC_LOCKED;
  }

  template<typename... T> friend void unlock(T &...);

  mutex(const mutex &) = delete;
  mutex(mutex &&) = delete;
  mutex &operator=(const mutex &) = delete;
};

// no-op lock satisfying is_mutex; lock-policy parameter for single-threaded
// instantiations, every call compiles to nothing
class null_lock
{
  void
  reset() noexcept
  {
  }

public:
  ~null_lock() = default;
  null_lock() = default;

  auto
  operator()() noexcept
  {
    return &null_lock::reset;
  }

  auto
  retrieve() noexcept
  {
    return &null_lock::reset;
  }

  bool
  operator!() const noexcept
  {
    return true;      // never held
  }

  // returns the PMF
  auto
  lock() noexcept
  {
    return &null_lock::reset;
  }

  bool
  try_lock() noexcept
  {
    return true;
  }

  void
  unlock() noexcept
  {
  }

  bool
  is_locked() const noexcept
  {
    return false;
  }

  null_lock(const null_lock &) = delete;
  null_lock(null_lock &&) = delete;
  null_lock &operator=(const null_lock &) = delete;
};

class weak_mutex
{
  atomic_token<bool> tk;

  void
  reset()
  {
    unlock();
  }

public:
  ~weak_mutex() = default;
  weak_mutex() = default;

  auto
  operator()()
  {
    default_backoff bo;
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed) ) {
      do {
        bo.relax();
      } while ( tk.get(memory_order::relaxed) == ATOMIC_LOCKED );      // TTAS: relaxed-read spin, CAS only when observed OPEN
    };
    return &weak_mutex::reset;
  }

  bool
  operator!()
  {
    return (tk.get(memory_order::relaxed) != ATOMIC_LOCKED);
  }

  auto
  lock()
  {
    default_backoff bo;
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed) ) {
      do {
        bo.relax();
      } while ( tk.get(memory_order::relaxed) == ATOMIC_LOCKED );      // TTAS: relaxed-read spin, CAS only when observed OPEN
    };
    return &weak_mutex::reset;
  }

  bool
  try_lock() noexcept
  {
    return tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed);
  }

  void
  unlock() noexcept
  {
    tk.store(ATOMIC_OPEN, memory_order::release);
  }

  auto
  retrieve()
  {
    return &weak_mutex::reset;
  }

  bool
  is_locked() const noexcept
  {
    return tk.get(memory_order::relaxed) == ATOMIC_LOCKED;
  }

  template<typename... T> friend void unlock(T &...);

  weak_mutex(const weak_mutex &) = delete;
  weak_mutex(weak_mutex &&) = delete;
  weak_mutex &operator=(const weak_mutex &) = delete;
};

// uncontended test-and-test-and-set spin lock (acq/rel)
class fast_mutex
{
  atomic_token<bool> tk;

  void
  reset()
  {
    unlock();
  }

public:
  ~fast_mutex() = default;
  fast_mutex() = default;

  auto
  operator()()
  {
    default_backoff bo;
    while ( !tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed) ) {
      do {
        bo.relax();
      } while ( tk.get(memory_order::relaxed) == ATOMIC_LOCKED );
    }
    return &fast_mutex::reset;
  }

  bool
  operator!()
  {
    return (tk.get(memory_order::relaxed) != ATOMIC_LOCKED);
  }

  auto
  lock()
  {
    return operator()();
  }

  bool
  try_lock() noexcept
  {
    return tk.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED, memory_order::acquire, memory_order::relaxed);
  }

  void
  unlock() noexcept
  {
    tk.store(ATOMIC_OPEN, memory_order::release);
  }

  auto
  retrieve()
  {
    return &fast_mutex::reset;
  }

  bool
  is_locked() const noexcept
  {
    return tk.get(memory_order::relaxed) == ATOMIC_LOCKED;
  }

  template<typename... T> friend void unlock(T &...);

  fast_mutex(const fast_mutex &) = delete;
  fast_mutex(fast_mutex &&) = delete;
  fast_mutex &operator=(const fast_mutex &) = delete;
};

struct mcs_node {
  atomic_token<mcs_node *> next;      // successor in the queue
  atomic_token<bool> waiting;

  // constexpr so a thread_local array of these is CONSTANT-initialized -- mcs_lock's per-thread
  // slot table would otherwise need a TLS guard variable and __cxa_thread_atexit, which is exactly
  // the machinery that drops registrations past MICRON_TDTOR_CAP in freestanding
  constexpr mcs_node() noexcept : next(nullptr), waiting(false) { }
};

class queuing_mutex
{
public:
  using node_type = mcs_node;

private:
  atomic_token<mcs_node *> tail;

#if defined(MICRON_LOCK_STATS)
  // arrivals, counted at the ONLY point that fixes queue position: after the tail swap. a test
  // cannot pin arrival order from outside -- a waiter blocks the instant it enqueues, so anything
  // it publishes beforehand races the swap. absent unless MICRON_LOCK_STATS (see mutex/backoff.hpp).
  atomic_token<u32> __enqueued{ 0 };
#endif

  void
  do_unlock(mcs_node &node) noexcept
  {
    mcs_node *expected_next = node.next.get(memory_order::acquire);

    if ( !expected_next ) {
      mcs_node *observed = &node;
      if ( tail.compare_exchange_strong(observed, nullptr, memory_order::acq_rel, memory_order::acquire) ) {
        return;
      }
      if ( observed == nullptr ) return;

      default_backoff bo;
      [[maybe_unused]] u64 __rounds = 0;
      while ( !(expected_next = node.next.get(memory_order::acquire)) ) {
        bo.relax();
        if ( ++__rounds == (1ull << 24) ) __micron_lock_misuse("queuing_mutex::unlock: node is not the queue head");
      }
    }

    expected_next->waiting.store(false, memory_order::release);
  }

public:
  queuing_mutex() noexcept : tail(nullptr) { }

  queuing_mutex(const queuing_mutex &) = delete;
  queuing_mutex(queuing_mutex &&) = delete;
  queuing_mutex &operator=(const queuing_mutex &) = delete;

  auto
  operator()(mcs_node &node) noexcept
  {
    node.next.store(nullptr, memory_order::relaxed);
    node.waiting.store(true, memory_order::relaxed);

    mcs_node *prev = tail.swap(&node);
#if defined(MICRON_LOCK_STATS)
    __enqueued.fetch_add(1, memory_order::acq_rel);
#endif

    if ( prev ) {
      prev->next.store(&node, memory_order::release);
      default_backoff bo;
      while ( node.waiting.get(memory_order::acquire) ) bo.relax();
    }
    return &queuing_mutex::do_unlock;
  }

  void
  lock(mcs_node &node) noexcept
  {
    operator()(node);
  }

  bool
  try_lock(mcs_node &node) noexcept
  {
    if ( tail.get(memory_order::relaxed) != nullptr ) return false;

    node.next.store(nullptr, memory_order::relaxed);
    node.waiting.store(false, memory_order::relaxed);

    mcs_node *expected = nullptr;
    return tail.compare_and_swap(expected, &node);
  }

  void
  unlock(mcs_node &node) noexcept
  {
    do_unlock(node);
  }

  bool
  is_locked() const noexcept
  {
    return tail.get(memory_order::relaxed) != nullptr;
  }

  // threads that have taken a queue position, in arrival order. 0 unless MICRON_LOCK_STATS.
  [[nodiscard]] u32
  enqueued() const noexcept
  {
#if defined(MICRON_LOCK_STATS)
    return __enqueued.get(memory_order::acquire);
#else
    return 0;
#endif
  }
};

};      // namespace micron
