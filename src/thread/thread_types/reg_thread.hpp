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
// thread, a regular thread
template <size_t Stack_Size = thread_stack_size> class thread
{
  using thread_type = thread_tag;

  void
  thread_handler()
  {
    sigaction_t sa = {};
    sa.sigaction_handler.sa_handler = __thread_sigchld;
    micron::sigemptyset(sa.sa_mask);
    sa.sa_flags = sa_restart;
    micron::sigaction(sig_chld, sa, nullptr);
  }

  template <typename F, typename... Args>
    requires(micron::is_invocable_v<F, Args...>)
  inline __attribute__((always_inline)) void
  __impl_makethread(F f, Args &&...args)
  {
    if ( attributes.stack_addr ) {
      micron::exc<except::thread_error>("micron thread::__impl_makethread(): a stack is already allocated");
    }
    attributes.stack_addr = micron::addrmap(Stack_Size);
    if ( micron::mmap_failed(attributes.stack_addr) )
      micron::exc<except::thread_error>("micron thread::__impl_makethread(): failed to allocate stack");

    thread_handler();
    attributes.pid = __as_thread_attached<Stack_Size, F, Args...>(&payload, attributes.stack_addr, f, micron::forward<Args &&>(args)...);
    // no longer needed
    // while ( pthread::thread_kill(parent_pid, pid, 0) != 0 )
    //  __cpu_pause();
    // massively important, wait for the thread to actually be
    // created by the scheduler, otherwise race conditions will occur

    // specifically, if we don't have for the actual process to be created we can get into a race condition where the
    // thread doesn't exist yet (it takes a few cycles for the scheduler to actually insert the thread into the process
    // table)
  }

  void
  __release(void)
  {
    int r = 0;
    if ( r = micron::try_unmap(attributes.stack_addr, Stack_Size); r < 0 ) {
      micron::exc<except::memory_error>("micron thread::__release(): failed to unmap thread stack");
    }
    attributes.clear();
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

  ~thread() { __release(); }

  thread(const thread &o) = delete;
  thread &operator=(const thread &) = delete;

  thread(void) : attributes(posix::getpid()), payload{} {}     // parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{} {}

  thread(thread &&o) : attributes(micron::move(o.attributes)), payload(micron::move(o.payload)) {}

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn &, Args &...>)
  thread(Fn &fn, Args &...args) : attributes(posix::getpid()), payload{}
  {
    __impl_makethread(fn, args...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args && ...>)
  thread(Fn &&fn, Args &&...args) : attributes(posix::getpid()), payload{}
  {
    __impl_makethread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<const Fn, const Args &...>)
  thread(const Fn &fn, const Args &...args) : attributes(posix::getpid()), payload{}
  {
    __impl_makethread(fn, args...);
  }

  thread &
  operator=(thread &&o)
  {
    attributes = micron::move(o.attributes);
    payload = micron::move(o.payload);
    return *this;
  }

  template <typename F, typename... Args>
    requires(micron::is_invocable_v<F, Args...>)
  thread &
  operator[](F f, Args &&...args)
  {
    join();
    __impl_makethread(f, args...);
    return *this;
  }

  auto swap(thread &o) = delete;

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

  long int
  signal(const signals s)
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
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signals::usr1);
    }
    return -1;
  }

  int
  sleep(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signals::stop);
    }
    return -1;
  }

  int
  awaken(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signals::cont);
    }
    return -1;
  }

  int
  cancel(void)
  {
    if ( alive() ) {
      int r = pthread::cancel_thread(attributes.pid);
      signal(signals::usr2);
      return r;
    }
    return -1;
  }

  void
  wait_for(void) const
  {
    until(false, &thread<Stack_Size>::alive, this);
  }

  template <typename R>
  auto
  result(void)
  {
    wait_for();
    R val = static_cast<R>(payload.ret_val.get());
    return val;
  }

  addr_t *
  stack()
  {
    return attributes.stack_addr;
  }

  constexpr size_t
  stack_size() const
  {
    return Stack_Size;
  }
};
};     // namespace micron
