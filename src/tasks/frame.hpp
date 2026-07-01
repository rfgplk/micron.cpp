//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../atomic/atomic.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../types.hpp"

namespace micron
{
namespace coro
{
struct __frame_base {
  std::coroutine_handle<> __self{};                         // this frame's own handle (resume point)
  __frame_base *__parent = nullptr;                         // parent frame (set on fork children only; non-null => forked)
  __frame_base *__child_head = nullptr;                     // intrusive list of THIS frame's forked children (reclaimed at join)
  __frame_base *__sibling = nullptr;                        // next sibling in the parent's __child_head list
  const char *__first_err = nullptr;                        // first error reported by a forked child (surfaced at co_await join)
  micron::atomic_token<u32> __steals{ 0 };                  // # of this frame's continuations stolen since last join
  micron::atomic_token<u32> __joins{ 0 };                   // slow-path child completions | joiner-armed bit
  u32 __expected = 0;                                       // # slow-path completions awaited (set at join)
  const micron::atomic_token<u32> *__cancel = nullptr;      // shared cancel flag (inherited at fork)

  static constexpr u32 __joiner_bit = 0x80000000u;
  static constexpr u32 __count_mask = 0x7fffffffu;
};

// raised when a forked child fails
[[noreturn]] inline void
__raise_eventual(const char *__msg)
{
  micron::exc<except::library_error>(__msg ? __msg : "micron::eventual: forked child failed");
  __builtin_unreachable();
}

template<class T> class eventual
{
  union {
    T __val;
  };

  const char *__err = nullptr;      // non-null => the forked child failed; reading the result raises
  bool __has = false;

public:
  eventual() noexcept { }

  eventual(const eventual &) = delete;
  eventual &operator=(const eventual &) = delete;

  ~eventual()
  {
    if ( __has ) __val.~T();
  }

  template<class U>
  void
  __set(U &&__v)
  {
    ::new (static_cast<void *>(&__val)) T(micron::forward<U>(__v));
    __has = true;
  }

  // record a failed child
  void
  __set_error(const char *__e) noexcept
  {
    __err = __e ? __e : "micron::eventual: forked child failed";
  }

  bool
  __failed() const noexcept
  {
    return __err != nullptr;
  }

  // consume the result
  T
  operator*() &&
  {
    if ( __err != nullptr ) [[unlikely]]
      __raise_eventual(__err);
    if ( !__has ) [[unlikely]]
      __raise_eventual("micron::eventual: result read before it was produced");
    return micron::move(__val);
  }

  T &
  get() &
  {
    if ( __err != nullptr ) [[unlikely]]
      __raise_eventual(__err);
    if ( !__has ) [[unlikely]]
      __raise_eventual("micron::eventual: result read before it was produced");
    return __val;
  }
};

template<> class eventual<void>
{
  const char *__err = nullptr;      // non-null => the forked child failed; *eventual raises

public:
  eventual() noexcept { }

  eventual(const eventual &) = delete;
  eventual &operator=(const eventual &) = delete;

  void
  __set() noexcept
  {
  }

  void
  __set_error(const char *__e) noexcept
  {
    __err = __e ? __e : "micron::eventual: forked child failed";
  }

  bool
  __failed() const noexcept
  {
    return __err != nullptr;
  }

  void
  operator*() &&
  {
    if ( __err != nullptr ) [[unlikely]]
      __raise_eventual(__err);
  }
};

};      // namespace coro
};      // namespace micron
