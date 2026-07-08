//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../type_traits.hpp"
#include "../../types.hpp"

#include "../../linux/__includes.hpp"
#include "../../linux/sys/micthread/threads.hpp"
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
template<usize Stack_Size = thread_stack_size> class void_thread
{
  using thread_type = thread_tag;

  void
  thread_handler()
  {
    micron::create_handler(__thread_sigchld, signal::child);
  }

  inline __attribute__((always_inline)) void
  __adopt(const thread_attr_t &a)
  {
    attributes.stack_addr = a.stack_addr;
    attributes.stack_size = a.stack_size;
    attributes.sched_policy = a.sched_policy;
    attributes.sched_priority = a.sched_priority;
  }

  inline __attribute__((always_inline)) void
  __impl_preparethread()
  {
    if ( attributes.stack_addr == nullptr ) {
      micron::exc<except::thread_error>("micron thread::__impl_preparethread(): a stack isn't allocated");
    }
    thread_handler();
    __link_launch<Stack_Size, &__worker_kernel>(attributes.pid, attributes.ctid, attributes.tls, attributes.pth, &payload,
                                                attributes.stack_addr);
  }

  template<typename Fn, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
  inline __attribute__((always_inline)) void
  __impl_runthread(Fn &&fn, Args &&...args)
  {
    if ( attributes.stack_addr == nullptr ) {
      micron::exc<except::thread_error>("micron thread::__impl_runthread(): a stack isn't allocated");
    }
    auto __fn = [f = micron::forward<Fn>(fn), ... a = micron::forward<Args>(args)] { f(a...); };
    payload.queue.push(micron::move(__fn));
    micron::release_futex(payload.has_work.ptr(), 1);
  }

  void
  __release(void)
  {
    // void_thread does NOT own its stack (the arena does); it DOES own its from-scratch TLS block
    micron::__tls_free_frame(attributes.tls);
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

  ~void_thread()
  {
    if ( __link_valid(attributes.pid, attributes.pth) ) {
      stop();
      __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    }
    __release();
  }

  void_thread(const void_thread &o) = delete;
  void_thread &operator=(const void_thread &) = delete;

  void_thread(void)
      : attributes(posix::getpid()), payload{} { }      // parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{} {}

  // moving an already-spawned worker is UB (kernel holds &payload/&ctid); guard rejects a live source
  void_thread(void_thread &&o)
      : attributes((micron::__thread_assert_movable_src(o.attributes.pid, o.attributes.pth), micron::move(o.attributes))),
        payload(micron::move(o.payload))
  {
  }

  void_thread(const thread_attr_t &_attrs) : attributes(posix::getpid()), payload{}
  {
    __adopt(_attrs);
    __impl_preparethread();
  }

  template<typename Fn, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
  void_thread(const thread_attr_t &_attrs, Fn &&fn, Args &&...args) : attributes(posix::getpid()), payload{}
  {
    __adopt(_attrs);
    __impl_preparethread();
    __impl_runthread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  void_thread &
  operator=(void_thread &&o)
  {
    if ( this == &o ) return *this;
    micron::__thread_assert_movable_src(o.attributes.pid, o.attributes.pth);      // o must be pre-spawn/joined (holds &o.payload)
    if ( __link_valid(attributes.pid, attributes.pth) ) {
      stop();
      __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    }
    __release();
    attributes = micron::move(o.attributes);
    payload = micron::move(o.payload);
    return *this;
  }

  auto swap(void_thread &o) = delete;

  inline posix::rusage_t
  stats(void) const
  {
    posix::rusage_t copy{};
    for ( ;; ) {
      u32 s1 = payload.usage_seq.get(memory_order_acquire);
      if ( s1 & 1u ) {
        __cpu_pause();
        continue;
      }
      copy = payload.usage;
      u32 s2 = payload.usage_seq.get(memory_order_acquire);
      if ( s1 == s2 ) break;
    }
    return copy;
  }

  template<typename F, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<F>, micron::decay_t<Args> &...>)
  void_thread &
  operator[](F &&f, Args &&...args)
  {
    __impl_runthread(micron::forward<F>(f), micron::forward<Args>(args)...);
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
    return (micthread::thread_kill(attributes.parent, __link_tid(attributes.pid, attributes.pth), 0) == 0 ? true : false);
  }

  auto
  can_join(void) -> int
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;
    int r = __link_try_join(attributes.pid, &attributes.ctid, attributes.pth);
    if ( r == 0 ) {
      return 1;
    }
    if ( r == error::busy or r == error::invalid_arg ) return r;
    return 0;
  }

  auto
  join(void) -> int      // worker
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;
    stop();
    auto r = __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    __safe_release();
    return r;
  }

  auto
  try_join(void) -> int
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;
    int r = __link_try_join(attributes.pid, &attributes.ctid, attributes.pth);
    if ( r == 0 ) {
      __release();
      return 0;
    }
    return r;
  }

  auto
  thread_id(void) const
  {
    return __link_tid(attributes.pid, attributes.pth);
  }

  auto
  native_handle(void) const
  {
    return __link_tid(attributes.pid, attributes.pth);
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
    payload.should_die.store(1, memory_order_seq_cst);
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
};      // namespace micron
