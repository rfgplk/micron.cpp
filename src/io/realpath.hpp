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
#include "posix/iosys.hpp"

namespace micron
{

inline char *
realpath(const char *__restrict path, char *__restrict resolved_path)
{
  if ( !path ) {
    return nullptr;
  }

  int fd = static_cast<int>(micron::syscall(SYS_open, path, posix::o_rdonly | posix::o_path));
  if ( fd < 0 ) {
    return nullptr;
  }

  char *result_buf = resolved_path;
  bool allocated = false;

  if ( !resolved_path ) {
    result_buf = (reinterpret_cast<char *>(
        micron::syscall(SYS_mmap, nullptr, posix::path_max, prot_read | prot_write, map_private | map_anonymous, -1, 0)));

    allocated = true;
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

  ssize_t len = (micron::syscall(SYS_readlink, proc_path, result_buf, posix::path_max - 1));

  close(fd);
  if ( len < 0 ) {
    if ( allocated ) {
      micron::syscall(SYS_munmap, result_buf, posix::path_max);
    }
    return nullptr;
  }

  result_buf[len] = '\0';

  return result_buf;
}

template <usize N = posix::path_max, usize M, typename T>
inline sstring<N>
realpath(const sstring<M, T> &path)
{
  sstring<N> result;
  char *res = realpath(path.c_str(), result.data());

  if ( !res ) {
    result.clear();
    return result;
  }

  result.adjust_size();
  return result;
}

template <usize N, typename T = schar>
inline char *
realpath(const char *path, sstring<N, T> &resolved)
{
  char *res = realpath(path, resolved.data());
  if ( res ) {
    resolved.adjust_size();
  } else {
    resolved.clear();
  }
  return res;
}

template <usize N, typename T = schar, usize M, typename U>
inline char *
realpath(const sstring<M, U> &path, sstring<N, T> &resolved)
{
  return realpath(path.c_str(), resolved);
}

};     // namespace micron
