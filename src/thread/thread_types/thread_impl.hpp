//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/__includes.hpp"

#include "sig_callbacks.hpp"

#include "../../linux/sys/__threads.hpp"
#include "../../linux/sys/resource.hpp"
#include "../../linux/sys/system.hpp"
#include "../../memory/stack.hpp"

#include "../../except.hpp"

#include "../../atomic/atomic.hpp"
#include "../../memory/cmemory.hpp"
#include "../../tuple.hpp"

// NOTE: this file should contain all the constructors and implementations for the underlying threads (separate from the
// classes themselves), for readability

namespace micron
{
enum thread_returns : i32 { return_success, return_fail };
enum join_status { join_success, join_fail, join_allowed, join_busy };

struct __thread_payload {
  atomic_token<bool> alive;
  atomic_token<u64> ret_val;
  posix::rusage_t usage;     // non atomic
};
// The kernel of the thread, encapsulating the req'd function
// status flags if the thread is running or not
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...>)
i32
__thread_kernel(__thread_payload *payload, Fn fn, Args &&...args)
{
  // prologue
  using ret_t = micron::invoke_result_t<Fn, Args...>;
  __thread_handler();
  pthread::set_cancel_state(pthread::cancel_state::enable);
  pthread::set_cancel_type(pthread::cancel_type::deferred);
  payload->alive.store(true, memory_order_seq_cst);

  // work

  if constexpr ( micron::is_void_v<ret_t> ) {
    fn(micron::forward<Args>(args)...);
  } else {
    ret_t ret = fn(micron::forward<Args>(args)...);
    payload->ret_val.store(static_cast<u64>(ret), memory_order_seq_cst);
    // nasty std::any workaround
    // auto ret = fn(micron::forward<Args>(args)...);
    // payload->ret_val = reinterpret_cast<void *>(&ret);
    // TODO: think about adding a futex here
  }

  // end work

  // epilogue
  posix::getrusage(posix::rusage_thread, payload->usage);
  payload->alive.store(false, memory_order_seq_cst);
  return return_success;
}

// preprepared threads, make absolutely sure attrs are configured correctly/are valid before passing to this, no error
// checking built in

// lvalue-only
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn &, Args &...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, Fn &fn, Args &...args)
{
  if ( payload == nullptr )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn &, Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

// rvalue-only
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args && ...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, Fn &&fn, Args &&...args)
{
  if ( payload == nullptr )
    micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn, Args...>, payload, micron::forward<Fn>(fn),
                                         micron::forward<Args>(args)...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): thread failed to spawn");
  return pid;
}

// const-lvalue / by-value
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<const Fn &, const Args &...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, const Fn &fn,
                                const Args &...args)
{
  if ( payload == nullptr )
    micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<const Fn &, const Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): thread failed to spawn");

  return pid;
}

// unprepared threads

// lvalue-only
template <int Stack_Size, typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn &, Args &...>)
pthread_t
__as_thread_attached(__thread_payload *payload, addr_t *fstack, Fn &fn, Args &...args)
{
  if ( payload == nullptr || fstack == nullptr )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): invalid arguments");

  auto attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);

  pthread::set_stack_thread(attrs, fstack, Stack_Size);

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn &, Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

// rvalue-only
template <int Stack_Size, typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args && ...>)
pthread_t
__as_thread_attached(__thread_payload *payload, addr_t *fstack, Fn &&fn, Args &&...args)
{
  if ( payload == nullptr || fstack == nullptr )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): invalid arguments");

  auto attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);

  pthread::set_stack_thread(attrs, fstack, Stack_Size);

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn, Args...>, payload, micron::forward<Fn>(fn),
                                         micron::forward<Args>(args)...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");
  return pid;
}

// const-lvalue / by-value
template <int Stack_Size, typename Fn, typename... Args>
  requires(micron::is_invocable_v<const Fn &, const Args &...>)
pthread_t
__as_thread_attached(__thread_payload *payload, addr_t *fstack, const Fn &fn, const Args &...args)
{
  if ( payload == nullptr || fstack == nullptr )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): invalid arguments");

  auto attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);

  pthread::set_stack_thread(attrs, fstack, Stack_Size);

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<const Fn &, const Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

struct thread_attr_t {
  ~thread_attr_t() = default;
  thread_attr_t(pid_t a)
      : parent(a), pid(0), sched_policy(0), sched_priority(0), sched_niceness(0), detach_state(0), stack_size(0),
        stack_addr(nullptr)
  {
  }
  thread_attr_t(pid_t a, const pthread_attr_t &attr) : parent(a), pid(0)
  {
    pthread::get_sched_policy(attr, sched_policy);
    pthread::get_sched_param(attr, sched_priority);     // NOTE: hacky workaround
    sched_niceness = 0;
    pthread::get_detach_state(attr, detach_state);
    pthread::get_stack_thread(attr, stack_addr, stack_size);
  }
  void
  clear()
  {
    pid = 0;
    stack_size = 0;
    stack_addr = nullptr;
  }
  pid_t parent;
  pthread_t pid;
  i32 sched_policy;
  u32 sched_priority;
  u32 sched_niceness;
  i32 detach_state;
  size_t stack_size;
  addr_t *stack_addr;
};

};
