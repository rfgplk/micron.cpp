//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// mic-thread
//
// micron's own novel threading backend
//
// threads are created via clone3, each gets a from-scratch per-thread TLS block
// join is a race-free futex wait on the kernel's CHILD_CLEARTID word
// handle is the kernel tid; control is futex/tgkill

#include "../../../bits/__arch.hpp"
#include "../../../errno.hpp"
#include "../../../memory/mman.hpp"
#include "../../../type_traits.hpp"

#include "../sched.hpp"

#include "../cpu.hpp"
#include "../prctl.hpp"
#include "../system.hpp"

#include "../../io/sys.hpp"

#include "../../../atomic/atomic.hpp"

#include "../../../bits/__thread_exit_hook.hpp"

#include "../../../exit.hpp"

#include "../../../sync/futex.hpp"

#include "../clone.hpp"
#include "tls.hpp"

// legacy + transitional reasons
// TODO: remove
#include "../../../__special/pthread"

namespace micron
{

// lives in micthread, not posix
namespace micthread
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

constexpr int thread_mutex_timed_np = static_cast<int>(thread_mutex_type::timed_np);
constexpr int thread_mutex_recursive_np = static_cast<int>(thread_mutex_type::recursive_np);
constexpr int thread_mutex_errorcheck_np = static_cast<int>(thread_mutex_type::errorcheck_np);
constexpr int thread_mutex_adaptive_np = static_cast<int>(thread_mutex_type::adaptive_np);

constexpr int thread_mutex_normal = static_cast<int>(thread_mutex_type::normal);
constexpr int thread_mutex_recursive = static_cast<int>(thread_mutex_type::recursive);
constexpr int thread_mutex_errorcheck = static_cast<int>(thread_mutex_type::errorcheck);
constexpr int thread_mutex_default = static_cast<int>(thread_mutex_type::default_type);
constexpr int thread_mutex_fast_np = static_cast<int>(thread_mutex_type::fast_np);

enum class thread_sched_inherit : int { inherit_sched = 0, explicit_sched };

constexpr int thread_inherit_sched = static_cast<int>(thread_sched_inherit::inherit_sched);
constexpr int thread_explicit_sched = static_cast<int>(thread_sched_inherit::explicit_sched);

// the handle is a raw/pure kernel tid now
using tid_t = pid_t;
constexpr tid_t thread_failed = static_cast<tid_t>(-1);

// the calling thread's kernel tid
inline tid_t
self(void) noexcept
{
  return static_cast<tid_t>(posix::gettid());
}

// the handle already IS the tid; kept for call-site compatibility
inline __attribute__((always_inline)) tid_t
get_thread_id(tid_t tid) noexcept
{
  return tid;
}

// directed signal to one thread of a thread group (tgkill); sig 0 probes liveness
inline auto
thread_kill(pid_t tgid, tid_t tid, int sig) noexcept
{
  return posix::tgkill(tgid, static_cast<long unsigned int>(tid), sig);
}

struct stack_region {
  addr_t *low;       // lowest usable byte -> clone_args.stack
  usize usable;      // bytes the kernel may use (it computes the top)
};

template<typename T>
inline stack_region
guard_stack(T *region, usize size) noexcept
{
  constexpr usize guard = __micron_page_size_default;
  addr_t *base = reinterpret_cast<addr_t *>(region);
  usize usable = size;
  // mprotect needs a page-aligned addr; if the region isn't aligned (e.g. an in-object stack), skip
  if ( size > guard && micron::mprotect(reinterpret_cast<addr_t *>(region), guard, micron::prot_none) == 0 ) {
    base = reinterpret_cast<addr_t *>(reinterpret_cast<char *>(region) + guard);
    usable = size - guard;
  }
  return { base, usable };
}

template<auto KFn, typename... KArgs>
inline tid_t
spawn(addr_t *stack_low, usize usable, unsigned long tls, int *ctid, KArgs &&...kargs)
{
  using payload_t = posix::__clone_payload<KFn, micron::decay_t<KArgs>...>;
  auto *p = new payload_t{ { reinterpret_cast<void *>(new micron::decay_t<KArgs>(static_cast<KArgs &&>(kargs)))... } };

  pid_t tid = posix::clone3_spawn(&payload_t::thunk, p, stack_low, usable, tls, nullptr, ctid);
  if ( micron::syscall_failed(tid) ) {
    payload_t::destroy(p);      // no child ran; reclaim the payload + arg copies
    errno = static_cast<int>(micron::syscall_errno(tid));
    return thread_failed;
  }
  return static_cast<tid_t>(tid);
}

inline int
join(int *ctid) noexcept
{
  for ( ;; ) {
    int v = atom::load(ctid, static_cast<int>(memory_order_acquire));
    if ( v == 0 ) return 0;
    // NOTE: deliberately NON-private FUTEX_WAIT (no futex_private_flag): the kernel's CHILD_CLEARTID
    // wake on thread exit is a shared-class wake, so a private waiter would miss it.
    auto r = micron::__futex(reinterpret_cast<u32 *>(ctid), futex_wait, static_cast<u32>(v), nullptr, nullptr, 0);
    (void)r;      // -EAGAIN (value changed) / -EINTR are benign; loop re-checks
  }
}

// non-blocking: returns 0 if the thread has exited (ctid cleared), error::busy if still running
inline int
try_join(int *ctid) noexcept
{
  return atom::load(ctid, static_cast<int>(memory_order_acquire)) == 0 ? 0 : static_cast<int>(error::busy);
}

// we name threads via /proc
inline void
set_name(tid_t tid, const char *name) noexcept
{
  // simple but dirty
  char path[64];
  char *q = path;
  for ( const char *s = "/proc/self/task/"; *s; ++s ) *q++ = *s;
  // itoa tid
  char num[16];
  int n = 0;
  unsigned t = static_cast<unsigned>(tid);
  if ( t == 0 ) num[n++] = '0';
  while ( t ) {
    num[n++] = static_cast<char>('0' + (t % 10));
    t /= 10;
  }
  while ( n-- ) *q++ = num[n];
  for ( const char *s = "/comm"; *s; ++s ) *q++ = *s;
  *q = '\0';

  i32 fd = posix::open(path, posix::o_wronly);
  if ( fd < 0 ) return;
  usize len = 0;
  while ( name[len] && len < 15 ) ++len;
  posix::write(fd, name, len);
  posix::close(fd);
}

inline auto
get_name(tid_t, char *buf, usize sz) noexcept
{
  char tmp[16] = {};
  micron::prctl(micron::PR_GET_NAME, reinterpret_cast<unsigned long>(tmp));
  usize i = 0;
  for ( ; i + 1 < sz && tmp[i]; ++i ) buf[i] = tmp[i];
  if ( sz ) buf[i] = '\0';
  return 0;
}

inline void __attribute__((noreturn))
__exit_thread(int code = 0) noexcept
{
  micron::sys_exit(code);
  __builtin_unreachable();
}

// explicit cancellation point
inline __attribute__((always_inline)) void
cancel(void) noexcept
{
  micron::__micron_park_checkpoint();
}

};      // namespace micthread

};      // namespace micron
