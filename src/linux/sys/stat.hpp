//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../bits/__arch.hpp"
#include "../../types.hpp"
#include "../sys/types.hpp"

#include "../io/io_structs.hpp"

#include "../../memory/cmemory/memcmp.hpp"

#include "fcntl.hpp"
#include "stat_structs.hpp"

namespace micron
{
namespace posix
{
// stat dispatchx:
//   amd64     -> SYS_stat / SYS_lstat / SYS_fstat / SYS_newfstatat    -> 64-bit native stat_t
//   arm64     -> SYS_fstat / SYS_newfstatat (no stat/lstat in asm-gen)-> 64-bit stat_t
//   arm32/x86 -> SYS_stat64 / SYS_lstat64 / SYS_fstat64 / SYS_fstatat64 -> stat64_t (32-bit LFS)
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstatat(posix::fd_t dirfd, const char *__restrict name, stat_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, dirfd.fd, name, &buf, flags));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstatat(posix::fd_t dirfd, const char *__restrict name, stat64_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstatat64, dirfd.fd, name, &buf, flags));
}
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstatat(int dirfd, const char *__restrict name, stat_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, dirfd, name, &buf, flags));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstatat(int dirfd, const char *__restrict name, stat64_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstatat64, dirfd, name, &buf, flags));
}
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstat(posix::fd_t fd, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat, fd.fd, &buf));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstat(posix::fd_t fd, stat64_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat64, fd.fd, &buf));
}
#endif

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstat(int fd, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat, fd, &buf));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
fstat(int fd, stat64_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat64, fd, &buf));
}
#endif

#if defined(__micron_arch_amd64)
auto
lstat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_lstat, path, &buf));
}
#elif defined(__micron_arch_arm64)
auto
lstat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, at_fdcwd, path, &buf, at_symlink_nofollow));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
lstat(const char *path, stat64_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_lstat64, path, &buf));
}
#endif

#if defined(__micron_arch_amd64)
auto
stat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_stat, path, &buf));
}
#elif defined(__micron_arch_arm64)
auto
stat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, at_fdcwd, path, &buf, 0));
}
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
auto
stat(const char *path, stat64_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_stat64, path, &buf));
}
#endif
};     // namespace posix
};     // namespace micron
