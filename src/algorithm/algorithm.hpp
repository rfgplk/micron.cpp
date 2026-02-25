//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"

#include "../concepts.hpp"
#include "../math/generic.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "find.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// clamps

template <typename T>
constexpr const T &
clamp(const T &v, const T &lo, const T &hi) noexcept
{
  return (v < lo) ? lo : (hi < v) ? hi : v;
}

template <typename T, typename C>
constexpr const T &
clamp(const T &v, const T &lo, const T &hi, C comp) noexcept
{
  return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// fills

template <auto Fn, typename T>
constexpr void
fill(T *first, T *end) noexcept
{
  for ( ; first != end; ++first )
    *first = Fn();
}

template <auto Fn, typename T>
constexpr T *
fill_n(T *first, size_t n) noexcept
{
  for ( size_t i = 0; i < n; ++i, ++first )
    *first = Fn();
  return first;
}

template <auto Fn, is_iterable_container C>
constexpr C &
fill(C &c) noexcept
{
  fill<Fn>(c.begin(), c.end());
  return c;
}

template <auto Fn, is_iterable_container C>
constexpr C &
fill_n(C &c, size_t n) noexcept
{
  fill_n<Fn>(c.begin(), n);
  return c;
}

template <typename T, class P>
constexpr void
fill(T *first, T *end, const P &value) noexcept
{
  if constexpr ( micron::is_class_v<T> ) {
    for ( ; first != end; ++first )
      *first = value;
  } else {
    constexpr_memset(first, value, end - first);
  }
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<T>;
  }
constexpr void
fill(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    *first = fn();
}

template <typename T, class P>
constexpr T *
fill_n(T *first, size_t n, const P &value) noexcept
{
  for ( size_t i = 0; i < n; ++i, ++first )
    *first = value;
  return first;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<T>;
  }
constexpr T *
fill_n(T *first, size_t n, Fn fn) noexcept
{
  for ( size_t i = 0; i < n; ++i, ++first )
    *first = fn();
  return first;
}

template <is_iterable_container C, class P>
constexpr C &
fill(C &c, const P &value) noexcept
{
  fill(c.begin(), c.end(), value);
  return c;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<typename C::value_type>;
  }
constexpr C &
fill(C &c, Fn fn) noexcept
{
  fill(c.begin(), c.end(), fn);
  return c;
}

template <is_iterable_container C, class P>
constexpr C &
fill_n(C &c, size_t n, const P &value) noexcept
{
  fill_n(c.begin(), n, value);
  return c;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<typename C::value_type>;
  }
constexpr C &
fill_n(C &c, size_t n, Fn fn) noexcept
{
  fill_n(c.begin(), n, fn);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// generates

template <auto Fn, typename T>
constexpr void
generate(T *first, T *end) noexcept
{
  for ( ; first != end; ++first )
    *first = Fn();
}

template <auto Fn, typename T, typename... Args>
constexpr void
generate(T *first, T *end, Args &&...args) noexcept
{
  for ( ; first != end; ++first )
    *first = Fn(micron::forward<Args>(args)...);
}

template <auto Fn, is_iterable_container C>
constexpr C &
generate(C &c) noexcept
{
  generate<Fn>(c.begin(), c.end());
  return c;
}

template <auto Fn, is_iterable_container C, typename... Args>
constexpr C &
generate(C &c, Args &&...args) noexcept
{
  generate<Fn>(c.begin(), c.end(), micron::forward<Args>(args)...);
  return c;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<T>;
  }
constexpr void
generate(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    *first = fn();
}

template <typename T, typename Fn, typename... Args>
  requires micron::is_invocable_v<Fn, Args...>
constexpr void
generate(T *first, T *end, Fn fn, Args &&...args) noexcept
{
  for ( ; first != end; ++first )
    *first = fn(micron::forward<Args>(args)...);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn> && requires(Fn f) {
    { f() } -> micron::same_as<typename C::value_type>;
  }
constexpr C &
generate(C &c, Fn fn) noexcept
{
  generate(c.begin(), c.end(), fn);
  return c;
}

template <is_iterable_container C, typename Fn, typename... Args>
  requires micron::is_invocable_v<Fn, Args...>
constexpr C &
generate(C &c, Fn fn, Args &&...args) noexcept
{
  generate(c.begin(), c.end(), fn, micron::forward<Args>(args)...);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// transforms

template <auto Fn, typename T>
constexpr void
transform(T *first, T *end) noexcept
{
  if constexpr ( micron::is_invocable_v<decltype(Fn), T *> ) {
    for ( ; first != end; ++first )
      *first = Fn(first);
  } else {
    for ( ; first != end; ++first )
      *first = Fn(*first);
  }
}

template <auto Fn, typename T, typename O>
constexpr O *
transform(const T *first1, const T *end1, const T *first2, O *out) noexcept
{
  if constexpr ( micron::is_invocable_v<decltype(Fn), const T *, const T *> ) {
    for ( ; first1 != end1; ++first1, ++first2, ++out )
      *out = Fn(first1, first2);
  } else {
    for ( ; first1 != end1; ++first1, ++first2, ++out )
      *out = Fn(*first1, *first2);
  }
  return out;
}

template <auto Fn, is_iterable_container C>
constexpr C &
transform(C &c) noexcept
{
  transform<Fn>(c.begin(), c.end());
  return c;
}

template <auto Fn, is_iterable_container C, is_iterable_container O>
  requires micron::is_same_v<typename C::value_type, typename O::value_type>
constexpr O &
transform(const C &a, const C &b, O &out) noexcept
{
  transform<Fn>(a.begin(), a.end(), b.begin(), out.begin());
  return out;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, T *>
constexpr void
transform(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    *first = fn(first);
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, T>
constexpr void
transform(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    *first = fn(*first);
}

template <typename T, typename O, typename Fn>
  requires micron::is_invocable_v<Fn, T, T>
constexpr O *
transform(const T *first1, const T *end1, const T *first2, O *out, Fn fn) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2, ++out )
    *out = fn(*first1, *first2);
  return out;
}

template <typename T, typename O, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr O *
transform(const T *first1, const T *end1, const T *first2, O *out, Fn fn) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2, ++out )
    *out = fn(first1, first2);
  return out;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, typename C::value_type *>
constexpr C &
transform(C &c, Fn fn) noexcept
{
  transform(c.begin(), c.end(), fn);
  return c;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, typename C::value_type>
constexpr C &
transform(C &c, Fn fn) noexcept
{
  transform(c.begin(), c.end(), fn);
  return c;
}

template <is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::is_same_v<typename C::value_type, typename C::value_type>
           && micron::is_invocable_v<Fn, typename C::value_type, typename C::value_type>
constexpr O &
transform(const C &a, const C &b, O &out, Fn fn) noexcept
{
  transform(a.begin(), a.end(), b.begin(), out.begin(), fn);
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// wheres

template <auto Fn, typename T>
constexpr T *
where(const T *first, const T *end, T *out) noexcept
{
  if constexpr ( micron::is_invocable_v<decltype(Fn), const T *> ) {
    for ( ; first != end; ++first )
      if ( Fn(first) )
        *out++ = *first;
  } else {
    for ( ; first != end; ++first )
      if ( Fn(*first) )
        *out++ = *first;
  }
  return out;
}

template <auto Fn, is_iterable_container C>
C
where(const C &c)
{
  C out;
  out.resize(c.size());
  auto *last = where<Fn>(c.begin(), c.end(), out.begin());
  out.resize(last - out.begin());
  return out;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, T>
constexpr T *
where(const T *first, const T *end, T *out, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(*first) )
      *out++ = *first;
  return out;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr T *
where(const T *first, const T *end, T *out, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(first) )
      *out++ = *first;
  return out;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, typename C::value_type>
C
where(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = where(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
C
where(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = where(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// shifts/rotates
template <typename T>
constexpr T *
shift_left(T *first, T *end, size_t n) noexcept
{
  if ( n == 0 || first == end )
    return end;
  const size_t len = static_cast<size_t>(end - first);
  if ( n >= len ) {
    for ( T *p = first; p != end; ++p )
      *p = T{};
    return first;
  }
  for ( size_t i = 0; i + n < len; ++i )
    first[i] = micron::move(first[i + n]);
  for ( size_t i = len - n; i < len; ++i )
    first[i] = T{};
  return first + (len - n);
}

template <typename T>
constexpr T *
shift_right(T *first, T *end, size_t n) noexcept
{
  if ( n == 0 || first == end )
    return first;
  const size_t len = static_cast<size_t>(end - first);
  if ( n >= len ) {
    for ( T *p = first; p != end; ++p )
      *p = T{};
    return end;
  }
  for ( size_t i = len; i-- > n; )
    first[i] = micron::move(first[i - n]);
  for ( size_t i = 0; i < n; ++i )
    first[i] = T{};
  return first + n;
}

template <is_iterable_container C>
constexpr C &
shift_left(C &c, size_t n) noexcept
{
  shift_left(c.begin(), c.end(), n);
  return c;
}

template <is_iterable_container C>
constexpr C &
shift_right(C &c, size_t n) noexcept
{
  shift_right(c.begin(), c.end(), n);
  return c;
}

template <typename T>
constexpr void
rotate_left(T *first, T *end, size_t n) noexcept
{
  const size_t len = static_cast<size_t>(end - first);
  if ( len == 0 )
    return;
  n %= len;
  if ( n == 0 )
    return;
  auto rev = [](T *a, T *b) {
    while ( a < b ) {
      auto tmp = *a;
      *a = *b;
      *b = tmp;
      ++a;
      --b;
    }
  };
  rev(first, first + n - 1);
  rev(first + n, end - 1);
  rev(first, end - 1);
}

template <typename T>
constexpr void
rotate_right(T *first, T *end, size_t n) noexcept
{
  const size_t len = static_cast<size_t>(end - first);
  if ( len == 0 )
    return;
  n %= len;
  if ( n == 0 )
    return;
  rotate_left(first, end, len - n);
}

template <is_iterable_container C>
constexpr C &
rotate_left(C &c, size_t n) noexcept
{
  rotate_left(c.begin(), c.end(), n);
  return c;
}

template <is_iterable_container C>
constexpr C &
rotate_right(C &c, size_t n) noexcept
{
  rotate_right(c.begin(), c.end(), n);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// sums/fills
template <is_iterable_container T>
  requires micron::is_floating_point_v<typename T::value_type>
constexpr f128
sum(const T &src) noexcept
{
  f128 sm = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    sm += static_cast<f128>(src[i]);
  return sm;
}

template <is_iterable_container T>
  requires micron::is_integral_v<typename T::value_type>
constexpr umax_t
sum(const T &src) noexcept
{
  umax_t sm = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    sm += static_cast<umax_t>(src[i]);
  return sm;
}

template <is_iterable_container T, typename R = typename T::value_type>
constexpr T &
clear(T &src, const R r = 0) noexcept
{
  if constexpr ( micron::is_object_v<micron::remove_cv_t<typename T::value_type>> ) {
    for ( auto &n : src )
      n = r;
  } else if constexpr ( micron::is_fundamental_v<micron::remove_cv_t<typename T::value_type>> ) {
    constexpr_memset(src.begin(), r, src.size());
  }
  return src;
}

template <typename R = f64, typename T>
  requires micron::is_object_v<T>
constexpr R
mean(const T &src) noexcept
{
  return static_cast<R>(sum(src)) / static_cast<R>(src.size());
}

template <typename R = flong, typename T>
  requires micron::is_object_v<T>
constexpr R
geomean(const T &src) noexcept
{
  R mulsm = static_cast<R>(src[0]);
  for ( size_t i = 1; i < src.size(); i++ )
    mulsm *= static_cast<R>(src[i]);
  return math::powerflong(mulsm, static_cast<R>(R(1) / R(src.size())));
}

template <typename R = flong, typename T>
  requires micron::is_object_v<T>
constexpr R
harmonicmean(const T &src) noexcept
{
  R recsum = 0;
  for ( size_t i = 0; i < src.size(); i++ )
    recsum += (R(1) / static_cast<R>(src[i]));
  return static_cast<R>(src.size()) / recsum;
}

template <typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
round(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start )
    *start = math::round(*start);
}

template <typename T>
constexpr void
round(T &t) noexcept
{
  round(t.begin(), t.end());
}

template <typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
ceil(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start )
    *start = math::ceil(*start);
}

template <is_iterable_container T>
constexpr T &
ceil(T &t) noexcept
{
  ceil(t.begin(), t.end());
  return t;
}

template <typename T>
  requires micron::is_arithmetic_v<T>
constexpr void
floor(T *__restrict start, T *__restrict end) noexcept
{
  for ( ; start != end; ++start )
    *start = math::floor(*start);
}

template <typename T>
constexpr void
floor(T &t) noexcept
{
  floor(t.begin(), t.end());
}

template <typename T>
  requires micron::is_pointer_v<T>
constexpr void
reverse(T __restrict first, T __restrict end) noexcept
{
  while ( first < end ) {
    auto tmp = *first;
    *first = *end;
    *end = tmp;
    ++first;
    --end;
  }
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr void
reverse(T *first, T *end, Fn fn) noexcept
{
  while ( first < end ) {
    if ( fn(first, end) ) {
      auto tmp = *first;
      *first = *end;
      *end = tmp;
    }
    ++first;
    --end;
  }
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, T, T>
constexpr void
reverse(T *first, T *end, Fn fn) noexcept
{
  while ( first < end ) {
    if ( fn(*first, *end) ) {
      auto tmp = *first;
      *first = *end;
      *end = tmp;
    }
    ++first;
    --end;
  }
}

template <is_iterable_container C>
constexpr C &
reverse(C &c) noexcept
{
  reverse(c.begin(), c.end() - 1);
  return c;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename C::value_type *>
constexpr C &
reverse(C &c, Fn fn) noexcept
{
  reverse(c.begin(), c.end() - 1, fn);
  return c;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, typename C::value_type, typename C::value_type>
constexpr C &
reverse(C &c, Fn fn) noexcept
{
  reverse(c.begin(), c.end() - 1, fn);
  return c;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// reverses

template <auto Fn, typename T>
constexpr void
reverse(T *first, T *end) noexcept
{
  while ( first < end ) {
    if constexpr ( micron::is_invocable_v<decltype(Fn), const T *, const T *> ) {
      if ( Fn(first, end) ) {
        auto tmp = *first;
        *first = *end;
        *end = tmp;
      }
    } else {
      if ( Fn(*first, *end) ) {
        auto tmp = *first;
        *first = *end;
        *end = tmp;
      }
    }
    ++first;
    --end;
  }
}

template <auto Fn, is_iterable_container C>
constexpr C &
reverse(C &c) noexcept
{
  reverse<Fn>(c.begin(), c.end() - 1);
  return c;
}

template <auto Fn, typename T>
constexpr T *
reverse_copy(const T *first, const T *end, T *out) noexcept
{
  const T *it = end;
  while ( it != first ) {
    --it;
    if constexpr ( micron::is_invocable_v<decltype(Fn), const T *> ) {
      if ( Fn(it) )
        *out++ = *it;
    } else {
      if ( Fn(*it) )
        *out++ = *it;
    }
  }
  return out;
}

template <auto Fn, is_iterable_container C>
C
reverse_copy(const C &c)
{
  C out;
  out.resize(c.size());
  auto *last = reverse_copy<Fn>(c.begin(), c.end(), out.begin());
  out.resize(last - out.begin());
  return out;
}

template <typename T>
constexpr T *
reverse_copy(const T *first, const T *end, T *out) noexcept
{
  while ( end != first )
    *out++ = *--end;
  return out;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr T *
reverse_copy(const T *first, const T *end, T *out, Fn fn) noexcept
{
  const T *it = end;
  while ( it != first ) {
    --it;
    if ( fn(it) )
      *out++ = *it;
  }
  return out;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, T>
constexpr T *
reverse_copy(const T *first, const T *end, T *out, Fn fn) noexcept
{
  const T *it = end;
  while ( it != first ) {
    --it;
    if ( fn(*it) )
      *out++ = *it;
  }
  return out;
}

template <is_iterable_container C>
C
reverse_copy(const C &c)
{
  C out;
  out.resize(c.size());
  reverse_copy(c.begin(), c.end(), out.begin());
  return out;
}

template <is_iterable_container C, is_iterable_container O>
  requires micron::is_same_v<typename C::value_type, typename O::value_type>
O &
reverse_copy(const C &c, O &out)
{
  reverse_copy(c.begin(), c.end(), out.begin());
  return out;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
C
reverse_copy(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = reverse_copy(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, typename C::value_type>
C
reverse_copy(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *last = reverse_copy(c.begin(), c.end(), out.begin(), fn);
  out.resize(last - out.begin());
  return out;
}

template <typename T>
typename T::const_iterator
max_at(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::const_iterator max_v = it;
  for ( ; it != end; ++it )
    if ( *it > *max_v )
      max_v = it;
  return max_v;
}

template <typename T>
typename T::const_iterator
min_at(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::const_iterator min_v = it;
  for ( ; it != end; ++it )
    if ( *it < *min_v )
      min_v = it;
  return min_v;
}

template <typename T>
typename T::const_iterator
max_at(const T *first, const T *end) noexcept
{
  typename T::const_iterator max_v = first;
  for ( ; first != end; ++first )
    if ( *first > *max_v )
      max_v = first;
  return max_v;
}

template <typename T>
typename T::const_iterator
min_at(const T *first, const T *end) noexcept
{
  typename T::const_iterator min_v = first;
  for ( ; first != end; ++first )
    if ( *first < *min_v )
      min_v = first;
  return min_v;
}

template <typename T>
typename T::value_type
max(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::value_type max_v = *it++;
  for ( ; it != end; ++it )
    if ( *it > max_v )
      max_v = *it;
  return max_v;
}

template <typename T>
typename T::value_type
min(const T &arr) noexcept
{
  auto it = arr.cbegin();
  auto end = arr.cend();
  typename T::value_type min_v = *it++;
  for ( ; it != end; ++it )
    if ( *it < min_v )
      min_v = *it;
  return min_v;
}

template <typename T>
T
max(const T *first, const T *end) noexcept
{
  T max_v = *first++;
  for ( ; first != end; ++first )
    if ( *first > max_v )
      max_v = *first;
  return max_v;
}

template <typename T>
T
min(const T *first, const T *end) noexcept
{
  T min_v = *first++;
  for ( ; first != end; ++first )
    if ( *first < min_v )
      min_v = *first;
  return min_v;
}

};     // namespace micron
