//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "concepts.hpp"
#include "memory/actions.hpp"
#include "type_traits.hpp"
#include "types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// io::echo and micron::format auto settling of threads && coros
//
// NOTE:
//   .. settle() consumes the result
//   .. arguments settle left to right, all of them, before the first byte is written
//   .. a settled value that is itself settleable is settled again
//   .. a bitfield argument cannot appear in a settling call (no lvalue ref binds to one)
//   .. micron::task<T> is never settled; hard requires cl_sched.hpp

namespace micron
{

struct settle_note {
  const char *pre = "";
  const char *post = "";
  i64 id = 0;
  bool has_id = false;
};

namespace __settle_impl
{

template<typename T>
concept __routine_like = micron::is_class_v<T> && requires(T &__t) {
  typename T::yield_type;
  typename T::result_type;
  { __t.done() } -> micron::convertible_to<bool>;
  __t.finish();
};

template<typename T>
concept __future_like = micron::is_class_v<T> && !__routine_like<T> && requires(T &__t, const T &__ct) {
  __t.get();
  __ct.wait();
  { __ct.valid() } -> micron::convertible_to<bool>;
};

template<typename T>
concept __eventual_like = micron::is_class_v<T> && !__routine_like<T> && !__future_like<T> && requires(const T &__ct) {
  { __ct.__failed() } -> micron::convertible_to<bool>;
};

// thread handles
template<typename T>
concept __thread_like = micron::is_class_v<T> && !__routine_like<T> && !__future_like<T> && !__eventual_like<T> && requires(T &__t) {
  { __t.join() } -> micron::convertible_to<int>;
  { __t.can_join() } -> micron::convertible_to<int>;
  { __t.thread_id() } -> micron::convertible_to<i64>;
};

template<typename T> struct __is_cast: micron::false_type {
};

};      // namespace __settle_impl

template<typename R, typename T> struct settle_cast {
  T &t;
};

template<typename R, typename T>
[[nodiscard]] [[gnu::always_inline]] inline micron::settle_cast<R, T>
as(T &__t) noexcept
{
  return micron::settle_cast<R, T>{ __t };
}

namespace __settle_impl
{

template<typename R, typename T> struct __is_cast<micron::settle_cast<R, T>>: micron::true_type {
};

};      // namespace __settle_impl

template<typename T>
concept settling = __settle_impl::__routine_like<micron::remove_cvref_t<T>> || __settle_impl::__future_like<micron::remove_cvref_t<T>>
                   || __settle_impl::__eventual_like<micron::remove_cvref_t<T>> || __settle_impl::__thread_like<micron::remove_cvref_t<T>>
                   || __settle_impl::__is_cast<micron::remove_cvref_t<T>>::value;

template<typename... Ts>
concept any_settling = (micron::settling<Ts> || ...);

// coro::routine<Y, R>, R != void
template<typename T>
  requires(__settle_impl::__routine_like<micron::remove_cvref_t<T>> && !micron::is_void_v<typename micron::remove_cvref_t<T>::result_type>)
[[gnu::always_inline]] inline typename micron::remove_cvref_t<T>::result_type
settle(T &&__x)
{
  static_assert(!micron::is_const_v<micron::remove_reference_t<T>>,
                "micron::settle: routine::result() is nonconst; settle a mutable handle");
  return __x.result();      // finish() + move the result out of the fiber frame; single-shot
}

// coro::routine<Y, void>
template<typename T>
  requires(__settle_impl::__routine_like<micron::remove_cvref_t<T>> && micron::is_void_v<typename micron::remove_cvref_t<T>::result_type>)
[[gnu::always_inline]] inline micron::settle_note
settle(T &&__x)
{
  static_assert(!micron::is_const_v<micron::remove_reference_t<T>>,
                "micron::settle: routine::finish() is nonconst; settle a mutable handle");
  __x.finish();
  return micron::settle_note{ "<routine done>", "", 0, false };
}

// future<T> / futex_future<T>, T != void (future<T &>::get() returns T &, preserved)
template<typename T>
  requires(__settle_impl::__future_like<micron::remove_cvref_t<T>>
           && !micron::is_void_v<decltype(micron::declval<micron::remove_cvref_t<T> &>().get())>)
[[gnu::always_inline]] inline decltype(auto)
settle(T &&__x)
{
  return __x.get();
}

// future<void> / futex_future<void>
template<typename T>
  requires(__settle_impl::__future_like<micron::remove_cvref_t<T>>
           && micron::is_void_v<decltype(micron::declval<micron::remove_cvref_t<T> &>().get())>)
[[gnu::always_inline]] inline micron::settle_note
settle(T &&__x)
{
  __x.get();
  return micron::settle_note{ "<future ready>", "", 0, false };
}

// coro::eventual<T>
template<typename T>
  requires(__settle_impl::__eventual_like<micron::remove_cvref_t<T>> && requires(micron::remove_cvref_t<T> &__t) { __t.get(); })
[[gnu::always_inline]] inline decltype(auto)
settle(T &&__x)
{
  return __x.get();      // T &; raises if the forked child failed
}

// coro::eventual<void>: has __failed() and operator*() && but no get()
template<typename T>
  requires(__settle_impl::__eventual_like<micron::remove_cvref_t<T>> && !requires(micron::remove_cvref_t<T> &__t) { __t.get(); })
[[gnu::always_inline]] inline micron::settle_note
settle(T &&__x)
{
  micron::move(__x).operator*();      // raises if the forked child failed
  return micron::settle_note{ "<eventual done>", "", 0, false };
}

// thread / group_thread / auto_thread / void_thread / async_thread
template<typename T>
  requires __settle_impl::__thread_like<micron::remove_cvref_t<T>>
[[gnu::always_inline]] inline micron::settle_note
settle(T &&__x)
{
  const i64 __id = static_cast<i64>(__x.thread_id());
  (void)__x.join();
  return micron::settle_note{ "<thread ", " joined>", __id, true };
}

// as<R>(t)
template<typename R, typename T>
[[gnu::always_inline]] inline R
settle(micron::settle_cast<R, T> __c)
{
  R __r = __c.t.template result<R>();      // result<R>() waits on its own
  (void)__c.t.join();
  return __r;
}

namespace __settle_impl
{

template<typename T>
[[gnu::always_inline]] inline decltype(auto)
__one(T &&__x)
{
  if constexpr ( micron::settling<T> )
    return micron::settle(micron::forward<T>(__x));
  else
    return static_cast<const micron::remove_reference_t<T> &>(__x);
}

// settle the args left to right, then invoke __f with the settled values
template<typename F>
[[gnu::always_inline]] inline decltype(auto)
__then(F &&__f)
{
  return __f();
}

template<typename F, typename H, typename... Ts>
inline decltype(auto)
__then(F &&__f, H &&__h, Ts &&...__ts)
{
  decltype(auto) __v = micron::__settle_impl::__one(micron::forward<H>(__h));
  return micron::__settle_impl::__then([&](auto &&...__r) -> decltype(auto) { return __f(__v, micron::forward<decltype(__r)>(__r)...); },
                                       micron::forward<Ts>(__ts)...);
}

};      // namespace __settle_impl

};      // namespace micron
