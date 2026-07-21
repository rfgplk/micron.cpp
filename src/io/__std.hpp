//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"

#include "../linux/sys/ioctl.hpp"      // posix::isatty (buffering-mode detection)
#include "os/iosys.hpp"

#include "stream.hpp"

#include "../attributes.hpp"

namespace micron
{

constexpr static const int stdin_fileno = 0;
constexpr static const int stdout_fileno = 1;
constexpr static const int stderr_fileno = 2;

namespace io
{

constexpr char __global_buffer_flush = '\n';
constexpr int __global_buffer_size = 4096;
constexpr int __global_buffer_chunk = 1024;

// inline so we don't collide with multiple TUs
inline fd_t stdin = posix::invalid_fd;
inline fd_t stdout = posix::invalid_fd;
inline fd_t stderr = posix::invalid_fd;

// TTY output is line buffered on \n by default
// non TTY (pipes/files) are fully buffered
inline bool __stdout_line_buffered = true;

// these must be constinit to prevent clobbering with the default ctor
inline constinit micron::__global_pointer<micron::io::stream<__global_buffer_size, __global_buffer_chunk>> __global_buffer_stdout(nullptr);
inline constinit micron::__global_pointer<micron::io::stream<__global_buffer_size, __global_buffer_chunk>> __global_buffer_stderr(nullptr);

inline i32
__verify_open(void)
{
  if ( micron::posix::fcntl(0, posix::f_getfl) < 0 ) return -1;
  if ( micron::posix::fcntl(1, posix::f_getfl) < 0 ) return -2;
  if ( micron::posix::fcntl(2, posix::f_getfl) < 0 ) return -3;
  return 0;
}

inline bool
__load_stdfd(void)
{
  if ( __verify_open() < 0 ) return false;
  stdin = { stdin_fileno };
  stdout = { stdout_fileno };
  stderr = { stderr_fileno };
  __stdout_line_buffered = micron::posix::isatty(stdout_fileno);      // pipe/file -> full buffering
  return true;
}

inline void
__close_stdin(void)
{
  if ( !stdin.open() or stdin.has_error() ) return;
  micron::posix::close(stdin.fd);
  stdin.reset();
}

inline void
__close_stdout(void)
{
  if ( !stdout.open() or stdout.has_error() ) return;
  micron::posix::close(stdout.fd);
  stdout.reset();
}

inline void
__close_stderr(void)
{
  if ( !stderr.open() or stderr.has_error() ) return;
  micron::posix::close(stderr.fd);
  stderr.reset();
}

// WARNING: (for the nth time) these two __MUST__ stay strong so they
// reliably override starts __attribute__((weak)) noop stubs __regardless__ of link
// order; SOMEHOW?! an inline/weak def can lose to the weak stub when it is linked first
extern "C" void
__boot_io_buffers(void)
{
  __load_stdfd();
  if ( !__global_buffer_stdout ) {
    __global_buffer_stdout = micron::make_global<micron::io::stream<__global_buffer_size, __global_buffer_chunk>>();
  }
  if ( !__global_buffer_stderr ) {
    __global_buffer_stderr = micron::make_global<micron::io::stream<__global_buffer_size, __global_buffer_chunk>>();
  }
}

extern "C" void
__shutdown_io_buffers(void)
{
  if ( __global_buffer_stdout && stdout.open() && !stdout.has_error() ) __global_buffer_stdout->flush_to(stdout);
  if ( __global_buffer_stderr && stderr.open() && !stderr.has_error() ) __global_buffer_stderr->flush_to(stderr);
}

// hosted init via .init_array (gconstructor_ is empty in freestanding builds)
inline void gconstructor_
__init_io_buffer(void)
{
  __boot_io_buffers();
}

inline void gdestructor_
__flush_io_buffer(void)
{
  __shutdown_io_buffers();
}

#ifdef __COMPILED_WITH_GLIBC
inline void
__closeall_default(void)
{
  posix::close(fileno(stdout));
  posix::close(fileno(stdin));
  posix::close(fileno(stderr));
}
#else
inline void
__closeall_default(void)
{
}
#endif

};      // namespace io
};      // namespace micron
