//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../concepts.hpp"

#include "../math/generic.hpp"
#include "../memory/actions.hpp"
#include "../memory/memory.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

#include "algorithm.hpp"

namespace micron
{
template <class T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
T *
filter(const T *first, const T *end, Fn fn, T *out)
{
  for ( ; first != end; ++first )
    if ( fn(first) )
      *out++ = *first;
  return out;     // pointer past last written element
}

template <class T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
T *
filter(const T *first, const T *end, Fn fn, T *out, size_t limit)
{
  for ( u64 i = 0; first != end and i < limit; ++first )
    if ( fn(first) ) {
      *out++ = *first;
      ++i;
    }
  return out;     // pointer past last written element
}

template <class T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
T *
filter(const T *first, const T *end, Fn fn, T *out, T *out_end)
{
  for ( ; first != end and out != out_end; ++first )
    if ( fn(first) ) {
      *out++ = *first;
    }
  return out;     // pointer past last written element
}

template <class T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
const T *
prune(const T *first, const T *end, Fn fn, T *out, size_t limit)
{
  for ( u64 i = 0; first != end and i < limit; ++first )
    if ( fn(first) ) {
      *out++ = *first;
      ++i;
    }
  return first;
}

template <class T, typename Fn>
  requires micron::is_invocable_v<Fn, const T *>
const T *
prune(const T *first, const T *end, Fn fn, T *out, T *out_end)
{
  for ( ; first != end and out != out_end; ++first )
    if ( fn(first) ) {
      *out++ = *first;
    }
  return first;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
C
filter(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *ptr = filter(c.begin(), c.end(), fn, out.begin());
  out.resize(ptr - out.begin());
  return out;
}

template <is_iterable_container C, typename F>
  requires micron::is_invocable_v<F, const typename C::value_type *>
C &
filter_inplace(C &c, F fn)
{
  auto *first = c.begin();
  auto *last = c.end();
  C *out = first;
  for ( ; first != last; ++first )
    if ( fn(first) )
      *out++ = *first;
  c.resize(out - c.begin());
  return c;
}

template <is_iterable_container C, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *>
C
filter(const C &c, Fn fn, size_t limit)
{
  C out;
  out.resize(micron::min(c.size(), limit));
  auto *ptr = filter(c.begin(), c.end(), fn, out.begin(), limit);
  out.resize(ptr - out.begin());
  return out;
}

template <is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
typename O::value_type *
filter(const C &c_in, Fn fn, O &c_out)
{
  return filter(c_in.begin(), c_in.end(), fn, c_out.begin(), c_out.end());
}

template <is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
typename O::value_type *
filter(const C &c_in, Fn fn, O &c_out, size_t limit)
{
  return filter(c_in.begin(), c_in.end(), fn, c_out.begin(), limit);
}

template <is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
const typename C::value_type *
prune(const C &c_in, Fn fn, O &c_out, size_t limit)
{
  return prune(c_in.begin(), c_in.end(), fn, c_out.begin(), limit);
}

template <is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
const typename C::value_type *
prune(const C &c_in, Fn fn, O &c_out)
{
  return prune(c_in.begin(), c_in.end(), fn, c_out.begin(), c_out.end());
}

};     // namespace micron
