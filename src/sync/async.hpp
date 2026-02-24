//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

#include "../thread/arena.hpp"
#include "../thread/pool.hpp"

namespace micron
{
// analogous to go(), but launches a lightweight worker thread instead into the concurrent runtime arena

template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...> && sizeof...(Args) == 0
           && ((micron::is_lvalue_reference_v<Args> && ...) or (micron::is_rvalue_reference_v<Args> && ...))
           && (!micron::is_same_v<micron::decay_t<Args>, Args> && ...))
auto &
async(Fn &&fn, Args &&...args)
{
  if ( __global_parallelpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron sync::async(): system arena is uninitialized/nullptr");
  return __global_parallelpool->add(fn, micron::forward<Args &&>(args)...);
}

template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...> && sizeof...(Args) > 0 && ((micron::is_lvalue_reference_v<Args> && ...))
           && (!micron::is_same_v<micron::decay_t<Args>, Args> && ...))
auto &
async(Fn fn, const Args &...args)
{
  if ( __global_parallelpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron sync::async(): system arena is uninitialized/nullptr");
  return __global_parallelpool->add(fn, micron::forward<const Args &>(args)...);
}

template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...> && sizeof...(Args) > 0
           && ((!micron::is_lvalue_reference_v<Args> && ...) and (!micron::is_rvalue_reference_v<Args> && ...))
           && (!micron::is_reference_v<Args> && ...) && (micron::is_same_v<micron::decay_t<Args>, Args> && ...)
           && (micron::is_same_v<micron::remove_reference_t<Args>, Args> && ...))
auto &
async(Fn fn, Args... args)
{
  if ( __global_parallelpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron sync::async(): system arena is uninitialized/nullptr");
  return __global_parallelpool->add(fn, micron::forward<Args>(args)...);
}

};     // namespace micron
