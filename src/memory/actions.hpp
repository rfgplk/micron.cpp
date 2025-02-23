#pragma once

#include <type_traits>

// thought really hard on what to name this file

namespace micron
{
template <typename T>
constexpr T &&
forward(std::remove_reference_t<T> &t) noexcept
{
  return static_cast<T &&>(t);
}

template <typename T>
constexpr T &&
forward(std::remove_reference_t<T> &&t) noexcept
{
  static_assert(!std::is_lvalue_reference_v<T>, "Cannot forward an rvalue as an lvalue.");
  return static_cast<T &&>(t);
}

template <typename T, typename U>
constexpr T &&
forward_like(std::remove_reference_t<U> &&u) noexcept
{
  static_assert(!std::is_lvalue_reference_v<U>, "Cannot forward an rvalue as an lvalue.");
  return static_cast<T &&>(u);
}
template <typename T>
constexpr std::remove_reference_t<T> &&
move(T &&t) noexcept
{
  return static_cast<std::remove_reference_t<T> &&>(t);
};
template <typename T>
  requires std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
void
swap(T &a, T &b)
{
  T tmp = move(a);
  a = move(b);
  b = move(tmp);
}


template <typename T, typename... Args>
  requires std::is_move_constructible_v<T>
void
reset(T &a, Args&&... args)
{
  T tmp = move(a);
  a = move(T{forward<Args>(args)...});
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
  if constexpr ( std::is_signed_v<T> == std::is_signed_v<F> )
    return t == f;
  else if constexpr ( std::is_signed_v<T> )
    return t >= 0 && std::make_unsigned_t<T>(t) == f;
  else if constexpr ( std::is_class_v<T> && std::is_class_v<F> )
    return t == f;
  else
    return f >= 0 && std::make_unsigned_t<F>(f) == t;
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
  if constexpr ( std::is_signed_v<T> == std::is_signed_v<F> )
    return t < f;
  else if constexpr ( std::is_signed_v<T> )
    return t < 0 && std::make_unsigned_t<T>(t) < f;
  else if constexpr ( std::is_class_v<T> && std::is_class_v<F> )
    return t < f;
  else
    return f >= 0 && std::make_unsigned_t<F>(f) > t;
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
};
