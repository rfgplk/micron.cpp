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

inline posix::pid_t
setsid(void)
{
  return static_cast<posix::pid_t>(micron::syscall(SYS_setsid));
}

inline posix::pid_t
setpgid(posix::pid_t pid, posix::pid_t gpid)
{
  return static_cast<posix::pid_t>(micron::syscall(SYS_setpgid, pid, gpid));
}

// NOTE: on i386/arm32 the bare setuid/getuid/setgid/getgid/geteuid/getegid syscalls are the legacy 16-bit-UID forms
#if defined(__micron_arch_x86) || defined(__micron_arch_arm32)
inline constexpr auto __sys_setuid = SYS_setuid32;
inline constexpr auto __sys_setgid = SYS_setgid32;
inline constexpr auto __sys_getuid = SYS_getuid32;
inline constexpr auto __sys_getgid = SYS_getgid32;
inline constexpr auto __sys_geteuid = SYS_geteuid32;
inline constexpr auto __sys_getegid = SYS_getegid32;
#else
inline constexpr auto __sys_setuid = SYS_setuid;
inline constexpr auto __sys_setgid = SYS_setgid;
inline constexpr auto __sys_getuid = SYS_getuid;
inline constexpr auto __sys_getgid = SYS_getgid;
inline constexpr auto __sys_geteuid = SYS_geteuid;
inline constexpr auto __sys_getegid = SYS_getegid;
#endif

inline i32
setuid(posix::uid_t uid)
{
  return static_cast<i32>(micron::syscall(__sys_setuid, uid));
}

inline i32
setgid(posix::gid_t gid)
{
  return static_cast<i32>(micron::syscall(__sys_setgid, gid));
}

inline posix::pid_t
getsid(void)
{
  return static_cast<posix::pid_t>(micron::syscall(SYS_getsid, 0));
}

inline pid_t
gettid(void)
{
  return static_cast<pid_t>(micron::syscall(SYS_gettid));
}

inline pid_t
getpid(void)
{
  return static_cast<pid_t>(micron::syscall(SYS_getpid));
}

inline pid_t
getppid(void)
{
  return static_cast<pid_t>(micron::syscall(SYS_getppid));
}

inline gid_t
getgid(void)
{
  return static_cast<gid_t>(micron::syscall(__sys_getgid));
}

inline gid_t
getegid(void)
{
  return static_cast<gid_t>(micron::syscall(__sys_getegid));
}

inline uid_t
getuid(void)
{
  return static_cast<uid_t>(micron::syscall(__sys_getuid));
}

inline uid_t
geteuid(void)
{
  return static_cast<uid_t>(micron::syscall(__sys_geteuid));
}

inline auto
get_scheduler(pid_t pid) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_sched_getscheduler, pid));
}

inline auto
set_scheduler(pid_t pid, int sched, sched_param &prio) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_sched_setscheduler, pid, sched, &prio));
}

inline auto
get_affinity(pid_t pid, cpu_set_t &mask) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_sched_getaffinity, pid, sizeof(mask), &mask));
}

inline auto
set_attrs(pid_t pid, sched_attr &mask, int options) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_sched_setattr, pid, &mask, options));
}

inline auto
get_attrs(pid_t pid, sched_attr &mask, int options) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_sched_getattr, pid, &mask, sizeof(struct sched_attr), options));
}

inline auto
set_affinity(pid_t pid, cpu_set_t &mask) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_sched_setaffinity, pid, sizeof(mask), &mask));
}

inline auto
get_priority(int which, pid_t pid) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_getpriority, which, pid));
}

inline auto
set_priority(int which, pid_t pid, int prio) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_setpriority, which, pid, prio));
}

inline auto
raise(i32 sig) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_tgkill, getpid(), gettid(), sig));
}

inline auto
tgkill(posix::pid_t pid, long unsigned int tid, int sig) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_tgkill, pid, tid, sig));
}

inline auto
kill(posix::pid_t pid, int sig) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_kill, pid, sig));
}

inline auto
pause(void) -> i32
{
#if defined(__micron_syscall_generic)
  // arm64 has no SYS_pause
  return static_cast<i32>(micron::syscall(SYS_ppoll, nullptr, 0, nullptr, nullptr, 8));
#else
  return static_cast<i32>(micron::syscall(SYS_pause));
#endif
}

inline auto
exit(int r) -> i32
{
  // only terminate CALLING thread, not all threads
  return static_cast<i32>(micron::syscall(SYS_exit, r));
}

struct utsname_t {
  char sysname[65];
  char nodename[65];
  char release[65];
  char version[65];
  char machine[65];
  char domainname[65];
};

inline auto
uname(utsname_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_uname, &buf));
}

constexpr unsigned int grnd_nonblock = 0x0001;
constexpr unsigned int grnd_random = 0x0002;
constexpr unsigned int grnd_insecure = 0x0004;

inline auto
getrandom(void *buf, usize len, unsigned int flags) -> max_t
{
  return micron::syscall(SYS_getrandom, buf, len, flags);
}

inline auto
membarrier(int cmd, unsigned int flags, int cpu_id) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_membarrier, cmd, flags, cpu_id));
}

inline auto
personality(unsigned long persona) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_personality, persona));
}

inline auto
setns(int fd, int nstype) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_setns, fd, nstype));
}

constexpr unsigned int linux_reboot_magic1 = 0xfee1deadU;
constexpr unsigned int linux_reboot_magic2 = 0x28121969U;
constexpr unsigned int linux_reboot_cmd_restart = 0x01234567U;
constexpr unsigned int linux_reboot_cmd_halt = 0xcdef0123U;
constexpr unsigned int linux_reboot_cmd_power_off = 0x4321fedcU;
constexpr unsigned int linux_reboot_cmd_cad_on = 0x89abcdefU;
constexpr unsigned int linux_reboot_cmd_cad_off = 0x00000000U;

inline auto
reboot(unsigned int cmd) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_reboot, linux_reboot_magic1, linux_reboot_magic2, cmd, nullptr));
}

// >=5.3
inline auto
pidfd_open(posix::pid_t pid, unsigned int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_pidfd_open, pid, flags));
}

// >=5.1
inline auto
pidfd_send_signal(int pidfd, int sig, void *info, unsigned int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_pidfd_send_signal, pidfd, sig, info, flags));
}
};      // namespace posix

inline uid_t
this_pid()
{
  return static_cast<uid_t>(posix::getpid());
}

};      // namespace micron
