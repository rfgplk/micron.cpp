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
    __link_launch<Stack_Size, &__thread_kernel<micron::decay_t<F>, micron::decay_t<Args>...>>(
        attributes.pid, attributes.ctid, attributes.tls, attributes.pth, &payload, attributes.stack_addr, f, micron::forward<Args>(args)...);
  }

  void
  __release(void) noexcept
  {
    if ( attributes.stack_addr != nullptr ) {
      micron::try_unmap(attributes.stack_addr, Stack_Size);
    }
    micron::__tls_free_frame(attributes.tls);      // free this thread's from-scratch TLS block
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
    if ( __link_valid(attributes.pid, attributes.pth) ) __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    __release();
  }

  thread(const thread &o) = delete;
  thread &operator=(const thread &) = delete;

  thread(void)
      : attributes(posix::getpid()), payload{} { }      // parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{} {}

  // moving an already-spawned thread is UB
  thread(thread &&o)
      : attributes((micron::__thread_assert_movable_src(o.attributes.pid, o.attributes.pth), micron::move(o.attributes))),
        payload(micron::move(o.payload))
  {
  }

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
    micron::__thread_assert_movable_src(o.attributes.pid, o.attributes.pth);      // o must be pre-spawn/joined
    // release *this's current thread+stack BEFORE adopting o; otherwise the running thread is orphaned
    // (never joined)
    if ( __link_valid(attributes.pid, attributes.pth) ) __link_join(attributes.pid, &attributes.ctid, attributes.pth);
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
    // more reliable, although slower: probe with signal 0 (tgkill)
    return (micthread::thread_kill(attributes.parent, __link_tid(attributes.pid, attributes.pth), 0) == 0 ? true : false);
  }

  auto
  can_join(void) -> int
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;
    return __link_try_join(attributes.pid, &attributes.ctid, attributes.pth) == 0 ? 1 : (int)error::busy;
  }

  auto
  join(void) -> int      // thread
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;      // nothing to join (default/moved-from/already-joined)
    __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    __safe_release();
    return 0;
  }

  auto
  dismiss(void) -> int      // thread
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;
    __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    __release();
    return 0;
  }

  auto
  try_join(void) -> int
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;
    if ( __link_try_join(attributes.pid, &attributes.ctid, attributes.pth) == 0 ) {
      __release();
      return 0;
    }
    return error::busy;
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

  long int
  signal(const signal s)
  {
    if ( alive() ) {
      return micthread::thread_kill(attributes.parent, __link_tid(attributes.pid, attributes.pth), (int)s);
    }
    return -1;
  }

  // throttle alias: park (the old USR1 pseudo-sleep is gone)
  int
  sleep_second(void)
  {
    return sleep();
  }

  // native per-thread suspend: PARK at the next checkpoint; resume with awaken(). no signals.
  int
  sleep(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_parked, memory_order_release);
    return 0;
  }

  int
  awaken(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_run, memory_order_release);
    micron::wake_futex(payload.park.ptr(), 1);
    return 0;
  }

  // cooperative stop-and-reap (kill): DYING at the next checkpoint
  int
  cancel(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_dying, memory_order_release);
    micron::wake_futex(payload.park.ptr(), 1);
    if ( __link_valid(attributes.pid, attributes.pth) ) __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    payload.alive.store(false, memory_order_seq_cst);
    __safe_release();
    return 0;
  }

  // hard stop
  int
  terminate(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_dying, memory_order_release);
    micron::wake_futex(payload.park.ptr(), 1);
    if ( micron::until_timeout(__thread_term_timeout_ms, [this]() noexcept { return !alive(); }) ) {
      if ( __link_valid(attributes.pid, attributes.pth) ) __link_join(attributes.pid, &attributes.ctid, attributes.pth);
      __release();
    } else {
      micthread::thread_kill(attributes.parent, __link_tid(attributes.pid, attributes.pth), (int)signal::kill9);
      __link_join(attributes.pid, &attributes.ctid, attributes.pth);
      __release();
    }
    return 0;
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
