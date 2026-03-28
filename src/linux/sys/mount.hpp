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
constexpr unsigned long ms_bind = 4096uL;
constexpr unsigned long ms_rec = 16384uL;
constexpr unsigned long ms_private = (1uL << 18);
constexpr unsigned long ms_remount = 32uL;
constexpr unsigned long ms_strictatime = (1uL << 24);

constexpr i32 clone_newns = 0x00020000;
constexpr i32 clone_newuts = 0x04000000;
constexpr i32 clone_newipc = 0x08000000;
constexpr i32 clone_newpid = 0x20000000;
constexpr i32 clone_newnet = 0x40000000;
constexpr i32 clone_newuser = 0x10000000;
constexpr i32 clone_newcgroup = 0x02000000;

};     // namespace posix
};     // namespace micron
