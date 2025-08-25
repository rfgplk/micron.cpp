//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"

// thought really hard on what to name this file

namespace micron
{
template <typename T>
constexpr T &&
forward(micron::remove_reference_t<T> &t) noexcept
{
  return static_cast<T &&>(t);
}

template <typename T>
constexpr T &&
forward(micron::remove_reference_t<T> &&t) noexcept
{
  static_assert(!micron::is_lvalue_reference_v<T>, "Cannot forward an rvalue as an lvalue.");
  return static_cast<T &&>(t);
}

template <typename T, typename U>
constexpr T &&
forward_like(micron::remove_reference_t<U> &&u) noexcept
{
  static_assert(!micron::is_lvalue_reference_v<U>, "Cannot forward an rvalue as an lvalue.");
  return static_cast<T &&>(u);
}
template <typename T>
constexpr micron::remove_reference_t<T> &&
move(T &&t) noexcept
{
  return static_cast<micron::remove_reference_t<T> &&>(t);
};
template <typename T>
  requires micron::is_copy_constructible_v<T> && micron::is_move_constructible_v<T>
void
swap(T &a, T &b)
{
  T tmp = move(a);
  a = move(b);
  b = move(tmp);
}

template <class T, class V = T>
constexpr T
exchange(T &obj, V &&v)
{
  T old = move(obj);
  obj = forward<V>(v);
  return old;
}

template <typename T, typename... Args>
  requires micron::is_move_constructible_v<T>
void
reset(T &a, Args &&...args)
{
  T tmp = move(a);
  a = move(T{ forward<Args>(args)... });
}

template <typename T, typename... Args>
constexpr T *
construct(T *p, Args &&...args)
{
  return new (p) T{ micron::forward<Args>(args)... };
}

template <typename T, typename F>
constexpr bool
cmp_equal(T t, F f)
{
  if constexpr ( micron::is_signed_v<T> == micron::is_signed_v<F> )
    return t == f;
  else if constexpr ( micron::is_signed_v<T> )
    return t >= 0 && micron::make_unsigned_t<T>(t) == f;
  else if constexpr ( micron::is_class_v<T> && micron::is_class_v<F> )
    return t == f;
  else
    return f >= 0 && micron::make_unsigned_t<F>(f) == t;
}

template <typename T, typename F>
constexpr bool
cmp_not_equal(T t, F f)
{
  return !cmp_equal(t, f);
}

template <typename T, typename F>
constexpr bool
cmp_less(T t, F f)
{
  if constexpr ( micron::is_signed_v<T> == micron::is_signed_v<F> )
    return t < f;
  else if constexpr ( micron::is_signed_v<T> )
    return t < 0 && micron::make_unsigned_t<T>(t) < f;
  else if constexpr ( micron::is_class_v<T> && micron::is_class_v<F> )
    return t < f;
  else
    return f >= 0 && micron::make_unsigned_t<F>(f) > t;
}

template <typename T, typename F>
constexpr bool
cmp_greater(T t, F f)
{
  return cmp_less(f, t);
}

template <typename T, typename F>
constexpr bool
cmp_less_equal(T t, F f)
{
  return !cmp_less(f, t);
}

[[noreturn]] inline void
unreachable(void)
{
  __builtin_unreachable();
}
};
