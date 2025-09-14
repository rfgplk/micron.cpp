//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "linux_types.hpp"

#include "../syscall.hpp"

#include "../linux/sys/signal.hpp"
#include "../linux/sys/time.hpp"

namespace micron
{
namespace posix
{
auto
raise(int sig)
{
  return micron::syscall(SYS_kill, static_cast<pid_t>(micron::syscall(SYS_getpid)), sig);
}

auto
kill(posix::pid_t pid, int sig)
{
  return micron::syscall(SYS_kill, pid, sig);
}

/*
 *           long clone(unsigned long flags, void *stack,
 *                      int *parent_tid, int *child_tid,
 *                     unsigned long tls);
 */

// NOTE: these functions are here only for general utility, they can not be used as is without setting up the stack
// beforehand

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

int
dup(int old)
{
  return static_cast<int>(micron::syscall(SYS_dup, old));
}

int
dup2(int old, int newfd)
{
  return static_cast<int>(micron::syscall(SYS_dup2, old, newfd));
}

int
dup3(int old, int newfd, int flags)
{
  return static_cast<int>(micron::syscall(SYS_dup2, old, newfd, flags));
}

/*
auto
wait(int *wstatus)
{
  return micron::syscall(SYS_wait4, -1, wstatus, 0, nullptr);
}

auto
waitid(posix::idtype_t idtype, posix::id_t id, siginfo_t &info, int options)
{
  // NOTE: last one is rusage*
  return micron::syscall(SYS_waitid, idtype, id, &info, options, nullptr);
}*/
};
};
