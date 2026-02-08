//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "type_traits.hpp"
#include "types.hpp"

#include "../linux/__includes.hpp"
#include "../linux/sys/__threads.hpp"
#include "../linux/sys/system.hpp"
#include "../memory/stack.hpp"

#include "../atomic/atomic.hpp"
#include "../control.hpp"
#include "../memory/cmemory.hpp"
#include "../memory/mman.hpp"
#include "../pointer.hpp"
#include "../sync/until.hpp"
#include "../sync/yield.hpp"
#include "../tags.hpp"
#include "../tuple.hpp"

#include "signal.hpp"
namespace micron
{

void
__thread_sigchld(int)
{
}
void
__thread_sigthrottle(int)
{
  // configure
  micron::ssleep(1);
}
void
__thread_yield(int)
{
  micron::yield();
}

void
__thread_stop(int)
{
  pthread::__exit_thread();
}

void
__thread_handler()
{
  sigaction_t sa = {};
  sa.sigaction_handler.sa_handler = __thread_sigthrottle;
  micron::sigemptyset(sa.sa_mask);
  sa.sa_flags = sa_restart;
  micron::sigaction(sig_usr1, sa, nullptr);
  // YIELD
  sa.sigaction_handler.sa_handler = __thread_yield;
  micron::sigaction(sig_alrm, sa, nullptr);
  // YIELD

  // TODO: investigate
  sa.sigaction_handler.sa_handler = __thread_stop;
  micron::sigemptyset(sa.sa_mask);
  sa.sa_flags = 0;
  micron::sigaction(sig_term, sa, nullptr);
  //
}

enum thread_returns : i32 { return_success, return_fail };

struct __thread_payload {
  atomic_token<bool> alive;
  atomic_token<u64> ret_val;
};
// The kernel of the thread, encapsulating the req'd function
// status flags if the thread is running or not
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args...>)
i32
__thread_kernel(__thread_payload *payload, Fn fn, Args &&...args)
{
  using ret_t = micron::invoke_result_t<Fn, Args...>;
  __thread_handler();
  payload->alive.store(true, memory_order_seq_cst);
  if constexpr ( micron::is_void_v<ret_t> ) {
    fn(micron::forward<Args>(args)...);
  } else {
    ret_t ret = fn(micron::forward<Args>(args)...);
    payload->ret_val.store(static_cast<u64>(ret), memory_order_seq_cst);
    // nasty std::any workaround
    // auto ret = fn(micron::forward<Args>(args)...);
    // payload->ret_val = reinterpret_cast<void *>(&ret);
    // TODO: think about adding a futex here
  }
  payload->alive.store(false, memory_order_seq_cst);
  return return_success;
}

// preprepared threads, make absolutely sure attrs are configured correctly/are valid before passing to this, no error
// checking built in

// lvalue-only
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn &, Args &...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, Fn &fn, Args &...args)
{
  if ( payload == nullptr )
    throw except::thread_error("micron thread::__as_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn &, Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    throw except::thread_error("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

// rvalue-only
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args && ...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, Fn &&fn, Args &&...args)
{
  if ( payload == nullptr )
    throw except::thread_error("micron thread::__as_unprepared_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn, Args...>, payload, micron::forward<Fn>(fn),
                                         micron::forward<Args>(args)...);

  if ( pid == pthread::thread_failed )
    throw except::thread_error("micron thread::__as_unprepared_thread_attached(): thread failed to spawn");
  return pid;
}

// const-lvalue / by-value
template <typename Fn, typename... Args>
  requires(micron::is_invocable_v<const Fn &, const Args &...>)
pthread_t
__as_unprepared_thread_attached(const pthread_attr_t &attrs, __thread_payload *payload, const Fn &fn,
                                const Args &...args)
{
  if ( payload == nullptr )
    throw except::thread_error("micron thread::__as_unprepared_thread_attached(): invalid arguments");

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<const Fn &, const Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    throw except::thread_error("micron thread::__as_unprepared_thread_attached(): thread failed to spawn");

  return pid;
}

// unprepared threads

// lvalue-only
template <int Stack_Size, typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn &, Args &...>)
pthread_t
__as_thread_attached(__thread_payload *payload, addr_t *fstack, Fn &fn, Args &...args)
{
  if ( payload == nullptr || fstack == nullptr )
    throw except::thread_error("micron thread::__as_thread_attached(): invalid arguments");

  auto attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);

  pthread::set_stack_thread(attrs, fstack, Stack_Size);

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn &, Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    throw except::thread_error("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

// rvalue-only
template <int Stack_Size, typename Fn, typename... Args>
  requires(micron::is_invocable_v<Fn, Args && ...>)
pthread_t
__as_thread_attached(__thread_payload *payload, addr_t *fstack, Fn &&fn, Args &&...args)
{
  if ( payload == nullptr || fstack == nullptr )
    throw except::thread_error("micron thread::__as_thread_attached(): invalid arguments");

  auto attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);

  pthread::set_stack_thread(attrs, fstack, Stack_Size);

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<Fn, Args...>, payload, micron::forward<Fn>(fn),
                                         micron::forward<Args>(args)...);

  if ( pid == pthread::thread_failed )
    throw except::thread_error("micron thread::__as_thread_attached(): thread failed to spawn");
  return pid;
}

