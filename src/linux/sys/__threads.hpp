//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

// NOTE: the rationale behind wrapping around pthreads:
// micron was never intended to be a complete rewrite of all OS components, reimplementing threading from scratch
// requires rebuilding the libc TLS system, which in itself prevents cross platform compileability (you'd have to compile
// without linking against libc at all), as well as dropping all stack canaries, fortification checks etc that the
// compiler may insert (they live in TLS). overall, it's too much work for too little gain (i've looked into it, and the
// tls system implemented by pthreads is already as slim/straightforward as it gets) threading via clone/clone3 is
// possible, BUT you must disable stack guard protection (of utmost importance), you must disable/remove all thread_local
// variables, you must not compile against -libc, and you need to reload the memory allocator if you don't do this,
// thread smashing will instantly occur

// NOTE: for future reference, and/or if i ever get around to doing this, tls lives in the TCB pointed to by %fs/%gs/C15
// (depends on arch), if you clone without setting this properly, or passing .tls to clone, it won't be set right, and
// stack unrolls will trigger stack smashing

#include "../../type_traits.hpp"
#include "../../errno.hpp"

#include "sched.hpp"

#include "system.hpp"
#include "cpu.hpp"

#include "../../__special/pthread"

namespace micron
{

// lives in pthread, not posix
namespace pthread
{

enum class thread_create_state : int { joinable = 0, detached = 1 };

constexpr int thread_create_joinable = static_cast<int>(thread_create_state::joinable);
constexpr int thread_create_detached = static_cast<int>(thread_create_state::detached);

enum class thread_mutex_type : int {
  timed_np = 0,
  recursive_np,
  errorcheck_np,
  adaptive_np,

  normal = timed_np,
  recursive = recursive_np,
  errorcheck = errorcheck_np,
  default_type = normal,

  fast_np = timed_np
};

enum cancel_state { enable, disable };
enum cancel_type { deferred, async };
// #define PTHREAD_CANCELED ((void *) -1)

constexpr int thread_mutex_timed_np = static_cast<int>(thread_mutex_type::timed_np);
constexpr int thread_mutex_recursive_np = static_cast<int>(thread_mutex_type::recursive_np);
constexpr int thread_mutex_errorcheck_np = static_cast<int>(thread_mutex_type::errorcheck_np);
constexpr int thread_mutex_adaptive_np = static_cast<int>(thread_mutex_type::adaptive_np);

constexpr int thread_mutex_normal = static_cast<int>(thread_mutex_type::normal);
constexpr int thread_mutex_recursive = static_cast<int>(thread_mutex_type::recursive);
constexpr int thread_mutex_errorcheck = static_cast<int>(thread_mutex_type::errorcheck);
constexpr int thread_mutex_default = static_cast<int>(thread_mutex_type::default_type);
constexpr int thread_mutex_fast_np = static_cast<int>(thread_mutex_type::fast_np);

enum class thread_mutex_robust : int { stalled = 0, stalled_np = stalled, robust, robust_np = robust };

constexpr int thread_mutex_stalled = static_cast<int>(thread_mutex_robust::stalled);
constexpr int thread_mutex_stalled_np = static_cast<int>(thread_mutex_robust::stalled_np);
constexpr int thread_mutex_robust = static_cast<int>(thread_mutex_robust::robust);
constexpr int thread_mutex_robust_np = static_cast<int>(thread_mutex_robust::robust_np);

enum class thread_prio_protocol : int { none = 0, inherit, protect };

constexpr int thread_prio_none = static_cast<int>(thread_prio_protocol::none);
constexpr int thread_prio_inherit = static_cast<int>(thread_prio_protocol::inherit);
constexpr int thread_prio_protect = static_cast<int>(thread_prio_protocol::protect);

enum class thread_rwlock_type : int {
  prefer_reader_np = 0,
  prefer_writer_np,
  prefer_writer_nonrecursive_np,
  default_np = prefer_reader_np
};

constexpr int thread_rwlock_prefer_reader_np = static_cast<int>(thread_rwlock_type::prefer_reader_np);
constexpr int thread_rwlock_prefer_writer_np = static_cast<int>(thread_rwlock_type::prefer_writer_np);
constexpr int thread_rwlock_prefer_writer_nonrecursive_np
    = static_cast<int>(thread_rwlock_type::prefer_writer_nonrecursive_np);
constexpr int thread_rwlock_default_np = static_cast<int>(thread_rwlock_type::default_np);

enum class thread_sched_inherit : int { inherit_sched = 0, explicit_sched };

constexpr int thread_inherit_sched = static_cast<int>(thread_sched_inherit::inherit_sched);
constexpr int thread_explicit_sched = static_cast<int>(thread_sched_inherit::explicit_sched);

enum class thread_scope : int { system = 0, process };

constexpr int thread_scope_system = static_cast<int>(thread_scope::system);
constexpr int thread_scope_process = static_cast<int>(thread_scope::process);

enum class thread_process_shared : int { process_private = 0, process_shared };

constexpr int thread_process_private = static_cast<int>(thread_process_shared::process_private);
constexpr int thread_process_shared_flag = static_cast<int>(thread_process_shared::process_shared);

// PTHREAD IMPLEMENTATION

constexpr pthread_t thread_failed = numeric_limits<pthread_t>::max();

using thread_fn = void *(*)(void *);

inline pthread_t
create_thread(thread_fn fn, void *arg)
{
  pthread_t tid{};
  pthread_create(&tid, nullptr, fn, arg);
  return tid;
}
template <typename Fn, typename... Args> struct __impl_thread {
  Fn fn;
  void *args[sizeof...(Args)];

  static void *
  trampoline(void *p)
  {
    auto *self = static_cast<__impl_thread *>(p);
    __invoke(self, micron::make_index_sequence<sizeof...(Args)>{});
    __cleanup(self, micron::make_index_sequence<sizeof...(Args)>{});
    delete self;
    return nullptr;
  }

private:
  template <size_t... I>
  static void
  __invoke(__impl_thread *self, micron::index_sequence<I...>)
  {
    self->fn(*static_cast<Args *>(self->args[I])...);
  }

  template <size_t... I>
  static void
  __cleanup(__impl_thread *self, micron::index_sequence<I...>)
  {
    (delete static_cast<Args *>(self->args[I]), ...);
  }
};

// i'm sorry for this

template <typename Fn, typename... Args>
inline pthread_t
create_thread(const pthread_attr_t &attrs, Fn fn, Args &&...args)
{
  auto *payload = new __impl_thread<Fn, micron::decay_t<Args>...>{
    fn, { reinterpret_cast<void *>(new micron::decay_t<Args>(static_cast<Args &&>(args)))... }
  };

  pthread_t tid{};
  if ( int err = pthread_create(&tid, &attrs, &__impl_thread<Fn, micron::decay_t<Args>...>::trampoline, payload);
       err != 0 ) {
    errno = err;
    delete payload;
    return thread_failed;
  }
  return tid;
}

// inits thread, should be called first
inline pthread_attr_t
prepare_thread(thread_create_state dstate = thread_create_state::joinable, int policy = posix::sched_other,
               int priority = 0)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  pthread_attr_setdetachstate(&attr, static_cast<int>(dstate));

