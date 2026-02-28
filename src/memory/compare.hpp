//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../memory/cmemory/memcmp.hpp"
#include "../memory/memory.hpp"

namespace micron
{

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compare ptr

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
__attribute__((nonnull)) constexpr long int
compare(const T *__restrict a, const T *__restrict b, const usize n) noexcept
{
  return bytecmp(reinterpret_cast<const byte *>(a), reinterpret_cast<const byte *>(b), n * sizeof(T));
}

template <typename T, typename Fn>
  requires(!micron::is_null_pointer_v<T>) && micron::is_invocable_v<Fn, const T &, const T &>
__attribute__((nonnull)) constexpr bool
compare(const T *__restrict a, const T *__restrict b, const usize n, Fn fn) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !fn(a[i], b[i]) )
      return false;
  return true;
}

template <typename T, typename Fn>
  requires(!micron::is_null_pointer_v<T>) && micron::is_invocable_v<Fn, const T *, const T *>
__attribute__((nonnull)) constexpr bool
compare(const T *__restrict a, const T *__restrict b, const usize n, Fn fn) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !fn(&a[i], &b[i]) )
      return false;
  return true;
}

template <auto Fn, typename T>
  requires(!micron::is_null_pointer_v<T>)
__attribute__((nonnull)) constexpr bool
compare(const T *__restrict a, const T *__restrict b, const usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    if constexpr ( micron::is_invocable_v<decltype(Fn), const T *, const T *> ) {
      if ( !Fn(&a[i], &b[i]) )
        return false;
    } else {
      if ( !Fn(a[i], b[i]) )
        return false;
    }
  }
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compare ranges

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
__attribute__((nonnull)) constexpr long int
compare(const T *__restrict first1, const T *__restrict end1, const T *__restrict first2) noexcept
{
  return bytecmp(reinterpret_cast<const byte *>(first1), reinterpret_cast<const byte *>(first2),
                 static_cast<usize>(end1 - first1) * sizeof(T));
}

template <typename T, typename Fn>
  requires(!micron::is_null_pointer_v<T>) && micron::is_invocable_v<Fn, const T &, const T &>
__attribute__((nonnull)) constexpr bool
compare(const T *__restrict first1, const T *__restrict end1, const T *__restrict first2, Fn fn) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2 )
    if ( !fn(*first1, *first2) )
      return false;
  return true;
}

template <typename T, typename Fn>
  requires(!micron::is_null_pointer_v<T>) && micron::is_invocable_v<Fn, const T *, const T *>
__attribute__((nonnull)) constexpr bool
compare(const T *__restrict first1, const T *__restrict end1, const T *__restrict first2, Fn fn) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2 )
    if ( !fn(first1, first2) )
      return false;
  return true;
}

template <auto Fn, typename T>
  requires(!micron::is_null_pointer_v<T>)
__attribute__((nonnull)) constexpr bool
compare(const T *__restrict first1, const T *__restrict end1, const T *__restrict first2) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2 ) {
    if constexpr ( micron::is_invocable_v<decltype(Fn), const T *, const T *> ) {
      if ( !Fn(first1, first2) )
        return false;
    } else {
      if ( !Fn(*first1, *first2) )
        return false;
    }
  }
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compare containers

