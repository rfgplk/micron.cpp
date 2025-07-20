//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../memory/memory.hpp"
#include <unistd.h> // for read
#include <cstdio>
#include "posix/iosys.hpp"
#include "../types.hpp"

// all system dependent functions should go here (ie syscalls/posix stuff)

namespace micron
{
namespace io
{
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
  //posix::write(fp->_fileno, s, len);
}
inline __attribute__((always_inline)) void
fput(const char *__restrict s, FILE *__restrict fp)
{
  ::fwrite_unlocked(s, sizeof(char), micron::strlen(s), fp);
  //posix::write(fp->_fileno, s, strlen(s));
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
fget_byte(T *__restrict s, FILE *__restrict fp) // equivalent to getchar
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

// no seeking alignment, just read
template <typename T>
inline ssize_t
read(int fd, T* buf, size_t cnt)
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
template <typename T>
inline ssize_t
read(FILE *fd, T* buf, size_t cnt)
{
  if ( fd == nullptr )
    return -1;
  return posix::read(fd->_fileno, buf, cnt);
  // ugly i know, but valid
}
template <typename T, size_t N>
inline ssize_t
read(FILE *fd, const T (&buf)[N])
{
  if ( fd == nullptr )
    return -1;
  return posix::read(fd->_fileno, &buf, N);
}

template <typename T = byte>
inline ssize_t
write(int fd, T* buf, size_t cnt)
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
};     // namespace io

};     // namespace micron
