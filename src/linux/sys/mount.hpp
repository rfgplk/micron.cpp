//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"

#include "system.hpp"

namespace micron
{
namespace posix
{
inline i32
mount(const char *s, const char *t, const char *fs, unsigned long fl, const void *d)
{
  return static_cast<i32>(micron::syscall(SYS_mount, s, t, fs, fl, d));
}

inline i32
umount2(const char *t, i32 fl)
{
  return static_cast<i32>(micron::syscall(SYS_umount2, t, fl));
}

inline i32
umount(const char *t)
{
  // arm64/generic has no 1-arg umount; the portable form is umount2(target, 0)
  return static_cast<i32>(micron::syscall(SYS_umount2, t, 0));
}

inline i32
pivot_root(const char *nr, const char *po)
{
  return static_cast<i32>(micron::syscall(SYS_pivot_root, nr, po));
}

inline i32
unshare(i32 fl)
{
  return static_cast<i32>(micron::syscall(SYS_unshare, fl));
}

constexpr unsigned long ms_rdonly = 1uL;
constexpr unsigned long ms_nosuid = 2uL;
constexpr unsigned long ms_nodev = 4uL;
constexpr unsigned long ms_noexec = 8uL;
constexpr unsigned long ms_synchronous = 16uL;
constexpr unsigned long ms_remount = 32uL;
constexpr unsigned long ms_mandlock = 64uL;
constexpr unsigned long ms_dirsync = 128uL;
constexpr unsigned long ms_nosymfollow = 256uL;
constexpr unsigned long ms_noatime = 1024uL;
constexpr unsigned long ms_nodiratime = 2048uL;
constexpr unsigned long ms_bind = 4096uL;
constexpr unsigned long ms_move = 8192uL;
constexpr unsigned long ms_rec = 16384uL;
constexpr unsigned long ms_silent = 32768uL;
constexpr unsigned long ms_relatime = (1uL << 21);
constexpr unsigned long ms_strictatime = (1uL << 24);
constexpr unsigned long ms_lazytime = (1uL << 25);

// umount2() flags
constexpr i32 mnt_force = 1;
constexpr i32 mnt_detach = 2;
constexpr i32 mnt_expire = 4;
constexpr i32 umount_nofollow = 8;

constexpr unsigned long ms_unbindable = (1uL << 17);
constexpr unsigned long ms_private = (1uL << 18);
constexpr unsigned long ms_slave = (1uL << 19);
constexpr unsigned long ms_shared = (1uL << 20);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%
// mount_setattr (5.12+)
//
// WARNING: MS_REC is ignored by a MS_REMOUNT|MS_BIND mount()
constexpr u64 mount_attr_rdonly = 0x00000001;
constexpr u64 mount_attr_nosuid = 0x00000002;
constexpr u64 mount_attr_nodev = 0x00000004;
constexpr u64 mount_attr_noexec = 0x00000008;
constexpr u64 mount_attr__atime = 0x00000070;      // value, not a bitmask
constexpr u64 mount_attr_relatime = 0x00000000;
constexpr u64 mount_attr_noatime = 0x00000010;
constexpr u64 mount_attr_strictatime = 0x00000020;
constexpr u64 mount_attr_nodiratime = 0x00000080;
constexpr u64 mount_attr_idmap = 0x00100000;
constexpr u64 mount_attr_nosymfollow = 0x00200000;

struct mount_attr_t {
  u64 attr_set = 0;
  u64 attr_clr = 0;
  u64 propagation = 0;
  u64 userns_fd = 0;
};

inline i32
mount_setattr(i32 dfd, const char *path, u32 flags, mount_attr_t &attr)
{
  return static_cast<i32>(micron::syscall(SYS_mount_setattr, dfd, path, flags, &attr, sizeof(attr)));
}

// NOTE: the clone_new* flags live in sched.hpp; they are shared with clone/clone3 and unshare
};      // namespace posix
};      // namespace micron
