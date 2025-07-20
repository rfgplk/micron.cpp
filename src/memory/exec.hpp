#pragma once

#include "../errno.hpp"
#include "../syscall.hpp"

namespace micron
{

int
execve(const char *path, char *const argv[], char *const envp[])
{
  micro::syscall(SYS_execve, path, argv, envp);
  return 0;
}

int
execveat(int dirfd, const char *path, char *const argv[], char *const envp[], int flags)
{
  return (int)micron::syscall(SYS_execveat, dirfd, path, &argv[], &envp[0], flags);
}

int
fexecve(int fd, char *const argv[], char *const envp[])
{
  if ( fd < 0 || argv == NULL || envp == NULL ) {
    set_errno<error::invalid_arg>();
    return -1;
  }
  micro::syscall(SYS_execveat, 5, fd, "", &argv[0], &envp[0], 0x1000);     // AT_EMPTY_PATH
  return 0;
}

};