// const-lvalue / by-value
template <int Stack_Size, typename Fn, typename... Args>
  requires(micron::is_invocable_v<const Fn &, const Args &...>)
pthread_t
__as_thread_attached(__thread_payload *payload, addr_t *fstack, const Fn &fn, const Args &...args)
{
  if ( payload == nullptr || fstack == nullptr )
    throw except::thread_error("micron thread::__as_thread_attached(): invalid arguments");

  auto attrs = pthread::prepare_thread(pthread::thread_create_state::joinable, posix::sched_other, 0);

  pthread::set_stack_thread(attrs, fstack, Stack_Size);

  pthread_t pid = pthread::create_thread(attrs, __thread_kernel<const Fn &, const Args &...>, payload, fn, args...);

  if ( pid == pthread::thread_failed )
    throw except::thread_error("micron thread::__as_thread_attached(): thread failed to spawn");

  return pid;
}

enum join_status { join_success, join_fail, join_allowed, join_busy };

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
  __impl_preparethread(addr_t *stack_ptr, const pthread_attr_t &attrs, F f, Args &&...args)
  {
    if ( fstack ) {
      throw except::thread_error("micron thread::__impl_preparethread(): a stack is already allocated");
    }
    if ( stack_ptr == nullptr ) {
      throw except::thread_error("micron thread::__impl_preparethread(): null pointer provided");
    }
    fstack = stack_ptr;     // NOTE: LEAVE THIS HERE
    thread_handler();
    pid = __as_unprepared_thread_attached<F, Args...>(attrs, &payload, f, micron::forward<Args &&>(args)...);
  }
  template <typename F, typename... Args>
    requires(micron::is_invocable_v<F, Args...>)
  inline __attribute__((always_inline)) void
  __impl_makethread(F f, Args &&...args)
  {
    if ( fstack ) {
      throw except::thread_error("micron thread::__impl_makethread(): a stack is already allocated");
    }
    fstack = micron::addrmap(Stack_Size);
    if ( micron::mmap_failed(fstack) )
      throw except::thread_error("micron thread::__impl_makethread(): failed to allocate stack");

    thread_handler();
    pid = __as_thread_attached<Stack_Size, F, Args...>(&payload, fstack, f, micron::forward<Args &&>(args)...);
    // no longer needed
    // while ( pthread::thread_kill(parent_pid, pid, 0) != 0 )
    //  cpu_pause();
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
    if ( r = micron::try_unmap(fstack, Stack_Size); r < 0 ) {
      throw except::memory_error("micron thread::__release(): failed to unmap thread stack");
    }
    fstack = nullptr;
    pid = 0;
  }
  void
  __safe_release(void)
  {
    if ( alive() )
      throw except::thread_error("micron thread::__safe_release(): tried to release a running thread");
    if ( micron::try_unmap(fstack, Stack_Size) < 0 ) {
      throw except::memory_error("micron thread::__safe_release(): failed to unmap thread stack");
    }
    fstack = nullptr;
    pid = 0;
  }
  pid_t parent_pid;
  pthread_t pid;
  addr_t *fstack;
  __thread_payload payload;
  // atomic_token<bool> status;

