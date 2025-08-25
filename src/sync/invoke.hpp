//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"

#include "../memory/actions.hpp"

namespace micron
{
// this is genuinely ridiculous :(
// need this for func compliance
template <class> constexpr bool is_reference_wrapper_v = false;
template <class U> constexpr bool is_reference_wrapper_v<micron::reference_wrapper<U>> = true;

template <class T> using remove_cvref_t = micron::remove_cv_t<micron::remove_reference_t<T>>;

template <class C, class Pointed, class Object, class... Args>
constexpr decltype(auto)
invoke_memptr(Pointed C::*member, Object &&object, Args &&...args)
{
  using object_t = remove_cvref_t<Object>;
  constexpr bool is_member_function = micron::is_function_v<Pointed>;
  constexpr bool is_wrapped = is_reference_wrapper_v<object_t>;
  constexpr bool is_derived_object = micron::is_same_v<C, object_t> || micron::is_base_of_v<C, object_t>;

  if constexpr ( is_member_function ) {
    if constexpr ( is_derived_object )
      return (micron::forward<Object>(object).*member)(micron::forward<Args>(args)...);
    else if constexpr ( is_wrapped )
      return (object.get().*member)(micron::forward<Args>(args)...);
    else
      return ((*micron::forward<Object>(object)).*member)(micron::forward<Args>(args)...);
  } else {
    static_assert(micron::is_object_v<Pointed> && sizeof...(args) == 0);
    if constexpr ( is_derived_object )
      return micron::forward<Object>(object).*member;
    else if constexpr ( is_wrapped )
      return object.get().*member;
    else
      return (*micron::forward<Object>(object)).*member;
  }
}

template <class F, class... Args>
constexpr micron::invoke_result_t<F, Args...>
invoke(F &&f, Args &&...args) noexcept(micron::is_nothrow_invocable_v<F, Args...>)
{
  if constexpr ( micron::is_member_pointer_v<micron::remove_cvref_t<F>> )
    return micron::invoke_memptr(f, micron::forward<Args>(args)...);
  else
    return micron::forward<F>(f)(micron::forward<Args>(args)...);
}
template <class R, class F, class... Args>
  requires micron::is_invocable_r_v<R, F, Args...>
constexpr R
invoke_r(F &&f, Args &&...args) noexcept(micron::is_nothrow_invocable_r_v<R, F, Args...>)
{
  if constexpr ( micron::is_void_v<R> )
    micron::invoke(micron::forward<F>(f), micron::forward<Args>(args)...);
  else
    return micron::invoke(micron::forward<F>(f), micron::forward<Args>(args)...);
}
template <class V, class F, class... Args> constexpr bool negate_invocable_impl = false;
template <class F, class... Args>
constexpr bool
    negate_invocable_impl<micron::void_t<decltype(!micron::invoke(micron::declval<F>(), micron::declval<Args>()...))>, F, Args...>
    = true;

template <class F, class... Args> constexpr bool negate_invocable_v = negate_invocable_impl<void, F, Args...>;

template <class F> struct not_fn_t {
  F f;

  template <class... Args, micron::enable_if_t<negate_invocable_v<F &, Args...>, int> = 0>
  constexpr decltype(auto)
  operator()(Args &&...args) & noexcept(noexcept(!micron::invoke(f, micron::forward<Args>(args)...)))
  {
    return !micron::invoke(f, micron::forward<Args>(args)...);
  }

  template <class... Args, micron::enable_if_t<negate_invocable_v<const F &, Args...>, int> = 0>
  constexpr decltype(auto)
  operator()(Args &&...args) const & noexcept(noexcept(!micron::invoke(f, micron::forward<Args>(args)...)))
  {
    return !micron::invoke(f, micron::forward<Args>(args)...);
  }

  template <class... Args, micron::enable_if_t<negate_invocable_v<F, Args...>, int> = 0>
  constexpr decltype(auto)
  operator()(Args &&...args) && noexcept(noexcept(!micron::invoke(micron::move(f), micron::forward<Args>(args)...)))
  {
    return !micron::invoke(micron::move(f), micron::forward<Args>(args)...);
  }

  template <class... Args, micron::enable_if_t<negate_invocable_v<const F, Args...>, int> = 0>
  constexpr decltype(auto)
  operator()(Args &&...args) const && noexcept(noexcept(!micron::invoke(micron::move(f),
                                                                        micron::forward<Args>(args)...)))
  {
    return !micron::invoke(micron::move(f), micron::forward<Args>(args)...);
  }

  // Deleted overloads are needed since C++20
  // for preventing a non-equivalent but well-formed overload to be selected.

  template <class... Args, micron::enable_if_t<!negate_invocable_v<F &, Args...>, int> = 0>
  void operator()(Args &&...) & = delete;

  template <class... Args, micron::enable_if_t<!negate_invocable_v<const F &, Args...>, int> = 0>
  void operator()(Args &&...) const & = delete;

  template <class... Args, micron::enable_if_t<!negate_invocable_v<F, Args...>, int> = 0>
  void operator()(Args &&...) && = delete;

  template <class... Args, micron::enable_if_t<!negate_invocable_v<const F, Args...>, int> = 0>
  void operator()(Args &&...) const && = delete;
};

template <class F>
constexpr micron::not_fn_t<micron::decay_t<F>>
not_fn(F &&f)
{
  return { micron::forward<F>(f) };
}
};
