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

#include "../errno.hpp"

#include "stream.hpp"

#include "../simd/bitwise.hpp"

#include "../concepts.hpp"

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// general io:: high level functions
// unlike the code from posix/io and posix/
// these fns include safety checks
// 0.6 each functions now has a raw i32 fd fd_t and T& overload

// NOTE: on naming conventions:
// all functions starting with f are buffered
// functions ending in d (where d isn't part of the noun of the name) are direct (ie. nonblocking)
// functions without a prefix are syscall maps

namespace micron
{
namespace io
{
template <typename T>
concept has_set_size = requires(T t, usize n) {
  { t.set_size(n) };
};

// no seeking alignment, just read
template <typename T>
max_t
read(i32 fd, T *buf, usize cnt)
{
  if ( buf == nullptr ) [[unlikely]]
    return -error::invalid_arg;
  if ( fd < 0 )
    return -error::bad_fd;
  return posix::read(fd, buf, cnt);
}

template <typename T>
max_t
read(fd_t fd, T *buf, usize cnt)
{
  if ( buf == nullptr ) [[unlikely]]
    return -error::invalid_arg;
  if ( fd.invalid() ) [[unlikely]]
    return -error::bad_fd;
  return posix::read(fd.fd, buf, cnt);
}

template <typename T>
max_t
read(i32 fd, T &buf, usize cnt)
{
  if ( fd < 0 )
    return -error::bad_fd;
  return posix::read(fd, buf, cnt);
}

template <typename T>
max_t
read(fd_t fd, T &buf, usize cnt)
{
  if ( fd.invalid() ) [[unlikely]]
    return -error::bad_fd;
  return posix::read(fd.fd, buf, cnt);
}

template <typename T, usize N>
max_t
read(i32 fd, const T (&buf)[N])
{
  if ( fd < 0 )
    return -error::bad_fd;
  return posix::read(fd, &buf, N);
}

template <typename T, usize N>
max_t
read(fd_t fd, const T (&buf)[N])
{
  if ( fd.invalid() )
    return -error::bad_fd;
  return posix::read(fd.fd, &buf, N);
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
max_t
read(i32 fd, T &buf, const usize cnt)
{
  if ( fd < 0 )
    return -error::bad_fd;
  // check alloc'd cap
  if ( buf.max_size() < cnt ) [[unlikely]]
    return -error::invalid_arg;
  max_t r = posix::read(fd, buf.data(), cnt);
  if constexpr ( has_set_size<T> ) {
    buf.set_size(r);
  }
  return r;
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
max_t
read(fd_t fd, T &buf, const usize cnt)
{
  if ( fd.invalid() ) [[unlikely]]
    return -error::bad_fd;
  // check alloc'd cap
  if ( buf.max_size() < cnt ) [[unlikely]]
    return -error::invalid_arg;
  max_t r = posix::read(fd.fd, buf.data(), cnt);
  if constexpr ( has_set_size<T> ) {
    buf.set_size(r);
  }
  return r;
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
max_t
read(i32 fd, T &buf)
{
  if ( fd < 0 )
    return -error::bad_fd;
  max_t r = posix::read(fd, buf.data(), buf.max_size());
  if constexpr ( has_set_size<T> ) {
    buf.set_size(r);
  }
  return r;
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
max_t
read(fd_t fd, T &buf)
{
  if ( fd.invalid() ) [[unlikely]]
    return -error::bad_fd;
  max_t r = posix::read(fd.fd, buf.data(), buf.max_size());
  if constexpr ( has_set_size<T> ) {
    buf.set_size(r);
  }
  return r;
}

template <typename T = byte>
max_t
write(i32 fd, const T *buf, usize cnt)
{
  if ( buf == nullptr ) [[unlikely]]
    return -error::bad_fd;
  if ( fd < 0 )
    return -error::bad_fd;
  return posix::write(fd, buf, cnt);
}

template <typename T = byte>
max_t
write(fd_t fd, const T *buf, usize cnt)
{
  if ( buf == nullptr ) [[unlikely]]
    return -error::invalid_arg;
  if ( fd.invalid() )
    return -error::bad_fd;
  return posix::write(fd, buf, cnt);
}

template <typename T = byte>
max_t
write(i32 fd, const T &buf, usize cnt)
{
  if ( fd < 0 )
    return -error::bad_fd;
  return posix::write(fd, buf, cnt);
}

template <typename T = byte>
max_t
write(fd_t fd, const T &buf, usize cnt)
{
  if ( fd.invalid() )
    return -error::bad_fd;
  return posix::write(fd, buf, cnt);
}

template <typename T, usize N>
max_t
write(i32 fd, const T (&buf)[N])
{
  if ( fd < 0 )
    return -error::bad_fd;
  return posix::write(fd, &buf, N);
}

template <typename T, usize N>
max_t
write(fd_t fd, const T (&buf)[N])
{
  if ( fd.invalid() )
    return -error::bad_fd;
  return posix::write(fd, &buf, N);
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
max_t
write(i32 fd, T &buf, const usize cnt)
{
  if ( fd < 0 )
    return -error::bad_fd;
  return posix::write(fd, buf.data(), cnt);
}

template <is_iterable_container T>
  requires(micron::is_fundamental_v<typename T::value_type>)
max_t
write(fd_t fd, T &buf, const usize cnt)
{
  if ( fd.invalid() )
    return -error::bad_fd;
  return posix::write(fd, buf.data(), cnt);
}

template <typename T = byte>
max_t
fwrited(T *ptr, usize num, const fd_t &handle)
{
  if ( handle.has_error() or handle.closed() )
    return -error::bad_fd;
  return posix::write(handle.fd, ptr, num);
}

template <typename T = byte>
max_t
fwrited(T &ptr, usize num, const fd_t &handle)
{
  if ( handle.has_error() or handle.closed() )
    return -error::bad_fd;
  return posix::write(handle.fd, ptr, num);
}

template <typename T = byte>
max_t
fwrite(T &ref, usize num, const fd_t &handle)
{
  if ( handle.has_error() or handle.closed() )
    return -error::bad_fd;
  // differentiate depending on handle, maintains cache loc.
  if ( handle == stdout ) {
    if ( __global_buffer_stdout->full(num * sizeof(byte)) ) {
      (*__global_buffer_stdout) >> handle;
      posix::write(handle.fd, ref, num);
      return num;
    force_flush_out:
      (*__global_buffer_stdout) >> handle;
      return num;
    }
    __global_buffer_stdout->append(ref, num);
    if ( simd::any_set_128(ref, num, __global_buffer_flush) )
      goto force_flush_out;
    return num;
  } else if ( handle == stderr ) {
    if ( __global_buffer_stderr->full(num * sizeof(byte)) ) {
      (*__global_buffer_stderr) >> handle;
      posix::write(handle.fd, ref, num);
      return num;
    force_flush_err:
      (*__global_buffer_stderr) >> handle;
      return num;
    }
    __global_buffer_stderr->append(ref, num);
    if ( simd::any_set_128(ref, num, __global_buffer_flush) )
      goto force_flush_err;
    return num;
  }
  return posix::write(handle.fd, ref, num);
}

template <typename T = byte>
max_t
fwrite(T *ptr, usize num, const fd_t &handle)
{
  if ( ptr == nullptr ) [[unlikely]]
    return -error::invalid_arg;
  if ( handle.has_error() or handle.closed() )
    return -error::bad_fd;
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
