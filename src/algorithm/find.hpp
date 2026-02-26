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
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../tuple.hpp"

namespace micron
{

template <typename T, typename C>
  requires(micron::same_as<T, C> or micron::comparable_with<T, C>)
constexpr bool
all_of(const T *first, const T *end, const C &comp) noexcept
{
  for ( ; first != end; ++first )
    if ( *first != comp )
      return false;
  return true;
}

template <class T, typename C>
  requires(micron::same_as<typename T::value_type, C> or micron::comparable_with<typename T::value_type, C>)
constexpr bool
all_of(const T *first, const T *end, const C &comp) noexcept
{
  for ( ; first != end; ++first )
    if ( *first != comp )
      return false;
  return true;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr bool
all_of(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( !fn(*first) )
      return false;
  return true;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr bool
all_of(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( !fn(first) )
      return false;
  return true;
}

template <typename T, typename C>
  requires(micron::same_as<T, C> or micron::comparable_with<T, C>)
constexpr bool
any_of(const T *first, const T *end, const C &comp) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == comp )
      return true;
  return false;
}

template <class T, typename C>
  requires(micron::same_as<typename T::value_type, C> or micron::comparable_with<typename T::value_type, C>)
constexpr bool
any_of(const T *first, const T *end, const C &comp) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == comp )
      return true;
  return false;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr bool
any_of(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(*first) )
      return true;
  return false;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr bool
any_of(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(first) )
      return true;
  return false;
}

template <typename T, typename C>
  requires(micron::same_as<T, C> or micron::comparable_with<T, C>)
constexpr bool
none_of(const T *first, const T *end, const C &comp) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == comp )
      return false;
  return true;
}

template <class T, typename C>
  requires(micron::same_as<typename T::value_type, C> or micron::comparable_with<typename T::value_type, C>)
constexpr bool
none_of(const T *first, const T *end, const C &comp) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == comp )
      return false;
  return true;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr bool
none_of(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(*first) )
      return false;
  return true;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr bool
none_of(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(first) )
      return false;
  return true;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
