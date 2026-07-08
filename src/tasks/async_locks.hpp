//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "async_sync.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// RAII guard for async_mutex

namespace micron
{
namespace coro
{

class [[nodiscard]] async_lock_guard
{
  async_mutex *__m = nullptr;
  bool __owns = false;

public:
  async_lock_guard() noexcept = default;

  async_lock_guard(async_mutex *__mtx, bool __owned) noexcept : __m(__mtx), __owns(__owned) { }

  async_lock_guard(async_lock_guard &&__o) noexcept : __m(__o.__m), __owns(__o.__owns)
  {
    __o.__m = nullptr;
    __o.__owns = false;
  }

  async_lock_guard &
  operator=(async_lock_guard &&__o) noexcept
  {
    if ( this != &__o ) {
      if ( __owns && __m ) __m->unlock();
      __m = __o.__m;
      __owns = __o.__owns;
      __o.__m = nullptr;
      __o.__owns = false;
    }
    return *this;
  }

  async_lock_guard(const async_lock_guard &) = delete;
  async_lock_guard &operator=(const async_lock_guard &) = delete;

  ~async_lock_guard()
  {
    if ( __owns && __m ) __m->unlock();
  }

  void
  unlock() noexcept
  {
    if ( __owns && __m ) {
      __m->unlock();
      __owns = false;
    }
  }

  [[nodiscard]] bool
  owns_lock() const noexcept
  {
    return __owns;
  }

  [[nodiscard]] async_mutex *
  mutex() const noexcept
  {
    return __m;
  }
};

struct [[nodiscard]] __scoped_lock_awaiter {
  async_mutex::__lock_awaiter __la;

  bool
  await_ready() noexcept
  {
    return __la.await_ready();
  }

  template<class P>
  bool
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    return __la.await_suspend(__h);
  }

  async_lock_guard
  await_resume() noexcept
  {
    return async_lock_guard{ __la.__m, true };
  }
};

[[nodiscard]] inline __scoped_lock_awaiter
scoped_lock(async_mutex &__m) noexcept
{
  return __scoped_lock_awaiter{ __m.lock() };
}

};      // namespace coro
};      // namespace micron
