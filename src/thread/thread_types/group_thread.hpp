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
// group_thread, a regular group_thread
template <usize Stack_Size = thread_stack_size> class group_thread
{
  using thread_type = thread_tag;

  void
  thread_handler()
  {
    micron::create_handler(__thread_sigchld, signal::child);
  }

  template <typename F, typename... Args>
    requires(micron::is_invocable_v<F, Args...>)
  inline __attribute__((always_inline)) void
  __impl_preparethread(const pthread_attr_t &attrs, F f, Args &&...args)
  {
    if ( attributes.stack_addr == nullptr ) {
      micron::exc<except::thread_error>("micron thread::__impl_preparethread(): a stack isn't allocated");
    }
    thread_handler();
    attributes.pid = __as_unprepared_thread_attached<F, Args...>(attrs, &payload, f, micron::forward<Args &&>(args)...);
  }

  void
  __release(void)
  {
    int r = 0;
    if ( r = micron::try_unmap(attributes.stack_addr, Stack_Size); r < 0 ) {
      micron::exc<except::memory_error>("micron thread::__release(): failed to unmap thread stack");
    }
    attributes.clear();
    payload.alive.store(false);
  }

  void
  __safe_release(void)
  {
    if ( alive() ) {
      micron::exc<except::thread_error>("micron thread::__safe_release(): tried to release a running thread");
    }
    if ( micron::try_unmap(attributes.stack_addr, Stack_Size) < 0 ) {
      micron::exc<except::memory_error>("micron thread::__safe_release(): failed to unmap thread stack");
    }
    attributes.clear();
  }

  // pid_t parent_pid;
  // pthread_t pid;
  // addr_t *fstack;
  thread_attr_t attributes;

public:
  __thread_payload payload;

  ~group_thread() { __release(); }

  group_thread(const group_thread &o) = delete;
  group_thread &operator=(const group_thread &) = delete;

  group_thread(void)
      : attributes(posix::getpid()), payload{} {}     // parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{} {}

  group_thread(group_thread &&o) : attributes(micron::move(o.attributes)), payload(micron::move(o.payload)) {}

  // prepared functions
  // fstack set by preparethread
  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args && ...>)
  group_thread(const pthread_attr_t &_attrs, Fn &&fn, Args &&...args) : attributes(posix::getpid(), _attrs), payload{}
  {
    __impl_preparethread(_attrs, micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<const Fn, const Args &...>)
  group_thread(const pthread_attr_t &_attrs, const Fn &fn, const Args &...args) : attributes(posix::getpid(), _attrs), payload{}
  {
    __impl_preparethread(_attrs, fn, args...);
  }

  group_thread &
  operator=(group_thread &&o)
  {
    attributes = micron::move(o.attributes);
    payload = micron::move(o.payload);
    return *this;
  }

  auto swap(group_thread &o) = delete;

  // yes, copying it out
  inline posix::rusage_t
  stats(void) const
  {
    if ( alive() )
      return {};
    return payload.usage;
  }

  inline bool
  alive(void) const
  {
    return payload.alive.get(micron::memory_order_seq_cst);
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
  join(void) -> int     // group_thread
  {
    auto r = pthread::__join_thread(attributes.pid);
    __safe_release();
    return r;
  }

  auto
  dismiss(void) -> int     // group_thread
  {
    if ( try_join() == error::busy ) {
      auto r = pthread::__join_thread(attributes.pid);
      __release();
      return r;
    }
    __release();
    return 1;
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

  long int
  signal(const micron::signal s)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)s);
    }
    return -1;
  }

  // pseudo sleep
  int
  sleep_second(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signal::usr1);
    }
    return -1;
  }

  int
  sleep(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signal::stop);
    }
    return -1;
  }

  int
  awaken(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signal::cont);
    }
    return -1;
  }

  int
  cancel(void)
  {
    if ( alive() ) {
      int r = pthread::cancel_thread(attributes.pid);
      signal(signal::usr2);
      return r;
    }
    return -1;
  }

  int
  terminate(void)
  {
    if ( alive() ) {
      return signal(signal::terminate);
    }
    return -1;
  }

  void
  wait_for(void) const
  {
    until(false, &group_thread<Stack_Size>::alive, this);
  }

  template <typename R>
  auto
  result(void)
  {
    wait_for();
    R val = static_cast<R>(payload.ret_val.get());
    return val;
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
