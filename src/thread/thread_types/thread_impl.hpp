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
#include "../../bits/__thread_exit_hook.hpp"
#include "../../memory/cmemory.hpp"
#include "../../tuple.hpp"

// NOTE: this file should contain all the constructors and implementations for the underlying threads (separate from the
// classes themselves), for readability

namespace micron
{
enum thread_returns : i32 { return_success, return_force, return_fail };

enum join_status { join_success, join_fail, join_allowed, join_busy };

template<typename T> struct __async_payload {
  atomic_token<bool> alive;
  atomic_token<u64> ret_val;
  posix::rusage_t usage;      // non atomic

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
  atomic_ptr<byte *> ret_val;      // stop gap measure for returning arbitrary return types
  enum class tag : u8 { none = 0, literal, heap, __end };
  atomic_token<u8> tag_val{ (u8)tag::none };
  void (*deleter)(byte *) = nullptr;

  posix::rusage_t usage;      // non atomic

  void
  clear(void)
  {
    alive.store(false);
    ret_val.store(nullptr, memory_order_release);
    tag_val.store((u8)tag::none, memory_order_release);
    deleter = nullptr;
    usage = posix::rusage_t{};
  }
};

struct __worker_payload {
  atomic_token<u32> has_work{ 0 };             // for futex compat.
  atomic_token<bool> should_die{ false };      // rude :/
  micron::lambda_queue<256> queue;
  atomic_token<u32> usage_seq{ 0 };
  posix::rusage_t usage;      // non atomic
};

// The kernel of the thread, encapsulating the req'd function
// status flags if the thread is running or not
// NOTE: args is always instantiated with decayed (value) types by __as_*_thread_attached
// taking args by value gives the kernel local storage; invoking fn(args...) passes lvalues
template<typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args &...>)
i32
__thread_kernel(__thread_payload *payload, Fn fn, Args... args)
{
  // prologue
  using ret_t = micron::invoke_result_t<Fn, Args &...>;
  __thread_handler();
  pthread::set_cancel_state(pthread::cancel_state::enable);
  pthread::set_cancel_type(pthread::cancel_type::deferred);
  payload->alive.store(true, memory_order_seq_cst);

  // work

  if constexpr ( micron::is_void_v<ret_t> ) {
    fn(args...);
    // function has _no_ return value, signal that it returned successfully (1)
    // NOTE: this may cause erronous behavior if 1 happens to be a valid pointer on your platform. be careful!
    payload->deleter = nullptr;
    payload->tag_val.store((u8)__thread_payload::tag::none, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(1), memory_order_release);
  } else if constexpr ( micron::is_class_v<ret_t> and !micron::is_trivially_constructible_v<ret_t> and !micron::is_literal_type_v<ret_t> ) {
    // NOTE: IMPORTANT
    // make sure this is freed either by the calling thread, or whatever handles returns
    ret_t *ret = new ret_t(fn(args...));
    payload->deleter = +[](byte *p) { delete reinterpret_cast<ret_t *>(p); };
    payload->tag_val.store((u8)__thread_payload::tag::heap, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(ret), memory_order_release);
  } else if constexpr ( sizeof(ret_t) > 8 ) {
    // bigger than 8 bytes, use new
    ret_t *ret = new ret_t(fn(args...));
    payload->deleter = +[](byte *p) { delete reinterpret_cast<ret_t *>(p); };
    payload->tag_val.store((u8)__thread_payload::tag::heap, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(ret), memory_order_release);
  } else {
    // type is literal, can be stored.
    ret_t __ret_val_storage = fn(args...);
    u64 __ret_raw = 0;
    __builtin_memcpy(&__ret_raw, &__ret_val_storage, sizeof(ret_t));
    payload->deleter = nullptr;
    payload->tag_val.store((u8)__thread_payload::tag::literal, memory_order_release);
    payload->ret_val.store(reinterpret_cast<byte *>(__ret_raw), memory_order_release);
  }
  // end work

  // epilogue
  // NOTE: run any registered per-thread cleanup (abcmalloc releasing this thread's arena slot) on the exiting thread while its TLS is still valid
  if ( micron::__thread_exit_hook ) micron::__thread_exit_hook();
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
  // must be an acq, matches release
  if ( payload->should_die.get(memory_order_acquire) ) return return_force;
  payload->queue.execute();

  // WARNING: hard setting has_work races with concurrent producers and can deadlock, CAS instead
  if ( payload->queue.head.get(memory_order_acquire) == payload->queue.tail.get(memory_order_acquire) ) {
    u32 expected = 1;
    if ( payload->has_work.compare_exchange_strong(expected, 0, memory_order_acq_rel, memory_order_acquire) ) {
      if ( payload->queue.head.get(memory_order_acquire) != payload->queue.tail.get(memory_order_acquire) ) {
        payload->has_work.store(1, memory_order_release);
      }
    }
  }
  // epilogue
  payload->usage_seq.add_fetch(1, memory_order_acq_rel);
  posix::getrusage(posix::rusage_thread, payload->usage);
  payload->usage_seq.add_fetch(1, memory_order_acq_rel);
  goto rerun_worker;
  return return_success;
}

// preprepared threads, make absolutely sure attrs are configured correctly/are valid before passing to this, no error
// checking built in
template<typename Fn, typename... Args>
  requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, Fn &&fn, Args &&...args)
{
  if ( payload == nullptr ) micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<micron::decay_t<Fn>, micron::decay_t<Args>...>, payload,
                                         micron::forward<Fn>(fn), micron::forward<Args>(args)...);

  if ( pid == pthread::thread_failed )
    micron::exc<except::thread_error>("micron thread::__as_unprepared_thread_attached(): thread failed to spawn");

  return pid;
}

// worker threads

template<auto Fn = __worker_kernel, typename P, typename... Args>
pthread_t
__as_unprepared_worker_thread_attached(const pthread_attr_t &attrs, P *payload)
{
  if ( payload == nullptr ) micron::exc<except::thread_error>("micron thread::__as_unprepared_worker_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, Fn, payload);

  if ( pid == pthread::thread_failed ) micron::exc<except::thread_error>("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

// unprepared threads
template<int Stack_Size, typename Fn, typename... Args>
  requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
pthread_t
__as_thread_attached(__thread_payload *payload, addr_t *fstack, Fn &&fn, Args &&...args)
{
  if ( payload == nullptr || fstack == nullptr )
    micron::exc<except::thread_error>("micron thread::__as_thread_attached(): invalid arguments");

  auto attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);

  pthread::set_stack_thread(attrs, fstack, Stack_Size);

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<micron::decay_t<Fn>, micron::decay_t<Args>...>, payload,
                                         micron::forward<Fn>(fn), micron::forward<Args>(args)...);

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
    pthread::get_sched_param(attr, sched_priority);      // NOTE: hacky workaround
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

};      // namespace micron
