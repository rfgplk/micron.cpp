//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../tuple.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "../vector.hpp"

#include "fork_join.hpp"
#include "quasi_pointer.hpp"
#include "task.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//  co_await spawn_many(n, fn)        fork fn(0..n-1), join, collect into a micron::vector<T>
//  co_await spawn_tuple(t1, t2, ..)  fork heterogeneous already-built tasks, join, return a micron::tuple<...>
//  co_await when_all(t1, t2, ...)    alias of spawn_tuple
//  co_await fork_task(&slot, task)

namespace micron
{
namespace coro
{

template<class I, class T>
[[nodiscard]] auto
fork_task(I *__ret, micron::task<T> &&__t)
{
  constexpr u8 __m = __cf_detail::__is_eventual<I>::value ? __rm_eventual : __rm_lvalue;
  auto __make = [__tk = micron::move(__t)]() mutable -> micron::task<T> { return micron::move(__tk); };
  return __fork_awaitable<T, decltype(__make)>{ static_cast<void *>(__ret), __m, micron::move(__make) };
}

template<class Fn> using __spawn_result_t = __task_value_t<micron::invoke_result_t<Fn &, usize>>;

template<class Fn>
micron::task<micron::vector<__spawn_result_t<Fn>>>
spawn_many(usize __n, Fn __fn)
{
  using T = __spawn_result_t<Fn>;
  if ( __n == 0 ) co_return micron::vector<T>{};
  micron::vector<T> __out(__n);      // n default-constructed slots; addresses stable (no realloc in the loop)
  for ( usize __i = 0; __i < __n; ++__i ) co_await fork(&__out[__i], __fn)(__i);
  co_await join;
  co_return __out;
}

template<usize... Is, class... Tk>
micron::task<micron::tuple<__task_value_t<Tk>...>>
__spawn_tuple_impl(micron::index_sequence<Is...>, Tk... __tasks)
{
  micron::tuple<__task_value_t<Tk>...> __out{};
  ((co_await fork_task(&micron::get<Is>(__out), micron::move(__tasks))), ...);
  co_await join;
  co_return __out;
}

template<class... Tk>
[[nodiscard]] auto
spawn_tuple(Tk... __tasks)
{
  return __spawn_tuple_impl(micron::make_index_sequence<sizeof...(Tk)>{}, micron::move(__tasks)...);
}

template<class... Tk>
[[nodiscard]] auto
when_all(Tk... __tasks)
{
  return __spawn_tuple_impl(micron::make_index_sequence<sizeof...(Tk)>{}, micron::move(__tasks)...);
}

};      // namespace coro
};      // namespace micron
