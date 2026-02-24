//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"
#include "../sys/types.hpp"

namespace micron
{

template <typename... Args>
auto
fcntl(int fd, int cmd, Args &&...args)
{
  return micron::syscall(SYS_fcntl, fd, cmd, args...);
}

struct flock_t {
  i16 l_type;
  i16 l_whence;
  posix::off64_t l_start;
  posix::off64_t l_end;
  posix::pid_t l_pid;
};

constexpr i32 at_fdcwd = -100;
constexpr i32 at_symlink_nofollow = 0x100;
constexpr i32 at_removedir = 0x200;
constexpr i32 at_symlink_follow = 0x400;
constexpr i32 at_no_automount = 0x800;
constexpr i32 at_empty_path = 0x1000;
constexpr i32 at_statx_sync_type = 0x6000;
constexpr i32 at_statx_sync_as_stat = 0x0000;
constexpr i32 at_statx_force_sync = 0x2000;
constexpr i32 at_statx_dont_sync = 0x4000;
constexpr i32 at_recursive = 0x9000;
constexpr i32 at_eaccess = 0x200;

constexpr i32 read_ok = 4;
constexpr i32 write_ok = 2;
constexpr i32 exe_ok = 1;
constexpr i32 execute_ok = 1;
constexpr i32 file_ok = 0;
constexpr i32 access_ok = 0;

constexpr i32 o_largefile = 00;
constexpr i32 o_accmode = 03;
constexpr i32 o_rdonly = 00;
constexpr i32 o_wronly = 01;
constexpr i32 o_rdwr = 02;
constexpr i32 o_create = 0100;
constexpr i32 o_excl = 0200;
constexpr i32 o_noctty = 0400;
constexpr i32 o_trunc = 01000;
constexpr i32 o_append = 02000;
constexpr i32 o_nonblock = 04000;
constexpr i32 o_ndelay = o_nonblock;
constexpr i32 o_sync = 04010000;
constexpr i32 o_fsync = o_sync;
constexpr i32 o_async = 020000;
constexpr i32 o_directory = 0200000;
constexpr i32 o_nofollow = 0400000;
constexpr i32 o_cloexec = 02000000;
constexpr i32 o_direct = 040000;
constexpr i32 o_noatime = 01000000;
constexpr i32 o_path = 010000000;
constexpr i32 o_dsync = 010000;

constexpr i32 f_dupfd = 0;
constexpr i32 f_getfd = 1;
constexpr i32 f_setfd = 2;
constexpr i32 f_getfl = 3;
constexpr i32 f_setfl = 4;
constexpr i32 f_setown = 8;
constexpr i32 f_getown = 9;
constexpr i32 f_setsig = 10;
constexpr i32 f_getsig = 11;
constexpr i32 f_setown_ex = 15;
constexpr i32 f_getown_ex = 16;

};     // namespace micron
