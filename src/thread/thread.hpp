//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../control.hpp"
#include "../memory/cmemory.hpp"
#include "../memory/mman.h"
#include "../pointer.hpp"
#include "../sync/yield.hpp"
#include "../tags.hpp"
#include "../tuple.hpp"
#include "process.hpp"
#include "signal.hpp"
#include "types.hpp"

#include "../memory/stack.hpp"
#include "../linux/calls.hpp"

#include <signal.h>        // for signal handling
//#include <sys/types.h>     // for waitpid
//#include <sys/wait.h>      // for wait
#include <tuple>
#include <utility>     // needed for now
#include <variant>

namespace micron
{

void
__thread_sigchld(int)
{
}
void
__thread_sigsleep(int)
{
  micron::ssleep(1);
}
void
__thread_yield(int)
{
  micron::yield();
}
void
__thread_handler()
{
  struct sigaction sa = {};
  sa.sa_handler = __thread_sigsleep;
  ::sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  ::sigaction(SIG_USR1, &sa, nullptr);
  sa.sa_handler = __thread_yield;
  ::sigaction(SIG_ALRM, &sa, nullptr);
}

template <typename T> struct get_return_type;

template <typename R, typename... Args> struct get_return_type<R(Args...)> {
  using type = R;
};

// why is this like this :(
template <typename Func, typename... Args> struct _clone_args {
  Func func;
  std::tuple<Args...> args;
};

template <typename Func, typename... Args, size_t... Is>
  requires(std::is_invocable_v<Func, Args...>)
auto
invoke_function(Func func, std::tuple<Args...> &args_tuple, std::index_sequence<Is...>)
{
  return func(std::get<Is>(args_tuple)...);
}

template <typename Func, typename... Args>
  requires(std::is_invocable_v<Func, Args...>)
int
_thread_kernel(void *cargs)     //-> typename get_return_type<Func>::type
{
  __thread_handler();
  auto *args = reinterpret_cast<_clone_args<Func, Args...> *>(cargs);
  invoke_function(args->func, args->args, std::index_sequence_for<Args...>{});
  return 0;
}
// index_sequence isn't implemented fully in micron so we have to use the STL

template <int Stack, typename F, typename... Args>
  requires(std::is_invocable_v<F, Args...>)
int
as_thread_read_only(byte *fstack, F f, Args... args)
{
  _clone_args<F, Args...> clone_args{ f, std::forward_as_tuple(micron::forward<Args>(args)...) };
  int pid = ::clone(_thread_kernel<F, Args...>, fstack + Stack, SIGCHLD, &clone_args);

  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  return pid;
}

template <int Stack = default_stack_size, typename F, typename... Args>
  requires(std::is_invocable_v<F, Args...>)
int
as_thread_attached(byte *fstack, F f, Args &&...args)
{
  _clone_args<F, Args...> clone_args{ f, std::forward_as_tuple(micron::forward<Args>(args)...) };
  //_clone_args<F, Args...> clone_args{ f, std::make_tuple(micron::forward<Args>(args)...) };
  int pid = ::clone(_thread_kernel<F, Args...>, reinterpret_cast<char *>(fstack) + Stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | SIGCHLD, &clone_args);

  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  return pid;
}

template <int Stack = default_stack_size, typename F, typename... Args>
  requires(std::is_invocable_v<F, Args...>)
int
as_thread_samegroup(byte *fstack, F f, Args &&...args)
{
  _clone_args<F, Args...> clone_args{ f, std::forward_as_tuple(micron::forward<Args>(args)...) };
  int pid = ::clone(_thread_kernel<F, Args...>, reinterpret_cast<char *>(fstack) + Stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | SIGCHLD, &clone_args);

  if ( pid == -1 )
    throw except::system_error("micron process failed to fork()");
  return pid;
}

// thread, a regular thread
// STL compatilibity
template <int Stack = default_stack_size> class thread
{
  using thread_type = thread_tag;
  void
  thread_handler()
  {
    struct sigaction sa;
    sa.sa_handler = __thread_sigchld;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ::sigaction(SIGCHLD, &sa, nullptr);
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  inline __attribute__((always_inline)) void
  __impl_makethread(F f, Args &&...args)
  {
    fstack = reinterpret_cast<byte *>(
        micron::mmap(NULL, Stack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
    if ( fstack == MAP_FAILED )
      throw except::system_error("micron::thread mmap failed to allocate stack");

    thread_handler();
    pid = as_thread_attached(fstack, f, micron::forward<Args>(args)...);
  }
  pid_t pid;
  byte *fstack;

public:
  ~thread() { release(); }     // try_join();
  thread(void) : pid(0), fstack(nullptr) {}
  thread(thread &&o) : pid(o.pid), fstack(o.fstack)
  {
    o.pid = 0;
    o.fstack = nullptr;
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  thread(F f, Args &&...args)
  {
    __impl_makethread(micron::forward<F>(f), micron::forward<Args>(args)...);
  }
  thread(const thread &o) = delete;
  thread &operator=(const thread &) = delete;
  thread &
  operator=(thread &&o)
  {
    pid = o.pid;
    fstack = o.fstack;
    o.pid = 0;
    o.fstack = nullptr;
    return *this;
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  thread &
  operator[](F f, Args &&...args)
  {
    try_join();
    __impl_makethread(f, args...);
    return *this;
  }
  auto
  swap(thread &o)
  {
    auto tmp1 = pid;
    auto *tmp2 = fstack;
    pid = o.pid;
    fstack = o.fstack;
    o.pid = tmp1;
    o.fstack = tmp2;
    return *this;
  }
  void
  release(void)
  {
    if ( fstack )
      micron::munmap(fstack, Stack);
    pid = 0;
  }
  inline bool
  active(void) const
  {
    return !pid;
  }
  auto
  join(void) -> int     // thread
  {
    auto r = wait_thread(pid);
    release();
    return r;
  }
  auto
  try_join(void) -> int
  {
    if ( try_wait_thread(pid) == 0 or errno == EAGAIN )
      return -1;
    wait_thread(pid);
    pid = 0;
    return 0;
  }
  auto
  thread_id(void) const
  {
    return pid;
  }
  auto
  native_handle(void) const
  {
    return pid;
  }
  void
  signal(const signals s)
  {
    if ( pid )
      posix::kill(pid, (int)s);
  }
  // pseudo sleep
  void
  sleep_second(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::usr1);
    }
  }
  void
  sleep(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::stop);
    }
  }
  void
  awaken(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::cont);
    }
  }
};

