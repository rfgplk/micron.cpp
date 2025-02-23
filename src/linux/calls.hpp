#pragma once

#include "../types.hpp"
#include "linux_types.hpp"

#include "../syscall.hpp"

namespace micron
{
namespace posix
{

auto
nanosleep(const struct timespec &req, struct timespec &rem)
{
  return ::syscall(SYS_nanosleep, &req, &rem);
}

auto
nanosleep(const struct timespec &req)
{
  return ::syscall(SYS_nanosleep, &req, nullptr);
}

auto
kill(pid_t pid, int sig)
{
  return ::syscall(SYS_kill, pid, sig);
}

/*
 *           long clone(unsigned long flags, void *stack,
 *                      int *parent_tid, int *child_tid,
 *                     unsigned long tls);
 */


// NOTE: these functions are here only for general utility, they can not be used as is without setting up the stack beforehand

auto
clone_kernel(unsigned long flags, void *stack, int *parent_tid, int *child_tid, unsigned long tls)
{
  return ::syscall(SYS_clone, flags, stack, parent_tid, child_tid, tls);
}

// NOTE: Linux 5.3+(5.7) req

struct clone_args {
  u64 flags;        /* Flags bit mask */
  u64 pidfd;        /* Where to store PID file descriptor
                       (int *) */
  u64 child_tid;    /* Where to store child TID,
                       in child's memory (pid_t *) */
  u64 parent_tid;   /* Where to store child TID,
                       in parent's memory (pid_t *) */
  u64 exit_signal;  /* Signal to deliver to parent on
                       child termination */
  u64 stack;        /* Pointer to lowest byte of stack */
  u64 stack_size;   /* Size of stack */
  u64 tls;          /* Location of new TLS */
  u64 set_tid;      /* Pointer to a pid_t array
                       (since Linux 5.5) */
  u64 set_tid_size; /* Number of elements in set_tid
                       (since Linux 5.5) */
  u64 cgroup;       /* File descriptor for target cgroup
                       of child (since Linux 5.7) */
};

auto
clone3_kernel(clone_args& args)
{
  return ::syscall(SYS_clone3, &args, sizeof(args));
}

auto
fork_kernel(void)
{
  return ::syscall(SYS_fork);
}

auto
wait(int *wstatus)
{
  return ::syscall(SYS_wait4, -1, wstatus, 0, nullptr);
}

auto
waitid(idtype_t idtype, id_t id, siginfo_t &info, int options)
{
  // NOTE: last one is rusage*
  return ::syscall(SYS_waitid, idtype, id, &info, options, nullptr);
}
};
};
