//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

#include "concepts.hpp"

namespace micron
{
// dispatch fn according to type, match exact types only
template <typename T, typename F>
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
template <typename T, typename F>
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
template <typename T, typename... Fs>
constexpr void
__dispatch_one_strict(T &&v, Fs &&...fs)
{
  (__match_call_one_strict(micron::forward<T>(v), micron::forward<Fs>(fs)), ...);
}

template <typename T, typename... Fs>
constexpr void
__dispatch_one_permissible(T &&v, Fs &&...fs)
{
  (__match_call_one_permissible(micron::forward<T>(v), micron::forward<Fs>(fs)), ...);
}

// strict
template <auto... Fs, typename... Ts>
constexpr void
match(Ts &&...vs)
{
  (__dispatch_one_strict(micron::forward<Ts>(vs), Fs...), ...);
}

// allow matching convertible types
template <auto... Fs, typename... Ts>
constexpr void
lazy_match(Ts &&...vs)
{
  (__dispatch_one_permissible(micron::forward<Ts>(vs), Fs...), ...);
}

};     // namespace micron
