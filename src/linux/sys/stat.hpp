
//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "../../types.hpp"
#include "../sys/types.hpp"

#include "../../memory/cmemory.hpp"

namespace micron
{
constexpr i32 __format_mask = 0170000; /* These bits determine file type.  */

/* File types.  */
constexpr i32 inode_dir = 0040000;  /* Directory.  */
constexpr i32 inode_char = 0020000; /* Character device.  */
constexpr i32 inode_blck = 0060000; /* Block device.  */
constexpr i32 inode_reg = 0100000;  /* Regular file.  */
constexpr i32 inode_fifo = 0010000; /* FIFO.  */
constexpr i32 inode_link = 0120000; /* Symbolic link.  */
constexpr i32 inode_sock = 0140000; /* Socket.  */

/* Protection bits.  */

constexpr i32 s_isuid = 04000; /* Set user ID on execution.  */
constexpr i32 s_isgid = 02000; /* Set group ID on execution.  */
constexpr i32 s_isvtx = 01000; /* Save swapped text after use (sticky).  */
constexpr i32 s_iread = 0400;  /* Read by owner.  */
constexpr i32 s_iwrite = 0200; /* Write by owner.  */
constexpr i32 s_iexec = 0100;  /* Execute by owner.  */

constexpr i32 s_irusr = s_iread;  /* Read by owner.  */
constexpr i32 s_iwusr = s_iwrite; /* Write by owner.  */
constexpr i32 s_ixusr = s_iexec;  /* Execute by owner.  */
/* Read, write, and execute by owner.  */
constexpr i32 s_irwxu = (s_iread | s_iwrite | s_iexec);

constexpr i32 s_irgrp = (s_irusr >> 3); /* Read by group.  */
constexpr i32 s_iwgrp = (s_iwusr >> 3); /* Write by group.  */
constexpr i32 s_ixgrp = (s_ixusr >> 3); /* Execute by group.  */
/* Read, write, and execute by group.  */
constexpr i32 s_irwxg = (s_irwxu >> 3);

constexpr i32 s_iroth = (s_irgrp >> 3); /* Read by others.  */
constexpr i32 s_iwoth = (s_iwgrp >> 3); /* Write by others.  */
constexpr i32 s_ixoth = (s_ixgrp >> 3); /* Execute by others.  */
/* Read, write, and execute by others.  */
constexpr i32 s_irwxo = (s_irwxg >> 3);

// legacy consts, keeping them around for backwards compatibility

constexpr i32 S_IFMT = 0170000; /* These bits determine file type.  */

/* File types.  */
constexpr i32 S_IFDIR = 0040000;  /* Directory.  */
constexpr i32 S_IFCHR = 0020000;  /* Character device.  */
constexpr i32 S_IFBLK = 0060000;  /* Block device.  */
constexpr i32 S_IFREG = 0100000;  /* Regular file.  */
constexpr i32 S_IFIFO = 0010000;  /* FIFO.  */
constexpr i32 S_IFLNK = 0120000;  /* Symbolic link.  */
constexpr i32 S_IFSOCK = 0140000; /* Socket.  */

/* Protection bits.  */

constexpr i32 S_ISUID = 04000; /* Set user ID on execution.  */
constexpr i32 S_ISGID = 02000; /* Set group ID on execution.  */
constexpr i32 S_ISVTX = 01000; /* Save swapped text after use (sticky).  */
constexpr i32 S_IREAD = 0400;  /* Read by owner.  */
constexpr i32 S_IWRITE = 0200; /* Write by owner.  */
constexpr i32 S_IEXEC = 0100;  /* Execute by owner.  */

constexpr i32 S_IRUSR = S_IREAD;  /* Read by owner.  */
constexpr i32 S_IWUSR = S_IWRITE; /* Write by owner.  */
constexpr i32 S_IXUSR = S_IEXEC;  /* Execute by owner.  */
/* Read, write, and execute by owner.  */
constexpr i32 S_IRWXU = (S_IREAD | S_IWRITE | S_IEXEC);

constexpr i32 S_IRGRP = (S_IRUSR >> 3); /* Read by group.  */
constexpr i32 S_IWGRP = (S_IWUSR >> 3); /* Write by group.  */
constexpr i32 S_IXGRP = (S_IXUSR >> 3); /* Execute by group.  */
/* Read, write, and execute by group.  */
constexpr i32 S_IRWXG = (S_IRWXU >> 3);

constexpr i32 S_IROTH = (S_IRGRP >> 3); /* Read by others.  */
constexpr i32 S_IWOTH = (S_IWGRP >> 3); /* Write by others.  */
constexpr i32 S_IXOTH = (S_IXGRP >> 3); /* Execute by others.  */
/* Read, write, and execute by others.  */
constexpr i32 S_IRWXO = (S_IRWXG >> 3);

#if __wordsize == 64
struct stat_t {
  posix::dev_t st_dev;     /* Device.  */
  posix::ino64_t st_ino;   /* File serial number.  */
  posix::nlink_t st_nlink; /* Link count.  */

