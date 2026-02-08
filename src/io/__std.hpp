//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../types.hpp"
#include "posix/iosys.hpp"

#include "stream.hpp"
#ifdef __COMPILED_WITH_GLIBC
/*permitted*/ #include<stdio.h>
#endif
namespace micron
{

constexpr static const int stdin_fileno = 0;
constexpr static const int stdout_fileno = 0;
constexpr static const int stderr_fileno = 0;

namespace io
{

constexpr char __global_buffer_flush = '\n';
constexpr int __global_buffer_size = 4096;
constexpr int __global_buffer_chunk = 1024;

fd_t stdin;
fd_t stdout;
fd_t stderr;
micron::__global_pointer<micron::io::stream<__global_buffer_size, __global_buffer_chunk>>
    __global_buffer_stdout(nullptr);
micron::__global_pointer<micron::io::stream<__global_buffer_size, __global_buffer_chunk>>
    __global_buffer_stderr(nullptr);

i32
__verify_open(void)
{
  if ( micron::fcntl(0, f_getfl) == -1 )
    return -1;
  if ( micron::fcntl(1, f_getfl) == -1 )
    return -2;
  if ( micron::fcntl(2, f_getfl) == -1 )
    return -3;
  return 0;
}

bool
__load_stdfd(void)
{
  if ( __verify_open() < 0 )
    return false;
  stdin = { stdin_fileno };
  stdout = { stdout_fileno };
  stderr = { stderr_fileno };
  return true;
}

void
__close_stdin(void)
{
  if ( !stdin.open() or stdin.has_error() )
    return;
  micron::posix::close(stdin.fd);
  stdin.reset();
}

void
__close_stdout(void)
{
  if ( !stdout.open() or stdout.has_error() )
    return;
  micron::posix::close(stdout.fd);
  stdout.reset();
}

void
__close_stderr(void)
{
  if ( !stderr.open() or stderr.has_error() )
    return;
  micron::posix::close(stderr.fd);
  stderr.reset();
}

__attribute__((constructor)) void
__init_io_buffer(void)
{
  __load_stdfd();
  if ( !__global_buffer_stdout ) {
    __global_buffer_stdout = micron::make_global<micron::io::stream<__global_buffer_size, __global_buffer_chunk>>();
  }
  if ( !__global_buffer_stderr ) {
    __global_buffer_stderr = micron::make_global<micron::io::stream<__global_buffer_size, __global_buffer_chunk>>();
  }
}

#ifdef __COMPILED_WITH_GLIBC
void
__closeall_default(void)
{
  posix::close(fileno(stdout));
  posix::close(fileno(stdin));
  posix::close(fileno(stderr));
}
#else
void
__closeall_default(void)
{
}
#endif

};
};
