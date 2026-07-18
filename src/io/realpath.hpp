//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/sys/fcntl.hpp"
#include "../linux/sys/limits.hpp"
#include "../string/strings.hpp"
#include "../syscall.hpp"
#include "../types.hpp"
#include "os/iosys.hpp"

namespace micron
{
namespace io
{

inline char *
realpath_into(const char *__restrict path, char *__restrict dst, usize dstcap)
{
  if ( !path || !dst || dstcap == 0 ) {
    return nullptr;
  }

#if defined(__micron_syscall_generic)
  // arm64 has no legacy open syscall
  int fd = static_cast<int>(micron::syscall(SYS_openat, posix::at_fdcwd, path, posix::o_rdonly | posix::o_path));
#else
  int fd = static_cast<int>(micron::syscall(SYS_open, path, posix::o_rdonly | posix::o_path));
#endif
  if ( fd < 0 ) {
    return nullptr;
  }

  char proc_path[32];
  const char *prefix = "/proc/self/fd/";
  int i = 0;
  while ( prefix[i] ) {
    proc_path[i] = prefix[i];
    i++;
  }

  int temp_fd = fd;
  char fd_str[16];
  int idx = 0;

  if ( temp_fd == 0 ) {
    fd_str[idx++] = '0';
  } else {
    int digits[16];
    int digit_count = 0;
    while ( temp_fd > 0 ) {
      digits[digit_count++] = temp_fd % 10;
      temp_fd /= 10;
    }
    for ( int j = digit_count - 1; j >= 0; --j ) {
      fd_str[idx++] = '0' + static_cast<char>(digits[j]);
    }
  }
  fd_str[idx] = '\0';

  int j = 0;
  while ( fd_str[j] ) {
    proc_path[i++] = fd_str[j++];
  }
  proc_path[i] = '\0';

  const usize cap = dstcap < posix::path_max ? dstcap : posix::path_max;
#if defined(__micron_syscall_generic)
  // asm-generic ABI (arm64) has no legacy readlink syscall; route via readlinkat
  max_t len = (micron::syscall(SYS_readlinkat, posix::at_fdcwd, proc_path, dst, cap - 1));
#else
  max_t len = (micron::syscall(SYS_readlink, proc_path, dst, cap - 1));
#endif

  close(fd);
  if ( len < 0 ) {
    return nullptr;
  }
  if ( static_cast<usize>(len) >= cap - 1 ) {
    return nullptr;
  }

  dst[len] = '\0';
  return dst;
}

inline char *
realpath(const char *__restrict path, char *__restrict resolved_path)
{
  if ( resolved_path ) return realpath_into(path, resolved_path, posix::path_max);

#if defined(__micron_arch_width_32)
  max_t __m = micron::syscall(SYS_mmap2, nullptr, posix::path_max, prot_read | prot_write, map_private | map_anonymous, -1, 0);
#else
  max_t __m = micron::syscall(SYS_mmap, nullptr, posix::path_max, prot_read | prot_write, map_private | map_anonymous, -1, 0);
#endif
  if ( micron::syscall_failed(__m) ) return nullptr;
  char *buf = reinterpret_cast<char *>(__m);
  if ( !realpath_into(path, buf, posix::path_max) ) {
    micron::syscall(SYS_munmap, buf, posix::path_max);
    return nullptr;
  }
  return buf;
}

template<usize N = posix::path_max, usize M, typename T>
inline sstring<N>
realpath(const sstring<M, T> &path)
{
  sstring<N> result;
  char *res = realpath_into(path.c_str(), result.data(), N * sizeof(schar));

  if ( !res ) {
    result.clear();
    return result;
  }

  result.adjust_size();
  return result;
}

template<usize N, typename T = schar>
inline char *
realpath(const char *path, sstring<N, T> &resolved)
{
  char *res = realpath_into(path, resolved.data(), N * sizeof(T));
  if ( res ) {
    resolved.adjust_size();
  } else {
    resolved.clear();
  }
  return res;
}

template<usize N, typename T = schar, usize M, typename U>
inline char *
realpath(const sstring<M, U> &path, sstring<N, T> &resolved)
{
  return realpath(path.c_str(), resolved);
}

};      // namespace io

// all io code lives in micron::io as of io_v3
using io::realpath;

};      // namespace micron