template <is_iterable_container C>
constexpr long int
compare(const C &a, const C &b) noexcept
{
  const usize n = a.size() < b.size() ? a.size() : b.size();
  const long int r
      = bytecmp(reinterpret_cast<const byte *>(a.begin()), reinterpret_cast<const byte *>(b.begin()), n * sizeof(typename C::value_type));
  if ( r != 0 )
    return r;
  return static_cast<long int>(a.size()) - static_cast<long int>(b.size());
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type &, const typename C::value_type &>
constexpr bool
compare(const C &a, const C &b, Fn fn) noexcept
{
  if ( a.size() != b.size() )
    return false;
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  for ( ; ia != ea; ++ia, ++ib )
    if ( !fn(*ia, *ib) )
      return false;
  return true;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename C::value_type *>
constexpr bool
compare(const C &a, const C &b, Fn fn) noexcept
{
  if ( a.size() != b.size() )
    return false;
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  for ( ; ia != ea; ++ia, ++ib )
    if ( !fn(ia, ib) )
      return false;
  return true;
}

template <auto Fn, is_iterable_container C>
constexpr bool
compare(const C &a, const C &b) noexcept
{
  if ( a.size() != b.size() )
    return false;
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  for ( ; ia != ea; ++ia, ++ib ) {
    if constexpr ( micron::is_invocable_v<decltype(Fn), const typename C::value_type *, const typename C::value_type *> ) {
      if ( !Fn(ia, ib) )
        return false;
    } else {
      if ( !Fn(*ia, *ib) )
        return false;
    }
  }
  return true;
}

template <is_iterable_container C, is_iterable_container D>
  requires micron::is_same_v<typename C::value_type, typename D::value_type>
constexpr long int
compare(const C &a, const D &b) noexcept
{
  const usize n = a.size() < b.size() ? a.size() : b.size();
  const long int r
      = bytecmp(reinterpret_cast<const byte *>(a.begin()), reinterpret_cast<const byte *>(b.begin()), n * sizeof(typename C::value_type));
  if ( r != 0 )
    return r;
  return static_cast<long int>(a.size()) - static_cast<long int>(b.size());
}

template <is_iterable_container C, is_iterable_container D, typename Fn>
  requires micron::is_same_v<typename C::value_type, typename D::value_type>
           && micron::is_invocable_v<Fn, const typename C::value_type &, const typename C::value_type &>
constexpr bool
compare(const C &a, const D &b, Fn fn) noexcept
{
  if ( a.size() != b.size() )
    return false;
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  for ( ; ia != ea; ++ia, ++ib )
    if ( !fn(*ia, *ib) )
      return false;
  return true;
}

template <auto Fn, is_iterable_container C, is_iterable_container D>
  requires micron::is_same_v<typename C::value_type, typename D::value_type>
constexpr bool
compare(const C &a, const D &b) noexcept
{
  if ( a.size() != b.size() )
    return false;
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  for ( ; ia != ea; ++ia, ++ib ) {
    if constexpr ( micron::is_invocable_v<decltype(Fn), const typename C::value_type *, const typename D::value_type *> ) {
      if ( !Fn(ia, ib) )
        return false;
    } else {
      if ( !Fn(*ia, *ib) )
        return false;
    }
  }
  return true;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// compare_ns

template <typename T>
  requires(!micron::is_null_pointer_v<T>)
__attribute__((nonnull)) constexpr long int
compare_n(const T *__restrict a, const T *__restrict b, const usize n) noexcept
{
  return bytecmp(reinterpret_cast<const byte *>(a), reinterpret_cast<const byte *>(b), n * sizeof(T));
}

template <typename T, typename Fn>
  requires(!micron::is_null_pointer_v<T>) && micron::is_invocable_v<Fn, const T &, const T &>
__attribute__((nonnull)) constexpr bool
compare_n(const T *__restrict a, const T *__restrict b, usize n, Fn fn) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !fn(a[i], b[i]) )
      return false;
  return true;
}

template <typename T, typename Fn>
  requires(!micron::is_null_pointer_v<T>) && micron::is_invocable_v<Fn, const T *, const T *>
__attribute__((nonnull)) constexpr bool
compare_n(const T *__restrict a, const T *__restrict b, usize n, Fn fn) noexcept
{
  for ( usize i = 0; i < n; ++i )
    if ( !fn(&a[i], &b[i]) )
      return false;
  return true;
}

template <auto Fn, typename T>
  requires(!micron::is_null_pointer_v<T>)
__attribute__((nonnull)) constexpr bool
compare_n(const T *__restrict a, const T *__restrict b, const usize n) noexcept
{
  for ( usize i = 0; i < n; ++i ) {
    if constexpr ( micron::is_invocable_v<decltype(Fn), const T *, const T *> ) {
      if ( !Fn(&a[i], &b[i]) )
        return false;
    } else {
      if ( !Fn(a[i], b[i]) )
        return false;
    }
  }
  return true;
}

template <is_iterable_container C>
constexpr long int
compare_n(const C &a, const C &b, const usize n) noexcept
{
  const usize clamped = n < a.size() && n < b.size() ? n : a.size() < b.size() ? a.size() : b.size();
  return bytecmp(reinterpret_cast<const byte *>(a.begin()), reinterpret_cast<const byte *>(b.begin()),
                 clamped * sizeof(typename C::value_type));
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type &, const typename C::value_type &>
constexpr bool
compare_n(const C &a, const C &b, usize n, Fn fn) noexcept
{
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  const auto eb = b.end();
  for ( usize i = 0; i < n && ia != ea && ib != eb; ++i, ++ia, ++ib )
    if ( !fn(*ia, *ib) )
      return false;
  return true;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename C::value_type *>
constexpr bool
compare_n(const C &a, const C &b, usize n, Fn fn) noexcept
{
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  const auto eb = b.end();
  for ( usize i = 0; i < n && ia != ea && ib != eb; ++i, ++ia, ++ib )
    if ( !fn(ia, ib) )
      return false;
  return true;
}

template <auto Fn, is_iterable_container C>
constexpr bool
compare_n(const C &a, const C &b, const usize n) noexcept
{
  auto ia = a.begin();
  auto ib = b.begin();
  const auto ea = a.end();
  const auto eb = b.end();
  for ( usize i = 0; i < n && ia != ea && ib != eb; ++i, ++ia, ++ib ) {
    if constexpr ( micron::is_invocable_v<decltype(Fn), const typename C::value_type *, const typename C::value_type *> ) {
      if ( !Fn(ia, ib) )
        return false;
    } else {
      if ( !Fn(*ia, *ib) )
        return false;
    }
  }
  return true;
}
};     // namespace micron
