//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// #include <sys/stat.h>

#include "../memory/addr.hpp"

#include "../linux/process/system.hpp"
#include "../syscall.hpp"
#include "sys/fcntl.hpp"
#include "sys/stat.hpp"

#include "linux_types.hpp"

namespace micron
{
using kernel_long_t = long long;
using kernel_ulong_t = unsigned long long;

struct __linux_kernel_dirent {
  unsigned long d_ino;     /* Inode number */
  unsigned long d_off;     /* Not an offset; see below */
  unsigned short d_reclen; /* Length of this linux_dirent */
  char d_name[256];        /* Filename (null-terminated) */
  /* length is actually (d_reclen - 2 -
     offsetof(struct linux_dirent, d_name)) */
  char pad;        // Zero padding byte
  char d_type;     // File type (only since Linux
                   // 2.6.4); offset is (d_reclen - 1)
};

struct __linux_kernel_dirent64 {
  posix::ino64_t d_ino;    /* 64-bit inode number */
  posix::off64_t d_off;    /* Not an offset; see getdents() */
  unsigned short d_reclen; /* Size of this dirent */
  unsigned char d_type;    /* File type */
  char d_name[256];        /* Filename (null-terminated) */
};

constexpr char f_ok = 0;
constexpr char x_ok = 1;
constexpr char w_ok = 2;
constexpr char r_ok = 4;

constexpr char seek_set = 0;
constexpr char seek_cur = 1;
constexpr char seek_end = 2;
constexpr char seek_data = 3;
constexpr char seek_hole = 4;

constexpr char dt_unknown = 0;
constexpr char dt_fifo = 1;
constexpr char dt_chr = 2;
constexpr char dt_dir = 4;
constexpr char dt_blk = 6;
constexpr char dt_reg = 8;
constexpr char dt_lnk = 10;
constexpr char dt_sock = 12;
constexpr char dt_why = 14;
constexpr char dt_end = 127;

inline posix::dev_t
makedev(unsigned int major, unsigned int minor)
{
  return ((posix::dev_t)(major & 0xfff) << 8) | (posix::dev_t)(minor & 0xff) | ((posix::dev_t)(minor & 0xfff00) << 12);
}

inline unsigned int
major(posix::dev_t dev)
{
  return (unsigned int)((dev >> 8) & 0xfff);
}

inline unsigned int
minor(posix::dev_t dev)
{
  return (unsigned int)((dev & 0xff) | ((dev >> 12) & 0xfff00));
}

template <typename P>
ssize_t
read(int fd, P *buf, size_t cnt)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_read, fd, micron::voidify(buf), cnt);
}
template <typename P>
ssize_t
write(int fd, P *buf, size_t cnt)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_write, fd, micron::voidify(buf), cnt);
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
openat(int dirfd, const char *pth, int flags, unsigned int mode [[maybe_unused]])
{
  return micron::syscall(SYS_openat, dirfd, pth, flags);
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
umask(mode_t mask)
{
  return micron::syscall(SYS_umask, mask);
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
chdir(const char *path)
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

long
fstatat(int dirfd, const char *__restrict name, stat_t &__restrict buf, int flags)
{
  return micron::syscall(SYS_newfstatat, dirfd, name, &buf, flags); // why?
}

long
fstat(int fd, stat_t &buf)
{
  return micron::syscall(SYS_fstat, fd, &buf);
}

long
lstat(const char *path, stat_t &buf)
{
  return micron::syscall(SYS_stat, path, &buf);
}

long
stat(const char *path, stat_t &buf)
{
  return micron::syscall(SYS_stat, path, &buf);
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
lseek(int fd, posix::off_t offset, int whence)
{
  return micron::syscall(SYS_lseek, fd, offset, whence);
}

auto
fallocate(int fd, int mode, posix::off_t offset, posix::off_t len)
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

auto
access(const char *path, int mode)
{
  return micron::syscall(SYS_access, path, mode);
}

auto
faccessat(int dirfd, const char *name, int mode, int flags)
{
  return micron::syscall(SYS_faccessat2, dirfd, name, mode, flags);
}

auto
getdents(int dirfd, __linux_kernel_dirent &dirp, unsigned int count)
{
  return micron::syscall(SYS_getdents, dirfd, &dirp, count);
}

auto
getdents64(int dirfd, __linux_kernel_dirent64 &dirp, unsigned int count)
{
  return micron::syscall(SYS_getdents64, dirfd, &dirp, count);
}

auto
getdents64(int dirfd, void *dirp, unsigned int count)
{
  return micron::syscall(SYS_getdents64, dirfd, dirp, count);
}

};
