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
  micron::syscall(SYS_execve, path, argv, envp);
  return 0;
}

int
execveat(int dirfd, const char *path, char *const argv[], char *const envp[], int flags)
{
  return (int)micron::syscall(SYS_execveat, dirfd, path, &argv[0], &envp[0], flags);
}

int
fexecve(int fd, char *const argv[], char *const envp[])
{
  if ( fd < 0 || argv == NULL || envp == NULL ) {
    set_errno<error::invalid_arg>();
    return -1;
  }
  micron::syscall(SYS_execveat, 5, fd, "", &argv[0], &envp[0], 0x1000);     // AT_EMPTY_PATH
  return 0;
}

};     // namespace posix
};     // namespace micron