public:
  ~thread() { __release(); }
  thread(const thread &o) = delete;
  thread &operator=(const thread &) = delete;
  thread(void) : parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{} {}
  thread(thread &&o) : parent_pid(o.parent_pid), pid(o.pid), fstack(o.fstack), payload(micron::move(o.payload))
  {
    o.parent_pid = 0;
    o.pid = 0;
    o.fstack = nullptr;
  }
  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn &, Args &...>)
  thread(Fn &fn, Args &...args) : parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{}
  {
    __impl_makethread(fn, args...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args && ...>)
  thread(Fn &&fn, Args &&...args) : parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{}
  {
    __impl_makethread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<const Fn, const Args &...>)
  thread(const Fn &fn, const Args &...args) : parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{}
  {
    __impl_makethread(fn, args...);
  }
  // prepared functions
  // fstack set by preparethread
  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn &, Args &...>)
  thread(addr_t *stack_ptr, const pthread_attr_t &attrs, Fn &fn, Args &...args)
      : parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{}
  {
    __impl_preparethread(stack_ptr, attrs, fn, args...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args && ...>)
  thread(addr_t *stack_ptr, const pthread_attr_t &attrs, Fn &&fn, Args &&...args)
      : parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{}
  {
    __impl_preparethread(stack_ptr, attrs, micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<const Fn, const Args &...>)
  thread(addr_t *stack_ptr, const pthread_attr_t &attrs, const Fn &fn, const Args &...args)
      : parent_pid(micron::posix::getpid()), pid(0), fstack(nullptr), payload{}
  {
    __impl_preparethread(stack_ptr, attrs, fn, args...);
  }
  thread &
  operator=(thread &&o)
  {
    parent_pid = o.parent_pid;
    pid = o.pid;
    fstack = o.fstack;
    payload = micron::move(o.payload);
    o.parent_pid = 0;
    o.pid = 0;
    o.fstack = nullptr;
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
    if ( r == error::busy or r == error::invalid_arg )
      return r;
    return 0;
  }
  auto
  join(void) -> int     // thread
  {
    if ( try_join() == error::busy ) {
      auto r = pthread::__join_thread(pid);
      __safe_release();
      return r;
    }
    __safe_release();
    return 1;
  }
  auto
  try_join(void) -> int
  {
    int r = pthread::__try_join_thread(pid);
    if ( r == 0 ) {
      __safe_release();
      return 1;
    }
    if ( r == error::busy or r == error::invalid_arg )
      return r;
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
  long int
  signal(const signals s)
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
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signals::usr1);
    }
    return -1;
  }
  int
  sleep(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signals::stop);
    }
    return -1;
  }
  int
  awaken(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signals::cont);
    }
    return -1;
  }
  void
  wait_for(void) const
  {
    //until(true, &thread<Stack_Size>::active, this);
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
    return fstack;
  }

  constexpr size_t
  stack_size() const
  {
    return Stack_Size;
  }
};