// group thread (or a regular pthread)
template <int Stack = default_stack_size> class group_thread
{
  using thread_type = thread_tag;
  void
  thread_handler()
  {
    struct sigaction sa;
    sa.sa_handler = __thread_sigchld;
    ::sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ::sigaction(SIGCHLD, &sa, nullptr);
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  inline __attribute__((always_inline)) void
  __impl_makethread(F f, Args &&...args)
  {
    fstack = reinterpret_cast<byte *>(
        micron::mmap(NULL, Stack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));
    if ( fstack == MAP_FAILED )
      throw except::system_error("micron::thread mmap failed to allocate stack");

    thread_handler();
    pid = as_thread_samegroup(fstack, f, micron::forward<Args>(args)...);
  }
  pid_t pid;
  byte *fstack;

public:
  ~group_thread() { release(); }     // try_join();
  group_thread(void) : pid(0), fstack(nullptr) {}
  group_thread(group_thread &&o) : pid(o.pid), fstack(o.fstack)
  {
    o.pid = 0;
    o.fstack = nullptr;
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  group_thread(F f, Args &&...args)
  {
    __impl_makethread(micron::forward<F>(f), micron::forward<Args>(args)...);
  }
  group_thread(const group_thread &o) = delete;
  group_thread &operator=(const group_thread &) = delete;
  group_thread &
  operator=(group_thread &&o)
  {
    pid = o.pid;
    fstack = o.fstack;
    o.pid = 0;
    o.fstack = nullptr;
    return *this;
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  group_thread &
  operator[](F f, Args &&...args)
  {
    try_join();
    __impl_makethread(f, args...);
    return *this;
  }
  auto
  swap(group_thread &o)
  {
    auto tmp1 = pid;
    auto *tmp2 = fstack;
    pid = o.pid;
    fstack = o.fstack;
    o.pid = tmp1;
    o.fstack = tmp2;
    return *this;
  }
  void
  release(void)
  {
    if ( fstack )
      micron::munmap(fstack, Stack);
    pid = 0;
  }
  inline bool
  active(void) const
  {
    return !pid;
  }
  auto
  join(void) -> int     // thread
  {
    auto r = wait_thread(pid);
    release();
    return r;
  }
  auto
  try_join(void) -> int
  {
    if ( try_wait_thread(pid) == 0 or errno == EAGAIN )
      return -1;
    wait_thread(pid);
    pid = 0;
    return 0;
  }
  auto
  thread_id(void) const
  {
    return pid;
  }
  auto
  native_handle(void) const
  {
    return pid;
  }
  void
  signal(const signals s)
  {
    if ( pid )
      posix::kill(pid, (int)s);
  }
  // pseudo sleep
  void
  sleep_second(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::usr1);
    }
  }
  void
  sleep(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::stop);
    }
  }
  void
  yield(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::alarm);
    }
  }
  void
  awaken(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::cont);
    }
  }
};

