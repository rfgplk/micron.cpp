//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "../tuple.hpp"

namespace micron
{

template <typename F, typename T>
concept can_be_transformed = micron::is_convertible_v<F, T>;

template <typename T, typename F>
  requires can_be_transformed<F, T>
T
transform_to(const F &value)
{
  return static_cast<T>(value);
}

template <class T, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const T *>
A
fold_left(const T *first, const T *end, A init, Fn fn)
{
  for ( ; first != end; ++first )
    init = fn(init, first);
  return init;
}

template <class T, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, A, const U *> && can_be_transformed<T, U>
A
fold_left(const T *first, const T *end, A init, Fn fn)
{
  for ( ; first != end; ++first ) {
    U transformed = transform_to<U>(*first);
    init = fn(init, &transformed);
  }
  return init;
}

template <class T, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const T *>
A
fold_left(const T *first, const T *end, A init, Fn fn, size_t limit)
{
  for ( u64 i = 0; first != end and i < limit; ++first, ++i )
    init = fn(init, first);
  return init;
}

template <class T, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, A, const U *> && can_be_transformed<T, U>
A
fold_left(const T *first, const T *end, A init, Fn fn, size_t limit)
{
  for ( u64 i = 0; first != end and i < limit; ++first, ++i ) {
    U transformed = transform_to<U>(*first);
    init = fn(init, &transformed);
  }
  return init;
}

template <is_iterable_container C, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const typename C::value_type *>
A
fold_left(const C &c, A init, Fn fn)
{
  return fold_left(c.begin(), c.end(), init, fn);
}

template <is_iterable_container C, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, A, const U *> && can_be_transformed<typename C::value_type, U>
A
fold_left(const C &c, A init, Fn fn)
{
  return fold_left<typename C::value_type, A, U, Fn>(c.begin(), c.end(), init, fn);
}

template <class T, class A, typename Fn>
  requires micron::is_invocable_v<Fn, const T *, A>
A
fold_right(const T *first, const T *end, Fn fn, A init)
{
  if ( first == end )
    return init;

  const T *it = end;
  while ( it != first ) {
    --it;
    init = fn(it, init);
  }
  return init;
}

template <class T, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, const U *, A> && can_be_transformed<T, U>
A
fold_right(const T *first, const T *end, Fn fn, A init)
{
  if ( first == end )
    return init;

  const T *it = end;
  while ( it != first ) {
    --it;
    U transformed = transform_to<U>(*it);
    init = fn(&transformed, init);
  }
  return init;
}

template <class T, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const T *>
A
fold(const T *first, const T *end, A init, Fn fn, size_t limit)
{
  return fold_left(first, end, init, fn, limit);
}

template <class T, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, A, const U *> && can_be_transformed<T, U>
A
fold(const T *first, const T *end, A init, Fn fn, size_t limit)
{
  return fold_left(first, end, init, fn, limit);
}

template <is_iterable_container C, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const typename C::value_type *>
A
fold(const C &c, A init, Fn fn)
{
  return fold_left(c.begin(), c.end(), init, fn);
}

template <is_iterable_container C, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, A, const U *> && can_be_transformed<typename C::value_type, U>
A
fold(const C &c, A init, Fn fn)
{
  return fold_left<typename C::value_type, A, U, Fn>(c.begin(), c.end(), init, fn);
}

template <is_iterable_container C, class A, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *, A>
A
fold_right(const C &c, Fn fn, A init)
{
  return fold_right(c.begin(), c.end(), fn, init);
}

template <is_iterable_container C, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, const U *, A> && can_be_transformed<typename C::value_type, U>
A
fold_right(const C &c, Fn fn, A init)
{
  return fold_right<typename C::value_type, A, U, Fn>(c.begin(), c.end(), fn, init);
}

template <is_iterable_container C, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const typename C::value_type *>
A
fold_left(const C &c, A init, Fn fn, size_t limit)
{
  return fold_left(c.begin(), c.end(), init, fn, limit);
}

template <is_iterable_container C, class A, class U, typename Fn>
  requires micron::is_invocable_v<Fn, A, const U *> && can_be_transformed<typename C::value_type, U>
A
fold_left(const C &c, A init, Fn fn, size_t limit)
{
  return fold_left<typename C::value_type, A, U, Fn>(c.begin(), c.end(), init, fn, limit);
}

template <class T, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const T *>
micron::pair<A, size_t>
fold_left_counted(const T *first, const T *end, A init, Fn fn)
{
  size_t count = 0;
  for ( ; first != end; ++first, ++count )
    init = fn(init, first);
  return { init, count };
}

template <is_iterable_container C, class A, typename Fn>
  requires micron::is_invocable_v<Fn, A, const typename C::value_type *>
micron::pair<A, size_t>
fold_left_counted(const C &c, A init, Fn fn)
{
  return fold_left_counted(c.begin(), c.end(), init, fn);
}

template <class T, class A, typename Fn, typename P>
  requires micron::is_invocable_v<Fn, A, const T *> && micron::is_invocable_v<P, A, const T *>
A
fold_left_while(const T *first, const T *end, A init, Fn fn, P pred)
{
  for ( ; first != end and pred(init, first); ++first )
    init = fn(init, first);
  return init;
}

template <is_iterable_container C, class A, typename Fn, typename P>
  requires micron::is_invocable_v<Fn, A, const typename C::value_type *> && micron::is_invocable_v<P, A, const typename C::value_type *>
A
fold_left_while(const C &c, A init, Fn fn, P pred)
{
  return fold_left_while(c.begin(), c.end(), init, fn, pred);
}

template <is_iterable_container I, is_iterable_container O, typename Fn>
  requires micron::is_invocable_v<Fn, O, const typename I::value_type *>
O
fold_build(const I &c, O init, Fn fn)
{
  return fold_left(c, init, fn);
}

template <is_iterable_container I, is_iterable_container O, class U, typename Fn>
  requires micron::is_invocable_v<Fn, O, const U *> && can_be_transformed<typename I::value_type, U>
O
fold_build(const I &c, O init, Fn fn)
{
  return fold_left<I, O, U, Fn>(c, init, fn);
}

};     // namespace micron
