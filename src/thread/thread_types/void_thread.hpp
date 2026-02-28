//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/__includes.hpp"
#include "../../linux/sys/__threads.hpp"
#include "../../linux/sys/resource.hpp"
#include "../../linux/sys/system.hpp"
#include "../../memory/stack_constants.hpp"

#include "../../except.hpp"

#include "../../atomic/atomic.hpp"
#include "../../control.hpp"
#include "../../memory/cmemory.hpp"
#include "../../memory/mman.hpp"
#include "../../pointer.hpp"
#include "../../sync/until.hpp"
#include "../../sync/yield.hpp"
#include "../../tags.hpp"
#include "../../tuple.hpp"

#include "sig_callbacks.hpp"
#include "thread_impl.hpp"

namespace micron
{
// void_thread, a thread type which *doesn't* allocate or manage it's own stack
template <usize Stack_Size = thread_stack_size> class void_thread
{
  using thread_type = thread_tag;

  void
  thread_handler()
  {
    micron::create_handler(__thread_sigchld, signal::child);
  }

  inline __attribute__((always_inline)) void
  __impl_preparethread(const pthread_attr_t &attrs)
  {
    if ( attributes.stack_addr == nullptr ) {
      micron::exc<except::thread_error>("micron thread::__impl_preparethread(): a stack isn't allocated");
    }
    thread_handler();
    attributes.pid = __as_unprepared_worker_thread_attached(attrs, &payload);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args...>)
  inline __attribute__((always_inline)) void
  __impl_runthread(Fn &&fn, Args &&...args)
  {
    if ( attributes.stack_addr == nullptr ) {
      micron::exc<except::thread_error>("micron thread::__impl_runthread(): a stack isn't allocated");
    }
    // creates the function to be inserted into the queue
    auto __fn = [f = micron::forward<Fn &&>(fn), ... a = micron::forward<Args &&>(args)] { f(a...); };
    payload.queue.push(micron::move(__fn));
    micron::release_futex(payload.has_work.ptr(), 1);
  }

  void
  __release(void)
  {
    attributes.clear();
  }

  void
  __safe_release(void)
  {
    __release();
  }

  thread_attr_t attributes;

public:
  __worker_payload payload;

  ~void_thread() { __release(); }

  void_thread(const void_thread &o) = delete;
  void_thread &operator=(const void_thread &) = delete;

  void_thread(void)
      : attributes(posix::getpid()), payload{} {}     // parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{} {}

  void_thread(void_thread &&o) : attributes(micron::move(o.attributes)), payload(micron::move(o.payload)) {}

  void_thread(const pthread_attr_t &_attrs) : attributes(posix::getpid(), _attrs), payload{} { __impl_preparethread(_attrs); }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args && ...>)
  void_thread(const pthread_attr_t &_attrs, Fn &&fn, Args &&...args) : attributes(posix::getpid(), _attrs), payload{}
  {
    __impl_preparethread(_attrs);
    __impl_runthread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<const Fn, const Args &...>)
  void_thread(const pthread_attr_t &_attrs, const Fn &fn, const Args &...args) : attributes(posix::getpid(), _attrs), payload{}
  {
    __impl_preparethread(_attrs);
    __impl_runthread(fn, args...);
  }

  void_thread &
  operator=(void_thread &&o)
  {
    attributes = micron::move(o.attributes);
    payload = micron::move(o.payload);
    return *this;
  }

  auto swap(void_thread &o) = delete;

  // yes, copying it out
  inline posix::rusage_t
  stats(void) const
  {
    return payload.usage;
  }

  template <typename F, typename... Args>
    requires(micron::is_invocable_v<F, Args...>)
  void_thread &
  operator[](F &&f, Args &&...args)
  {
    __impl_runthread(micron::forward<F &&>(f), micron::forward<Args &&>(args)...);
    return *this;
  }

  inline bool alive(void) = delete;

  const auto &
  get_status(void) const
  {
    return payload.has_work;
  }

  bool
  is_working(void) const
  {
    return payload.has_work.get(memory_order_acquire);
  }

  inline bool
  active(void) const
  {
    // more reliable, although slower
    return (pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), 0) == 0 ? true : false);
  }

  auto
  can_join(void) -> int
  {
    int r = pthread::__try_join_thread(attributes.pid);
    if ( r == 0 ) {
      return 1;
    }
    if ( r == error::busy or r == error::invalid_arg )
      return r;
    return 0;
  }

  auto
  join(void) -> int     // thread
  {
    auto r = pthread::__join_thread(attributes.pid);
    __safe_release();
    return r;
  }

  auto
  try_join(void) -> int
  {
    int r = pthread::__try_join_thread(attributes.pid);
    if ( r == 0 ) {
      __release();
      return 0;
    }
    return r;
  }

  auto
  thread_id(void) const
  {
    return pthread::get_thread_id(attributes.pid);
  }

  auto
  native_handle(void) const
  {
    return attributes.pid;
  }

  long int signal(const signal s) = delete;
  int sleep_second(void) = delete;
  int sleep(void) = delete;
  int awaken(void) = delete;
  int cancel(void) = delete;
  void wait_for(void) = delete;
  auto result(void) = delete;

  void
  stop(void)
  {
    payload.should_die.store(1, memory_order_release);
    micron::release_futex(payload.has_work.ptr(), 1);
  }

  const auto &
  attrs() const
  {
    return attributes;
  }

  addr_t *
  stack()
  {
    return attributes.stack_addr;
  }

  constexpr usize
  stack_size() const
  {
    return Stack_Size;
  }
};
};     // namespace micron
