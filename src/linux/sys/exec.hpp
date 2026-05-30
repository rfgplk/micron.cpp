//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../errno.hpp"
#include "../../syscall.hpp"

namespace micron
{
namespace posix
{
int
execve(const char *path, char *const *argv, char *const *envp)
{
  return static_cast<int>(micron::syscall(SYS_execve, path, argv, envp));
}

int
execveat(int dirfd, const char *path, char *const argv[], char *const envp[], int flags)
{
  return (int)micron::syscall(SYS_execveat, dirfd, path, &argv[0], &envp[0], flags);
}

int
fexecve(int fd, char *const argv[], char *const envp[])
{
  if ( fd < 0 || argv == nullptr || envp == nullptr ) return -error::invalid_arg;
  return static_cast<int>(micron::syscall(SYS_execveat, fd, "", &argv[0], &envp[0], 0x1000));      // AT_EMPTY_PATH
}

};      // namespace posix
};      // namespace micron
