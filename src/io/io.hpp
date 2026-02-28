//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/memory.hpp"
#include "../types.hpp"
#include "__std.hpp"
#include "posix/iosys.hpp"

#include "stream.hpp"

#include "../simd/bitwise.hpp"

#include "../concepts.hpp"

// NOTE: on naming conventions:
// all functions starting with f are buffered
// functions ending in d (where d isn't part of the noun of the name) are direct (ie. nonblocking)
// functions without a prefix are syscall maps

namespace micron
{
namespace io
{
// no seeking alignment, just read
template <typename T>
ssize_t
read(int fd, T *buf, usize cnt)
{
  if ( fd <= 0 )
    return -1;
  return posix::read(fd, buf, cnt);
  // ugly i know, but valid
}

template <typename T, usize N>
ssize_t
read(int fd, const T (&buf)[N])
{
  if ( fd <= 0 )
    return -1;
  return posix::read(fd, &buf, N);
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
ssize_t
read(int fd, T &buf, const usize cnt)
{
  return posix::read(fd, buf.data(), cnt);
}

template <typename T = byte>
ssize_t
write(int fd, T *buf, usize cnt)
{
  if ( fd <= 0 )
    return -1;
  return posix::write(fd, buf, cnt);
}

template <typename T, usize N>
ssize_t
write(int fd, const T (&buf)[N])
{
  if ( fd <= 0 )
    return -1;
  return posix::write(fd, &buf, N);
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
ssize_t
write(int fd, T &buf, const usize cnt)
{
  return posix::write(fd, buf.data(), cnt);
}

template <typename T = byte>
ssize_t
fwrited(T *ptr, usize num, const fd_t &handle)
{
  if ( handle.has_error() or handle.closed() )
    return -1;
  return posix::write(handle.fd, ptr, num);
}

template <typename T = byte>
ssize_t
fwrite(T *ptr, usize num, const fd_t &handle)
{
  if ( handle.has_error() or handle.closed() )
    return -1;
  // differentiate depending on handle, maintains cache loc.
  if ( handle == stdout ) {
    if ( __global_buffer_stdout->full(num * sizeof(byte)) ) {
      (*__global_buffer_stdout) >> handle;
      posix::write(handle.fd, ptr, num);
      return num;
    force_flush_out:
      (*__global_buffer_stdout) >> handle;
      return num;
    }
    __global_buffer_stdout->append(ptr, num);
    if ( simd::any_set_128(ptr, num, __global_buffer_flush) )
      goto force_flush_out;
    return num;
  } else if ( handle == stderr ) {
    if ( __global_buffer_stderr->full(num * sizeof(byte)) ) {
      (*__global_buffer_stderr) >> handle;
      posix::write(handle.fd, ptr, num);
      return num;
    force_flush_err:
      (*__global_buffer_stderr) >> handle;
      return num;
    }
    __global_buffer_stderr->append(ptr, num);
    if ( simd::any_set_128(ptr, num, __global_buffer_flush) )
      goto force_flush_err;
    return num;
  }
  return posix::write(handle.fd, ptr, num);
}

void
fflush(const fd_t &handle)
{
  if ( handle.has_error() or handle.closed() )
    return;
  // differentiate depending on handle, maintains cache loc.
  if ( handle == stdout ) {
    (*__global_buffer_stdout) >> handle;
    return;
  } else if ( handle == stderr ) {
    (*__global_buffer_stderr) >> handle;
    return;
  }
}

inline __attribute__((always_inline)) void
fput(const char *__restrict s, usize len, const fd_t &handle)
{
  io::fwrite(s, len, handle);
}

inline __attribute__((always_inline)) void
fput(const char *__restrict s, const fd_t &handle)
{
  io::fwrite(s, micron::strlen(s), handle);
}

inline __attribute__((always_inline)) void
fput(const char s, const fd_t &handle)
{
  io::fwrite(&s, 1, handle);
}

inline __attribute__((always_inline)) void
wfput(const wchar_t *__restrict s, const fd_t &handle)
{
  io::fwrite(s, micron::wstrlen(s), handle);
}

inline __attribute__((always_inline)) void
unifput(const char32_t *__restrict s, const fd_t &handle)
{
  io::fwrite(s, micron::ustrlen(s), handle);
}

template <typename T>
inline __attribute__((always_inline)) auto
fget(T *__restrict s, const usize n, const fd_t &handle)
{
  return posix::read(s, n, handle.fd);
}

template <typename T>
inline __attribute__((always_inline)) auto
fget_byte(T *__restrict s, const fd_t &handle)     // equivalent to getchar
{
  return posix::read(handle.fd, s, 1);
}

// equivalent to fputs
inline __attribute__((always_inline)) void
put(const char *__restrict s, const fd_t &handle)
{
  for ( ; *s != 0x0; s++ )
    io::fput(*s, handle.fd);
  //::fputc_unlocked(*s, fp);
}
};     // namespace io

};     // namespace micron
