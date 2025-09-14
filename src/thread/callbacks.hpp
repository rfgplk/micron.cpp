//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../io/posix/iosys.hpp"

#include "../array.hpp"
#include "../mutex/mutex.hpp"
#include "../mutex/locks.hpp"

namespace micron
{
// NOTE: this C style callback handler is here since it's far too tricky to pass through a full template resolved function prototype and actually have it (re)bind to a templated function. technically, the program would be illformed
// ie: func ptr of T(Args...) calling function R(LArgs...) for any typenames is almost impossible to do accurately especially since clone() kills/flattens proper resolution
mutex __global_callback_mtx;
array<void(*)(void), 64> __global_callback_ptrs;
size_t __global_callback_size = 0;

void add_callback(void(*f)(void)){
  lock_guard l(__global_callback_mtx);
  __global_callback_ptrs[__global_callback_size++] = f;
  l.~lock_guard();
}

int
__default_callback(void *)
{
  lock_guard l(__global_callback_mtx);
  if(!__global_callback_size) return -1;
  __global_callback_ptrs.at(--__global_callback_size)();
  l.~lock_guard();
  return 0;
}

int
__default_daemon_callback(void *)
{
  if ( posix::setsid() < 0 )
    throw except::runtime_error("micron process daemon failed to create new session");
  // don't change dir
  posix::umask(0);
  posix::close(STDIN_FILENO);
  posix::close(STDOUT_FILENO);
  posix::close(STDERR_FILENO);

  posix::open("/dev/null", O_RDWR);
  posix::dup(0);
  posix::dup(0);
  if(!__global_callback_size) return -1;
  __global_callback_ptrs.at(__global_callback_size--)();
  return 0;
}

int
__default_stop_callback(void *)
{
  _Exit(1);
  return 1;
}


int
__default_sleep_callback(void *)
{
  pause();
  lock_guard l(__global_callback_mtx);
  if(!__global_callback_size) return -1;
  __global_callback_ptrs.at(__global_callback_size--)();
  l.~lock_guard();
  return 0;
}

};
