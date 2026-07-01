//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../defs.hpp"
#include "../except.hpp"
#include "../memory/actions.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "coroutine/fiber.hpp"

#include "coro_core.hpp"
#include "frame.hpp"
#include "manual_lifetime.hpp"
#include "quasi_pointer.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::task<T>
//
//
// C++20 coroutine tasks, lazy, move-only, symmetric transfer, result lives inline in the promise

namespace micron
{

template<class T> class task;

namespace __task_impl
{

template<class Promise> struct __final_awaiter {
  bool
  await_ready() const noexcept
  {
    return false;
  }

  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<Promise> __h) const noexcept
  {
    return __h.promise().__on_final();
  }

  void
  await_resume() const noexcept
  {
  }
};

[[noreturn]] inline void
__raise_broken(const char *__msg)
{
  micron::exc<except::library_error>(__msg ? __msg : "micron::task: broken promise");
#if defined(__micron_freestanding) && !defined(__micron_eh)
  __builtin_unreachable();      // exc<> aborted
#endif
}

};      // namespace __task_impl

// by value spec
template<class T> struct __task_promise: coro::__frame_base {
  std::coroutine_handle<> __cont{};      // awaiting continuation (call/awaited) or inline-fork-fallback parent
  coro::manual_lifetime<T> __val;        // holds the result ONLY in __rm_unbound (plain co_await / root)
  void *__ret = nullptr;                 // bound destination: eventual<T>* (__rm_eventual) or T* (__rm_lvalue)
  const char *__err = nullptr;
  u8 __state = 0;      // 0 empty / 1 value / 2 error
  u8 __ret_mode = coro::__rm_unbound;

  __task_promise() noexcept { }

  ~__task_promise()
  {
    if ( __state == 1 && __ret_mode == coro::__rm_unbound ) __val.destroy();
  }

  task<T> get_return_object() noexcept;

  // next handle at final_suspend
  std::coroutine_handle<>
  __on_final() noexcept
  {
    if ( this->__parent != nullptr ) {
      if ( __state == 2 ) {      // a failed child
        if ( __ret_mode == coro::__rm_eventual && __ret != nullptr )
          static_cast<coro::eventual<T> *>(__ret)->__set_error(__err);                          // explicit-read path raises with this
        if ( this->__parent->__first_err == nullptr ) this->__parent->__first_err = __err;      // first-child-error-wins (surfaced at join)
      }
      if ( __cont ) return __cont;      // inline-fork fallback
      return coro::__child_complete(this);
    }
    return __cont ? __cont : std::noop_coroutine();
  }

  std::suspend_always
  initial_suspend() noexcept
  {
    return {};
  }

  __task_impl::__final_awaiter<__task_promise>
  final_suspend() noexcept
  {
    return {};
  }

  template<class U = T>
  void
  return_value(U &&__v) noexcept(micron::is_nothrow_constructible_v<T, U &&>)
  {
    if ( __ret_mode == coro::__rm_eventual ) {
      static_cast<coro::eventual<T> *>(__ret)->__set(micron::forward<U>(__v));      // construct into the eventual
    } else if ( __ret_mode == coro::__rm_lvalue ) {
      if constexpr ( requires(T &__d, U &&__s) { __d = micron::forward<U>(__s); } )
        *static_cast<T *>(__ret) = micron::forward<U>(__v);      // assign into the caller lvalue
      else
        __builtin_trap();      // raw-lvalue fork binding requires an assignable T; use eventual<T>
    } else if ( __ret_mode == coro::__rm_discard ) {
      (void)micron::forward<U>(__v);      // evaluate, drop
    } else {
      __val.construct(micron::forward<U>(__v));      // unbound: into the slot
    }
    __state = 1;
  }

  void
  unhandled_exception() noexcept
  {
    __state = 2;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    __err = "micron::task: unhandled exception";
#else
    __builtin_trap();      // unreachable
#endif
  }

  static void *
  operator new(usize __n) noexcept
  {
    return micron::fiber::__frame_alloc(__n);
  }

  static void
  operator delete(void *__p, usize __n) noexcept
  {
    micron::fiber::__frame_free(__p, __n);
  }

  static task<T> get_return_object_on_allocation_failure() noexcept;

  T
  __take()
  {
    if ( __state == 2 ) [[unlikely]]
      __task_impl::__raise_broken(__err);
    return micron::move(__val).get();
  }
};

