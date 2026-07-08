//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../__special/coroutine"

#include "../concepts.hpp"
#include "../memory/actions.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "coro_core.hpp"
#include "frame.hpp"
#include "task.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fork/join (continuation stealing)

namespace micron
{
namespace coro
{

namespace __fork_detail
{

template<class F> struct __args_by_value: micron::true_type {
};

template<class R, class... P> struct __args_by_value<R (*)(P...)>: micron::bool_constant<(... && !micron::is_reference<P>::value)> {
};

template<class R, class... P>
struct __args_by_value<R (*)(P...) noexcept>: micron::bool_constant<(... && !micron::is_reference<P>::value)> {
};

template<class M> struct __mfn_byval: micron::true_type {
};

template<class R, class C, class... P> struct __mfn_byval<R (C::*)(P...)>: micron::bool_constant<(... && !micron::is_reference<P>::value)> {
};

template<class R, class C, class... P>
struct __mfn_byval<R (C::*)(P...) const>: micron::bool_constant<(... && !micron::is_reference<P>::value)> {
};

template<class R, class C, class... P>
struct __mfn_byval<R (C::*)(P...) noexcept>: micron::bool_constant<(... && !micron::is_reference<P>::value)> {
};

template<class R, class C, class... P>
struct __mfn_byval<R (C::*)(P...) const noexcept>: micron::bool_constant<(... && !micron::is_reference<P>::value)> {
};

template<class F>
consteval bool
__fork_args_ok()
{
  using D = micron::decay_t<F>;
  if constexpr ( requires { &D::operator(); } )
    return __mfn_byval<decltype(&D::operator())>::value;
  else
    return __args_by_value<D>::value;
}
};      // namespace __fork_detail

template<class T, class Maker> struct __fork_awaitable {
  void *__dst;      // eventual<T>* | T* | nullptr, interpreted per __mode
  u8 __mode;
  Maker __make;      // () -> task<T>

  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<P> __parent) noexcept
  {
    __frame_base *__par = &__parent.promise();
    const char *__msg = nullptr;
    typename task<T>::__handle __ch{};
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      task<T> __child = __make();
      __ch = __child.release();
    } catch ( ... ) {
      __msg = "micron::fork: child construction threw";
    }
#else
    {
      task<T> __child = __make();
      __ch = __child.release();
    }
#endif
    if ( !__ch && __msg == nullptr ) [[unlikely]]
      __msg = "micron::fork: child frame allocation failed (arena exhausted)";
    if ( __msg != nullptr ) [[unlikely]] {
      // no fork happened
      if ( __mode == __rm_eventual && __dst != nullptr ) static_cast<eventual<T> *>(__dst)->__set_error(__msg);
      if ( __par->__first_err == nullptr ) __par->__first_err = __msg;
      return __parent;
    }
    auto &__cb = __ch.promise();
    __cb.__parent = __par;
    __cb.__cancel = __par->__cancel;      // inherit the cooperative cancel flag (cancellation.hpp)
    __cb.__ret = __dst;                   // deliver the result straight into the bound destination
    __cb.__ret_mode = __mode;
    __cb.__sibling = __par->__child_head;      // link into the parent's reclaim list (owns the frame)
    __par->__child_head = &__cb;

    worker *__w = current_worker();
    __par->__pushed_kind = __frame_base::__kind_fork;                          // tag before the release-publish
    bool __published = (__w != nullptr) && __w->deque.push_bottom(__par);      // publish the parent continuation
    if ( __published ) {
      __notify_work();      // wake a parked worker if any (cheap when all busy)
    } else {
      __cb.__cont = __parent;
    }
    return __ch;      // run the child eagerly (symmetric transfer)
  }

  void
  await_resume() noexcept
  {
  }
};

template<class T, class Maker> struct __call_awaitable {
  void *__dst;      // eventual<T>* | T* | nullptr, interpreted per __mode
  u8 __mode;
  Maker __make;      // () -> task<T>
  typename task<T>::__handle __h{};

  ~__call_awaitable()
  {
    if ( __h ) __h.destroy();
  }

  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<P> __parent) noexcept
  {
    const char *__msg = nullptr;
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      task<T> __child = __make();
      __h = __child.release();
    } catch ( ... ) {
      __msg = "micron::call: child construction threw";
    }
#else
    {
      task<T> __child = __make();
      __h = __child.release();
    }
#endif
    if ( !__h && __msg == nullptr ) [[unlikely]]
      __msg = "micron::call: child frame allocation failed (arena exhausted)";
    if ( __msg != nullptr ) [[unlikely]] {
      if ( __mode == __rm_eventual && __dst != nullptr ) static_cast<eventual<T> *>(__dst)->__set_error(__msg);
      return __parent;
    }
    auto &__cb = __h.promise();
    __cb.__cont = __parent;                           // serial continuation (no __parent, no deque publish)
    __cb.__cancel = __parent.promise().__cancel;      // inherit the cooperative cancel flag
    __cb.__ret = __dst;                               // construct-once delivery into the bound destination
    __cb.__ret_mode = __mode;
    return __h;      // symmetric transfer into the child (eager, serial)
  }

  void
  await_resume() noexcept
  {
  }
};

