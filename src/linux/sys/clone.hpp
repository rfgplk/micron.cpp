//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../allocation/abcmalloc/tapi.hpp"

#include "../../errno.hpp"
#include "../__includes.hpp"

#include "sched.hpp"

#include "signal.hpp"
#include "system.hpp"

#include "types.hpp"

namespace micron
{

/*
 *           long clone(unsigned long flags, void *stack,
 *                      int *parent_tid, int *child_tid,
 *                     unsigned long tls);
 */

namespace posix
{

enum class clone_flags : u32 {
  clear_tid = clone_child_cleartid,
  set_tid = clone_child_settid,
  detached_obsolete = clone_detached,
  new_cgroup = clone_newcgroup,
  copy_files = clone_files,
  same_io = clone_io,
  newcgroup = clone_newcgroup,
  newipc = clone_newipc,
  newnet = clone_newnet,
  newns = clone_newns,
  newpid = clone_newpid,
  newuser = clone_newuser,
  parent_copy = clone_parent,
  parent_settid = clone_parent_settid,
  pid_pointer = clone_pidfd,
  ptrace = clone_ptrace,
  enable_tls = clone_settls,
  share_signals = clone_sighand,
  start_stopped_removed = 0,
  share_semaphores = clone_sysvsem,
  as_thread = clone_thread,
  no_trace = clone_untraced,
  vfork_obsolete = clone_vfork,
  same_memory = clone_vm
};

enum class posix_process_flags {
  reset_ids = 0x01,
  set_pgroup = 0x02,
  sig_def = 0x04,
  sig_mask = 0x08,
  sched_param = 0x10,
  set_scheduler = 0x20,
  use_vfork = 0x40,
  new_session = 0x80,
  set_cgroup = 0x100
};

auto
clone_kernel(unsigned long flags, void *stack, int *parent_tid, int *child_tid, unsigned long tls)
{
  return micron::syscall(SYS_clone, flags, stack, parent_tid, child_tid, tls);
}

// NOTE: Linux 5.3+(5.7) req

struct clone_args {
  u64 flags;        /* Flags bit mask */
  u64 pidfd;        /* Where to store PID file descriptor
                       (int *) */
  u64 child_tid;    /* Where to store child TID,
                       in child's memory (posix::pid_t *) */
  u64 parent_tid;   /* Where to store child TID,
                       in parent's memory (posix::pid_t *) */
  u64 exit_signal;  /* Signal to deliver to parent on
                       child termination */
  u64 stack;        /* Pointer to lowest byte of stack */
  u64 stack_size;   /* Size of stack */
  u64 tls;          /* Location of new TLS */
  u64 set_tid;      /* Pointer to a posix::pid_t array
                       (since Linux 5.5) */
  u64 set_tid_size; /* Number of elements in set_tid
                       (since Linux 5.5) */
  u64 cgroup;       /* File descriptor for target cgroup
                       of child (since Linux 5.7) */
};

auto
clone3_kernel(clone_args &args)
{
  return micron::syscall(SYS_clone3, &args, sizeof(args));
}

auto
fork_kernel(void)
{
  return micron::syscall(SYS_fork);
}

template <size_t Sz, auto Fn, typename... Args>
pid_t
clone(addr_t *stack, int flags, Args &&...args)
{
  if ( !stack )
    return -EINVAL;
  posix::clone_args clone_args{};
  clone_args.flags = static_cast<u64>(flags);
  if ( flags & clone_parent or (flags & clone_sighand and flags & clone_thread) )
    clone_args.exit_signal = 0;     // NOTE: kernel constraint otherwise -22
  else
    clone_args.exit_signal = sig_chld;

  uintptr_t sptr = reinterpret_cast<uintptr_t>(stack);
  sptr += Sz;         // top of allocated stack
  sptr &= ~0xFUL;     // 16-byte alignment
  clone_args.stack = sptr;
  clone_args.stack_size = Sz;
  clone_args.parent_tid = 0;
  clone_args.child_tid = 0;
  clone_args.tls = 0;

  pid_t pid = static_cast<pid_t>(posix::clone3_kernel(clone_args));

  if ( pid == 0 ) {
    using ret_t = decltype(Fn(args...));
    if constexpr ( micron::is_same_v<ret_t, int> ) {
      int ret = Fn(micron::forward<Args>(args)...);
      micron::syscall(SYS_exit, ret);
      __builtin_unreachable();
    } else {
      Fn(micron::forward<Args>(args)...);
      micron::syscall(SYS_exit, 0);
      __builtin_unreachable();
    }
  }
  // errored out
  if ( pid < 0 ) {
    errno = -pid;
    pid = -1;
  }
  return pid;
}

// special clone func
pid_t
__fork_clone(int exit_signal)
{
  // these should be exactly like this
  posix::clone_args clone_args{};
  clone_args.flags = 0;
  clone_args.exit_signal = exit_signal;     // should be sig_chld
  clone_args.stack = 0;
  clone_args.stack_size = 0;
  clone_args.parent_tid = 0;
  clone_args.child_tid = 0;
  clone_args.tls = 0;

  pid_t pid = static_cast<pid_t>(posix::clone3_kernel(clone_args));

  if ( pid < 0 ) {
    errno = pid;
    pid = -1;
  }
  return pid;
}

// c-style, legacy
pid_t
clone(int (*fn)(void *), void *stack, int flags, void *arg, int *parent_tid = nullptr, void *tls = nullptr, int *child_tid = nullptr)
{
  if ( !stack )
    return -EINVAL;

  uintptr_t sp = reinterpret_cast<uintptr_t>(stack);
  sp += 1024 * 1024;
  sp &= ~uintptr_t(0xF);
  void *stack_top = reinterpret_cast<void *>(sp);

  posix::clone_args args{};
  args.flags = static_cast<u64>(flags);
  args.exit_signal = sig_chld;
  args.stack = reinterpret_cast<u64>(stack_top);
  args.stack_size = 0;     // opt
  args.parent_tid = reinterpret_cast<u64>(parent_tid);
  args.child_tid = reinterpret_cast<u64>(child_tid);
  args.tls = reinterpret_cast<u64>(tls);

  pid_t pid = static_cast<pid_t>(posix::clone3_kernel(args));
  if ( pid == -1 && errno == ENOSYS ) {
    pid = static_cast<pid_t>(posix::clone_kernel(flags, stack_top, parent_tid, child_tid, reinterpret_cast<unsigned long>(tls)));
  }

  if ( pid == 0 ) {
    if ( fn != nullptr ) {
      int ret = fn(arg);
      micron::syscall(SYS_exit, ret);
      __builtin_unreachable();
    }
  }

  return pid;
}
/*
pid_t
clone(int (*fn)(void *), void *arg, unsigned long flags, void *stack, size_t stack_size, int *parent_tid = nullptr,
      int *child_tid = nullptr, unsigned long tls = 0, int exit_signal = sig_chld)
{
  if ( !stack || stack_size == 0 )
    return -EINVAL;

  // Align stack to 16 bytes for x86_64 (downward-growing stack)
  uintptr_t sp = reinterpret_cast<uintptr_t>(stack) + stack_size;
  sp &= ~uintptr_t(0xF);
  void *stack_top = reinterpret_cast<void *>(sp);

  posix::clone_args args{};
  args.flags = flags;
  args.exit_signal = exit_signal;
  args.stack = reinterpret_cast<u64>(stack_top);
  args.stack_size = stack_size;
  args.parent_tid = reinterpret_cast<u64>(parent_tid);
  args.child_tid = reinterpret_cast<u64>(child_tid);
  args.tls = tls;

  pid_t pid = static_cast<pid_t>(posix::clone3_kernel(args));
  if ( pid == -1 && errno == ENOSYS ) {
    pid = static_cast<pid_t>(
        posix::clone_kernel(flags, stack_top, parent_tid, child_tid, reinterpret_cast<unsigned long>(tls)));
  }

  if ( pid == 0 ) {
    // Child executes the function
    int ret = fn(arg);
    micron::syscall(SYS_exit, ret);     // ensures child exits safely
    __builtin_unreachable();
  }

  return pid;     // Parent receives child's PID
}*/
};
};
