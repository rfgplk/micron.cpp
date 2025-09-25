#pragma once

#include "../../types.hpp"
#include "../linux_types.hpp"

#include "../../memory/cmemory.hpp"

namespace micron
{
// constexpr int seek_set = 0; /* Seek from beginning of file.  */
// constexpr int seek_cur = 1; /* Seek from current position.  */
// constexpr int seek_end = 2; /* Seek from end of file.  */

constexpr int S_IFMT = 0170000; /* These bits determine file type.  */

/* File types.  */
constexpr int S_IFDIR = 0040000;  /* Directory.  */
constexpr int S_IFCHR = 0020000;  /* Character device.  */
constexpr int S_IFBLK = 0060000;  /* Block device.  */
constexpr int S_IFREG = 0100000;  /* Regular file.  */
constexpr int S_IFIFO = 0010000;  /* FIFO.  */
constexpr int S_IFLNK = 0120000;  /* Symbolic link.  */
constexpr int S_IFSOCK = 0140000; /* Socket.  */

/* Protection bits.  */

constexpr int S_ISUID = 04000; /* Set user ID on execution.  */
constexpr int S_ISGID = 02000; /* Set group ID on execution.  */
constexpr int S_ISVTX = 01000; /* Save swapped text after use (sticky).  */
constexpr int S_IREAD = 0400;  /* Read by owner.  */
constexpr int S_IWRITE = 0200; /* Write by owner.  */
constexpr int S_IEXEC = 0100;  /* Execute by owner.  */

constexpr int S_IRUSR = S_IREAD;  /* Read by owner.  */
constexpr int S_IWUSR = S_IWRITE; /* Write by owner.  */
constexpr int S_IXUSR = S_IEXEC;  /* Execute by owner.  */
/* Read, write, and execute by owner.  */
constexpr int S_IRWXU = (S_IREAD | S_IWRITE | S_IEXEC);

constexpr int S_IRGRP = (S_IRUSR >> 3); /* Read by group.  */
constexpr int S_IWGRP = (S_IWUSR >> 3); /* Write by group.  */
constexpr int S_IXGRP = (S_IXUSR >> 3); /* Execute by group.  */
/* Read, write, and execute by group.  */
constexpr int S_IRWXG = (S_IRWXU >> 3);

constexpr int S_IROTH = (S_IRGRP >> 3); /* Read by others.  */
constexpr int S_IWOTH = (S_IWGRP >> 3); /* Write by others.  */
constexpr int S_IXOTH = (S_IXGRP >> 3); /* Execute by others.  */
/* Read, write, and execute by others.  */
constexpr int S_IRWXO = (S_IRWXG >> 3);

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
    return micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t*>(&st_blksize) - reinterpret_cast<const addr_t*>(&st_dev));
  }
  bool
  operator==(const stat_t &o) const
  {
    return !micron::memcmp<byte>(this, &o, reinterpret_cast<const addr_t*>(&st_blksize) - reinterpret_cast<const addr_t*>(&st_dev));
  }
};

};