// void spec
template<> struct __task_promise<void>: coro::__frame_base {
  std::coroutine_handle<> __cont{};
  void *__ret = nullptr;
  const char *__err = nullptr;
  u8 __state = 0;
  u8 __ret_mode = coro::__rm_unbound;

  __task_promise() noexcept { }

  task<void> get_return_object() noexcept;

  std::coroutine_handle<>
  __on_final() noexcept
  {
    if ( this->__parent != nullptr ) {
      if ( __state == 2 ) {      // a failed child
        if ( __ret_mode == coro::__rm_eventual && __ret != nullptr ) static_cast<coro::eventual<void> *>(__ret)->__set_error(__err);
        if ( this->__parent->__first_err == nullptr ) this->__parent->__first_err = __err;
      }
      if ( __cont ) return __cont;      // inline-fork fallback
      return coro::__child_complete(this);
    }
    return __cont ? __cont : std::noop_coroutine();
  }

  std::suspend_always
  initial_suspend() noexcept
  {
    return {};
  }

  __task_impl::__final_awaiter<__task_promise>
  final_suspend() noexcept
  {
    return {};
  }

  void
  return_void() noexcept
  {
    __state = 1;
  }

  void
  unhandled_exception() noexcept
  {
    __state = 2;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    __err = "micron::task: unhandled exception";
#else
    __builtin_trap();
#endif
  }

  static void *
  operator new(usize __n) noexcept
  {
    return micron::fiber::__frame_alloc(__n);
  }

  static void
  operator delete(void *__p, usize __n) noexcept
  {
    micron::fiber::__frame_free(__p, __n);
  }

  static task<void> get_return_object_on_allocation_failure() noexcept;

  void
  __take()
  {
    if ( __state == 2 ) [[unlikely]]
      __task_impl::__raise_broken(__err);
  }
};

template<class T>
class [[nodiscard("a dropped micron::task never runs and immediately destroys its frame; "
                  "co_await it, sync_wait it, or hand it to the scheduler")]] task
{
public:
  using promise_type = __task_promise<T>;
  using __handle = std::coroutine_handle<promise_type>;

  task() noexcept = default;

  explicit task(__handle __h) noexcept : __coro(__h) { }

  task(const task &) = delete;
  task &operator=(const task &) = delete;

  task(task &&__o) noexcept : __coro(__o.__coro) { __o.__coro = {}; }

  task &
  operator=(task &&__o) noexcept
  {
    if ( this != &__o ) {
      if ( __coro ) __coro.destroy();
      __coro = __o.__coro;
      __o.__coro = {};
    }
    return *this;
  }

  ~task()
  {
    if ( __coro ) __coro.destroy();
  }

  bool
  valid() const noexcept
  {
    return static_cast<bool>(__coro);
  }

  bool
  done() const noexcept
  {
    return __coro && __coro.done();
  }

  __handle
  handle() const noexcept
  {
    return __coro;
  }

  __handle
  release() noexcept
  {
    __handle __h = __coro;
    __coro = {};
    return __h;
  }

  decltype(auto)
  result()
  {
    if ( !__coro ) [[unlikely]]
      __task_impl::__raise_broken("micron::task: result() on an empty/moved-from/allocation-failed task");
    if ( !__coro.done() ) [[unlikely]]
      __task_impl::__raise_broken("micron::task: result() called before the task completed");
    return __coro.promise().__take();
  }

  auto
  operator co_await() && noexcept
  {
    struct awaiter {
      __handle __h;

      bool
      await_ready() const noexcept
      {
        return !__h || __h.done();
      }

      std::coroutine_handle<>
      await_suspend(std::coroutine_handle<> __awaiting) noexcept
      {
        __h.promise().__cont = __awaiting;
        return __h;
      }

      decltype(auto)
      await_resume()
      {
        if ( !__h ) [[unlikely]]
          __task_impl::__raise_broken("micron::task: co_await on a null/allocation-failed task");
        return __h.promise().__take();
      }
    };

    return awaiter{ __coro };
  }

private:
  __handle __coro{};
};

template<class T>
inline task<T>
__task_promise<T>::get_return_object() noexcept
{
  auto __h = task<T>::__handle::from_promise(*this);
  this->__self = __h;      // record the frame's own resume handle for the scheduler
  return task<T>{ __h };
}

template<class T>
inline task<T>
__task_promise<T>::get_return_object_on_allocation_failure() noexcept
{
  return task<T>{};
}

inline task<void>
__task_promise<void>::get_return_object() noexcept
{
  auto __h = task<void>::__handle::from_promise(*this);
  this->__self = __h;
  return task<void>{ __h };
}

inline task<void>
__task_promise<void>::get_return_object_on_allocation_failure() noexcept
{
  return task<void>{};
}

};      // namespace micron
