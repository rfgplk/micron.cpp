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
constexpr byte dt_wht = 14;
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
constexpr u32 mode_file = 0644;      // rw-r--r--
constexpr u32 mode_exec = 0755;      // rwxr-xr-x
constexpr u32 mode_dir = 0755;       // rwxr-xr-x

template<typename T> struct iovec_t {
  T *iov_base;
  usize iov_len;
};

struct __linux_kernel_dirent {
  kernel_ino_t d_ino;
  kernel_off_t d_off;
  u16 d_reclen;
  char d_name[256];      // flexible; the d_type byte lives at offset d_reclen-1
};

struct __linux_kernel_dirent64 {
  posix::ino64_t d_ino;
  posix::off64_t d_off;
  u16 d_reclen;
  u8 d_type;
  char d_name[256];
};

inline u16
__dirent64_validate(const void *buf, usize pos, usize nread, const __linux_kernel_dirent64 *&out) noexcept
{
  constexpr usize hdr = __builtin_offsetof(__linux_kernel_dirent64, d_name);
  if ( pos >= nread || pos + hdr > nread ) return 0;
  out = reinterpret_cast<const __linux_kernel_dirent64 *>(static_cast<const char *>(buf) + pos);
  const u16 reclen = out->d_reclen;
  if ( reclen <= hdr ) return 0;
  if ( (reclen % alignof(__linux_kernel_dirent64)) != 0 ) return 0;
  if ( pos + reclen > nread ) return 0;
  const usize navail = reclen - hdr;
  const usize scan = navail < (name_max + 1) ? navail : (name_max + 1);
  for ( usize i = 0; i < scan; ++i )
    if ( out->d_name[i] == '\0' ) return reclen;
  return 0;
}

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
  ino64_t i_no;      // full 64-bit inode

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
};      // namespace posix
};      // namespace micron
