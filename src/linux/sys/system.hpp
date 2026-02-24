//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../linux/sys/types.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../process/sched.hpp"
#include "cpu.hpp"
#include "sched.hpp"

namespace micron
{

namespace posix
{

posix::pid_t
setsid(void)
{
  return static_cast<posix::pid_t>(micron::syscall(SYS_setsid));
}

posix::pid_t
setpgid(posix::pid_t pid, posix::pid_t gpid)
{
  return static_cast<posix::pid_t>(micron::syscall(SYS_setpgid, pid, gpid));
}

int
setuid(posix::uid_t uid)
{
  return static_cast<int>(micron::syscall(SYS_setuid, uid));
}

int
setgid(posix::gid_t gid)
{
  return static_cast<int>(micron::syscall(SYS_setgid, gid));
}

posix::pid_t
getsid(void)
{
  return static_cast<posix::pid_t>(micron::syscall(SYS_getsid));
}

pid_t
gettid(void)
{
  return static_cast<pid_t>(micron::syscall(SYS_gettid));
}

pid_t
getpid(void)
{
  return static_cast<pid_t>(micron::syscall(SYS_getpid));
}

pid_t
getppid(void)
{
  return static_cast<pid_t>(micron::syscall(SYS_getppid));
}

gid_t
getgid(void)
{
  return static_cast<gid_t>(micron::syscall(SYS_getgid));
}

uid_t
getuid(void)
{
  return static_cast<uid_t>(micron::syscall(SYS_getuid));
}

uid_t
geteuid(void)
{
  return static_cast<uid_t>(micron::syscall(SYS_geteuid));
}

auto
get_scheduler(pid_t pid)
{
  return micron::syscall(SYS_sched_getscheduler, pid);
}

auto
set_scheduler(pid_t pid, int sched, sched_param &prio)
{
  return micron::syscall(SYS_sched_setscheduler, pid, sched, &prio);
}

void
get_affinity(pid_t pid, cpu_set_t &mask)
{
  micron::syscall(SYS_sched_getaffinity, pid, sizeof(mask), &mask);
}

auto
set_attrs(pid_t pid, sched_attr &mask, int options)
{
  return micron::syscall(SYS_sched_setattr, pid, &mask, options);
}

auto
get_attrs(pid_t pid, sched_attr &mask, int options)
{
  return micron::syscall(SYS_sched_getattr, pid, &mask, sizeof(struct sched_attr), options);
}

void
set_affinity(pid_t pid, cpu_set_t &mask)
{
  micron::syscall(SYS_sched_setaffinity, pid, sizeof(mask), &mask);
}

auto
get_priority(int which, pid_t pid)
{
  return micron::syscall(SYS_getpriority, which, pid);
}

void
set_priority(int which, pid_t pid, int prio)
{
  micron::syscall(SYS_setpriority, which, pid, prio);
}

auto
raise(int sig)
{
  return micron::syscall(SYS_kill, getpid(), sig);
}

auto
tgkill(posix::pid_t pid, long unsigned int tid, int sig)
{
  return micron::syscall(SYS_tgkill, pid, tid, sig);
}

auto
kill(posix::pid_t pid, int sig)
{
  return micron::syscall(SYS_kill, pid, sig);
}

int
exit(int r)
{
  return static_cast<int>(micron::syscall(SYS_exit, r));
}
};     // namespace posix

uid_t
this_pid()
{
  return posix::getpid();
}

};     // namespace micron
