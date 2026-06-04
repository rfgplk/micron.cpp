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
    pid = __as_thread_attached<Stack_Size>(&payload, micron::real_addr(fstack), f, micron::forward<Args>(args)...);
  }

  void
  __release(void)
  {
    micron::czero<Stack_Size>(fstack);
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
  pthread_t pid;
  alignas(16) byte fstack[Stack_Size];      // must be 16 byte aligned, stack will be stack allocated and survive for the
                                            // duration of this class
  __thread_payload payload;

public:
  ~auto_thread()
  {
    // join iff a thread is actually live; guard on pid AND alive
    if ( pid == 0 && !payload.alive.get(micron::memory_order_acquire) ) return;
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
    // more reliable, although slower
    return (pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), 0) == 0 ? true : false);
  }

  auto
  can_join(void) -> int
  {
    int r = pthread::__try_join_thread(pid);
    if ( r == 0 ) {
      return 1;
    }
    if ( r == error::busy or r == error::invalid_arg ) return r;
    return 0;
  }

  auto
  dismiss(void) -> int      // auto_thread
  {
    int r = try_join();
    if ( r == error::busy ) {
      auto rr = pthread::__join_thread(pid);
      __release();
      return rr;
    }
    return r;
  }

  auto
  join(void) -> int      // auto_thread
  {
    int r = try_join();
    if ( r == error::busy ) {
      auto rr = pthread::__join_thread(pid);
      __safe_release();
      return rr;
    }
    return r;
  }

  auto
  try_join(void) -> int
  {
    if ( pid == 0 ) return 1;      // already joined/released, idempotent
    int r = pthread::__try_join_thread(pid);
    if ( r == 0 ) {
      __safe_release();
      return 1;
    }
    if ( r == error::busy or r == error::invalid_arg ) return r;
    return error::busy;
  }

  auto
  thread_id(void) const
  {
    return pthread::get_thread_id(pid);
  }

  auto
  native_handle(void) const
  {
    return pid;
  }

  int
  signal(const micron::signal s)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)s);
    }
    return -1;
  }

  // pseudo sleep
  int
  sleep_second(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signal::usr1);
    }
    return -1;
  }

  int
  sleep(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signal::stop);
    }
    return -1;
  }

  int
  awaken(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signal::cont);
    }
    return -1;
  }

  int
  cancel(void)
  {
    if ( alive() ) {
      int r = pthread::cancel_thread(pid);
      signal(signal::usr2);
      pthread::__join_thread(pid);
      payload.alive.store(false, micron::memory_order_seq_cst);
      __safe_release();
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
      __builtin_memcpy(&val, &raw, sizeof(R));
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
