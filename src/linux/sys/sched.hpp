#pragma once

#include "../../linux/sys/types.hpp"
#include "../../types.hpp"
#include "cpu.hpp"

#ifndef _BITS_TYPES_STRUCT_SCHED_PARAM
#define _BITS_TYPES_STRUCT_SCHED_PARAM 1

struct sched_param {
  int sched_priority;
};

#endif

namespace micron
{

namespace posix
{

struct sched_param {
  int sched_priority;
};

int
sched_yield()
{
  return static_cast<int>(micron::syscall(SYS_sched_yield));
}

int
sched_getparam(pid_t pid, sched_param &params)
{
  return static_cast<int>(micron::syscall(SYS_sched_getparam, pid, &params));
}

int
sched_setparam(pid_t pid, const sched_param &params)
{
  return static_cast<int>(micron::syscall(SYS_sched_setparam, pid, &params));
}

int
sched_setscheduler(pid_t pid, int policy, sched_param &params)
{
  return static_cast<int>(micron::syscall(SYS_sched_setscheduler, pid, policy, &params));
}

int
sched_getscheduler(pid_t pid)
{
  return static_cast<int>(micron::syscall(SYS_sched_setscheduler, pid));
}

int
sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t &mask)
{
  return static_cast<int>(micron::syscall(SYS_sched_setaffinity, pid, cpusetsize, &mask));
}

int
sched_getaffinity(pid_t pid, size_t cpusetsize, cpu_set_t &mask)
{
  return static_cast<int>(micron::syscall(SYS_sched_getaffinity, pid, cpusetsize, &mask));
}

constexpr static const int sched_other = 0;
constexpr static const int sched_fifo = 1;
constexpr static const int sched_rr = 2;
constexpr static const int sched_normal = 0;
constexpr static const int sched_batch = 3;
constexpr static const int sched_iso = 4;
constexpr static const int sched_idle = 5;
constexpr static const int sched_deadline = 6;
constexpr static const int sched_ext = 7;
constexpr static const int sched_reset_on_fork = 0x40000000;
constexpr static const int sched_flag_reset_on_fork = 0x01;
constexpr static const int sched_flag_reclaim = 0x02;
constexpr static const int sched_flag_dl_overrun = 0x04;
constexpr static const int sched_flag_keep_policy = 0x08;
constexpr static const int sched_flag_keep_params = 0x10;
constexpr static const int sched_flag_util_clamp_min = 0x20;
constexpr static const int sched_flag_util_clamp_max = 0x40;
constexpr static const unsigned long long csignal = 0x000000ff;
constexpr static const unsigned long long clone_vm = 0x00000100;
constexpr static const unsigned long long clone_fs = 0x00000200;
constexpr static const unsigned long long clone_files = 0x00000400;
constexpr static const unsigned long long clone_sighand = 0x00000800;
constexpr static const unsigned long long clone_pidfd = 0x00001000;
constexpr static const unsigned long long clone_ptrace = 0x00002000;
constexpr static const unsigned long long clone_vfork = 0x00004000;
constexpr static const unsigned long long clone_parent = 0x00008000;
constexpr static const unsigned long long clone_thread = 0x00010000;
constexpr static const unsigned long long clone_newns = 0x00020000;
constexpr static const unsigned long long clone_sysvsem = 0x00040000;
constexpr static const unsigned long long clone_settls = 0x00080000;
constexpr static const unsigned long long clone_parent_settid = 0x00100000;
constexpr static const unsigned long long clone_child_cleartid = 0x00200000;
constexpr static const unsigned long long clone_detached = 0x00400000;
constexpr static const unsigned long long clone_untraced = 0x00800000;
constexpr static const unsigned long long clone_child_settid = 0x01000000;
constexpr static const unsigned long long clone_newcgroup = 0x02000000;
constexpr static const unsigned long long clone_newuts = 0x04000000;
constexpr static const unsigned long long clone_newipc = 0x08000000;
constexpr static const unsigned long long clone_newuser = 0x10000000;
constexpr static const unsigned long long clone_newpid = 0x20000000;
constexpr static const unsigned long long clone_newnet = 0x40000000;
constexpr static const unsigned long long clone_io = 0x80000000;
constexpr static const unsigned long long clone_newtime = 0x00000080;

};     // namespace posix
};     // namespace micron