// auto_thread
// unlike a regular thread, this one auto cleans up after itself
template <int Stack = auto_thread_stack_size> class auto_thread
{
  using thread_type = thread_tag;

  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  inline __attribute__((always_inline)) void
  __impl_makethread(F f, Args &&...args)
  {
    pid = as_thread_attached<Stack, F, Args...>(fstack, f, micron::forward<Args>(args)...);
    // pid = (pid_t)as_thread<Stack, F, Args...>(fstack, f, micron::forward<Args>(args)...);
  }
  auto
  __join(void) -> int
  {
    wait_thread(pid);
    pid = 0;
    return 0;
  }

  alignas(64) byte fstack[Stack];     // stack will be stack allocated and survive for the duration of this class (which
                                      // is globally allocated to the front of the heap - see later)
  pid_t pid;

public:
  ~auto_thread() { __join(); }
  auto_thread(void) = delete;
  auto_thread(auto_thread &&o) : pid(o.pid)
  {
    o.pid = 0;
    micron::cmemcpy<Stack>(fstack, o.fstack);
    micron::czero<Stack>(o.fstack);
  }
  
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  auto_thread(F f, Args &&...args) : pid(0)
  {
    micron::czero<Stack>(fstack);

    __impl_makethread(micron::forward<F>(f), micron::forward<Args>(args)...);
    while ( posix::kill(pid, 0) != 0 )     // massively important, wait for the thread to actually be
                                           // created by the scheduler, otherwise race conditions will occur
      __builtin_ia32_pause();
    // specifically, if we don't have for the actual process to be created we can get into a race condition where the
    // thread doesn't exist yet (it takes a few cycles for the scheduler to actually insert the thread into the process
    // table)
  }
  auto_thread(const auto_thread &o) = delete;
  auto_thread &operator=(const auto_thread &) = delete;
  auto_thread &
  operator=(auto_thread &&o)
  {
    pid = o.pid;
    micron::cmemcpy<Stack>(fstack, o.fstack);
    o.pid = 0;
    micron::czero<Stack>(o.fstack);
    return *this;
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  auto_thread &
  operator[](F f, Args &&...args)
  {
    try_join();
    __impl_makethread(f, args...);
    return *this;
  }
  auto
  swap(auto_thread &o)
  {
    auto tmp1 = pid;
    pid = o.pid;
    o.pid = tmp1;

    byte tmp2[Stack] = {};
    micron::cmemcpy<Stack>(tmp2, fstack);
    micron::cmemcpy<Stack>(fstack, o.fstack);
    micron::cmemcpy<Stack>(o.fstack, tmp2);
    return *this;
  }
  inline bool
  active(void) const
  {
    return !pid;
  }
  auto
  try_join(void) -> int
  {
    if ( try_wait_thread(pid) == 0 or errno == EAGAIN )
      return -1;
    wait_thread(pid);
    pid = 0;
    return 0;
  }
  auto
  thread_id(void) const
  {
    return pid;
  }
  auto
  native_handle(void) const
  {
    return pid;
  }
  void
  signal(const signals s)
  {
    if ( pid )
      posix::kill(pid, (int)s);
  }
  // pseudo sleep
  void
  sleep_second(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::usr1);
    }
  }
  void
  sleep(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::stop);
    }
  }
  void
  awaken(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::cont);
    }
  }
  void
  yield(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::alarm);
    }
  }
  // equivalent to __join()
  void
  cleanup()
  {
    __join();
  }
};

