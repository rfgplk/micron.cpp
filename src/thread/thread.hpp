//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/ptrs.hpp"

#include "../linux/__includes.hpp"
#include "../linux/process/signals.hpp"
#include "../linux/sys/__threads.hpp"
#include "../memory/stack_constants.hpp"

#include "threads.hpp"

namespace micron
{

// is_joinable == is thread joinable
// dismiss == joins with main thread but without checking for state
// join == joins with main thread
// signal == send signal to thread
// spawn == create a thread
// yield == give up run access as in pthread_yield
// terminate == terminate thread
// kill == kill thread (if called from itself, stops the thread, otherwise stop indicated thread)
// sleep == pause execution
// throttle == throttle execution
// park == park thread on a core or core mask

template<typename Tr> using __thread_pointer = micron::unique_pointer<Tr>;

// NOTE: solo as in these are free threads unattached to any arena
namespace solo
{

// now only one spawn
template<typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(micron::is_invocable_v<micron::decay_t<Func>, micron::decay_t<Args> &...>)
auto
spawn(Func f, Args &&...args) -> __thread_pointer<Tr>
{
  return __thread_pointer<Tr>(f, micron::forward<Args>(args)...);
}

// joining functions

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) bool
is_joinable(Tr &t)
{
  return t.can_join();
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) bool
is_joinable(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return false;
  return t->can_join();
}

// NOTE: using join() on threads which were abruptly terminated or otherwise killed/stopped will yield an error, this is by design. join()
// is meant only for rejoining properly exited threads, otherwise, use dismiss

template<typename Tr = auto_thread<>>
inline int
dismiss(Tr &t)
{
  int r = 0;
  r = t.dismiss();
  return r;
}

template<typename Tr = auto_thread<>>
inline int
dismiss(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  int r = 0;
  r = t->dismiss();
  t.clear();
  return r;
}

template<typename... Args>
inline void
dismiss(Args &...t)
{
  (dismiss(t), ...);
}

template<typename Tr = auto_thread<>>
inline int
join(Tr &t)
{
  int r = 0;
  r = t.join();
  return r;
}

template<typename Tr = auto_thread<>>
inline int
join(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  int r = 0;
  r = t->join();
  t.clear();
  return r;
}

template<typename... Args>
inline void
join(Args &...t)
{
  (join(t), ...);
}

template<typename Tr = auto_thread<>>
inline int
try_join(Tr &t)
{
  int r = 0;
  r = t.try_join();
  return r;
}

template<typename Tr = auto_thread<>>
inline int
try_join(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  int r = 0;
  r = t->try_join();
  if ( r == 1 ) t.clear();
  return r;
}

template<typename... Args>
inline void
try_join(Args &...t)
{
  (try_join(t), ...);
}

template<typename Tr = auto_thread<>>
inline int
wait_for(Tr &t)
{
  t.wait_for();
  return 0;
}

template<typename Tr = auto_thread<>>
inline int
wait_for(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  t->wait_for();
  return 0;
}

template<typename... Args>
inline void
wait_for(Args &...t)
{
  (wait_for(t), ...);
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
throttle(Tr &t)
{
  return t.sleep_second();      // throttle == the USR1 pseudo-sleep; no thread type defines throttle()
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
throttle(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  return t->sleep_second();      // throttle == the USR1 pseudo-sleep; no thread type defines throttle()
}

template<typename... Args>
inline int
throttle(Args &...t)
{
  int r = 0;
  ((r = r ? r : throttle(t)), ...);
  return r;
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
sleep(Tr &t)
{
  return t.sleep();
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
sleep(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  return t->sleep();
}

template<typename... Args>
inline int
sleep(Args &...t)
{
  int r = 0;
  ((r = r ? r : sleep(t)), ...);
  return r;
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
awaken(Tr &t)
{
  return t.awaken();
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
awaken(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  return t->awaken();
}

template<typename... Args>
inline int
awaken(Args &...t)
{
  int r = 0;
  ((r = r ? r : awaken(t)), ...);
  return r;
}

// SIGALRM -> __thread_yield handler -> micron::yield()
template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
yield(Tr &t)
{
  t.signal(signal::alarm);
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
yield(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return;
  t->signal(signal::alarm);
}

template<typename... Args>
inline void
yield(Args &...t)
{
  (yield(t), ...);
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
park(posix::cpu_set_t &set, __thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return;
  park_cpu(t->thread_id(), set);
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
park(posix::cpu_set_t &set, Tr &t)
{
  park_cpu(t.thread_id(), set);
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
interrupt(Tr &t)
{
  return t.signal(signal::interrupt);
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
interrupt(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  return t->signal(signal::interrupt);
}

template<typename... Args>
inline int
interrupt(Args &...t)
{
  int r = 0;
  ((r = r ? r : interrupt(t)), ...);
  return r;
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
force_stop(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  return t->terminate();      // self-reaping hard stop
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
force_stop(Tr &t)
{
  // per-thread stop
  return t.terminate();
}

template<typename... Args>
inline int
force_stop(Args &...t)
{
  int r = 0;
  ((r = r ? r : force_stop(t)), ...);
  return r;
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) int
terminate(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  return t->terminate();      // self-reaping hard stop
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) auto
terminate(Tr &t)
{
  return t.terminate();
}

template<typename... Args>
inline int
terminate(Args &...t)
{
  int r = 0;
  ((r = r ? r : terminate(t)), ...);
  return r;
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) auto
kill(Tr &t)
{
  return t.cancel();
}

template<typename Tr = auto_thread<>>
inline __attribute__((always_inline)) auto
kill(__thread_pointer<Tr> &t)
{
  if ( !micron::is_alive_ptr(t) ) return -1;
  return t->cancel();
}

template<typename... Args>
inline int
kill(Args &...t)
{
  int r = 0;
  ((r = r ? r : kill(t)), ...);
  return r;
}

};      // namespace solo
};      // namespace micron
