//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/addr.hpp"
#include "../../string/sstring.hpp"
#include "../../syscall.hpp"
#include "../../types.hpp"
#include "../sys/fcntl.hpp"
#include "../sys/types.hpp"

#include "../sys/stat_structs.hpp"

#include "../sys/limits.hpp"

#include "fd.hpp"

//%%%%%%%%%%%%%%%%%%%%%%%%
// io_structs
// syscalls DON'T go in here

namespace micron
{
namespace posix
{

constexpr static const i32 ififo = inode_fifo;
constexpr static const i32 chr = inode_char;
constexpr static const i32 dir = inode_dir;
constexpr static const i32 blk = inode_blck;
constexpr static const i32 reg = inode_reg;
constexpr static const i32 lnk = inode_link;
constexpr static const i32 scket = inode_sock;
constexpr static const i32 mask = __format_mask;

constexpr byte f_ok = 0;
constexpr byte x_ok = 1;
constexpr byte w_ok = 2;
constexpr byte r_ok = 4;

constexpr byte seek_set = 0;
constexpr byte seek_cur = 1;
constexpr byte seek_end = 2;
constexpr byte seek_data = 3;
constexpr byte seek_hole = 4;

constexpr byte dt_unknown = 0;
constexpr byte dt_fifo = 1;
constexpr byte dt_chr = 2;
constexpr byte dt_dir = 4;
constexpr byte dt_blk = 6;
constexpr byte dt_reg = 8;
constexpr byte dt_lnk = 10;
constexpr byte dt_sock = 12;
constexpr byte dt_why = 14;
constexpr byte dt_end = 127;

constexpr i32 shutdown_reads = 0;
constexpr i32 shutdown_writes = 1;
constexpr i32 shutdown_rdwr = 2;

constexpr u32 mode_irwxu = 0700;
constexpr u32 mode_irusr = 0400;
constexpr u32 mode_iwusr = 0200;
constexpr u32 mode_ixusr = 0100;
constexpr u32 mode_irwxg = 070;
constexpr u32 mode_irgrp = 040;
constexpr u32 mode_iwgrp = 020;
constexpr u32 mode_ixgrp = 010;
constexpr u32 mode_irwxo = 07;
constexpr u32 mode_iroth = 04;
constexpr u32 mode_iwoth = 02;
constexpr u32 mode_ixoth = 01;
constexpr u32 mode_isuid = 04000;
constexpr u32 mode_isgid = 02000;
constexpr u32 mode_isvtx = 01000;
constexpr u32 mode_file = 0644;     // rw-r--r--
constexpr u32 mode_exec = 0755;     // rwxr-xr-x
constexpr u32 mode_dir = 0755;      // rwxr-xr-x

template <typename T> struct iovec_t {
  T *iov_base;
  usize iov_len;
};

struct __linux_kernel_dirent {
  kernel_ino_t d_ino;
  kernel_off_t d_off;
  u16 d_reclen;
  char d_name[256];
  i8 pad;
  i8 d_type;
};

struct __linux_kernel_dirent64 {
  posix::ino64_t d_ino;
  posix::off64_t d_off;
  u16 d_reclen;
  u8 d_type;
  char d_name[256];
};

// legacy glibc-style dirent
struct __dirent {
  kernel_ino_t d_ino;
  kernel_off_t d_off;
  u16 d_reclen;
  u8 d_type;
  char d_name[posix::path_max];
};

struct __impl_dir {
  micron::sstring<name_max + 1> d_name;
  u8 type;
  ino_t i_no;

  bool
  is_reg() const
  {
    return type == dt_reg;
  }

  bool
  is_dir() const
  {
    return type == dt_dir;
  }

  bool
  is_lnk() const
  {
    return type == dt_lnk;
  }

  bool
  is_fifo() const
  {
    return type == dt_fifo;
  }

  bool
  is_sock() const
  {
    return type == dt_sock;
  }

  bool
  is_chr() const
  {
    return type == dt_chr;
  }

  bool
  is_blk() const
  {
    return type == dt_blk;
  }

  bool
  at_end() const
  {
    return type == dt_end;
  }
};
};     // namespace posix
};     // namespace micron