constexpr bool
all_of(const C &c, Fn fn) noexcept
{
  return all_of(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
constexpr bool
all_of(const C &c, Fn fn) noexcept
{
  return all_of(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
constexpr bool
any_of(const C &c, Fn fn) noexcept
{
  return any_of(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
constexpr bool
any_of(const C &c, Fn fn) noexcept
{
  return any_of(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
constexpr bool
none_of(const C &c, Fn fn) noexcept
{
  return none_of(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
constexpr bool
none_of(const C &c, Fn fn) noexcept
{
  return none_of(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Cmp>
constexpr bool
all_of(const C &c, const Cmp &cmp) noexcept
{
  return all_of(c.begin(), c.end(), cmp);
}

template <is_iterable_container C, typename Cmp>
constexpr bool
any_of(const C &c, const Cmp &cmp) noexcept
{
  return any_of(c.begin(), c.end(), cmp);
}

template <is_iterable_container C, typename Cmp>
constexpr bool
none_of(const C &c, const Cmp &cmp) noexcept
{
  return none_of(c.begin(), c.end(), cmp);
}

template <auto Fn, typename T>
constexpr bool
all_of(const T *first, const T *end) noexcept
{
  for ( ; first != end; ++first )
    if ( !Fn(first) )
      return false;
  return true;
}

template <auto Fn, typename T>
constexpr bool
any_of(const T *first, const T *end) noexcept
{
  for ( ; first != end; ++first )
    if ( Fn(first) )
      return true;
  return false;
}

template <auto Fn, typename T>
constexpr bool
none_of(const T *first, const T *end) noexcept
{
  for ( ; first != end; ++first )
    if ( Fn(first) )
      return false;
  return true;
}

template <auto Fn, is_iterable_container C>
constexpr bool
all_of(const C &c) noexcept
{
  return all_of<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
constexpr bool
any_of(const C &c) noexcept
{
  return any_of<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
constexpr bool
none_of(const C &c) noexcept
{
  return none_of<Fn>(c.begin(), c.end());
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr T *
find(T *first, T *end, const P &f) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == static_cast<T>(f) )
      return first;
  return nullptr;
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr const T *
find(const T *first, const T *end, const P &f) noexcept
{
  for ( ; first != end; ++first )
    if ( *first == static_cast<T>(f) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr const T *
find_if(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(*first) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr T *
find_if(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(*first) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr const T *
find_if(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(first) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr T *
find_if(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( fn(first) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr const T *
find_if_not(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( !fn(first) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr T *
find_if_not(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( !fn(first) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr const T *
find_if_not(const T *first, const T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( !fn(first) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr T *
find_if_not(T *first, T *end, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    if ( !fn(first) )
      return first;
  return nullptr;
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::iterator
find(C &c, const P &v) noexcept
{
  return find(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::const_iterator
find(const C &c, const P &v) noexcept
{
  return find(c.begin(), c.end(), v);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
typename C::iterator
find_if(C &c, Fn fn) noexcept
{
  return find_if(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
typename C::const_iterator
find_if(const C &c, Fn fn) noexcept
{
  return find_if(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::iterator
find_if(C &c, Fn fn) noexcept
{
  return find_if(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::const_iterator
find_if(const C &c, Fn fn) noexcept
{
  return find_if(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
typename C::iterator
find_if_not(C &c, Fn fn) noexcept
{
  return find_if_not(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
typename C::const_iterator
find_if_not(const C &c, Fn fn) noexcept
{
  return find_if_not(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::iterator
find_if_not(C &c, Fn fn) noexcept
{
  return find_if_not(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::const_iterator
find_if_not(const C &c, Fn fn) noexcept
{
  return find_if_not(c.begin(), c.end(), fn);
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr T *
find_last(T *first, T *end, const P &f) noexcept
{
  T *found = nullptr;
  for ( ; first != end; ++first )
    if ( *first == static_cast<T>(f) )
      found = first;
  return found;
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr const T *
find_last(const T *first, const T *end, const P &f) noexcept
{
  const T *found = nullptr;
  for ( ; first != end; ++first )
    if ( *first == static_cast<T>(f) )
      found = first;
  return found;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr const T *
find_last_if(const T *first, const T *end, Fn fn) noexcept
{
  const T *found = nullptr;
  for ( ; first != end; ++first )
    if ( fn(*first) )
      found = first;
  return found;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr T *
find_last_if(T *first, T *end, Fn fn) noexcept
{
  T *found = nullptr;
  for ( ; first != end; ++first )
    if ( fn(*first) )
      found = first;
  return found;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr const T *
find_last_if(const T *first, const T *end, Fn fn) noexcept
{
  const T *found = nullptr;
  for ( ; first != end; ++first )
    if ( fn(first) )
      found = first;
  return found;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr T *
find_last_if(T *first, T *end, Fn fn) noexcept
{
  T *found = nullptr;
  for ( ; first != end; ++first )
    if ( fn(first) )
      found = first;
  return found;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr const T *
find_last_if_not(const T *first, const T *end, Fn fn) noexcept
{
  const T *found = nullptr;
  for ( ; first != end; ++first )
    if ( !fn(first) )
      found = first;
  return found;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr T *
find_last_if_not(T *first, T *end, Fn fn) noexcept
{
  T *found = nullptr;
  for ( ; first != end; ++first )
    if ( !fn(first) )
      found = first;
  return found;
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::iterator
find_last(C &c, const P &v) noexcept
{
  return find_last(c.begin(), c.end(), v);
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
typename C::const_iterator
find_last(const C &c, const P &v) noexcept
{
  return find_last(c.begin(), c.end(), v);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::iterator
find_last_if(C &c, Fn fn) noexcept
{
  return find_last_if(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::const_iterator
find_last_if(const C &c, Fn fn) noexcept
{
  return find_last_if(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::iterator
find_last_if_not(C &c, Fn fn) noexcept
{
  return find_last_if_not(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
typename C::const_iterator
find_last_if_not(const C &c, Fn fn) noexcept
{
  return find_last_if_not(c.begin(), c.end(), fn);
}

template <typename T, class P>
const T *
find_end(const T *first, const T *end, const P *pfirst, const P *pend) noexcept
{
  if ( pfirst == pend )
    return end;
  const T *result = nullptr;
  for ( ; first != end; ++first ) {
    const T *it1 = first;
    const P *it2 = pfirst;
    while ( it1 != end && it2 != pend && *it1 == static_cast<T>(*it2) ) {
      ++it1;
      ++it2;
    }
    if ( it2 == pend )
      result = first;
  }
  return result;
}

template <typename T, class P, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const P *>
const T *
find_end(const T *first, const T *end, const P *pfirst, const P *pend, Fn fn) noexcept
{
  if ( pfirst == pend )
    return end;
  const T *result = nullptr;
  for ( ; first != end; ++first ) {
    const T *it1 = first;
    const P *it2 = pfirst;
    while ( it1 != end && it2 != pend && fn(it1, it2) ) {
      ++it1;
      ++it2;
    }
    if ( it2 == pend )
      result = first;
  }
  return result;
}

template <is_iterable_container C, is_iterable_container P>
const typename C::value_type *
find_end(const C &c, const P &p) noexcept
{
  return find_end(c.begin(), c.end(), p.begin(), p.end());
}

template <is_iterable_container C, is_iterable_container P, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename P::value_type *>
const typename C::value_type *
find_end(const C &c, const P &p, Fn fn) noexcept
{
  return find_end(c.begin(), c.end(), p.begin(), p.end(), fn);
}

template <typename T, class P>
const T *
find_first_of(const T *first, const T *end, const P *sfirst, const P *send) noexcept
{
  for ( ; first != end; ++first )
    for ( const P *s = sfirst; s != send; ++s )
      if ( *first == static_cast<T>(*s) )
        return first;
  return nullptr;
}

template <typename T, class P, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const P *>
const T *
find_first_of(const T *first, const T *end, const P *sfirst, const P *send, Fn fn) noexcept
{
  for ( ; first != end; ++first )
    for ( const P *s = sfirst; s != send; ++s )
      if ( fn(first, s) )
        return first;
  return nullptr;
}

template <is_iterable_container C, is_iterable_container S>
  requires micron::is_same_v<typename C::value_type, typename S::value_type>
const typename C::value_type *
find_first_of(const C &c, const S &s) noexcept
{
  return find_first_of(c.begin(), c.end(), s.begin(), s.end());
}

template <is_iterable_container C, is_iterable_container S, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename S::value_type *>
const typename C::value_type *
find_first_of(const C &c, const S &s, Fn fn) noexcept
{
  return find_first_of(c.begin(), c.end(), s.begin(), s.end(), fn);
}

template <auto Fn, typename T>
constexpr const T *
find_if(const T *first, const T *end) noexcept
{
  for ( ; first != end; ++first )
    if ( Fn(first) )
      return first;
  return nullptr;
}

template <auto Fn, typename T>
constexpr T *
find_if(T *first, T *end) noexcept
{
  for ( ; first != end; ++first )
    if ( Fn(first) )
      return first;
  return nullptr;
}

template <auto Fn, typename T>
constexpr const T *
find_if_not(const T *first, const T *end) noexcept
{
  for ( ; first != end; ++first )
    if ( !Fn(first) )
      return first;
  return nullptr;
}

template <auto Fn, typename T>
constexpr T *
find_if_not(T *first, T *end) noexcept
{
  for ( ; first != end; ++first )
    if ( !Fn(first) )
      return first;
  return nullptr;
}

template <auto Fn, typename T>
constexpr const T *
find_last_if(const T *first, const T *end) noexcept
{
  const T *found = nullptr;
  for ( ; first != end; ++first )
    if ( Fn(first) )
      found = first;
  return found;
}

template <auto Fn, typename T>
constexpr T *
find_last_if(T *first, T *end) noexcept
{
  T *found = nullptr;
  for ( ; first != end; ++first )
    if ( Fn(first) )
      found = first;
  return found;
}

template <auto Fn, typename T>
constexpr const T *
find_last_if_not(const T *first, const T *end) noexcept
{
  const T *found = nullptr;
  for ( ; first != end; ++first )
    if ( !Fn(first) )
      found = first;
  return found;
}

template <auto Fn, typename T>
constexpr T *
find_last_if_not(T *first, T *end) noexcept
{
  T *found = nullptr;
  for ( ; first != end; ++first )
    if ( !Fn(first) )
      found = first;
  return found;
}

template <auto Fn, is_iterable_container C>
typename C::const_iterator
find_if(const C &c) noexcept
{
  return find_if<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
typename C::iterator
find_if(C &c) noexcept
{
  return find_if<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
typename C::const_iterator
find_if_not(const C &c) noexcept
{
  return find_if_not<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
typename C::iterator
find_if_not(C &c) noexcept
{
  return find_if_not<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
typename C::const_iterator
find_last_if(const C &c) noexcept
{
  return find_last_if<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
typename C::iterator
find_last_if(C &c) noexcept
{
  return find_last_if<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
typename C::const_iterator
find_last_if_not(const C &c) noexcept
{
  return find_last_if_not<Fn>(c.begin(), c.end());
}

template <auto Fn, is_iterable_container C>
typename C::iterator
find_last_if_not(C &c) noexcept
{
  return find_last_if_not<Fn>(c.begin(), c.end());
}

template <typename T>
constexpr const T *
adjacent_find(const T *first, const T *end) noexcept
{
  if ( first == end )
    return nullptr;
  for ( const T *next = first + 1; next != end; ++first, ++next )
    if ( *first == *next )
      return first;
  return nullptr;
}

template <typename T>
constexpr T *
adjacent_find(T *first, T *end) noexcept
{
  if ( first == end )
    return nullptr;
  for ( T *next = first + 1; next != end; ++first, ++next )
    if ( *first == *next )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr const T *
adjacent_find(const T *first, const T *end, Fn fn) noexcept
{
  if ( first == end )
    return nullptr;
  for ( const T *next = first + 1; next != end; ++first, ++next )
    if ( fn(first, next) )
      return first;
  return nullptr;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr T *
adjacent_find(T *first, T *end, Fn fn) noexcept
{
  if ( first == end )
    return nullptr;
  for ( T *next = first + 1; next != end; ++first, ++next )
    if ( fn(first, next) )
      return first;
  return nullptr;
}

template <is_iterable_container C>
typename C::const_iterator
adjacent_find(const C &c) noexcept
{
  return adjacent_find(c.begin(), c.end());
}

template <is_iterable_container C>
typename C::iterator
adjacent_find(C &c) noexcept
{
  return adjacent_find(c.begin(), c.end());
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename C::value_type *>
typename C::const_iterator
adjacent_find(const C &c, Fn fn) noexcept
{
  return adjacent_find(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename C::value_type *>
typename C::iterator
adjacent_find(C &c, Fn fn) noexcept
{
  return adjacent_find(c.begin(), c.end(), fn);
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr umax_t
count(const T *first, const T *end, const P &v) noexcept
{
  umax_t n = 0;
  for ( ; first != end; ++first )
    if ( *first == static_cast<T>(v) )
      ++n;
  return n;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T>
constexpr umax_t
count_if(const T *first, const T *end, Fn fn) noexcept
{
  umax_t n = 0;
  for ( ; first != end; ++first )
    if ( fn(*first) )
      ++n;
  return n;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
constexpr umax_t
count_if(const T *first, const T *end, Fn fn) noexcept
{
  umax_t n = 0;
  for ( ; first != end; ++first )
    if ( fn(first) )
      ++n;
  return n;
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
umax_t
count(const C &c, const P &v) noexcept
{
  return count(c.begin(), c.end(), v);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type>
umax_t
count_if(const C &c, Fn fn) noexcept
{
  return count_if(c.begin(), c.end(), fn);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
umax_t
count_if(const C &c, Fn fn) noexcept
{
  return count_if(c.begin(), c.end(), fn);
}

template <auto Fn, typename T>
constexpr umax_t
count_if(const T *first, const T *end) noexcept
{
  umax_t n = 0;
  for ( ; first != end; ++first )
    if ( Fn(first) )
      ++n;
  return n;
}

template <auto Fn, is_iterable_container C>
constexpr umax_t
count_if(const C &c) noexcept
{
  return count_if<Fn>(c.begin(), c.end());
}

template <typename T>
constexpr micron::pair<const T *, const T *>
mismatch(const T *first1, const T *end1, const T *first2) noexcept
{
  for ( ; first1 != end1 && *first1 == *first2; ++first1, ++first2 )
    ;
  return { first1, first2 };
}

template <typename T>
constexpr micron::pair<const T *, const T *>
mismatch(const T *first1, const T *end1, const T *first2, const T *end2) noexcept
{
  for ( ; first1 != end1 && first2 != end2 && *first1 == *first2; ++first1, ++first2 )
    ;
  return { first1, first2 };
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr micron::pair<const T *, const T *>
mismatch(const T *first1, const T *end1, const T *first2, Fn fn) noexcept
{
  for ( ; first1 != end1 && fn(first1, first2); ++first1, ++first2 )
    ;
  return { first1, first2 };
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr micron::pair<const T *, const T *>
mismatch(const T *first1, const T *end1, const T *first2, const T *end2, Fn fn) noexcept
{
  for ( ; first1 != end1 && first2 != end2 && fn(first1, first2); ++first1, ++first2 )
    ;
  return { first1, first2 };
}

template <is_iterable_container C1, is_iterable_container C2>
  requires micron::is_same_v<typename C1::value_type, typename C2::value_type>
micron::pair<const typename C1::value_type *, const typename C2::value_type *>
mismatch(const C1 &a, const C2 &b) noexcept
{
  return mismatch(a.begin(), a.end(), b.begin(), b.end());
}

template <is_iterable_container C1, is_iterable_container C2, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C1::value_type *, const typename C2::value_type *>
micron::pair<const typename C1::value_type *, const typename C2::value_type *>
mismatch(const C1 &a, const C2 &b, Fn fn) noexcept
{
  return mismatch(a.begin(), a.end(), b.begin(), b.end(), fn);
}

template <typename T>
constexpr bool
equal(const T *first1, const T *end1, const T *first2) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2 )
    if ( !(*first1 == *first2) )
      return false;
  return true;
}

template <typename T>
constexpr bool
equal(const T *first1, const T *end1, const T *first2, const T *end2) noexcept
{
  if ( (end1 - first1) != (end2 - first2) )
    return false;
  return equal(first1, end1, first2);
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr bool
equal(const T *first1, const T *end1, const T *first2, Fn fn) noexcept
{
  for ( ; first1 != end1; ++first1, ++first2 )
    if ( !fn(first1, first2) )
      return false;
  return true;
}

template <is_iterable_container C1, is_iterable_container C2>
  requires micron::is_same_v<typename C1::value_type, typename C2::value_type>
bool
equal(const C1 &a, const C2 &b) noexcept
{
  return equal(a.begin(), a.end(), b.begin(), b.end());
}

template <is_iterable_container C1, is_iterable_container C2, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C1::value_type *, const typename C2::value_type *>
bool
equal(const C1 &a, const C2 &b, Fn fn) noexcept
{
  if ( a.size() != b.size() )
    return false;
  return equal(a.begin(), a.end(), b.begin(), fn);
}

template <typename T, class P>
const T *
search(const T *first, const T *end, const P *pfirst, const P *pend) noexcept
{
  for ( ; first != end; ++first ) {
    const T *it1 = first;
    const P *it2 = pfirst;
    while ( it1 != end && it2 != pend && *it1 == static_cast<T>(*it2) ) {
      ++it1;
      ++it2;
    }
    if ( it2 == pend )
      return first;
  }
  return nullptr;
}

template <typename T, class P, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const P *>
const T *
search(const T *first, const T *end, const P *pfirst, const P *pend, Fn fn) noexcept
{
  for ( ; first != end; ++first ) {
    const T *it1 = first;
    const P *it2 = pfirst;
    while ( it1 != end && it2 != pend && fn(it1, it2) ) {
      ++it1;
      ++it2;
    }
    if ( it2 == pend )
      return first;
  }
  return nullptr;
}

template <typename T, class P>
const T *
search_n(const T *first, const T *end, size_t n, const P &value) noexcept
{
  for ( ; first != end; ++first ) {
    size_t i = 0;
    while ( first + i != end && i < n && *(first + i) == static_cast<T>(value) )
      ++i;
    if ( i == n )
      return first;
  }
  return nullptr;
}

template <typename T, class P, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const P *>
const T *
search_n(const T *first, const T *end, size_t n, const P &value, Fn fn) noexcept
{
  for ( ; first != end; ++first ) {
    size_t i = 0;
    while ( first + i != end && i < n && fn(first + i, &value) )
      ++i;
    if ( i == n )
      return first;
  }
  return nullptr;
}

template <is_iterable_container C, is_iterable_container P>
const typename C::value_type *
search(const C &c, const P &p) noexcept
{
  return search(c.begin(), c.end(), p.begin(), p.end());
}

template <is_iterable_container C, is_iterable_container P, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename P::value_type *>
const typename C::value_type *
search(const C &c, const P &p, Fn fn) noexcept
{
  return search(c.begin(), c.end(), p.begin(), p.end(), fn);
}

template <is_iterable_container C, class V>
const typename C::value_type *
search_n(const C &c, size_t n, const V &v) noexcept
{
  return search_n(c.begin(), c.end(), n, v);
}

template <is_iterable_container C, class V, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const V *>
const typename C::value_type *
search_n(const C &c, size_t n, const V &v, Fn fn) noexcept
{
  return search_n(c.begin(), c.end(), n, v, fn);
}

template <typename T, class P>
  requires(micron::is_convertible_v<T, P>)
constexpr bool
contains(const T *first, const T *end, const P &v) noexcept
{
  return find(first, end, v) != nullptr;
}

template <typename T, class P>
constexpr bool
contains_subrange(const T *first, const T *end, const P *pfirst, const P *pend) noexcept
{
  return search(first, end, pfirst, pend) != nullptr;
}

template <is_iterable_container C, class P>
  requires micron::convertible_to<P, typename C::value_type>
bool
contains(const C &c, const P &v) noexcept
{
  return contains(c.begin(), c.end(), v);
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
bool
contains_if(const C &c, Fn fn) noexcept
{
  return find_if(c.begin(), c.end(), fn) != nullptr;
}

template <is_iterable_container C, is_iterable_container P>
  requires micron::is_same_v<typename C::value_type, typename P::value_type>
bool
contains_subrange(const C &c, const P &p) noexcept
{
  return contains_subrange(c.begin(), c.end(), p.begin(), p.end());
}

template <typename T>
constexpr bool
starts_with(const T *first, const T *end, const T *pfirst, const T *pend) noexcept
{
  for ( ; pfirst != pend; ++first, ++pfirst ) {
    if ( first == end || !(*first == *pfirst) )
      return false;
  }
  return true;
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr bool
starts_with(const T *first, const T *end, const T *pfirst, const T *pend, Fn fn) noexcept
{
  for ( ; pfirst != pend; ++first, ++pfirst ) {
    if ( first == end || !fn(first, pfirst) )
      return false;
  }
  return true;
}

template <typename T>
constexpr bool
ends_with(const T *first, const T *end, const T *pfirst, const T *pend) noexcept
{
  const auto n = end - first;
  const auto pn = pend - pfirst;
  if ( pn > n )
    return false;
  return equal(end - pn, end, pend - pn);
}

template <typename T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, const T *>
constexpr bool
ends_with(const T *first, const T *end, const T *pfirst, const T *pend, Fn fn) noexcept
{
  const auto n = end - first;
  const auto pn = pend - pfirst;
  if ( pn > n )
    return false;
  return equal(end - pn, end, pend - pn, fn);
}

template <is_iterable_container C, is_iterable_container P>
  requires micron::is_same_v<typename C::value_type, typename P::value_type>
bool
starts_with(const C &c, const P &p) noexcept
{
  return starts_with(c.begin(), c.end(), p.begin(), p.end());
}

template <is_iterable_container C, is_iterable_container P, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename P::value_type *>
bool
starts_with(const C &c, const P &p, Fn fn) noexcept
{
  return starts_with(c.begin(), c.end(), p.begin(), p.end(), fn);
}

template <is_iterable_container C, is_iterable_container P>
  requires micron::is_same_v<typename C::value_type, typename P::value_type>
bool
ends_with(const C &c, const P &p) noexcept
{
  return ends_with(c.begin(), c.end(), p.begin(), p.end());
}

template <is_iterable_container C, is_iterable_container P, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, const typename P::value_type *>
bool
ends_with(const C &c, const P &p, Fn fn) noexcept
{
  return ends_with(c.begin(), c.end(), p.begin(), p.end(), fn);
}

};     // namespace micron
