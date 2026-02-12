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
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->create_burden(fn, micron::forward<Args &&>(args)...);
}

// new thread at core
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...>)
auto &
go(u32 at, Fn fn, Args &&...args)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->create_at(at, fn, micron::forward<Args &&>(args)...);
}

// start a realtime thread
// specifically start a thread with a sched_fifo scheduling policy
// NOTE: process must have the cap_sys_nice or cap_sys_admin capability OR rlimit_rtprio must not be zero
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...>)
auto &
realtime(Fn fn, Args &&...args)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->create_burden_realtime(fn, micron::forward<Args &&>(args)...);
}

// at core, see above
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...>)
auto &
realtime(u32 at, u32 prio, Fn fn, Args &&...args)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->create_realtime_at(at, prio, fn, micron::forward<Args &&>(args)...);
}

template <typename Tr>
void
sleep(thread_t<Tr> &t)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->sleep(t);
}

template <typename Tr>
void
awaken(thread_t<Tr> &t)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->awaken(t);
}

template <typename Tr>
void
snooze(thread_t<Tr> &t)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->throttle(t);
}

template <typename Tr>
void
cancel(thread_t<Tr> &t)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->cancel(t);
}

template <typename Tr>
void
await(thread_t<Tr> &t)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->await(t);
}

// lists all threads
auto
threads(void)
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
  return __global_threadpool->list();
  // auto lck = __global_threadpool->lock();
  //__global_threadpool->unlock(lck);
}

auto
lock()
{
  if ( __global_threadpool == nullptr ) [[unlikely]]
    exc<except::system_error>("micron thread::go(): system arena is uninitialized/nullptr");
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