// read_thread
// a read only thread. self explanatory
template <int Stack = auto_thread_stack_size> class read_thread
{
  using thread_type = thread_tag;

  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  inline __attribute__((always_inline)) void
  __impl_makethread(F f, Args &&...args)
  {
    pid = as_thread_read_only<Stack, F, Args...>(
        fstack, f,
        micron::forward<Args>(args)...);     // NOTE: auto_thread will be detached
  }
  auto
  __join(void) -> int
  {
    wait_thread(pid);
    pid = 0;
    return 0;
  }

  byte fstack[Stack];     // stack will be stack allocated and survive for the duration of this class (which is globally
                          // allocated to the front of the heap - see later)
  pid_t pid;

public:
  ~read_thread() { __join(); }
  read_thread(void) = delete;
  read_thread(read_thread &&o) : pid(o.pid)
  {
    o.pid = 0;
    micron::cmemcpy<Stack>(fstack, o.fstack);
    micron::czero<Stack>(o.fstack);
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  read_thread(F f, Args &&...args) : pid(0)
  {
    micron::czero<Stack>(fstack);
    __impl_makethread(micron::forward<F>(f), micron::forward<Args>(args)...);
    while ( posix::kill(pid, 0) != 0 )     // massively important, wait for the thread to actually be
                                           // created by the scheduler, otherwise race conditions will occur
      __builtin_ia32_pause();
    // specifically, if we don't have for the actual process to be created we can get into a race condition where the
    // thread doesn't exist yet (it takes a few cycles for the scheduler to actually insert the thread into the process
    // table)
  }
  read_thread(const read_thread &o) = delete;
  read_thread &operator=(const read_thread &) = delete;
  read_thread &
  operator=(read_thread &&o)
  {
    pid = o.pid;
    micron::cmemcpy<Stack>(fstack, o.fstack);
    o.pid = 0;
    micron::czero<Stack>(o.fstack);
    return *this;
  }
  template <typename F, typename... Args>
    requires(std::is_invocable_v<F, Args...>)
  read_thread &
  operator[](F f, Args &&...args)
  {
    try_join();
    __impl_makethread(f, args...);
    return *this;
  }
  auto
  swap(read_thread &o)
  {
    auto tmp1 = pid;
    pid = o.pid;
    o.pid = tmp1;

    byte tmp2[Stack] = {};
    micron::cmemcpy<Stack>(tmp2, fstack);
    micron::cmemcpy<Stack>(fstack, o.fstack);
    micron::cmemcpy<Stack>(o.fstack, tmp2);
    return *this;
  }
  inline bool
  active(void) const
  {
    return !pid;
  }
  auto
  try_join(void) -> int
  {
    if ( try_wait_thread(pid) == 0 or errno == EAGAIN )
      return -1;
    wait_thread(pid);
    pid = 0;
    return 0;
  }
  auto
  thread_id(void) const
  {
    return pid;
  }
  auto
  native_handle(void) const
  {
    return pid;
  }
  void
  signal(const signals s)
  {
    if ( pid )
      posix::kill(pid, (int)s);
  }
  // pseudo sleep
  void
  sleep_second(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::usr1);
    }
  }
  void
  yield(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::alarm);
    }
  }
  void
  sleep(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::stop);
    }
  }
  void
  awaken(void)
  {
    if ( pid ) {
      posix::kill(pid, (int)signals::cont);
    }
  }
  // equivalent to __join()
  void
  cleanup()
  {
    __join();
  }
};