  posix::sched_param sp{};
  sp.sched_priority = priority;
  pthread_attr_setschedpolicy(&attr, static_cast<int>(policy));
  pthread_attr_setschedparam(&attr, reinterpret_cast<sched_param *>(&sp));
  pthread_attr_setinheritsched(&attr, thread_explicit_sched);

  return attr;
}

inline pthread_attr_t
prepare_thread_with_stack(thread_create_state dstate, int policy, addr_t *ptr, size_t stack_size, int priority = 0)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  pthread_attr_setdetachstate(&attr, static_cast<int>(dstate));

  posix::sched_param sp{};
  sp.sched_priority = priority;
  pthread_attr_setschedpolicy(&attr, static_cast<int>(policy));
  pthread_attr_setschedparam(&attr, reinterpret_cast<sched_param *>(&sp));
  pthread_attr_setinheritsched(&attr, thread_explicit_sched);
  pthread_attr_setstacksize(&attr, stack_size);
  pthread_attr_setstack(&attr, ptr, stack_size);

  return attr;
}

void
set_affinity(pthread_attr_t &attr, const posix::cpu_set_t &cpu)
{
  pthread_attr_setaffinity_np(&attr, sizeof(cpu), reinterpret_cast<const cpu_set_t *>(&cpu));
}

template <typename T>
  requires(micron::is_fundamental_v<T>)
inline void
set_stack_thread(pthread_attr_t &attr, T *ptr, size_t size)
{
  pthread_attr_setstacksize(&attr, size);
  pthread_attr_setstack(&attr, ptr, size);
}

inline void
get_attrs(pthread_t pid, pthread_attr_t &attr)
{
  pthread_getattr_np(pid, &attr);
}
inline void
get_stack_thread(const pthread_attr_t &attr, addr_t *&ptr, size_t &size)
{
  pthread_attr_getstack(&attr, reinterpret_cast<void **>(&ptr), &size);
}

inline void
get_sched_param(const pthread_attr_t &attr, u32 &param)
{
  pthread_attr_getschedparam(&attr, reinterpret_cast<sched_param *>(&param));
}

inline void
get_detach_state(const pthread_attr_t &attr, i32 &detach)
{
  pthread_attr_getdetachstate(&attr, &detach);
}
inline void
get_sched_policy(const pthread_attr_t &attr, i32 &policy)
{
  pthread_attr_getschedpolicy(&attr, &policy);
}
auto
get_name(pthread_t pt, char *name, size_t sz)
{
  return pthread_getname_np(pt, name, sz);
}

auto
self(void) -> pthread_t
{
  return pthread_self();
}

auto
set_name(pthread_t pt, const char *name)
{
  return pthread_setname_np(pt, name);
}

auto
__join_thread(pthread_t thread, void **rval = nullptr)
{
  return pthread_join(thread, rval);
}
inline void __attribute__((noreturn))
__exit_thread(void *ret = nullptr)
{
  if ( ret == nullptr ) {
    int __t = 0;
    pthread_exit(&__t);
  }
  pthread_exit(ret);
}
auto
__try_join_thread(pthread_t thread, void **rval = nullptr)
{
  return pthread_tryjoin_np(thread, rval);
}

inline auto
thread_kill(pid_t pid, pthread_t thread, int sig)
{
  return posix::tgkill(pid, thread, sig);
}
inline auto
get_thread_id(pthread_t thread)
{
  return pthread_gettid_np(thread);
}

inline auto
set_cancel_state(cancel_state state)
{
  return pthread_setcancelstate(static_cast<int>(state), nullptr);
}
inline auto
set_cancel_type(cancel_type type)
{
  return pthread_setcanceltype(static_cast<int>(type), nullptr);
}
inline auto
cancel_thread(pthread_t th)
{
  return pthread_cancel(th);
}
inline __attribute__((always_inline)) void
cancel(void)
{
  pthread_testcancel();
}

};

};
