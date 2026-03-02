//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/addr.hpp"

#include "../syscall.hpp"
#include "sys/fcntl.hpp"
#include "sys/stat.hpp"

#include "../types.hpp"
#include "sys/types.hpp"

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

constexpr i32 shutdown_reads = 0;
constexpr i32 shutdown_writes = 2;
constexpr i32 shutdown_rdwr = 2;

inline posix::dev_t
makedev(u32 major, u32 minor)
{
  return ((posix::dev_t)(major & 0xfff) << 8) | (posix::dev_t)(minor & 0xff) | ((posix::dev_t)(minor & 0xfff00) << 12);
}

inline u32
major(posix::dev_t dev)
{
  return (u32)((dev >> 8) & 0xfff);
}

inline u32
minor(posix::dev_t dev)
{
  return (u32)((dev & 0xff) | ((dev >> 12) & 0xfff00));
}

template <typename P>
ssize_t
read(i32 fd, P *buf, usize cnt)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_read, fd, micron::voidify(buf), cnt);
}

template <typename P>
ssize_t
write(i32 fd, P *buf, usize cnt)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return micron::syscall(SYS_write, fd, micron::voidify(buf), cnt);
}

i32
pipe(i32 *fd)
{
  return static_cast<i32>(micron::syscall(SYS_pipe, fd));
}

i32
pipe2(i32 *fd, i32 a)
{
  return static_cast<i32>(micron::syscall(SYS_pipe2, fd, a));
}

i32
dup(i32 old)
{
  return static_cast<i32>(micron::syscall(SYS_dup, old));
}

i32
dup2(i32 old, i32 newfd)
{
  return static_cast<i32>(micron::syscall(SYS_dup2, old, newfd));
}

i32
dup3(i32 old, i32 newfd, i32 flags)
{
  return static_cast<i32>(micron::syscall(SYS_dup2, old, newfd, flags));
}

auto
close(i32 fd)
{
  return static_cast<i32>(micron::syscall(SYS_close, fd));
}

auto
shutdown(i32 fd, i32 how)
{
  return static_cast<i32>(micron::syscall(SYS_shutdown, fd, how));
}

auto
open(const char *name, i32 flags, u32 mode)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return static_cast<i32>(micron::syscall(SYS_open, name, flags, mode));
}

auto
open(const char *name, i32 flags)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return static_cast<i32>(micron::syscall(SYS_open, name, flags, 0));
}

auto
openat(i32 dirfd, const char *pth, i32 flags, u32 mode [[maybe_unused]])
{
  return static_cast<i32>(micron::syscall(SYS_openat, dirfd, pth, flags));
}

auto
creat(const char *pth, u32 mode)
{
  //  NOTE : On Linux, read() (and similar system calls) will transfer at most 0x7ffff000 (2,147,479,552) bytes,
  //  returning
  //        the number of bytes actually transferred.  (This is true on both 32-bit and 64-bit systems.)
  return static_cast<i32>(micron::syscall(SYS_creat, pth, mode));
}

auto
umask(mode_t mask)
{
  return micron::syscall(SYS_umask, mask);
}

auto
sync(void)
{
  return micron::syscall(SYS_sync);
}

auto
fsync(i32 fd)
{
  return micron::syscall(SYS_fsync, fd);
}

auto
syncfs(i32 fd)
{
  return micron::syscall(SYS_syncfs, fd);
}

auto
fdatasync(i32 fd)
{
  return micron::syscall(SYS_fdatasync, fd);
}

auto
getcwd(char *buf, usize size)
{
  return micron::syscall(SYS_getcwd, buf, size);
}

auto
chdir(const char *path)
{
  return micron::syscall(SYS_chdir, path);
}

auto
fchdir(i32 fd)
{
  return micron::syscall(SYS_fchdir, fd);
}

auto
rmdir(const char *name)
{
  return micron::syscall(SYS_rmdir, name);
}

auto
chmod(const char *name, u32 mode)
{
  return micron::syscall(SYS_chmod, name, mode);
}

auto
fchmod(i32 fd, u32 mode)
{
  return micron::syscall(SYS_fchmod, fd, mode);
}

auto
chown(const char *name, uid_t owner, gid_t group)
{
  return micron::syscall(SYS_chown, name, owner, group);
}

auto
fchown(i32 fd, uid_t owner, gid_t group)
{
  return micron::syscall(SYS_fchown, fd, owner, group);
}

auto
lchown(const char *name, uid_t owner, gid_t group)
{
  return micron::syscall(SYS_fchown, name, owner, group);
}

auto
fchownat(i32 dirfd, const char *name, uid_t owner, gid_t group, i32 flags)
{
  return micron::syscall(SYS_fchown, dirfd, name, owner, group, flags);
}

auto
flock(i32 fd, i32 op)
{
  return micron::syscall(SYS_flock, fd, op);
}

auto
lseek(i32 fd, posix::off_t offset, i32 whence)
{
  return micron::syscall(SYS_lseek, fd, offset, whence);
}

auto
fallocate(i32 fd, i32 mode, posix::off_t offset, posix::off_t len)
{
  return micron::syscall(SYS_lseek, fd, mode, offset, len);
}

auto
rename(const char *__restrict oldpath, const char *__restrict newpath)
{
  return micron::syscall(SYS_rename, oldpath, newpath);
}

auto
renameat(i32 oldfd, const char *__restrict oldpath, i32 newfd, const char *__restrict newpath)
{
  return micron::syscall(SYS_rename, oldfd, oldpath, newfd, newpath);
}

auto
renameat2(i32 oldfd, const char *__restrict oldpath, i32 newfd, const char *__restrict newpath, i32 flags)
{
  return micron::syscall(SYS_rename, oldfd, oldpath, newfd, newpath, flags);
}

auto
access(const char *path, i32 mode)
{
  return micron::syscall(SYS_access, path, mode);
}

auto
faccessat(i32 dirfd, const char *name, i32 mode, i32 flags)
{
  return micron::syscall(SYS_faccessat2, dirfd, name, mode, flags);
}

auto
getdents(i32 dirfd, __linux_kernel_dirent &dirp, u32 count)
{
  return micron::syscall(SYS_getdents, dirfd, &dirp, count);
}

auto
getdents64(i32 dirfd, __linux_kernel_dirent64 &dirp, u32 count)
{
  return micron::syscall(SYS_getdents64, dirfd, &dirp, count);
}

auto
getdents64(i32 dirfd, void *dirp, u32 count)
{
  return micron::syscall(SYS_getdents64, dirfd, dirp, count);
}

i32
mkfifo(const char *path, posix::mode_t mode)
{
  return static_cast<i32>(micron::syscall(SYS_mknod, path, mode | S_IFIFO, 0));
}

i32
unlink(const char *path)
{
  return static_cast<i32>(micron::syscall(SYS_unlink, path));
}

};     // namespace micron
