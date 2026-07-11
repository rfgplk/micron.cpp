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
  __impl_preparethread(F f, Args &&...args)
  {
    if ( attributes.stack_addr == nullptr ) {
      micron::exc<except::thread_error>("micron thread::__impl_preparethread(): a stack isn't allocated");
    }
    thread_handler();
    payload.alive.store(true, memory_order_seq_cst);
    payload.started.store(false, memory_order_seq_cst);
    __link_launch<Stack_Size, &__thread_kernel<micron::decay_t<F>, micron::decay_t<Args>...>>(
        attributes.pid, attributes.ctid, attributes.tls, attributes.pth, &payload, attributes.stack_addr, f,
        micron::forward<Args>(args)...);

    // bounded start handshake
    if ( !micron::until_timeout(true, __thread_start_timeout_ms, [this]() noexcept {
           const bool s = payload.started.get(memory_order_acquire);
           if ( !s ) micron::yield();
           return s;
         }) ) {
      // WARNING: child was created (clone3 succeeded) but never published in time: scheduler-starved
      // or wedged; NEVER throw and unwind out from under it, the live child WILL write &payload
      // and the kernel still clears &attributes.ctid, so reclaiming this object's
      // storage would be a use-after-free / cross-object corruption
      payload.park.store(__park_dying, memory_order_release);
      micron::wake_futex(payload.park.ptr(), 1);
      if ( __link_valid(attributes.pid, attributes.pth) ) __link_join(attributes.pid, &attributes.ctid, attributes.pth);
      __release();
      micron::exc<except::thread_error>(
          "micron thread::__impl_preparethread(): spawned thread never reached its kernel (faulted on entry?)");
    }
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

  thread_attr_t attributes;

public:
  __thread_payload payload;

  ~group_thread()
  {
    if ( __link_valid(attributes.pid, attributes.pth) ) __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    __release();
  }

  group_thread(const group_thread &o) = delete;
  group_thread &operator=(const group_thread &) = delete;

  group_thread(void) : attributes(posix::getpid()), payload{} { }

  // moving an already-spawned thread is UB (kernel holds &payload/&ctid); guard rejects a live source
  group_thread(group_thread &&o)
      : attributes((micron::__thread_assert_movable_src(o.attributes.pid, o.attributes.pth), micron::move(o.attributes))),
        payload(micron::move(o.payload))
  {
  }

  // prepared functions
  template<typename Fn, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
  group_thread(const thread_attr_t &_attrs, Fn &&fn, Args &&...args) : attributes(posix::getpid()), payload{}
  {
    // adopt the caller-allocated stack + scheduling intent (group owns the stack now)
    attributes.stack_addr = _attrs.stack_addr;
    attributes.stack_size = _attrs.stack_size;
    attributes.sched_policy = _attrs.sched_policy;
    attributes.sched_priority = _attrs.sched_priority;
    __impl_preparethread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  group_thread &
  operator=(group_thread &&o)
  {
    if ( this == &o ) return *this;
    micron::__thread_assert_movable_src(o.attributes.pid, o.attributes.pth);      // o must be pre-spawn/joined
    // release *this's current thread+stack BEFORE adopting o; otherwise the running thread is orphaned
    if ( __link_valid(attributes.pid, attributes.pth) ) __link_join(attributes.pid, &attributes.ctid, attributes.pth);
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
  join(void) -> int      // group_thread
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;      // nothing to join (default/moved-from/already-joined)
    auto r = __link_join(attributes.pid, &attributes.ctid, attributes.pth);
    __safe_release();
    return r;
  }

  auto
  dismiss(void) -> int      // group_thread
  {
    int r = try_join();
    if ( r == error::busy ) {
      auto rr = __link_join(attributes.pid, &attributes.ctid, attributes.pth);
      __release();
      return rr;
    }
    return r;
  }

  auto
  try_join(void) -> int
  {
    if ( !__link_valid(attributes.pid, attributes.pth) ) return 0;      // already joined
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

  long int
  signal(const micron::signal s)
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
      micron::cbytecpy<sizeof(R)>(reinterpret_cast<byte *>(&val), reinterpret_cast<const byte *>(&raw));
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
