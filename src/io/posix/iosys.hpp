//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include <sys/stat.h>

#include "../../syscall.hpp"
#include "../../linux/process/system.hpp"

// #include <sys/syscall.h>

namespace micron
{
namespace posix
{
using kernel_long_t = long long;
using kernel_ulong_t = unsigned long long;

template <typename P>
auto
read(int fd, P *buf, size_t cnt)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_read, fd, reinterpret_cast<void *>(buf), cnt);
}
template <typename P>
auto
write(int fd, P *buf, size_t cnt)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_write, fd, reinterpret_cast<void *>(buf), cnt);
}

auto
close(int fd)
{
  return micron::syscall(SYS_close, fd);
}

auto
open(const char *name, int flags, unsigned int mode)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_open, name, flags, mode);
}

auto
open(const char *name, int flags)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_open, name, flags, 0);
}

auto
openat(int dirfd, const char *pth, unsigned int mode)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_openat, dirfd, pth, mode);
}

auto
creat(const char *pth, unsigned int mode)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_creat, pth, mode);
}

auto
fsync(int fd)
{
  return micron::syscall(SYS_fsync, fd);
}

auto
syncfs(int fd)
{
  return micron::syscall(SYS_syncfs, fd);
}
auto
fdatasync(int fd)
{
  return micron::syscall(SYS_fdatasync, fd);
}

auto
getcwd(char *buf, size_t size)
{
  return micron::syscall(SYS_getcwd, buf, size);
}

auto
chdir(const char* path)
{
  return micron::syscall(SYS_chdir, path);
}

auto
fchdir(int fd)
{
  return micron::syscall(SYS_fchdir, fd);
}

auto
rmdir(const char *name)
{
  return micron::syscall(SYS_rmdir, name);
}

auto
chmod(const char *name, unsigned int mode)
{
  return micron::syscall(SYS_chmod, name, mode);
}

auto
fchmod(int fd, unsigned int mode)
{
  return micron::syscall(SYS_fchmod, fd, mode);
}

auto
fstatat(const char *__restrict name, struct stat &__restrict buf)
{
  return micron::syscall(SYS_stat, name, &buf);
}

auto
fstat(int fd, struct stat &buf)
{
  return micron::syscall(SYS_fstat, fd, &buf);
}

auto
chown(const char *name, uid_t owner, gid_t group)
{
  return micron::syscall(SYS_chown, name, owner, group);
}

auto
fchown(int fd, uid_t owner, gid_t group)
{
  return micron::syscall(SYS_fchown, fd, owner, group);
}

auto
lchown(const char *name, uid_t owner, gid_t group)
{
  return micron::syscall(SYS_fchown, name, owner, group);
}

auto
fchownat(int dirfd, const char *name, uid_t owner, gid_t group, int flags)
{
  return micron::syscall(SYS_fchown, dirfd, name, owner, group, flags);
}

auto
flock(int fd, int op)
{
  return micron::syscall(SYS_flock, fd, op);
}

auto
lseek(int fd, off_t offset, int whence)
{
  return micron::syscall(SYS_lseek, fd, offset, whence);
}

auto
fallocate(int fd, int mode, off_t offset, off_t len)
{
  return micron::syscall(SYS_lseek, fd, mode, offset, len);
}

auto
rename(const char *__restrict oldpath, const char *__restrict newpath)
{
  return micron::syscall(SYS_rename, oldpath, newpath);
}

auto
renameat(int oldfd, const char *__restrict oldpath, int newfd, const char *__restrict newpath)
{
  return micron::syscall(SYS_rename, oldfd, oldpath, newfd, newpath);
}

auto
renameat2(int oldfd, const char *__restrict oldpath, int newfd, const char *__restrict newpath, int flags)
{
  return micron::syscall(SYS_rename, oldfd, oldpath, newfd, newpath, flags);
}

constexpr static const int access_ok = 0;
constexpr static const int execute_ok = 1;
constexpr static const int write_ok = 2;
constexpr static const int read_ok = 4;

auto
access(const char* path, int mode)
{
  return micron::syscall(SYS_access, path, mode);
}

auto
faccessat(int dirfd, const char* name, int mode, int flags)
{
  return micron::syscall(SYS_faccessat2, dirfd, name, mode, flags);
}
};
};
