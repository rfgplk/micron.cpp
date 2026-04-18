//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../tuple.hpp"

#include "../../linux/__includes.hpp"

#include "sig_callbacks.hpp"

#include "../../linux/sys/__threads.hpp"
#include "../../linux/sys/resource.hpp"
#include "../../linux/sys/system.hpp"
#include "../../memory/stack_constants.hpp"
#include "../../new.hpp"
#include "../../queue/lambda_queue.hpp"
#include "../../sync/futex.hpp"

#include "../../except.hpp"

#include "../../atomic/atomic.hpp"
#include "../../memory/cmemory.hpp"
#include "../../tuple.hpp"

// NOTE: this file should contain all the constructors and implementations for the underlying threads (separate from the
// classes themselves), for readability

namespace micron
{
enum thread_returns : i32 { return_success, return_force, return_fail };

enum join_status { join_success, join_fail, join_allowed, join_busy };

template <typename T> struct __async_payload {
  atomic_token<bool> alive;
  atomic_token<u64> ret_val;
  posix::rusage_t usage;     // non atomic

  void
  clear(void)
  {
    alive.store(false);
    ret_val.store(0);
    usage = posix::rusage_t{};
  }
};

struct __thread_payload {
  atomic_token<bool> alive;
  // NOTE:
  // the pointer is used as the operand of a static_cast (7.6.1.8), except when the conversion is to pointer
  // to cv void, or to pointer to cv void and subsequently to pointer to cv char, cv unsigned char, or
  // cv std::byte (17.2.1)....
  // If a program attempts to access (3.1) the stored value of an object through a glvalue whose type is not
  // similar (7.3.5) to one of the following types the behavior is undefined:51
  //(11.1) — the dynamic type of the object,
  //(11.2) — a type that is the signed or unsigned type corresponding to the dynamic type of the object, or
  //(11.3) — a char, unsigned char, or std::byte type.
  atomic_ptr<byte *> ret_val;     // stop gap measure for returning arbitrary return types
  enum class tag : u8 { none = 0, literal, heap, __end } tag_val;

  posix::rusage_t usage;     // non atomic

  void
  clear(void)
  {
    alive.store(false);
    ret_val.store(nullptr, memory_order_acq_rel);
    usage = posix::rusage_t{};
  }
};

struct __worker_payload {
  atomic_token<u32> has_work{ 0 };            // for futex compat.
  atomic_token<bool> should_die{ false };     // rude :/
  micron::lambda_queue<256> queue;
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
    // function has _no_ return value, signal that it returned successfully (1)
    // NOTE: this may cause erronous behavior if 1 happens to be a valid pointer on your platform. be careful!
    payload->ret_val.store(reinterpret_cast<byte *>(1), memory_order_release);
    payload->tag_val = __thread_payload::tag::none;
  } else if constexpr ( micron::is_class_v<ret_t> and !micron::is_trivially_constructible_v<ret_t> and !micron::is_literal_type_v<ret_t> ) {
    // NOTE: IMPORTANT
    // make sure this is freed either by the calling thread, or whatever handles returns
    ret_t *ret = new ret_t(fn(micron::forward<Args>(args)...));
    payload->ret_val.store(reinterpret_cast<byte *>(ret), memory_order_release);
    payload->tag_val = __thread_payload::tag::heap;
  } else if constexpr ( sizeof(ret_t) > 8 ) {
    // bigger than 8 bytes, use new
    ret_t *ret = new ret_t(fn(micron::forward<Args>(args)...));
    payload->ret_val.store(reinterpret_cast<byte *>(ret), memory_order_release);
    payload->tag_val = __thread_payload::tag::heap;
  } else {
    // type is literal, can be stored
    payload->ret_val.store(reinterpret_cast<byte *>(fn(micron::forward<Args>(args)...)), memory_order_release);
    payload->tag_val = __thread_payload::tag::literal;
  }
  // end work

  // epilogue
  posix::getrusage(posix::rusage_thread, payload->usage);
  payload->alive.store(false, memory_order_seq_cst);
  return return_success;
}

// worker kernel, for concurrent arenas and parallelism
// main difference compared to a regular kernel is that this function NEVER returns, instead constantly loops checking
// for new work

i32
__worker_kernel(__worker_payload *payload)
{
  // prologue
  __thread_handler();
  pthread::set_cancel_state(pthread::cancel_state::enable);
  pthread::set_cancel_type(pthread::cancel_type::deferred);
rerun_worker:
  while ( payload->has_work.get(memory_order_acquire) == 0 ) {
    wait_futex(payload->has_work.ptr(), 0);
  }
  // doesn't need to be atomic
  if ( *payload->should_die.ptr() ) return return_force;
  payload->queue.execute();

  if ( payload->queue.head.get(memory_order_acquire) == payload->queue.tail.get(memory_order_acquire) )
    payload->has_work.store(0, memory_order_release);
  // epilogue
  posix::getrusage(posix::rusage_thread, payload->usage);
  goto rerun_worker;
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
  if ( payload == nullptr ) micron::exc<except::thread_error>("micron thread::__as_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn &, Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed ) micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

// rvalue-only
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args && ...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, Fn &&fn, Args &&...args)
{
  if ( payload == nullptr ) micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): invalid arguments");

  pthread_t pid
      = pthread::create_thread(attrs, __thread_kernel<Fn, Args...>, payload, micron::forward<Fn>(fn), micron::forward<Args>(args)...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): thread failed to spawn");
  return pid;
}

// const-lvalue / by-value
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<const Fn &, const Args &...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, const Fn &fn, const Args &...args)
{
  if ( payload == nullptr ) micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<const Fn &, const Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): thread failed to spawn");

  return pid;
}

// worker threads

template <auto Fn = __worker_kernel, typename P, typename... Args>
pthread_t
__as_unprepared_worker_thread_attached(const pthread_attr_t &attrs, P *payload)
{
  if ( payload == nullptr ) micron::exc<except::thread_error>("micron thread::__as_unprepared_worker_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, Fn, payload);

  if ( pid == pthread::thread_failed ) micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

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

  if ( pid == pthread::thread_failed ) micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

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

  pthread_t pid
      = pthread::create_thread(attrs, __thread_kernel<Fn, Args...>, payload, micron::forward<Fn>(fn), micron::forward<Args>(args)...);

  if ( pid == pthread::thread_failed ) micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");
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

  if ( pid == pthread::thread_failed ) micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

struct thread_attr_t {
  ~thread_attr_t() = default;

  thread_attr_t(pid_t a)
      : parent(a), pid(0), sched_policy(0), sched_priority(0), sched_niceness(0), detach_state(0), stack_size(0), stack_addr(nullptr)
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
  usize stack_size;
  addr_t *stack_addr;
};

};     // namespace micron
