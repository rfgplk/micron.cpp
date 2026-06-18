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
// bounded start handshake budget
// regular time for a new thread to reach its kernel worker prologue is ~microseconds;
// this is a generous ceiling above which we declare it faulted and hard-error rather
inline constexpr f64 __thread_start_timeout_ms = 1000.0;

// group_thread, a regular group_thread
template<usize Stack_Size = thread_stack_size> class group_thread
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
  __impl_preparethread(const pthread_attr_t &attrs, F f, Args &&...args)
  {
    if ( attributes.stack_addr == nullptr ) {
      micron::exc<except::thread_error>("micron thread::__impl_preparethread(): a stack isn't allocated");
    }
    thread_handler();
    payload.alive.store(true, memory_order_seq_cst);
    payload.started.store(false, memory_order_seq_cst);
    attributes.pid = __as_unprepared_thread_attached(attrs, &payload, f, micron::forward<Args>(args)...);

    // bounded start handshake
    if ( !micron::until_timeout(true, __thread_start_timeout_ms, [this]() noexcept {
           const bool s = payload.started.get(memory_order_acquire);
           if ( !s ) micron::yield();
           return s;
         }) ) {
      micron::exc<except::thread_error>("micron thread::__impl_preparethread(): spawned thread never reached its kernel (faulted on entry?)");
    }
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

  thread_attr_t attributes;

public:
  __thread_payload payload;

  ~group_thread()
  {
    if ( attributes.pid != 0 ) pthread::__join_thread(attributes.pid);
    __release();
  }

  group_thread(const group_thread &o) = delete;
  group_thread &operator=(const group_thread &) = delete;

  group_thread(void) : attributes(posix::getpid()), payload{} { }

  // moving an already-spawned thread is UB (kernel holds &payload)
  group_thread(group_thread &&o) : attributes(micron::move(o.attributes)), payload(micron::move(o.payload)) { }

  // prepared functions
  template<typename Fn, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
  group_thread(const pthread_attr_t &_attrs, Fn &&fn, Args &&...args) : attributes(posix::getpid(), _attrs), payload{}
  {
    __impl_preparethread(_attrs, micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  group_thread &
  operator=(group_thread &&o)
  {
    if ( this == &o ) return *this;
    // release *this's current thread+stack BEFORE adopting o; otherwise the running thread is orphaned
    if ( attributes.pid != 0 ) pthread::__join_thread(attributes.pid);
    __release();
    attributes = micron::move(o.attributes);      // thread_attr_t move nulls o.pid / o.stack_addr
    payload = micron::move(o.payload);
    return *this;
  }

  auto swap(group_thread &o) = delete;

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
  join(void) -> int      // group_thread
  {
    if ( attributes.pid == 0 ) return 0;      // nothing to join (default/moved-from/already-joined)
    auto r = pthread::__join_thread(attributes.pid);
    __safe_release();
    return r;
  }

  auto
  dismiss(void) -> int      // group_thread
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
    if ( attributes.pid == 0 ) return 0;      // already joined
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
