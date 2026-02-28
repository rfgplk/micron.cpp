//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../string/sstring.hpp"

#include "../../linux/io.hpp"
#include "../bits.hpp"

#include "../../linux/sys/limits.hpp"
#include "bits.hpp"

namespace micron
{
namespace posix
{

constexpr static const int fifo = S_IFIFO;
constexpr static const int chr = S_IFCHR;
constexpr static const int dir = S_IFDIR;
constexpr static const int blk = S_IFBLK;
constexpr static const int reg = S_IFREG;
constexpr static const int lnk = S_IFLNK;
constexpr static const int socket = S_IFSOCK;
constexpr static const int mask = S_IFMT;

// propagate from io.hpp into ::posix
using micron::access;
using micron::chdir;
using micron::chmod;
using micron::chown;
using micron::close;
using micron::faccessat;
using micron::fallocate;
using micron::fchdir;
using micron::fchmod;
using micron::fchownat;
using micron::fdatasync;
using micron::flock;
using micron::fstat;
using micron::fstatat;
using micron::fsync;
using micron::getcwd;
using micron::lchown;
using micron::lseek;
using micron::lstat;
using micron::open;
using micron::openat;
using micron::pipe;
using micron::read;
using micron::rename;
using micron::renameat;
using micron::renameat2;
using micron::rmdir;
using micron::stat;
using micron::syncfs;
using micron::umask;
using micron::write;

// in posix now

// old glibc dirent, entry, obsolete
struct __dirent {
  ino_t d_ino;                  /* Inode number */
  off_t d_off;                  /* Not an offset; see below */
  unsigned short d_reclen;      /* Length of this record */
  unsigned char d_type;         /* Type of file; not supported
                                   by all filesystem types */
  char d_name[posix::path_max]; /* Null-terminated filename */
};

struct __impl_dir {
  micron::sstring<256> d_name;
  unsigned char type;
  ino_t i_no;
};

dir_t
opendir(const char *path)
{
  dir_t r((micron::openat(at_fdcwd, path, o_rdonly | o_directory | o_cloexec, 0)));
  return r;
}

int
closedir(const io::fd_t &ds)
{
  return micron::close(ds.fd);
}

__impl_dir
readdir(const io::fd_t &_f)
{
  static char __readdir_buf[8192];
  static usize __readdir_bufpos = 0;
  static usize __readdir_nread = 0;
  if ( __readdir_bufpos >= __readdir_nread ) {
    __readdir_nread = micron::getdents64(_f.fd, &__readdir_buf, sizeof(__readdir_buf));
    __readdir_bufpos = 0;
    if ( __readdir_nread <= 0 ) {
      return __impl_dir{ "", dt_end, 0 };
    }
  }
  __linux_kernel_dirent64 *p = reinterpret_cast<__linux_kernel_dirent64 *>(__readdir_buf + __readdir_bufpos);
  __readdir_bufpos += p->d_reclen;
  return { p->d_name, p->d_type, p->d_ino };
}

}     // namespace posix
};     // namespace micron
