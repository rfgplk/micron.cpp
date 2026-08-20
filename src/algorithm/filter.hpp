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

namespace __impl
{

// T is an explicit parameter, not deduced from the call
template<typename Fn, typename T> struct __deref_pred {
  [[no_unique_address]] Fn __fn;

  [[gnu::always_inline]] constexpr bool
  operator()(const T *__p) const
  {
    return static_cast<bool>(__fn(*__p));
  }
};

};      // namespace __impl

template<class T, typename Fn>
  requires(!micron::fn_predicate<Fn, T>) && micron::is_invocable_v<Fn, const T *>
T *
filter(const T *first, const T *end, Fn fn, T *out)
{
  for ( ; first != end; ++first )
    if ( fn(first) ) *out++ = *first;
  return out;
}

template<class T, typename Fn>
  requires(!micron::fn_predicate<Fn, T>) && micron::is_invocable_v<Fn, const T *>
T *
filter(const T *first, const T *end, Fn fn, T *out, usize limit)
{
  for ( u64 i = 0; first != end and i < limit; ++first )
    if ( fn(first) ) {
      *out++ = *first;
      ++i;
    }
  return out;
}

template<class T, typename Fn>
  requires(!micron::fn_predicate<Fn, T>) && micron::is_invocable_v<Fn, const T *>
T *
filter(const T *first, const T *end, Fn fn, T *out, T *out_end)
{
  for ( ; first != end and out != out_end; ++first )
    if ( fn(first) ) {
      *out++ = *first;
    }
  return out;
}

template<class T, typename Fn>
  requires(!micron::fn_predicate<Fn, T>) && micron::is_invocable_v<Fn, const T *>
const T *
prune(const T *first, const T *end, Fn fn, T *out, usize limit)
{
  for ( u64 i = 0; first != end and i < limit; ++first )
    if ( fn(first) ) {
      *out++ = *first;
      ++i;
    }
  return first;
}

template<class T, typename Fn>
  requires(!micron::fn_predicate<Fn, T>) && micron::is_invocable_v<Fn, const T *>
const T *
prune(const T *first, const T *end, Fn fn, T *out, T *out_end)
{
  for ( ; first != end and out != out_end; ++first )
    if ( fn(first) ) {
      *out++ = *first;
    }
  return first;
}

template<is_iterable_container C, typename Fn>
  requires(!micron::fn_predicate<Fn, typename C::value_type>) && micron::is_invocable_v<Fn, const typename C::value_type *>
C
filter(const C &c, Fn fn)
{
  C out;
  out.resize(c.size());
  auto *ptr = filter(c.begin(), c.end(), fn, out.begin());
  out.resize(ptr - out.begin());
  return out;
}

template<is_iterable_container C, typename F>
  requires(!micron::fn_predicate<F, typename C::value_type>) && micron::is_invocable_v<F, const typename C::value_type *>
C &
filter_inplace(C &c, F fn)
{
  auto *first = c.begin();
  auto *last = c.end();
  auto *out = first;
  for ( ; first != last; ++first )
    if ( fn(first) ) *out++ = *first;
  c.resize(static_cast<typename C::size_type>(out - c.begin()));
  return c;
}

template<is_iterable_container C, typename Fn>
  requires(!micron::fn_predicate<Fn, typename C::value_type>) && micron::is_invocable_v<Fn, const typename C::value_type *>
