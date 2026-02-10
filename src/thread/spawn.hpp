//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../type_traits.hpp"
#include "../types.hpp"

#include "arena.hpp"
#include "pool.hpp"

// Main Threading API
// all threading requests should go through here unless
// a) you are building some type of thread pool (in which case use the solo interface)
// b) you need true standalone threads for configuration (in which case use raw threads)
// c) you need to customize your own arenas via callbacks/whatnot
namespace micron
{

// new thread
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...>)
auto &
go(Fn fn, Args &&...args)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    throw except::system_error("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->create_burden(fn, micron::forward<Args &&>(args)...);
}

// lists all threads
auto
threads(void)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    throw except::system_error("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->list();
  // auto lck = __global_threadpool->lock();
  //__global_threadpool->unlock(lck);
}

auto
lock()
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    throw except::system_error("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->lock();
  //__global_threadpool->unlock(lck);
}

// new proc
// fork and run process at path location specified by T
template <is_string T, is_string... R>
void
run(const T &t, const R &...args)
{
  process(t, args...);
}
// new thread
void spawn();

};
