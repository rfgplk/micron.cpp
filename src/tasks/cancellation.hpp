//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__abc_mt.hpp"      // autofires MICRON_ABC_MT; must precede abcmalloc

#include "../atomic/atomic.hpp"
#include "../types.hpp"

#include "frame.hpp"
#include "task.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cooperative cancellations

namespace micron
{
namespace coro
{

inline void (*__io_cancel_hook)(const micron::atomic_token<u32> *) noexcept = nullptr;

class cancellation_token
{
  const micron::atomic_token<u32> *__f = nullptr;

public:
  cancellation_token() noexcept = default;

  explicit cancellation_token(const micron::atomic_token<u32> *__flag) noexcept : __f(__flag) { }

  [[nodiscard]] bool
  cancelled() const noexcept
  {
    return __f != nullptr && __f->get(micron::memory_order_relaxed) != 0u;
  }

  // true if this token is attached to a source (vs a default-constructed empty token)
  explicit
  operator bool() const noexcept
  {
    return __f != nullptr;
  }

  const micron::atomic_token<u32> *
  __raw() const noexcept
  {
    return __f;
  }
};

class cancellation_source
{
  micron::atomic_token<u32> __flag{ 0 };

public:
  cancellation_source() noexcept = default;
  cancellation_source(const cancellation_source &) = delete;
  cancellation_source &operator=(const cancellation_source &) = delete;

  void
  cancel() noexcept
  {
    __flag.store(1u, micron::memory_order_release);
    void (*__hook)(const micron::atomic_token<u32> *) noexcept = __io_cancel_hook;
    if ( __hook != nullptr ) __hook(&__flag);
  }

  [[nodiscard]] bool
  cancelled() const noexcept
  {
    return __flag.get(micron::memory_order_relaxed) != 0u;
  }

  [[nodiscard]] cancellation_token
  token() const noexcept
  {
    return cancellation_token(&__flag);
  }

  template<class T>
  void
  bind(micron::task<T> &__t) noexcept
  {
    if ( __t.valid() ) __t.handle().promise().__cancel = &__flag;
  }

  void
  __bind_frame(__frame_base *__f) noexcept
  {
    if ( __f != nullptr ) __f->__cancel = &__flag;
  }
};

struct __cancelpoint_awaiter {
  bool __c = false;

  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    const __frame_base *__f = &__h.promise();
    __c = (__f->__cancel != nullptr) && (__f->__cancel->get(micron::memory_order_relaxed) != 0u);
    return __h;
  }

  bool
  await_resume() const noexcept
  {
    return __c;
  }
};

[[nodiscard]] inline __cancelpoint_awaiter
cancelpoint() noexcept
{
  return {};
}

};      // namespace coro
};      // namespace micron