// WARNING: a forked callable must take its arguments by value
namespace __cf_detail
{
template<class> struct __is_eventual: micron::false_type {
};

template<class T> struct __is_eventual<eventual<T>>: micron::true_type {
};

template<class Fn> struct __fork_binder {
  void *__dst;
  u8 __mode;
  Fn __fn;

  template<class... Args>
  [[nodiscard]] auto
  operator()(Args &&...__args)
  {
    static_assert(__fork_detail::__fork_args_ok<Fn>(),
                  "micron::fork: a forked callable must take its arguments by value; a reference parameter binds "
                  "to the awaitable-owned argument copy and dangles when the continuation is stolen");
    using T = coro::__task_value_t<decltype(__fn(micron::forward<Args>(__args)...))>;
    auto __make
        = [__f = micron::move(__fn), ... __a = micron::forward<Args>(__args)]() mutable -> task<T> { return __f(micron::move(__a)...); };
    return __fork_awaitable<T, decltype(__make)>{ __dst, __mode, micron::move(__make) };
  }
};

template<class Fn> struct __call_binder {
  void *__dst;
  u8 __mode;
  Fn __fn;

  template<class... Args>
  [[nodiscard]] auto
  operator()(Args &&...__args)
  {
    static_assert(__fork_detail::__fork_args_ok<Fn>(), "micron::call: a result-bound callable must take its arguments by value");
    using T = coro::__task_value_t<decltype(__fn(micron::forward<Args>(__args)...))>;
    auto __make
        = [__f = micron::move(__fn), ... __a = micron::forward<Args>(__args)]() mutable -> task<T> { return __f(micron::move(__a)...); };
    return __call_awaitable<T, decltype(__make)>{ __dst, __mode, micron::move(__make) };
  }
};
};      // namespace __cf_detail

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fork: second-order spawn

// fork(eventual<T>& r, fn, args...)
// fork[&a, fn](args...) / fork(&a, fn)(..)
// fork(discard, fn)(args...)
struct __fork_fn {
  template<class T, class Fn, class... Args>
  [[nodiscard]] auto
  operator()(eventual<T> &__out, Fn &&__fn, Args &&...__args) const
  {
    static_assert(__fork_detail::__fork_args_ok<Fn>(), "micron::fork: a forked callable must take its arguments BY VALUE");
    auto __make = [__fn = micron::forward<Fn>(__fn), ... __a = micron::forward<Args>(__args)]() mutable -> task<T> {
      return __fn(micron::move(__a)...);
    };
    return __fork_awaitable<T, decltype(__make)>{ static_cast<void *>(&__out), __rm_eventual, micron::move(__make) };
  }

  template<class I, class Fn>
    requires(!micron::is_function_v<I>)
  [[nodiscard]] auto
  operator()(I *__ret, Fn &&__fn) const
  {
    constexpr u8 __m = __cf_detail::__is_eventual<I>::value ? __rm_eventual : __rm_lvalue;
    return __cf_detail::__fork_binder<micron::decay_t<Fn>>{ static_cast<void *>(__ret), __m, micron::forward<Fn>(__fn) };
  }

  template<class I, class Fn>
    requires(!micron::is_function_v<I>)
  [[nodiscard]] auto
  operator[](I *__ret, Fn &&__fn) const
  {
    return (*this)(__ret, micron::forward<Fn>(__fn));
  }

  template<class Fn>
  [[nodiscard]] auto
  operator()(discard_t, Fn &&__fn) const
  {
    return __cf_detail::__fork_binder<micron::decay_t<Fn>>{ nullptr, __rm_discard, micron::forward<Fn>(__fn) };
  }

  template<class Fn>
  [[nodiscard]] auto
  operator[](discard_t, Fn &&__fn) const
  {
    return (*this)(discard, micron::forward<Fn>(__fn));
  }
};

inline constexpr __fork_fn fork{};

// call: serial async child
//
// call(fn, args...)
// call[&a, fn](args...) / call(&a, fn)(..)
// call(discard, fn)(args...)
struct __call_fn {
  template<class I, class Fn>
    requires(!micron::is_function_v<I>)
  [[nodiscard]] auto
  operator()(I *__ret, Fn &&__fn) const
  {
    constexpr u8 __m = __cf_detail::__is_eventual<I>::value ? __rm_eventual : __rm_lvalue;
    return __cf_detail::__call_binder<micron::decay_t<Fn>>{ static_cast<void *>(__ret), __m, micron::forward<Fn>(__fn) };
  }

  template<class I, class Fn>
    requires(!micron::is_function_v<I>)
  [[nodiscard]] auto
  operator[](I *__ret, Fn &&__fn) const
  {
    return (*this)(__ret, micron::forward<Fn>(__fn));
  }

  template<class Fn>
  [[nodiscard]] auto
  operator()(discard_t, Fn &&__fn) const
  {
    return __cf_detail::__call_binder<micron::decay_t<Fn>>{ nullptr, __rm_discard, micron::forward<Fn>(__fn) };
  }

  template<class Fn>
  [[nodiscard]] auto
  operator[](discard_t, Fn &&__fn) const
  {
    return (*this)(discard, micron::forward<Fn>(__fn));
  }

  template<class Fn, class... Args>
  [[nodiscard]] auto
  operator()(Fn &&__fn, Args &&...__args) const
  {
    return micron::forward<Fn>(__fn)(micron::forward<Args>(__args)...);
  }
};

inline constexpr __call_fn call{};

struct __join_awaitable {
  __frame_base *__self = nullptr;

  bool
  await_ready() const noexcept
  {
    return false;
  }

  template<class P>
  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<P> __h) noexcept
  {
    __self = &__h.promise();
    return __join_suspend(__self);
  }

  void
  await_resume()
  {
    if ( __self != nullptr ) {
      __join_resume(__self);
      if ( __self->__first_err != nullptr ) [[unlikely]] {
        const char *__e = __self->__first_err;
        __self->__first_err = nullptr;
        __raise_eventual(__e);
      }
    }
  }
};

struct __join_tag {
  __join_awaitable
  operator co_await() const noexcept
  {
    return {};
  }
};

inline constexpr __join_tag join{};

};      // namespace coro
};      // namespace micron
