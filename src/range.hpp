//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

namespace micron
{

template <typename F, typename T>
concept range_size_t = micron::convertible_to<T, F>;

template <umax_t From, range_size_t<umax_t> auto To>
  requires(From < To && micron::is_arithmetic_v<umax_t>)
struct range {
  ~range() = default;
  range(void) = default;
  range(const range &o) = default;
  range(range &&o) = default;
  range &operator=(const range &) = delete;
  range &operator=(range &&) = delete;

  template <typename F>
    requires micron::is_invocable_v<F, umax_t>
  static void
  perform(F &f)
  {
    for ( umax_t i = From; i < To; i++ )
      f();
  }

  // lightly summoning god knows what
  // this bonds to most things it should bind to
  template <class C, typename... Fargs>
  static void
  perform(C &obj, void (C::*f)(Fargs...), Fargs &&...args)
  {
    for ( umax_t i = From; i < To; i++ )
      (obj.*f)(args...);
  }
};

template <typename T, T From, range_size_t<T> auto To>
  requires(From < To && micron::is_arithmetic_v<T>)
struct count_range {
  ~count_range() = default;
  count_range(void) = default;
  count_range(const count_range &o) = default;
  count_range(count_range &&o) = default;
  count_range &operator=(const count_range &) = delete;
  count_range &operator=(count_range &&) = delete;

  template <typename F>
    requires micron::is_invocable_v<F, T>
  static void
  perform(F &f)
  {
    for ( T i = From; i < To; i++ )
      f(i);
  }

  // lightly summoning god knows what
  // this bonds to most things it should bind to
  template <class C, typename R, typename Arg = T>
  static void
  perform(C &obj, R (C::*f)(const Arg &))
  {
    for ( T i = From; i < To; i++ )
      (obj.*f)(i);
  }
  template <class C, typename R, typename Arg = T>
  static void
  perform(C &obj, R (C::*f)(Arg))
  {
    for ( T i = From; i < To; i++ )
      (obj.*f)(i);
  }

  template <class C, typename R, typename Arg = T>
  static void
  perform(C &obj, R (C::*f)(Arg &&))
  {

    for ( T i = From; i < To; i++ )
      (obj.*f)(i);
  }
  // explicitly deleting this
};

template <typename T, range_size_t<size_t> auto Cnt> struct range_of {
  ~range_of() = default;
  range_of(void) = default;
  range_of(const range_of &o) = default;
  range_of(range_of &&o) = default;
  range_of &operator=(const range_of &) = delete;
  range_of &operator=(range_of &&) = delete;

  template <typename C, typename F>
  static void
  perform(C &obj, F f)
  {
    for ( typename T::iterator itr = obj.begin(); itr != (itr + Cnt); ++itr )
      f(*itr);
  }
};

template <i32 From, range_size_t<i32> auto To> using int_range = count_range<i32, From, To>;

template <float From, range_size_t<float> auto To> using float_range = count_range<float, From, To>;

template <u32 From, range_size_t<u32> auto To> using u32_range = count_range<u32, From, To>;

template <u64 From, range_size_t<u64> auto To> using u64_range = count_range<u64, From, To>;

template <i64 From, range_size_t<i64> auto To> using i64_range = count_range<i64, From, To>;

};
