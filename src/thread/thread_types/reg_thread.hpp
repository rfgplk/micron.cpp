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
template<usize Stack_Size = thread_stack_size> class thread
{
  using thread_type = thread_tag;

  void
  thread_handler()
  {
    micron::create_handler(__thread_sigchld, signal::child);
  }

  template<typename F, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<F>, micron::decay_t<Args> &...>)
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
    payload.alive.store(true, memory_order_seq_cst);
    attributes.pid = __as_thread_attached<Stack_Size>(&payload, attributes.stack_addr, f, micron::forward<Args>(args)...);
  }

  void
  __release(void) noexcept
  {
    if ( attributes.stack_addr != nullptr ) {
      micron::try_unmap(attributes.stack_addr, Stack_Size);
    }
    attributes.clear();
    payload.alive.store(false, memory_order_seq_cst);
    if ( static_cast<__thread_payload::tag>(payload.tag_val.get(memory_order_acquire)) == __thread_payload::tag::heap ) {
      byte *ptr = payload.ret_val.exchange(nullptr, memory_order_acq_rel);
      auto del = payload.deleter;
      payload.deleter = nullptr;
      if ( ptr && del ) del(ptr);
    }
    payload.tag_val.store((u8)__thread_payload::tag::none, memory_order_release);
  }

  void
  __safe_release(void)
  {
    if ( alive() ) {
      micron::exc<except::thread_error>("micron thread::__safe_release(): tried to release a running thread");
    }
    __release();
  }

  // pid_t parent_pid;
  // pthread_t pid;
  // addr_t *fstack;
  thread_attr_t attributes;

public:
  __thread_payload payload;

  ~thread()
  {
    if ( attributes.pid != 0 ) pthread::__join_thread(attributes.pid);
    __release();
  }

  thread(const thread &o) = delete;
  thread &operator=(const thread &) = delete;

  thread(void)
      : attributes(posix::getpid()), payload{} { }      // parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{} {}

  // NOTE: moving an already-spawned thread is UB
  thread(thread &&o) : attributes(micron::move(o.attributes)), payload(micron::move(o.payload)) { }

  template<typename Fn, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
  thread(Fn &&fn, Args &&...args) : attributes(posix::getpid()), payload{}
  {
    __impl_makethread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  thread &
  operator=(thread &&o)
  {
    if ( this == &o ) return *this;
    // release *this's current thread+stack BEFORE adopting o; otherwise the running thread is orphaned
    // (never joined)
    if ( attributes.pid != 0 ) pthread::__join_thread(attributes.pid);
    __release();
    attributes = micron::move(o.attributes);      // thread_attr_t move nulls o.pid / o.stack_addr
    payload = micron::move(o.payload);
    return *this;
  }

  template<typename F, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<F>, micron::decay_t<Args> &...>)
  thread &
  operator[](F f, Args &&...args)
  {
    join();
    __impl_makethread(f, micron::forward<Args>(args)...);
    return *this;
  }

  auto swap(thread &o) = delete;

  inline posix::rusage_t
  stats(void) const
  {
    if ( alive() ) return {};
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
    if ( attributes.pid == 0 ) return 0;
    int r = pthread::__try_join_thread(attributes.pid);
    if ( r == 0 ) {
      return 1;
    }
    if ( r == error::busy or r == error::invalid_arg ) return r;
    return 0;
  }

  auto
  join(void) -> int      // thread
  {
    if ( attributes.pid == 0 ) return 0;      // nothing to join (default/moved-from/already-joined)
    auto r = pthread::__join_thread(attributes.pid);
    __safe_release();
    return r;
  }

  auto
  dismiss(void) -> int      // thread
  {
    int r = try_join();
    if ( r == error::busy ) {
      auto rr = pthread::__join_thread(attributes.pid);
      __release();
      return rr;
    }
    return r;
  }

  auto
  try_join(void) -> int
  {
    if ( attributes.pid == 0 ) return 0;
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
  signal(const signal s)
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

  // true per-thread suspend
  int
  sleep(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signal::thread_suspend);
    }
    return -1;
  }

  int
  awaken(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(attributes.parent, pthread::get_thread_id(attributes.pid), (int)signal::thread_resume);
    }
    return -1;
  }

  // cooperative stop-and-reap
  int
  cancel(void)
  {
    if ( alive() ) {
      int r = pthread::cancel_thread(attributes.pid);
      signal(signal::usr2);
      if ( attributes.pid != 0 ) pthread::__join_thread(attributes.pid);
      payload.alive.store(false, memory_order_seq_cst);
      __safe_release();
      return r;
    }
    return -1;
  }

  // hard stop
  int
  terminate(void)
  {
    if ( !alive() ) return -1;
    int r = signal(signal::terminate);
    if ( micron::until_timeout(__thread_term_timeout_ms, [this]() noexcept { return !alive(); }) ) {
      // thread is kernel-dead now
      if ( attributes.pid != 0 ) pthread::__join_thread(attributes.pid);
      __release();
    }
    return r;
  }

  void
  wait_for(void) const
  {
    until(false, &thread<Stack_Size>::alive, this);
  }

  template<typename R>
    requires(micron::is_trivially_copyable_v<R> && sizeof(R) <= sizeof(byte *))
  R
  result(void)
  {
    wait_for();
    R val{};
    if ( static_cast<__thread_payload::tag>(payload.tag_val.get(memory_order_acquire)) == __thread_payload::tag::literal ) {
      byte *p = payload.ret_val.get(memory_order_acquire);
      u64 raw = reinterpret_cast<u64>(p);
      __builtin_memcpy(&val, &raw, sizeof(R));
    }
    return val;
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