// auto_thread
// unlike a regular thread, this one auto cleans up after itself (similar to a std::jthread)
//
// WARNING: this thread automatically allocates it's stack on the parent process stack, the normal stack limit on linux
// is 8MB, while auto_thread_stack_size is 262kB, meaning spawning ~32 threads WILL segfault
// the advantage is that this is MUCH faster (no mmap)
template <size_t Stack_Size = auto_thread_stack_size> class auto_thread
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
    thread_handler();
    pid = __as_thread_attached<Stack_Size, F, Args...>(&payload, micron::real_addr(fstack), f,
                                                       micron::forward<Args &&>(args)...);

    // no longer needed
    // while ( pthread::thread_kill(parent_pid, pid, 0) != 0 )
    //  cpu_pause();
    // massively important, wait for the thread to actually be
    // created by the scheduler, otherwise race conditions will occur

    // specifically, if we don't have for the actual process to be created we can get into a race condition where the
    // thread doesn't exist yet (it takes a few cycles for the scheduler to actually insert the thread into the process
    // table)
  }
  void
  __release(void)
  {
    micron::czero<Stack_Size>(fstack);
    pid = 0;
  }
  void
  __safe_release(void)
  {
    if ( alive() )
      throw except::thread_error("micron thread::__safe_release(): tried to release a running thread");
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
  alignas(16) byte fstack[Stack_Size];     // must be 16 byte aligned, stack will be stack allocated and survive for the
                                           // duration of this class
  __thread_payload payload;

public:
  ~auto_thread() { __join(); }
  auto_thread(const auto_thread &o) = delete;
  auto_thread(void) = delete;
  auto_thread &operator=(const auto_thread &) = delete;
  auto_thread(auto_thread &&o) : parent_pid(o.parent_pid), pid(o.pid), payload(micron::move(o.payload))
  {
    o.parent_pid = 0;
    o.pid = 0;
    micron::cmemcpy<Stack_Size>(fstack, o.fstack);
    micron::czero<Stack_Size>(o.fstack);
  }
  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn &, Args &...>)
  auto_thread(Fn &fn, Args &...args) : parent_pid(micron::posix::getpid()), pid(0), payload{}
  {
    micron::czero<Stack_Size>(fstack);
    __impl_makethread(fn, args...);
  }

  // rvalue-only
  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<Fn, Args && ...>)
  auto_thread(Fn &&fn, Args &&...args) : parent_pid(micron::posix::getpid()), pid(0), payload{}
  {
    micron::czero<Stack_Size>(fstack);
    __impl_makethread(micron::forward<Fn>(fn), micron::forward<Args>(args)...);
  }

  // by-value / const-lvalue
  template <typename Fn, typename... Args>
    requires(micron::is_invocable_v<const Fn &, const Args &...>)
  auto_thread(const Fn &fn, const Args &...args) : parent_pid(micron::posix::getpid()), pid(0), payload{}
  {
    micron::czero<Stack_Size>(fstack);
    __impl_makethread(fn, args...);
  }
  auto_thread &
  operator=(auto_thread &&o)
  {
    parent_pid = o.parent_pid;
    pid = o.pid;
    micron::cmemcpy<Stack_Size>(fstack, o.fstack);
    micron::czero<Stack_Size>(o.fstack);
    payload = micron::move(o.payload);
    o.parent_pid = 0;
    o.pid = 0;
    return *this;
  }
  template <typename F, typename... Args>
    requires(micron::is_invocable_v<F, Args...>)
  auto_thread &
  operator[](F f, Args &&...args)
  {
    join();
    __impl_makethread(f, args...);
    return *this;
  }
  void swap(auto_thread &o) = delete;
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
    if ( r == error::busy or r == error::invalid_arg )
      return r;
    return 0;
  }
  auto
  join(void) -> int     // auto_thread
  {
    if ( try_join() == error::busy ) {
      auto r = pthread::__join_thread(pid);
      __safe_release();
      return r;
    }
    __safe_release();
    return 1;
  }
  auto
  try_join(void) -> int
  {
    int r = pthread::__try_join_thread(pid);
    if ( r == 0 ) {
      __safe_release();
      return 1;
    }
    if ( r == error::busy or r == error::invalid_arg )
      return r;
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
  signal(const signals s)
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
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signals::usr1);
    }
    return -1;
  }
  int
  sleep(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signals::stop);
    }
    return -1;
  }
  int
  awaken(void)
  {
    if ( alive() ) {
      return pthread::thread_kill(parent_pid, pthread::get_thread_id(pid), (int)signals::cont);
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
    //until(true, &auto_thread<Stack_Size>::active, this);
    until(false, &auto_thread<Stack_Size>::alive, this);
  }
  template <typename R>
    requires(micron::is_convertible_v<R, u64>)
  R
  result(void)
  {
    wait_for();
    R val = static_cast<R>(payload.ret_val.get());
    return val;
  }
  constexpr size_t
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
};
