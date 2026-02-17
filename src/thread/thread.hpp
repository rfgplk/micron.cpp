//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/ptrs.hpp"

#include "../linux/__includes.hpp"
#include "../linux/sys/__threads.hpp"
#include "../memory/stack_constants.hpp"

#include "threads.hpp"

namespace micron
{

// is_joinable == is thread joinable
// join == joins with main thread
// signal == send signal to thread
// spawn == create a thread
// yield == give up run access as in pthread_yield
// terminate == terminate thread
// kill == kill thread (if called from itself, stops the thread, otherwise stop indicated thread)
// sleep == pause execution
// throttle == throttle execution
// park == park thread on a core or core mask

template <typename Tr> using __thread_pointer = micron::unique_pointer<Tr>;

// NOTE: solo as in these are free threads unattached to any arena
namespace solo
{

// NOTE: it's _absolutely_ important you leave the template constraints in place to have proper binding (will cause chaos
// otherwise)
// NOTE: compiler_bug?! "auto in template"

template <typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(micron::is_invocable_v<Func, Args...> && sizeof...(Args) == 0
           && ((micron::is_lvalue_reference_v<Args> && ...) or (micron::is_rvalue_reference_v<Args> && ...))
           && (!micron::is_same_v<micron::decay_t<Args>, Args>
               && ...))     // && (micron::is_same_v<micron::remove_reference_t<Args>, Args> && ...))
auto
spawn(Func f, Args &&...args) -> __thread_pointer<Tr>
{
  return __thread_pointer<Tr>(f, micron::forward<Args &&>(args)...);
}

/*
template <typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(micron::is_invocable_v<Func, Args...> && sizeof...(Args) > 0
           && ((micron::is_lvalue_reference_v<Args> && ...) or (micron::is_rvalue_reference_v<Args> && ...))
           && (!micron::is_same_v<micron::decay_t<Args>, Args>
               && ...))     // && (micron::is_same_v<micron::remove_reference_t<Args>, Args> && ...))
// requires(micron::is_invocable_v<Func, Args...>)
auto
spawn(Func f, Args &&...args) -> __thread_pointer<Tr>
{
  return micron::unique_pointer<Tr>(f, args...);
}*/

template <typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(micron::is_invocable_v<Func, Args...> && sizeof...(Args) > 0 && ((micron::is_lvalue_reference_v<Args> && ...))
           && (!micron::is_same_v<micron::decay_t<Args>, Args> && ...))
auto
spawn(Func f, const Args &...args) -> __thread_pointer<Tr>
{
  return __thread_pointer<Tr>(f, micron::forward<Args &>(args)...);
}

template <typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(micron::is_invocable_v<Func, Args...> && sizeof...(Args) > 0
           && ((!micron::is_lvalue_reference_v<Args> && ...) and (!micron::is_rvalue_reference_v<Args> && ...))
           && (!micron::is_reference_v<Args> && ...) && (micron::is_same_v<micron::decay_t<Args>, Args> && ...)
           && (micron::is_same_v<micron::remove_reference_t<Args>, Args> && ...))
auto
spawn(Func f, Args... args) -> __thread_pointer<Tr>
{
  return __thread_pointer<Tr>(f, args...);
}

// joining functions

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) bool
is_joinable(Tr &t)
{
  return t.can_join();
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) bool
is_joinable(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return false;
  return t->can_join();
}

template <typename Tr = auto_thread<>>
inline int
join(Tr &t)
{
  int r = 0;
  r = t.join();
  return r;
}

template <typename Tr = auto_thread<>>
inline int
join(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  int r = 0;
  r = t->join();
  t.clear();
  return r;
}

template <typename... Args>
inline void
join(Args &...t)
{
  (join(t), ...);
}

template <typename Tr = auto_thread<>>
inline int
try_join(Tr &t)
{
  int r = 0;
  r = t.try_join();
  return r;
}

template <typename Tr = auto_thread<>>
inline int
try_join(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  int r = 0;
  r = t->try_join();
  if ( r == 1 )
    t.clear();
  return r;
}

template <typename... Args>
inline void
try_join(Args &...t)
{
  (try_join(t), ...);
}

template <typename Tr = auto_thread<>>
inline int
wait_for(Tr &t)
{
  t.wait_for();
  return 0;
}

template <typename Tr = auto_thread<>>
inline int
wait_for(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  t->wait_for();
  return 0;
}

template <typename... Args>
inline void
wait_for(Args &...t)
{
  (wait_for(t), ...);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
throttle(Tr &t)
{
  return t.throttle();
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
throttle(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  return t->throttle();
}

template <typename... Args>
inline int
throttle(Args &...t)
{
  if ( int r = (throttle(t), ...) != 0 )
    return r;
  return 0;
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
sleep(Tr &t)
{
  return t.sleep();
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
sleep(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  return t->sleep();
}

template <typename... Args>
inline int
sleep(Args &...t)
{
  if ( int r = (sleep(t), ...) != 0 )
    return r;
  return 0;
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
awaken(Tr &t)
{
  return t.awaken();
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
awaken(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  return t->awaken();
}

template <typename... Args>
inline int
awaken(Args &...t)
{
  if ( int r = (awaken(t), ...) != 0 )
    return r;
  return 0;
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
yield(Tr &t)
{
  t.yield();
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
yield(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return;
  t->yield();
}

template <typename... Args>
inline void
yield(Args &...t)
{
  (yield(t), ...);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
park(posix::cpu_set_t &set, __thread_pointer<Tr> &t)
{
  park_cpu(t->thread_id(), set);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
park(posix::cpu_set_t &set, Tr &t)
{
  park_cpu(t.thread_id(), set);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
interrupt(Tr &t)
{
  return t.signal(signals::interrupt);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
interrupt(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  return t->signal(signals::interrupt);
}

template <typename... Args>
inline int
interrupt(Args &...t)
{
  if ( int r = (interrupt(t), ...) != 0 )
    return r;
  return 0;
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
force_stop(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  return t->signal(signals::stop);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
force_stop(Tr &t)
{
  return t.signal(signals::stop);
}

template <typename... Args>
inline int
force_stop(Args &...t)
{
  if ( int r = (force_stop(t), ...) != 0 )
    return r;
  return 0;
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
terminate(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  return t->signal(signals::terminate);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) auto
terminate(Tr &t)
{
  return t.signal(signals::terminate);
}

template <typename... Args>
inline int
terminate(Args &...t)
{
  if ( int r = (terminate(t), ...) != 0 )
    return r;
  return 0;
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) auto
kill(Tr &t)
{
  return t.signal(signals::kill9);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) auto
kill(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) )
    return -1;
  return t->signal(signals::kill9);
}

template <typename... Args>
inline int
kill(Args &...t)
{
  if ( int r = (kill(t), ...) != 0 )
    return r;
  return 0;
}

};
};
