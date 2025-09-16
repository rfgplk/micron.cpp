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

// NOTE: on naming conventions:
// all functions starting with f are buffered
// functions ending in d (where d isn't part of the noun of the name) are direct (ie. nonblocking)
// functions without a prefix are syscall maps

namespace micron
{
namespace io
{
template <typename T>
inline ssize_t
fread(int fd, T *buf, size_t cnt)
{
  if ( fd <= 0 )
    return -1;
  return posix::read(fd, buf, cnt);
  // ugly i know, but valid
}

template <typename T, size_t N>
inline ssize_t
fread(int fd, const T (&buf)[N])
{
  if ( fd <= 0 )
    return -1;
  return posix::read(fd, &buf, N);
}
// no seeking alignment, just read
template <typename T>
inline ssize_t
read(int fd, T *buf, size_t cnt)
{
  if ( fd <= 0 )
    return -1;
  return posix::read(fd, buf, cnt);
  // ugly i know, but valid
}

template <typename T, size_t N>
inline ssize_t
read(int fd, const T (&buf)[N])
{
  if ( fd <= 0 )
    return -1;
  return posix::read(fd, &buf, N);
}
/*
template <typename T>
inline ssize_t
__read(FILE *fd, T *buf, size_t cnt)
{
  if ( fd == nullptr )
    return -1;
  return posix::read(fd->_fileno, buf, cnt);
  // ugly i know, but valid
}
template <typename T, size_t N>
inline ssize_t
__read(FILE *fd, const T (&buf)[N])
{
  if ( fd == nullptr )
    return -1;
  return posix::read(fd->_fileno, &buf, N);
}
*/
template <typename T = byte>
inline ssize_t
write(int fd, T *buf, size_t cnt)
{
  if ( fd <= 0 )
    return -1;
  return posix::write(fd, buf, cnt);
}
template <typename T, size_t N>
inline ssize_t
write(int fd, const T (&buf)[N])
{
  if ( fd <= 0 )
    return -1;
  return posix::write(fd, &buf, N);
}

template <typename T = byte>
inline ssize_t
fwrited(T *ptr, size_t num, const fd_t &handle)
{
  if ( handle.has_error() or handle.closed() )
    return -1;
  return posix::write(handle.fd, ptr, num);
}

template <typename T = byte>
inline ssize_t
fwrite(T *ptr, size_t num, const fd_t &handle)
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

inline void
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
fput(const char *__restrict s, size_t len, const fd_t &handle)
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
fget(T *__restrict s, const size_t n, const fd_t &handle)
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

/* OLD OBSOLETE CODE
inline void
buffer_flush_stdout(void)
{
  ::fflush(stdout);
}
inline void
set_buffering(char **buf)
{
  ::setvbuf(stdout, *buf, _IOFBF, sizeof(*buf));
}
inline void
disable_buffering(void)
{
  ::setvbuf(stdout, NULL, _IONBF, BUFSIZ);
}

inline __attribute__((always_inline)) void
fput(const char *__restrict s, size_t len, FILE *__restrict fp)
{
  ::fwrite_unlocked(s, sizeof(char), len, fp);
  // posix::write(fp->_fileno, s, len);
}
inline __attribute__((always_inline)) void
fput(const char *__restrict s, FILE *__restrict fp)
{
  ::fwrite_unlocked(s, sizeof(char), micron::strlen(s), fp);
  // posix::write(fp->_fileno, s, strlen(s));
}
inline __attribute__((always_inline)) void
fput(const char s, FILE *__restrict fp)
{
  ::fwrite_unlocked(&s, sizeof(char), 1, fp);
  // posix::write(fp->_fileno, s, strlen(s));
}

inline __attribute__((always_inline)) void
wfput(const wchar_t *__restrict s, FILE *__restrict fp)
{
  ::fwrite_unlocked(s, sizeof(wchar_t), micron::wstrlen(s), fp);
}

inline __attribute__((always_inline)) void
unifput(const char32_t *__restrict s, FILE *__restrict fp)
{
  ::fwrite_unlocked(s, sizeof(char32_t), micron::ustrlen(s), fp);
}

template <typename T>
inline __attribute__((always_inline)) auto
fget(T *__restrict s, const size_t n, FILE *__restrict fp)
{
  return ::fread_unlocked(s, sizeof(char), n, fp);
}
template <typename T>
inline __attribute__((always_inline)) auto
fget_byte(T *__restrict s, FILE *__restrict fp)     // equivalent to getchar
{
  return posix::read(fp->_fileno, s, 1);
}
// equivalent to fputs
inline __attribute__((always_inline)) void
put(const char *__restrict s, FILE *__restrict fp)
{
  for ( ; *s != 0x0; s++ )
    ::fputc_unlocked(*s, fp);
}
*/

};     // namespace io

};     // namespace micron
