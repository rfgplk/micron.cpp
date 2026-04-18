//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../types.hpp"
#include "../sys/types.hpp"

#include "../io/io_structs.hpp"

#include "../../memory/cmemory/memcmp.hpp"

#include "stat_structs.hpp"

namespace micron
{
namespace posix
{
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstatat(posix::fd_t dirfd, const char *__restrict name, stat_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, dirfd.fd, name, &buf, flags));     // why?
}
#elif defined(__micron_arch_arm32)
auto
fstatat(posix::fd_t dirfd, const char *__restrict name, stat64_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstatat64, dirfd.fd, name, &buf, flags));     // why?
}
#endif
auto
fstat(posix::fd_t fd, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat, fd.fd, &buf));
}

#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
auto
fstatat(int dirfd, const char *__restrict name, stat_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_newfstatat, dirfd, name, &buf, flags));     // why?
}
#elif defined(__micron_arch_arm32)
auto
fstatat(int dirfd, const char *__restrict name, stat_t &__restrict buf, int flags) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstatat64, dirfd, name, &buf, flags));     // why?
}
#endif
auto
fstat(int fd, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_fstat, fd, &buf));
}

auto
lstat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_stat, path, &buf));
}

auto
stat(const char *path, stat_t &buf) -> i32
{
  return static_cast<i32>(micron::syscall(SYS_stat, path, &buf));
}
};     // namespace posix
};     // namespace micron
