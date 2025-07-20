//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../syscall.hpp"

namespace micron
{
typedef enum {
  P_ALL,   /* Wait for any child.  */
  P_PID,   /* Wait for specified process.  */
  P_PGID,  /* Wait for members of process group.  */
  P_PIDFD, /* Wait for the child referred by the PID file
              descriptor.  */
} idtype_t;

inline __attribute__((always_inline)) posix::pid_t
wait4(posix::pid_t pid, int *stat, int options, struct rusage *usage)
{
  return (posix::pid_t)micron::syscall(SYS_wait4, pid, stat, options, usage);
}

posix::pid_t
wait(int *status)
{
  return wait4(-1, status, 0, nullptr);
}
posix::pid_t
waitpid(posix::pid_t pid, int *status, int options)
{
  return wait4(pid, status, options, nullptr);
}

auto
waitid(idtype_t idtype, posix::id_t id, siginfo_t &info, int options)
{
  // NOTE: last one is rusage*
  return micron::syscall(SYS_waitid, idtype, id, &info, options, nullptr);
}

};