// TODO: add more functions
// fork == creates new process (removed)
// child
// join == joins with main thread
// signal == send signal to main thread
// accept == receive signal from thread
// spawn == create a thread
// yield == give up run access as in pthread_yield
// kill == stop thread (if called from itself, stops the thread, otherwise stop indicated thread)
// sleep == pause execution
// park == park thread on a core or core mask
// wait == wait until signal or value is set
// await == await a signal or value

template <typename Tr> using __thread_pointer = micron::weak_pointer<Tr>;

// NOTE: solo as in these are free threads unattached to any arena
namespace solo
{

// WARNING: SUMMONING CTHULHU
// NOTE: it's _ABSOLUTELY_ important you leave the template constraints in place to have proper binding (will cause chaos
// otherwise)
// NOTE: COMPILER_BUG?! "auto in template"



template <typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(std::is_invocable_v<Func, Args...> && sizeof...(Args) == 0
           && ((std::is_lvalue_reference_v<Args> && ...) or (std::is_rvalue_reference_v<Args> && ...))
           && (!std::is_same_v<std::decay_t<Args>, Args> && ...))// && (std::is_same_v<std::remove_reference_t<Args>, Args> && ...))
auto
spawn(Func f, Args &&...args) -> __thread_pointer<Tr>
{
  return micron::weak_pointer<Tr>(f, args...);
}

template <typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(std::is_invocable_v<Func, Args...> && sizeof...(Args) > 0
           && ((std::is_lvalue_reference_v<Args> && ...) or (std::is_rvalue_reference_v<Args> && ...))
           && (!std::is_same_v<std::decay_t<Args>, Args> && ...))// && (std::is_same_v<std::remove_reference_t<Args>, Args> && ...))
// requires(std::is_invocable_v<Func, Args...>)
auto
spawn(Func f, Args &&...args) -> __thread_pointer<Tr>
{
  return micron::weak_pointer<Tr>(f, args...);
}


template <typename Tr = auto_thread<>, typename Func, typename... Args>
  requires(std::is_invocable_v<Func, Args...> && sizeof...(Args) > 0
           && ((std::is_lvalue_reference_v<Args> && ...))
           && (!std::is_same_v<std::decay_t<Args>, Args> && ...))
// requires(std::is_invocable_v<Func, Args...>)
auto
spawn(Func f, const Args &...args) -> __thread_pointer<Tr>
{
  return micron::weak_pointer<Tr>(f, args...);
}

template <typename Tr = auto_thread<>, typename Func, typename... Args>
// requires(std::is_invocable_v<Func, Args...>)
  requires(std::is_invocable_v<Func, Args...> && sizeof...(Args) > 0 && ((!std::is_lvalue_reference_v<Args> && ...) and (!std::is_rvalue_reference_v<Args> && ...))
           && (!std::is_reference_v<Args> && ...) && (std::is_same_v<std::decay_t<Args>, Args> && ...) && (std::is_same_v<std::remove_reference_t<Args>, Args> && ...))
auto
spawn(Func f, Args... args) -> __thread_pointer<Tr>
{
  return micron::weak_pointer<Tr>(f, args...);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) bool
is_joinable(__thread_pointer<Tr> &t)
{
  return can_wait(t->thread_id());
}
template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) bool
join(__thread_pointer<Tr> &t)
{
  if constexpr ( std::same_as<Tr, group_thread<>> ) {
    t->try_join();
    t.clear();
  } else {
    t.clear();
  }
  return true;
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
sleep(__thread_pointer<Tr> &t)
{
  t->sleep();
}
template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
awaken(__thread_pointer<Tr> &t)
{
  t->awaken();
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
yield(__thread_pointer<Tr> &t)
{
  t->yield();
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
park(cpu_set_t &set, __thread_pointer<Tr> &t)
{
  park_cpu(t->thread_id(), set);
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
interrupt(__thread_pointer<Tr> &t)
{
  t->signal(signals::interrupt);     // sending term so there's possibility of cleanup
}

template <typename Tr = auto_thread<>>
inline __attribute__((always_inline)) void
kill(__thread_pointer<Tr> &t)
{
  t->signal(signals::terminate);     // sending term so there's possibility of cleanup
}
};
};
