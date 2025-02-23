#pragma once

#include "sched.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../../linux/linux_types.hpp"

#include <sys/syscall.h>
#include <sched.h>
#include <unistd.h>

namespace micron
{

namespace posix
{
/*
 * NOTE: moved to linux/linux_types.hpp
 * using pid_t = i32;
using key_t = i32;
using clockid_t = i32;
using uid_t = u32;
using gid_t = u32;
using rlim_t = u64;
*/
pid_t
getpid(void)
{
  return static_cast<pid_t>(::syscall(SYS_getpid));
}
pid_t
getppid(void)
{
  return static_cast<pid_t>(::syscall(SYS_getppid));
}
gid_t
getgid(void)
{
  return static_cast<gid_t>(::syscall(SYS_getgid));
}
uid_t
getuid(void)
{
  return static_cast<uid_t>(::syscall(SYS_getuid));
}
uid_t
geteuid(void)
{
  return static_cast<uid_t>(::syscall(SYS_geteuid));
}


auto
get_scheduler(pid_t pid)
{
  return ::syscall(SYS_sched_getscheduler, pid);
}


auto
set_scheduler(pid_t pid, int sched, sched_param& prio)
{
  return ::syscall(SYS_sched_setscheduler, pid, sched, &prio);
}

void
get_affinity(pid_t pid, cpu_set_t &mask)
{
  ::syscall(SYS_sched_getaffinity, pid, sizeof(mask), &mask);
}


auto
set_attrs(pid_t pid, sched_attr &mask, int options)
{
  return ::syscall(SYS_sched_setattr, pid, &mask, options);
}


auto
get_attrs(pid_t pid, sched_attr &mask, int options)
{
  return ::syscall(SYS_sched_getattr, pid, &mask, sizeof(struct sched_attr), options);
}

void
set_affinity(pid_t pid, cpu_set_t &mask)
{
  ::syscall(SYS_sched_setaffinity, pid, sizeof(mask), &mask);
}


auto
get_priority(int which, pid_t pid)
{
  return ::syscall(SYS_getpriority, which, pid);
}


void
set_priority(int which, pid_t pid, int prio)
{
  ::syscall(SYS_setpriority, which, pid, prio);
}

};

uid_t
this_pid()
{
  return posix::getpid();
}

};
