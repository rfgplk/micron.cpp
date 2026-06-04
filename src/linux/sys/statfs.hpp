//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../syscall.hpp"
#include "../../types.hpp"
#include "types.hpp"

namespace micron
{
namespace posix
{

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
struct statfs_t {
  i64 f_type;
  i64 f_bsize;
  u64 f_blocks;
  u64 f_bfree;
  u64 f_bavail;
  u64 f_files;
  u64 f_ffree;
  i32 f_fsid[2];
  i64 f_namelen;
  i64 f_frsize;
  i64 f_flags;
  i64 f_spare[4];
};

auto
statfs(const char *path, statfs_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_statfs, path, &buf));
}

auto
fstatfs(i32 fd, statfs_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstatfs, fd, &buf));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
#if defined(__micron_arch_arm32)
struct __attribute__((packed, aligned(4))) statfs_t {      // kernel struct statfs64 (ARM)
#else
struct statfs_t {      // kernel struct statfs64 (i386)
#endif
  u32 f_type;
  u32 f_bsize;
  u64 f_blocks;
  u64 f_bfree;
  u64 f_bavail;
  u64 f_files;
  u64 f_ffree;
  i32 f_fsid[2];
  u32 f_namelen;
  u32 f_frsize;
  u32 f_flags;
  u32 f_spare[4];
};

auto
statfs(const char *path, statfs_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_statfs64, path, sizeof(buf), &buf));
}

auto
fstatfs(i32 fd, statfs_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstatfs64, fd, sizeof(buf), &buf));
}
#endif

// f_flags bits (ST_*)
constexpr u64 st_rdonly = 0x0001;
constexpr u64 st_nosuid = 0x0002;
constexpr u64 st_nodev = 0x0004;
constexpr u64 st_noexec = 0x0008;
constexpr u64 st_synchronous = 0x0010;
constexpr u64 st_mandlock = 0x0040;
constexpr u64 st_noatime = 0x0400;
constexpr u64 st_nodiratime = 0x0800;
constexpr u64 st_relatime = 0x1000;

};      // namespace posix
};      // namespace micron
