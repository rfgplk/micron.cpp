//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../bits/__arch.hpp"
#include "../concepts.hpp"
#include "../syscall.hpp"
#include "../types.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// named syscall groups

namespace micron
{
namespace sec
{

#define __micron_sec_has_groups_namespaces 1
#define __micron_sec_has_groups_mount_api 1
#define __micron_sec_has_groups_keyring 1
#define __micron_sec_has_groups_kernel_debug 1
#define __micron_sec_has_groups_uring 1

struct syscall_group_tag {
};

template<typename T>
concept is_syscall_group = requires {
  typename T::policy_tag;
  { T::calls } -> micron::convertible_to<const i32 *>;
  { T::count } -> micron::convertible_to<usize>;
} && micron::is_same_v<typename T::policy_tag, syscall_group_tag>;

namespace groups
{
struct baseline {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_exit,
    SYS_exit_group,
    SYS_rt_sigreturn,
    SYS_brk,
    SYS_munmap,
    SYS_futex,
    SYS_sysinfo,
    SYS_set_tid_address,
    SYS_set_robust_list,
    SYS_rseq,
    SYS_getrandom,
    SYS_close,
    // NOTE: SYS_ioctl must not be here; see (CVE-2019-10063)
    // if you need terminal control use groups::baseline_tty
    SYS_read,
    SYS_write,
    SYS_openat,
    SYS_clock_gettime,
    SYS_gettid,
    SYS_sched_yield,
    SYS_faccessat,
    SYS_mprotect,
#if defined(__micron_arch_amd64)
    SYS_mmap,
    SYS_arch_prctl,
    SYS_open,
    SYS_access,
#elif defined(__micron_arch_x86)
    SYS_mmap2,
    SYS_arch_prctl,
    SYS_open,
    SYS_access,
    SYS_futex_time64,
    SYS_clock_gettime64,
#elif defined(__micron_arch_arm32)
    SYS_mmap2,
    SYS_open,
    SYS_access,
    SYS_futex_time64,
    SYS_clock_gettime64,
#elif defined(__micron_arch_arm64)
    SYS_mmap,
#else
#error "micron::sec::groups::baseline: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

// baseline plus ioctl
struct baseline_tty {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = { SYS_ioctl };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct memory {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_memfd_create, SYS_brk,      SYS_munmap,        SYS_mprotect,      SYS_madvise,       SYS_mremap,     SYS_msync, SYS_mlock,
    SYS_munlock,      SYS_mlockall, SYS_munlockall,    SYS_mlock2,        SYS_shmget,        SYS_shmat,      SYS_shmdt, SYS_shmctl,
    SYS_mincore,      SYS_mbind,    SYS_set_mempolicy, SYS_get_mempolicy, SYS_migrate_pages, SYS_move_pages,
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
    SYS_mmap,
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    SYS_mmap2,
#else
#error "micron::sec::groups::memory: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct io {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_read,
    SYS_write,
    SYS_close,
    SYS_openat,
    SYS_pread64,
    SYS_pwrite64,
    SYS_readv,
    SYS_writev,
    SYS_preadv,
    SYS_pwritev,
    SYS_lseek,
    SYS_dup,
    SYS_dup3,
    SYS_fstat,
    SYS_statx,
    SYS_fcntl,
    SYS_ftruncate,
    SYS_truncate,
    SYS_fsync,
    SYS_fdatasync,
    SYS_sync,
    SYS_syncfs,
    SYS_fallocate,
    SYS_sendfile,
    SYS_splice,
    SYS_vmsplice,
    SYS_copy_file_range,
    SYS_readlinkat,
    SYS_unlinkat,
    SYS_renameat2,
    SYS_fchmod,
    SYS_fchmodat,
    SYS_fchown,
    SYS_fchownat,
    SYS_close_range,
#if defined(__micron_arch_amd64)
    SYS_open,
    SYS_stat,
    SYS_lstat,
    SYS_newfstatat,
    SYS_dup2,
    SYS_readlink,
    SYS_unlink,
    SYS_rename,
    SYS_renameat,
    SYS_chmod,
    SYS_chown,
    SYS_lchown,
    SYS_sync_file_range,
#elif defined(__micron_arch_x86)
    SYS_open,       SYS_stat64,          SYS_lstat64,         SYS_fstat64,    SYS_fstatat64, SYS_dup2,      SYS_readlink,    SYS_unlink,
    SYS_rename,     SYS_renameat,        SYS_chmod,           SYS_chown,      SYS_lchown,    SYS_fcntl64,   SYS_ftruncate64, SYS_truncate64,
    SYS_sendfile64, SYS_sync_file_range,
#elif defined(__micron_arch_arm32)
    SYS_open,       SYS_stat64,      SYS_lstat64,
    SYS_fstat64,    SYS_fstatat64,   SYS_dup2,
    SYS_readlink,   SYS_unlink,      SYS_rename,
    SYS_renameat,   SYS_chmod,       SYS_chown,
    SYS_lchown,     SYS_fcntl64,     SYS_ftruncate64,
    SYS_truncate64, SYS_sendfile64,  SYS_sync_file_range2,
#elif defined(__micron_arch_arm64)
    SYS_newfstatat, SYS_sync_file_range,
#else
#error "micron::sec::groups::io: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct filesystem {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_mount,      SYS_umount2,   SYS_chroot,   SYS_pivot_root, SYS_sync,       SYS_statfs,     SYS_fstatfs,    SYS_mknodat,
    SYS_linkat,     SYS_symlinkat, SYS_unlinkat, SYS_renameat2,  SYS_readlinkat, SYS_getdents64, SYS_mkdirat,    SYS_faccessat,
    SYS_faccessat2, SYS_fsopen,    SYS_fsconfig, SYS_fsmount,    SYS_fspick,     SYS_open_tree,  SYS_move_mount, SYS_mount_setattr,
#if defined(__micron_arch_amd64)
    SYS_mknod,      SYS_link,      SYS_symlink,  SYS_unlink,     SYS_rename,     SYS_renameat,   SYS_readlink,   SYS_getdents,
    SYS_mkdir,      SYS_rmdir,     SYS_access,
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    SYS_mknod,      SYS_link,      SYS_symlink,  SYS_unlink,     SYS_rename,     SYS_renameat,   SYS_readlink,   SYS_getdents,
    SYS_mkdir,      SYS_rmdir,     SYS_access,   SYS_statfs64,   SYS_fstatfs64,
#elif defined(__micron_arch_arm64)
  // asm-generic: only the *at forms exist
#else
#error "micron::sec::groups::filesystem: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct filesystem_readonly {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_sync,     SYS_statfs,   SYS_fstatfs, SYS_readlinkat, SYS_getdents64, SYS_faccessat, SYS_faccessat2,
#if defined(__micron_arch_amd64)
    SYS_readlink, SYS_getdents, SYS_access,
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    SYS_readlink, SYS_getdents, SYS_access,  SYS_statfs64,   SYS_fstatfs64,
#elif defined(__micron_arch_arm64)
  // asm-generic: only the *at forms exist
#else
#error "micron::sec::groups::filesystem_readonly: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct filesystem_no_mount {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_sync,      SYS_statfs,     SYS_fstatfs,    SYS_linkat,  SYS_symlinkat, SYS_unlinkat,
    SYS_renameat2, SYS_readlinkat, SYS_getdents64, SYS_mkdirat, SYS_faccessat, SYS_faccessat2,
#if defined(__micron_arch_amd64)
    SYS_link,      SYS_symlink,    SYS_unlink,     SYS_rename,  SYS_renameat,  SYS_readlink,
    SYS_getdents,  SYS_mkdir,      SYS_rmdir,      SYS_access,
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    SYS_link,      SYS_symlink,    SYS_unlink,     SYS_rename,  SYS_renameat,  SYS_readlink,
    SYS_getdents,  SYS_mkdir,      SYS_rmdir,      SYS_access,  SYS_statfs64,  SYS_fstatfs64,
#elif defined(__micron_arch_arm64)
  // asm-generic: only the *at forms exist
#else
#error "micron::sec::groups::filesystem_no_mount: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct process {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_pipe2,     SYS_restart_syscall, SYS_clone,    SYS_clone3,   SYS_execve, SYS_execveat, SYS_wait4,   SYS_waitid,
    SYS_getpid,    SYS_getppid,         SYS_getuid,   SYS_geteuid,  SYS_getgid, SYS_getegid,  SYS_setuid,  SYS_setgid,
    SYS_setresuid, SYS_setresgid,       SYS_setreuid, SYS_setregid, SYS_setsid, SYS_setpgid,  SYS_getpgid, SYS_getsid,
    SYS_setgroups, SYS_getgroups,       SYS_prctl,    SYS_unshare,  SYS_setns,
#if defined(__micron_arch_amd64)
    SYS_fork,      SYS_vfork,           SYS_pipe,
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    // the bare forms above are the legacy 16-bit-uid syscalls; system.hpp issues the *32 ones, so a
    // policy without these denies micron's own getuid()/setresuid()/... (see __sys_getuid)
    SYS_fork,      SYS_vfork,           SYS_pipe,        SYS_setuid32,    SYS_setgid32,
    SYS_setgroups32, SYS_getgroups32,   SYS_getuid32,    SYS_geteuid32,   SYS_getgid32,
    SYS_getegid32, SYS_setresuid32,     SYS_setresgid32, SYS_setreuid32,  SYS_setregid32,
#elif defined(__micron_arch_arm64)
  // asm-generic: clone/clone3 only, and pipe2 only
#else
#error "micron::sec::groups::process: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

// SYS_clone3 is absent and SYS_clone is present; clone3 passes a struct which bpf cannot read through; see CVE-2022-0185 and CVE-2026-63917/63921
//
// WARNING: micron's freestanding threads use clone3, enabling this means you can't use them, opt for groups:process and concede that it can create namespaces arbitrarily
struct process_no_ns {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_pipe2,    SYS_restart_syscall, SYS_clone,   SYS_execve,  SYS_execveat, SYS_wait4,     SYS_waitid,    SYS_getpid,    SYS_getppid,
    SYS_getuid,   SYS_geteuid,         SYS_getgid,  SYS_getegid, SYS_setuid,   SYS_setgid,    SYS_setresuid, SYS_setresgid, SYS_setreuid,
    SYS_setregid, SYS_setsid,          SYS_setpgid, SYS_getpgid, SYS_getsid,   SYS_setgroups, SYS_getgroups, SYS_prctl,
#if defined(__micron_arch_amd64)
    SYS_fork,     SYS_vfork,           SYS_pipe,
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    SYS_fork,      SYS_vfork,           SYS_pipe,      SYS_setuid32,    SYS_setgid32,    SYS_setgroups32, SYS_getgroups32, SYS_getuid32,
    SYS_geteuid32, SYS_getgid32,        SYS_getegid32, SYS_setresuid32, SYS_setresgid32, SYS_setreuid32,  SYS_setregid32,
#elif defined(__micron_arch_arm64)
  // asm-generic
#else
#error "micron::sec::groups::process_no_ns: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct signal {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_rt_sigaction,    SYS_rt_sigprocmask,    SYS_rt_sigreturn,      SYS_kill,          SYS_tgkill,
    SYS_tkill,           SYS_sigaltstack,       SYS_rt_sigsuspend,     SYS_rt_sigpending, SYS_rt_sigtimedwait,
    SYS_rt_sigqueueinfo, SYS_rt_tgsigqueueinfo, SYS_pidfd_send_signal,
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    SYS_rt_sigtimedwait_time64,
#else
#error "micron::sec::groups::signal: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

// %%%%%%%%%%%%%%
// network

struct network {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_socket,   SYS_connect,    SYS_accept4,    SYS_bind,        SYS_listen,      SYS_sendto,     SYS_recvfrom, SYS_sendmsg,  SYS_recvmsg,
    SYS_shutdown, SYS_setsockopt, SYS_getsockopt, SYS_getpeername, SYS_getsockname, SYS_socketpair, SYS_recvmmsg, SYS_sendmmsg,
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
    SYS_accept,
#elif defined(__micron_arch_arm32)
    SYS_accept,      SYS_recvmmsg_time64,
#elif defined(__micron_arch_x86)
    SYS_socketcall,  SYS_recvmmsg_time64,
#else
#error "micron::sec::groups::network: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct time {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_clock_gettime,    SYS_clock_getres,  SYS_clock_nanosleep, SYS_nanosleep,    SYS_gettimeofday,  SYS_settimeofday,
    SYS_adjtimex,         SYS_clock_adjtime, SYS_times,           SYS_timer_create, SYS_timer_settime, SYS_timer_gettime,
    SYS_timer_getoverrun, SYS_timer_delete,  SYS_getitimer,       SYS_setitimer,
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    SYS_clock_gettime64,  SYS_clock_getres_time64, SYS_clock_nanosleep_time64,
    SYS_clock_adjtime64,  SYS_timer_settime64,     SYS_timer_gettime64,
#else
#error "micron::sec::groups::time: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// ipc
//
// NOTE: i386 multiplexes sysv ipc through SYS_ipc, so it has no semop/semtimedop of its own

struct ipc {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_semget,      SYS_semctl,     SYS_msgget,         SYS_msgsnd,          SYS_msgrcv,          SYS_msgctl,        SYS_futex,
    SYS_eventfd2,    SYS_signalfd4,  SYS_timerfd_create, SYS_timerfd_settime, SYS_timerfd_gettime, SYS_epoll_create1, SYS_epoll_ctl,
    SYS_epoll_pwait, SYS_shmget,     SYS_shmat,          SYS_shmdt,           SYS_shmctl,
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
    SYS_semop,       SYS_semtimedop,
#elif defined(__micron_arch_arm32)
    SYS_semop, SYS_semtimedop, SYS_futex_time64, SYS_timerfd_settime64, SYS_timerfd_gettime64,
#elif defined(__micron_arch_x86)
    SYS_ipc, SYS_futex_time64, SYS_timerfd_settime64, SYS_timerfd_gettime64,
#else
#error "micron::sec::groups::ipc: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct capabilities {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_capget,    SYS_capset,    SYS_setuid,    SYS_setgid, SYS_setreuid, SYS_setregid,
    SYS_setresuid, SYS_setresgid, SYS_setgroups, SYS_prctl,  SYS_seccomp,  SYS_personality,
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
#elif defined(__micron_arch_x86) || defined(__micron_arch_arm32)
    // as in groups::process: micron issues the *32 forms, not the 16-bit ones named above
    SYS_setuid32,  SYS_setgid32,  SYS_setgroups32, SYS_setreuid32, SYS_setregid32,
    SYS_setresuid32, SYS_setresgid32, SYS_getuid32, SYS_geteuid32, SYS_getgid32, SYS_getegid32,
#else
#error "micron::sec::groups::capabilities: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

// WARNING: micron's own thread spawn issues clone3
struct namespaces {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_unshare,
    SYS_setns,
    SYS_clone3,
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct mount_api {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_fsopen, SYS_fsconfig, SYS_fsmount, SYS_fspick, SYS_open_tree, SYS_move_mount, SYS_mount_setattr,
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct keyring {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_keyctl,
    SYS_add_key,
    SYS_request_key,
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct kernel_debug {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_ptrace,      SYS_process_vm_readv, SYS_process_vm_writev, SYS_bpf,           SYS_perf_event_open,
    SYS_userfaultfd, SYS_init_module,      SYS_finit_module,      SYS_delete_module, SYS_kexec_load,
    SYS_quotactl,    SYS_swapon,           SYS_swapoff,           SYS_reboot,        SYS_pivot_root,
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

// WARNING: SECCOMP DOES NOT SEE IO_URING OPERATIONS
struct uring {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_io_uring_setup,
    SYS_io_uring_enter,
    SYS_io_uring_register,
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

// the union, for a caller who wants one name for "none of this"
struct kernel_attack_surface {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    // namespaces
    SYS_unshare,
    SYS_setns,
    SYS_clone3,
    // mount api
    SYS_fsopen,
    SYS_fsconfig,
    SYS_fsmount,
    SYS_fspick,
    SYS_open_tree,
    SYS_move_mount,
    SYS_mount_setattr,
    // keyring
    SYS_keyctl,
    SYS_add_key,
    SYS_request_key,
    // kernel debug / module / task introspection
    SYS_ptrace,
    SYS_process_vm_readv,
    SYS_process_vm_writev,
    SYS_bpf,
    SYS_perf_event_open,
    SYS_userfaultfd,
    SYS_init_module,
    SYS_finit_module,
    SYS_delete_module,
    SYS_kexec_load,
    SYS_quotactl,
    SYS_swapon,
    SYS_swapoff,
    SYS_reboot,
    SYS_pivot_root,
    // the legacy mount API too: this is the "none of this" group, so it means none of it
    SYS_mount,
    SYS_umount2,
    SYS_chroot,
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

struct io_multiplexing {
  using policy_tag = syscall_group_tag;
  static constexpr i32 calls[] = {
    SYS_pselect6,  SYS_ppoll,    SYS_epoll_create1,  SYS_epoll_ctl,       SYS_epoll_pwait,     SYS_epoll_pwait2,
    SYS_signalfd4, SYS_eventfd2, SYS_timerfd_create, SYS_timerfd_settime, SYS_timerfd_gettime,
#if defined(__micron_arch_amd64)
    SYS_select,    SYS_poll,     SYS_epoll_create,   SYS_epoll_wait,      SYS_signalfd,        SYS_eventfd,
#elif defined(__micron_arch_x86)
    SYS_select,    SYS__newselect,      SYS_poll,           SYS_epoll_create,      SYS_epoll_wait,        SYS_signalfd,
    SYS_eventfd,   SYS_pselect6_time64, SYS_ppoll_time64,   SYS_timerfd_settime64, SYS_timerfd_gettime64,
#elif defined(__micron_arch_arm32)
    SYS__newselect,      SYS_poll,         SYS_epoll_create,      SYS_epoll_wait,        SYS_signalfd,        SYS_eventfd,
    SYS_pselect6_time64, SYS_ppoll_time64, SYS_timerfd_settime64, SYS_timerfd_gettime64,
#elif defined(__micron_arch_arm64)
  // asm-generic: only the p*/create1/*2 forms exist
#else
#error "micron::sec::groups::io_multiplexing: unhandled architecture"
#endif
  };
  static constexpr usize count = sizeof(calls) / sizeof(calls[0]);
};

};      // namespace groups
};      // namespace sec
};      // namespace micron
