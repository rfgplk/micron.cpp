//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

#include "concepts.hpp"

#include "sum.hpp"

namespace micron
{
// dispatch fn according to type, match exact types only
template<typename T, typename F>
constexpr void
__match_call_one_strict(T &&v, F &&f)
{
  using D = micron::remove_cvref_t<T>;

  if constexpr ( micron::is_invocable_v<F, D &> and micron::is_same_v<arg_type_t<decltype(f)>, D &> ) {
    f(static_cast<D &>(v));
  } else if constexpr ( micron::is_invocable_v<F, const D &> and micron::is_same_v<arg_type_t<decltype(f)>, const D &> ) {
    f(static_cast<const D &>(v));
  } else if constexpr ( micron::is_invocable_v<F, D> and micron::is_same_v<arg_type_t<decltype(f)>, D> ) {
    f(static_cast<D>(v));
  }
}

// dispatch fn according to type, allow matching convertible types
// ie allow int to bind char/long etc
template<typename T, typename F>
constexpr void
__match_call_one_permissible(T &&v, F &&f)
{
  using D = micron::remove_cvref_t<T>;

  if constexpr ( micron::is_invocable_v<F, D &> ) {
    f(static_cast<D &>(v));
  } else if constexpr ( micron::is_invocable_v<F, const D &> ) {
    f(static_cast<const D &>(v));
  } else if constexpr ( micron::is_invocable_v<F, D> ) {
    f(static_cast<D>(v));
  }
}

// dispatch one argument to all functions
template<typename T, typename... Fs>
constexpr void
__dispatch_one_strict(T &&v, Fs &&...fs)
{
  (__match_call_one_strict(micron::forward<T>(v), micron::forward<Fs>(fs)), ...);
}

template<typename T, typename... Fs>
constexpr void
__dispatch_one_permissible(T &&v, Fs &&...fs)
{
  (__match_call_one_permissible(micron::forward<T>(v), micron::forward<Fs>(fs)), ...);
}

template<auto... Fs> struct __match_dispatch {
  template<typename T>
  static constexpr void
  strict(T &&v)
  {
    (__match_call_one_strict(micron::forward<T>(v), Fs), ...);
  }

  template<typename T>
  static constexpr void
  permissible(T &&v)
  {
    (__match_call_one_permissible(micron::forward<T>(v), Fs), ...);
  }
};

template<auto... Fs, typename T>
constexpr void
__dispatch_values_strict(T &&v)
{
  __match_dispatch<Fs...>::strict(micron::forward<T>(v));
}

template<auto... Fs, typename T>
constexpr void
__dispatch_values_permissible(T &&v)
{
  __match_dispatch<Fs...>::permissible(micron::forward<T>(v));
}

// Dispatch each tuple element through the strict path.
template<auto... Fs, typename TupleT, usize... Is>
constexpr void
__match_tuple_impl(TupleT &&t, index_sequence<Is...>)
{
  (__match_dispatch<Fs...>::strict(get<Is>(micron::forward<TupleT>(t))), ...);
}

template<auto... Fs, typename TupleT, usize... Is>
constexpr void
__lazy_match_tuple_impl(TupleT &&t, index_sequence<Is...>)
{
  (__match_dispatch<Fs...>::permissible(get<Is>(micron::forward<TupleT>(t))), ...);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// pairs

template<auto... Fs, typename T, typename F>
constexpr void
match(pair<T, F> &p)
{
  __match_dispatch<Fs...>::strict(p.a);
  __match_dispatch<Fs...>::strict(p.b);
}

template<auto... Fs, typename T, typename F>
constexpr void
match(const pair<T, F> &p)
{
  __match_dispatch<Fs...>::strict(p.a);
  __match_dispatch<Fs...>::strict(p.b);
}

template<auto... Fs, typename T, typename F>
constexpr void
lazy_match(pair<T, F> &p)
{
  __match_dispatch<Fs...>::permissible(p.a);
  __match_dispatch<Fs...>::permissible(p.b);
}

template<auto... Fs, typename T, typename F>
constexpr void
lazy_match(const pair<T, F> &p)
{
  __match_dispatch<Fs...>::permissible(p.a);
  __match_dispatch<Fs...>::permissible(p.b);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// tuples

template<auto... Fs, typename... Ts>
constexpr void
match(tuple<Ts...> &t)
{
  __match_tuple_impl<Fs...>(t, index_sequence_for<Ts...>{});
}

template<auto... Fs, typename... Ts>
constexpr void
match(const tuple<Ts...> &t)
{
  __match_tuple_impl<Fs...>(t, index_sequence_for<Ts...>{});
}

template<auto... Fs, typename... Ts>
constexpr void
lazy_match(tuple<Ts...> &t)
{
  __lazy_match_tuple_impl<Fs...>(t, index_sequence_for<Ts...>{});
}

template<auto... Fs, typename... Ts>
constexpr void
lazy_match(const tuple<Ts...> &t)
{
  __lazy_match_tuple_impl<Fs...>(t, index_sequence_for<Ts...>{});
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// index_sequences

template<auto... Fs, usize... Is>
constexpr void
match(index_sequence<Is...>)
{
  (__match_dispatch<Fs...>::strict(integral_constant<usize, Is>{}), ...);
}

template<auto... Fs, usize... Is>
constexpr void
lazy_match(index_sequence<Is...>)
{
  (__match_dispatch<Fs...>::permissible(integral_constant<usize, Is>{}), ...);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%
// anys

template<auto... Fs, typename... Ts>
constexpr void
match(any<Ts...> &a)
{
  if ( !a.has_value() ) return;
  bool found = false;
  (
      [&]<typename T>() {
        if ( !found && a.template is<T>() ) {
          found = true;
          __match_dispatch<Fs...>::strict(a.template cast<T>());
        }
      }.template operator()<Ts>(),
      ...);
}

template<auto... Fs, typename... Ts>
constexpr void
match(const any<Ts...> &a)
{
  if ( !a.has_value() ) return;
  bool found = false;
  (
      [&]<typename T>() {
        if ( !found && a.template is<T>() ) {
          found = true;
          __match_dispatch<Fs...>::strict(a.template cast<T>());
        }
      }.template operator()<Ts>(),
      ...);
}

template<auto... Fs, typename... Ts>
constexpr void
lazy_match(any<Ts...> &a)
{
  if ( !a.has_value() ) return;
  bool found = false;
  (
      [&]<typename T>() {
        if ( !found && a.template is<T>() ) {
          found = true;
          __match_dispatch<Fs...>::permissible(a.template cast<T>());
        }
      }.template operator()<Ts>(),
      ...);
}

template<auto... Fs, typename... Ts>
constexpr void
lazy_match(const any<Ts...> &a)
{
  if ( !a.has_value() ) return;
  bool found = false;
  (
      [&]<typename T>() {
        if ( !found && a.template is<T>() ) {
          found = true;
          __match_dispatch<Fs...>::permissible(a.template cast<T>());
        }
      }.template operator()<Ts>(),
      ...);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// options (only target loaded type)

template<auto... Fs, typename T, typename F>
constexpr void
match(option<T, F> &o)
{
  if ( !o.has_value() ) return;
  if ( o.template is<T>() )
    __match_dispatch<Fs...>::strict(o.template cast<T>());
  else
    __match_dispatch<Fs...>::strict(o.template cast<F>());
}

template<auto... Fs, typename T, typename F>
constexpr void
match(const option<T, F> &o)
{
  if ( !o.has_value() ) return;
  if ( o.template is<T>() )
    __match_dispatch<Fs...>::strict(o.template cast<T>());
  else
    __match_dispatch<Fs...>::strict(o.template cast<F>());
}

template<auto... Fs, typename T, typename F>
constexpr void
lazy_match(option<T, F> &o)
{
  if ( !o.has_value() ) return;
  if ( o.template is<T>() )
    __match_dispatch<Fs...>::permissible(o.template cast<T>());
  else
    __match_dispatch<Fs...>::permissible(o.template cast<F>());
}

template<auto... Fs, typename T, typename F>
constexpr void
lazy_match(const option<T, F> &o)
{
  if ( !o.has_value() ) return;
  if ( o.template is<T>() )
    __match_dispatch<Fs...>::permissible(o.template cast<T>());
  else
    __match_dispatch<Fs...>::permissible(o.template cast<F>());
}

// strict
template<auto... Fs, typename... Ts>
constexpr void
match(Ts &&...vs)
{
  (__match_dispatch<Fs...>::strict(micron::forward<Ts>(vs)), ...);
}

// allow matching convertible types
template<auto... Fs, typename... Ts>
constexpr void
lazy_match(Ts &&...vs)
{
  (__match_dispatch<Fs...>::permissible(micron::forward<Ts>(vs)), ...);
}

};      // namespace micron
