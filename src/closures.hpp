//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

#include "tuple.hpp"

namespace micron
{
template <typename F> struct lambda_return;

template <typename F, typename R, typename... Args> struct lambda_return<R (F::*)(Args...) const> {
  using type = R;
};

template <typename F> using lambda_return_t = typename lambda_return<decltype(&F::operator())>::type;
};     // namespace micron

template <typename T> struct function_traits;

// free functions
template <typename R, typename... Args> struct function_traits<R(Args...)> {
  using return_type = R;
  static constexpr size_t arity = sizeof...(Args);
  using args_tuple = micron::tuple<Args...>;

  template <size_t N> using arg_type = typename micron::tuple_element<N, args_tuple>::type;
};

// function pointers
template <typename R, typename... Args> struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {
};

// member functions
template <typename C, typename R, typename... Args> struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {
};

template <typename C, typename R, typename... Args> struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
};

// callable objects (functors/lambdas)
template <typename F> struct function_traits : function_traits<decltype(&F::operator())> {
};

template <typename Tuple, typename F, size_t... I>
constexpr void
for_each_type_impl(F &&f, micron::index_sequence<I...>)
{
  (f.template operator()<micron::tuple_element_t<I, Tuple>>(), ...);
}

template <typename Tuple, typename F>
constexpr void
for_each_type(F &&f)
{
  for_each_type_impl<Tuple>(micron::forward<F>(f), micron::make_index_sequence<micron::tuple_size_v<Tuple>>{});
}
