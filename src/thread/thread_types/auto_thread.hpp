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
// auto_thread
// unlike a regular thread, this one auto cleans up after itself (similar to a std::jthread)
//
// WARNING: this thread automatically allocates it's stack on the parent process stack, the normal stack limit on linux
// is 8MB, while auto_thread_stack_size is 262kB, meaning spawning ~32 threads WILL segfault
// the advantage is that this is MUCH faster (no mmap)
template<usize Stack_Size = auto_thread_stack_size> class auto_thread
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
    thread_handler();
    parent_pid = micron::posix::getpid();
    // WARNING: alive must be set to true before invoking the kernel, else we risk a race (rare but happens)
    payload.alive.store(true, micron::memory_order_seq_cst);
    __link_launch<Stack_Size, &__thread_kernel<micron::decay_t<F>, micron::decay_t<Args>...>>(
        pid, __ctid, __tls, __pth, &payload, micron::real_addr(fstack), f, micron::forward<Args>(args)...);
  }

  void
  __release(void)
  {
    // WARNING: HEISENBUG; since auto_thread is the only class which stack allocs its own stack (on the parent stack)
    // if it happened to be page-aligned, guard_stack() mprotect'd its first page to PROT_NONE as a low guard
    // and ___NOTHING___ restored it. czero'ing the whole array wrote through that guard page -> SIGSEGV (~1/page-size of placements),
    // and even ____WITHOUT____ the czero the leftover PROT_NONE page would fault later when this object's storage is reused
    {
      constexpr usize guard = __micron_page_size_default;
      if constexpr ( Stack_Size > guard ) {
        if ( (reinterpret_cast<uintptr_t>(fstack) & (guard - 1)) == 0 )
          micron::mprotect(reinterpret_cast<addr_t *>(fstack), guard, micron::prot_read | micron::prot_write);
      }
    }
    micron::czero<Stack_Size>(fstack);
    // free this thread's from-scratch TLS block (the spawner owns it; safe now the thread is reaped)
    micron::__tls_release_frame(__tls);
    __tls = micron::__tls_frame{};
    __ctid = 0;
    __pth = 0;
    // NOTE: do NOT zero parent_pid here; tt is the tgid used to signal the thread and must stay valid for
    // the object's whole life
    pid = 0;
    payload.alive.store(false, micron::memory_order_seq_cst);
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
    if ( alive() ) micron::exc<except::thread_error>("micron thread::__safe_release(): tried to release a running thread");
    __release();
  }

  auto
  __join(void) -> int
  {
    join();
    return 0;
  }

  pid_t parent_pid;
  pid_t pid;                                // kernel tid (freestanding; 0 = none/joined)
  int __ctid = 0;                           // CHILD_CLEARTID join futex word (freestanding; address stable for the object's life)
  micron::__tls_frame __tls{};              // per-thread TLS block (freestanding; freed on release)
  unsigned long __pth = 0;                  // pthread_t handle (hosted backend)
  alignas(16) byte fstack[Stack_Size];      // must be 16 byte aligned, stack will be stack allocated and survive for the
                                            // duration of this class
  __thread_payload payload;

public:
  ~auto_thread()
  {
    // join iff a thread is actually live; guard on pid AND alive
    if ( !__link_valid(pid, __pth) && !payload.alive.get(micron::memory_order_acquire) ) return;
    __join();
  }

  auto_thread(const auto_thread &o) = delete;
  auto_thread(void) = delete;
  auto_thread &operator=(const auto_thread &) = delete;
  // can't move, stack is local
  auto_thread(auto_thread &&) = delete;
  auto_thread &operator=(auto_thread &&) = delete;

  template<typename Fn, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<Fn>, micron::decay_t<Args> &...>)
  auto_thread(Fn &&fn, Args &&...args) : parent_pid(micron::posix::getpid()), pid(0), payload{}
  {
    micron::czero<Stack_Size>(fstack);
    __impl_makethread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  template<typename F, typename... Args>
    requires(micron::is_invocable_v<micron::decay_t<F>, micron::decay_t<Args> &...>)
  auto_thread &
  operator[](F f, Args &&...args)
  {
    join();
    __impl_makethread(f, micron::forward<Args>(args)...);
    return *this;
  }

  void swap(auto_thread &o) = delete;

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
    // more reliable, although slower: probe the thread with signal 0 (tgkill)
    return (micthread::thread_kill(parent_pid, __link_tid(pid, __pth), 0) == 0 ? true : false);
  }

  auto
  can_join(void) -> int
  {
    if ( !__link_valid(pid, __pth) ) return 0;
    return __link_try_join(pid, &__ctid, __pth) == 0 ? 1 : (int)error::busy;
  }

  auto
  dismiss(void) -> int      // auto_thread
  {
    if ( !__link_valid(pid, __pth) ) return 1;
    __link_join(pid, &__ctid, __pth);      // futex-wait on the CHILD_CLEARTID word
    __release();
    return 0;
  }

  auto
  join(void) -> int      // auto_thread
  {
    if ( !__link_valid(pid, __pth) ) return 1;
    __link_join(pid, &__ctid, __pth);
    __safe_release();
    return 0;
  }

  auto
  try_join(void) -> int
  {
    if ( !__link_valid(pid, __pth) ) return 1;      // already joined/released, idempotent
    if ( __link_try_join(pid, &__ctid, __pth) == 0 ) {
      __safe_release();
      return 1;
    }
    return error::busy;
  }

  auto
  thread_id(void) const
  {
    return __link_tid(pid, __pth);
  }

  auto
  native_handle(void) const
  {
    return __link_tid(pid, __pth);
  }

  int
  signal(const micron::signal s)
  {
    if ( alive() ) {
      return micthread::thread_kill(parent_pid, __link_tid(pid, __pth), (int)s);
    }
    return -1;
  }

  // throttle alias: park (the old USR1 pseudo-sleep is gone)
  int
  sleep_second(void)
  {
    return sleep();
  }

  // native per-thread suspend: set the control word PARKED; the target blocks at its next checkpoint
  int
  sleep(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_parked, micron::memory_order_release);
    return 0;
  }

  int
  awaken(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_run, micron::memory_order_release);
    micron::wake_futex(payload.park.ptr(), 1);      // wake the parked checkpoint (private futex)
    return 0;
  }

  int
  cancel(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_dying, micron::memory_order_release);
    micron::wake_futex(payload.park.ptr(), 1);
    __link_join(pid, &__ctid, __pth);
    payload.alive.store(false, micron::memory_order_seq_cst);
    __safe_release();
    return 0;
  }

  int
  terminate(void)
  {
    if ( !alive() ) return -1;
    payload.park.store(__park_dying, micron::memory_order_release);
    micron::wake_futex(payload.park.ptr(), 1);
    if ( micron::until_timeout(__thread_term_timeout_ms, [this]() noexcept { return !alive(); }) ) {
      __link_join(pid, &__ctid, __pth);      // cooperative exit observed; reap
      __release();
    } else {
      // runaway thread missed the checkpoint within budget: last-resort hard kill
      micthread::thread_kill(parent_pid, __link_tid(pid, __pth), (int)signal::kill9);
      __link_join(pid, &__ctid, __pth);
      __release();
    }
    return 0;
  }

  byte *
  stack()
  {
    return &fstack[0];
  }

  void
  wait_for(void) const
  {
    // until(true, &auto_thread<Stack_Size>::active, this);
    until(false, &auto_thread<Stack_Size>::alive, this);
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

  constexpr usize
  stack_size() const
  {
    return Stack_Size;
  }

  void
  cleanup()
  {
    __join();
  }
};
};      // namespace micron
