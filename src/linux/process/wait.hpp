//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../errno.hpp"
#include "../../syscall.hpp"
#include "../sys/resource.hpp"

namespace micron
{
constexpr static const int wnohang = 0x00000001;
constexpr static const int wuntraced = 0x00000002;
constexpr static const int wcontinued = 0x00000008;

constexpr static const int wexited = 0x00000004;
constexpr static const int wstopped = wuntraced;
constexpr static const int wnowait = 0x01000000;

constexpr int
wexitstatus(int status) noexcept
{
  return (status & 0xff00) >> 8;
}

constexpr int
wtermsig(int status) noexcept
{
  return status & 0x7f;
}

constexpr int
wstopsig(int status) noexcept
{
  return wexitstatus(status);
}

constexpr bool
wifexited(int status) noexcept
{
  return wtermsig(status) == 0;
}

constexpr bool
wifsignaled(int status) noexcept
{
  return static_cast<signed char>(((status & 0x7f) + 1) >> 1) > 0;
}

constexpr bool
wifstopped(int status) noexcept
{
  return (status & 0xff) == 0x7f;
}

constexpr bool
wifcontinued(int status) noexcept
{
  return status == 0xffff;
}

constexpr bool
wcoredump(int status) noexcept
{
  return (status & 0x80) != 0;
}

constexpr int
w_exitcode(int ret, int sig) noexcept
{
  return (ret << 8) | sig;
}

constexpr int
w_stopcode(int sig) noexcept
{
  return (sig << 8) | 0x7f;
}

struct status_t {
  posix::pid_t pid;
  int status;
  ~status_t() = default;

  status_t(void) : pid(0), status(0) { }

  status_t(const status_t &) = default;
  status_t(status_t &&) = default;
  status_t &operator=(const status_t &) = default;
  status_t &operator=(status_t &&) = default;
};

constexpr int
wexitstatus(status_t status) noexcept
{
  return (status.status & 0xff00) >> 8;
}

constexpr int
wtermsig(status_t status) noexcept
{
  return status.status & 0x7f;
}

constexpr int
wstopsig(status_t status) noexcept
{
  return wexitstatus(status);
}

constexpr bool
wifexited(status_t status) noexcept
{
  return wtermsig(status) == 0;
}

constexpr bool
wifsignaled(status_t status) noexcept
{
  return static_cast<signed char>(((status.status & 0x7f) + 1) >> 1) > 0;
}

constexpr bool
wifstopped(status_t status) noexcept
{
  return (status.status & 0xff) == 0x7f;
}

constexpr bool
wifcontinued(status_t status) noexcept
{
  return status.status == 0xffff;
}

constexpr bool
wcoredump(status_t status) noexcept
{
  return (status.status & 0x80) != 0;
}

typedef enum {
  P_ALL,   /* Wait for any child.  */
  P_PID,   /* Wait for specified process.  */
  P_PGID,  /* Wait for members of process group.  */
  P_PIDFD, /* Wait for the child referred by the pid file
              descriptor.  */
} idtype_t;

inline __attribute__((always_inline)) posix::pid_t
wait4(posix::pid_t pid, int *stat, int options, posix::rusage_t *usage)
{
  long r;
  do {
    r = micron::syscall(SYS_wait4, pid, stat, options, usage);
  } while ( r == -static_cast<long>(error::interrupted) );      // retry spurious EINTR (was a silent failure)
  return (posix::pid_t)r;
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

posix::pid_t
waitpid(status_t &status)
{
  return wait4(status.pid, &status.status, 0, nullptr);
}

auto
waitid(idtype_t idtype, posix::id_t id, posix::siginfo_t &info, int options)
{
  info = posix::siginfo_t{};
  long r;
  do {
    r = micron::syscall(SYS_waitid, idtype, id, &info, options, nullptr);
  } while ( r == -static_cast<long>(error::interrupted) );      // retry spurious EINTR
  return r;
}

};      // namespace micron