C
filter(const C &c, Fn fn, usize limit)
{
  C out;
  out.resize(micron::min(c.size(), limit));
  auto *ptr = filter(c.begin(), c.end(), fn, out.begin(), limit);
  out.resize(ptr - out.begin());
  return out;
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires(!micron::fn_predicate<Fn, typename C::value_type>)
          && micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
typename O::value_type *
filter(const C &c_in, Fn fn, O &c_out)
{
  return filter(c_in.begin(), c_in.end(), fn, c_out.begin(), c_out.end());
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires(!micron::fn_predicate<Fn, typename C::value_type>)
          && micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
typename O::value_type *
filter(const C &c_in, Fn fn, O &c_out, usize limit)
{
  return filter(c_in.begin(), c_in.end(), fn, c_out.begin(), limit);
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires(!micron::fn_predicate<Fn, typename C::value_type>)
          && micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
const typename C::value_type *
prune(const C &c_in, Fn fn, O &c_out, usize limit)
{
  return prune(c_in.begin(), c_in.end(), fn, c_out.begin(), limit);
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires(!micron::fn_predicate<Fn, typename C::value_type>)
          && micron::is_invocable_v<Fn, const typename C::value_type *> && micron::is_same_v<typename C::value_type, typename O::value_type>
const typename C::value_type *
prune(const C &c_in, Fn fn, O &c_out)
{
  return prune(c_in.begin(), c_in.end(), fn, c_out.begin(), c_out.end());
}

template<class T, typename Fn>
  requires micron::fn_predicate<Fn, T>
T *
filter(const T *first, const T *end, Fn fn, T *out)
{
  return filter(first, end, __impl::__deref_pred<Fn, T>{ micron::move(fn) }, out);
}

template<class T, typename Fn>
  requires micron::fn_predicate<Fn, T>
T *
filter(const T *first, const T *end, Fn fn, T *out, usize limit)
{
  return filter(first, end, __impl::__deref_pred<Fn, T>{ micron::move(fn) }, out, limit);
}

template<class T, typename Fn>
  requires micron::fn_predicate<Fn, T>
T *
filter(const T *first, const T *end, Fn fn, T *out, T *out_end)
{
  return filter(first, end, __impl::__deref_pred<Fn, T>{ micron::move(fn) }, out, out_end);
}

template<class T, typename Fn>
  requires micron::fn_predicate<Fn, T>
const T *
prune(const T *first, const T *end, Fn fn, T *out, usize limit)
{
  return prune(first, end, __impl::__deref_pred<Fn, T>{ micron::move(fn) }, out, limit);
}

template<class T, typename Fn>
  requires micron::fn_predicate<Fn, T>
const T *
prune(const T *first, const T *end, Fn fn, T *out, T *out_end)
{
  return prune(first, end, __impl::__deref_pred<Fn, T>{ micron::move(fn) }, out, out_end);
}

template<is_iterable_container C, typename Fn>
  requires micron::fn_predicate<Fn, typename C::value_type>
C
filter(const C &c, Fn fn)
{
  return filter(c, __impl::__deref_pred<Fn, typename C::value_type>{ micron::move(fn) });
}

template<is_iterable_container C, typename F>
  requires micron::fn_predicate<F, typename C::value_type>
C &
filter_inplace(C &c, F fn)
{
  return filter_inplace(c, __impl::__deref_pred<F, typename C::value_type>{ micron::move(fn) });
}

template<is_iterable_container C, typename Fn>
  requires micron::fn_predicate<Fn, typename C::value_type>
C
filter(const C &c, Fn fn, usize limit)
{
  return filter(c, __impl::__deref_pred<Fn, typename C::value_type>{ micron::move(fn) }, limit);
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::fn_predicate<Fn, typename C::value_type> && micron::is_same_v<typename C::value_type, typename O::value_type>
typename O::value_type *
filter(const C &c_in, Fn fn, O &c_out)
{
  return filter(c_in, __impl::__deref_pred<Fn, typename C::value_type>{ micron::move(fn) }, c_out);
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::fn_predicate<Fn, typename C::value_type> && micron::is_same_v<typename C::value_type, typename O::value_type>
typename O::value_type *
filter(const C &c_in, Fn fn, O &c_out, usize limit)
{
  return filter(c_in, __impl::__deref_pred<Fn, typename C::value_type>{ micron::move(fn) }, c_out, limit);
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::fn_predicate<Fn, typename C::value_type> && micron::is_same_v<typename C::value_type, typename O::value_type>
const typename C::value_type *
prune(const C &c_in, Fn fn, O &c_out, usize limit)
{
  return prune(c_in, __impl::__deref_pred<Fn, typename C::value_type>{ micron::move(fn) }, c_out, limit);
}

template<is_iterable_container C, is_iterable_container O, typename Fn>
  requires micron::fn_predicate<Fn, typename C::value_type> && micron::is_same_v<typename C::value_type, typename O::value_type>
const typename C::value_type *
prune(const C &c_in, Fn fn, O &c_out)
{
  return prune(c_in, __impl::__deref_pred<Fn, typename C::value_type>{ micron::move(fn) }, c_out);
}

// NTTP forms
template<auto Fn, typename T>
constexpr T *
filter(const T *first, const T *end, T *out) noexcept
{
  for ( ; first != end; ++first )
    if ( Fn(first) ) *out++ = *first;
  return out;
}

template<auto Fn, typename T>
constexpr T *
filter(const T *first, const T *end, T *out, usize limit) noexcept
{
  for ( u64 i = 0; first != end && i < limit; ++first )
    if ( Fn(first) ) {
      *out++ = *first;
      ++i;
    }
  return out;
}

template<auto Fn, is_iterable_container C>
C
filter(const C &c)
{
  C out;
  out.resize(c.size());
  auto *last = filter<Fn>(c.begin(), c.end(), out.begin());
  out.resize(last - out.begin());
  return out;
}

template<auto Fn, is_iterable_container C>
C
filter(const C &c, usize limit)
{
  C out;
  out.resize(micron::min(c.size(), limit));
  auto *last = filter<Fn>(c.begin(), c.end(), out.begin(), limit);
  out.resize(last - out.begin());
  return out;
}
};      // namespace micron