  posix::mode_t st_mode; /* File mode.  */
  posix::uid_t st_uid;   /* User ID of the file’s owner.  */
  posix::gid_t st_gid;   /* Group ID of the file’s group. */
  i32 __pad0;
  posix::dev_t st_rdev;        /* Device number, if device.  */
  posix::off64_t st_size;      /* Size of file, in bytes.  */
  posix::blksize_t st_blksize; /* Optimal block size for I/O.  */

  posix::blkcnt_t st_blocks; /* Number of 512-byte blocks allocated. */

  struct timespec st_atim;      /* Time of last access.  */
  struct timespec st_mtim;      /* Time of last modification.  */
  struct timespec st_ctim;      /* Time of last status change.  */
#define st_atime st_atim.tv_sec /* Backward compatibility.  */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec

  __syscall_slong_t __glibc_reserved[3];

  bool
  operator!=(const stat_t &o) const
  {
    return micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t *>(&st_blksize) - (&st_dev));
  }

  bool
  operator==(const stat_t &o) const
  {
    return !micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t *>(&st_blksize) - (&st_dev));
  }
};
#elif __wordsize == 32
// ARCH
struct stat {
  dev_t st_dev;
  unsigned short int __pad1;
  ino_t __st_ino;
  mode_t st_mode;
  nlink_t st_nlink;
  uid_t st_uid;
  gid_t st_gid;
  dev_t st_rdev;
  unsigned short int __pad2;
  off_t st_size;
  blksize_t st_blksize;
  blkcnt_t st_blocks;
  struct timespec st_atim;
  struct timespec st_mtim;
  struct timespec st_ctim;
  unsigned long int __glibc_reserved4;
  unsigned long int __glibc_reserved5;

  bool
  operator!=(const stat_t &o) const
  {
    return micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t *>(&st_blksize) - (&st_dev));
  }

  bool
  operator==(const stat_t &o) const
  {
    return !micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t *>(&st_blksize) - (&st_dev));
  }
};

struct stat64 {
  dev_t st_dev;
  unsigned short int __pad1;
  ino_t __st_ino;
  mode_t st_mode;
  nlink_t st_nlink;
  uid_t st_uid;
  gid_t st_gid;
  dev_t st_rdev;
  unsigned short int __pad2;
  off64_t st_size;
  blksize_t st_blksize;
  blkcnt64_t st_blocks;
  struct timespec st_atim;
  struct timespec st_mtim;
  struct timespec st_ctim;
  ino64_t st_ino;

  bool
  operator!=(const stat_t &o) const
  {
    return micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t *>(&st_blksize) - (&st_dev));
  }

  bool
  operator==(const stat_t &o) const
  {
    return !micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t *>(&st_blksize) - (&st_dev));
  }
};
#endif

long
fstatat(int dirfd, const char *__restrict name, stat_t &__restrict buf, int flags)
{
  return micron::syscall(SYS_newfstatat, dirfd, name, &buf, flags);     // why?
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

};     // namespace micron
