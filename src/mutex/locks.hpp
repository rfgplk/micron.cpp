//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../except.hpp"
#include "mutex.hpp"

#include "../atomic/flag.hpp"
#include "../sync/yield.hpp"
#include "../types.hpp"

namespace micron
{

enum class lock_starts { defer, adopt, locked, unlocked, attempt };

struct adopt_lock_t {
  explicit adopt_lock_t() = default;
};

inline constexpr adopt_lock_t adopt_lock{};

struct defer_lock_t {
  explicit defer_lock_t() = default;
};

inline constexpr defer_lock_t defer_lock{};

struct try_to_lock_t {
  explicit try_to_lock_t() = default;
};

inline constexpr try_to_lock_t try_to_lock{};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// left-to-right, no ordering, no rollback

template<typename... Locks>
bool
try_lock_in_order(Locks &...locks)      // by-reference (locks are non-copyable)
{
  return (... && locks.try_lock());
}

template<typename... Locks>
void
lock_in_order(Locks &...locks)
{
  (locks.lock(), ...);
}

// use with care
template<typename... Locks>
void
unlock(Locks &...locks)
{
  (locks.unlock(), ...);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// deadlock-free multi-acquire

namespace __impl
{

// the acquisition order has to be chosen at runtime
struct __lock_ref {
  void *obj;
  void (*lock)(void *);
  bool (*try_lock)(void *);
  void (*unlock)(void *);
};

template<typename L>
inline __lock_ref
__make_lock_ref(L &l) noexcept
{
  return __lock_ref{ static_cast<void *>(__builtin_addressof(l)), [](void *p) { (void)static_cast<L *>(p)->lock(); },
                     [](void *p) -> bool { return static_cast<L *>(p)->try_lock(); }, [](void *p) { static_cast<L *>(p)->unlock(); } };
}

inline void
__lock_all_n(__lock_ref *r, usize n)
{
  usize start = 0;
  for ( ;; ) {
    r[start].lock(r[start].obj);

    usize failed = n;
    for ( usize i = 0; i < n; ++i ) {
      if ( i == start ) continue;
      if ( !r[i].try_lock(r[i].obj) ) {
        failed = i;
        break;
      }
    }
    if ( failed == n ) return;

    for ( usize i = 0; i < failed; ++i )
      if ( i != start ) r[i].unlock(r[i].obj);
    r[start].unlock(r[start].obj);

    start = failed;      // next round, block on the one that refused
    micron::yield();
  }
}

inline int
__try_lock_all_n(__lock_ref *r, usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    if ( !r[i].try_lock(r[i].obj) ) {
      for ( usize j = 0; j < i; ++j ) r[j].unlock(r[j].obj);
      return static_cast<int>(i);
    }
  }
  return -1;
}

}      // namespace __impl

// takes every lock, releasing everything it holds whenever one refuses
template<typename... Locks>
void
lock_all(Locks &...locks)
{
  if constexpr ( sizeof...(Locks) == 0 ) {
    return;
  } else if constexpr ( sizeof...(Locks) == 1 ) {
    (locks.lock(), ...);
  } else {
    __impl::__lock_ref r[sizeof...(Locks)] = { __impl::__make_lock_ref(locks)... };
    __impl::__lock_all_n(r, sizeof...(Locks));
  }
}

// -1 when every lock was taken, else the 0-based index of the first that refused
template<typename... Locks>
[[nodiscard]] int
try_lock_all(Locks &...locks) noexcept
{
  if constexpr ( sizeof...(Locks) == 0 ) {
    return -1;
  } else {
    __impl::__lock_ref r[sizeof...(Locks)] = { __impl::__make_lock_ref(locks)... };
    return __impl::__try_lock_all_n(r, sizeof...(Locks));
  }
}

template<typename... Locks>
bool
try_lock(Locks &...locks)      // true iff every lock was acquired
{
  return try_lock_all(locks...) < 0;
}

template<typename... Locks>
void
lock(Locks &...locks)
{
  lock_all(locks...);
}

// variadic RAII over lock_all
template<typename... Locks> class lock_set
{
  static_assert(sizeof...(Locks) > 0, "lock_set needs at least one lock");

  __impl::__lock_ref r[sizeof...(Locks)];
  bool held;

public:
  explicit lock_set(Locks &...locks) : r{ __impl::__make_lock_ref(locks)... }, held(false)
  {
    if constexpr ( sizeof...(Locks) == 1 ) {
      r[0].lock(r[0].obj);
    } else {
      __impl::__lock_all_n(r, sizeof...(Locks));
    }
    held = true;
  }

  lock_set(adopt_lock_t, Locks &...locks) noexcept : r{ __impl::__make_lock_ref(locks)... }, held(true) { }

  ~lock_set() { unlock(); }

  lock_set(const lock_set &) = delete;
  lock_set(lock_set &&) = delete;
  lock_set &operator=(const lock_set &) = delete;

  void
  unlock() noexcept
  {
    if ( !held ) return;
    for ( usize i = sizeof...(Locks); i-- > 0; ) r[i].unlock(r[i].obj);
    held = false;
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

#include "locks/auto_lock.hpp"
#include "locks/guard_lock.hpp"
#include "locks/queue_lock.hpp"
#include "locks/recursive_lock.hpp"
#include "locks/spin_lock.hpp"
#include "locks/unique_lock.hpp"

// the backoff/queue/parking family
#include "locks/clh_lock.hpp"
#include "locks/futex_mutex.hpp"
#include "locks/mcs_lock.hpp"
#include "locks/seqlock.hpp"
#include "locks/shared_mutex.hpp"
#include "locks/ticket_lock.hpp"
#include "locks/ttas_lock.hpp"
