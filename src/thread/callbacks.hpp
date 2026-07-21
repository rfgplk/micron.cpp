//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../io/__std.hpp"      // std{in,out,err}_fileno
#include "../io/os/iosys.hpp"

#include "../array/arrays.hpp"
#include "../mutex/locks.hpp"
#include "../mutex/mutex.hpp"

namespace micron
{
// NOTE: this C style callback handler is here since it's far too tricky to pass through a full template resolved
// function prototype and actually have it (re)bind to a templated function. technically, the program would be illformed
// ie: func ptr of T(Args...) calling function R(LArgs...) for any typenames is almost impossible to do accurately
// especially since clone() kills/flattens proper resolution
// NOTE: must be inline so we don't accidentally produce duplicate sym link errors
inline mutex __global_callback_mtx;
inline array<void (*)(void), 64> __global_callback_ptrs;
inline usize __global_callback_size = 0;

inline void
add_callback(void (*f)(void))
{
  lock_guard l(__global_callback_mtx);
  if ( __global_callback_size >= __global_callback_ptrs.size() ) return;      // fixed 64-slot registry: drop overflow, don't write OOB
  __global_callback_ptrs[__global_callback_size++] = f;
}

inline int
__default_callback(void *)
{
  void (*f)(void) = nullptr;
  {
    lock_guard l(__global_callback_mtx);
    if ( !__global_callback_size ) return -1;
    f = __global_callback_ptrs.at(--__global_callback_size);
  }
  if ( f ) f();
  return 0;
}

inline int
__default_daemon_callback(void *)
{
  if ( posix::setsid() < 0 ) exc<except::runtime_error>("micron process daemon failed to create new session");
  // don't change dir
  posix::umask(0);
  posix::close(stdin_fileno);
  posix::close(stdout_fileno);
  posix::close(stderr_fileno);

  posix::open("/dev/null", posix::o_rdwr);
  micron::dup(0);
  micron::dup(0);
  void (*f)(void) = nullptr;
  {
    lock_guard l(__global_callback_mtx);
    if ( !__global_callback_size ) return -1;
    f = __global_callback_ptrs.at(--__global_callback_size);
  }
  if ( f ) f();
  return 0;
}

inline int
__default_stop_callback(void *)
{
  // NOTE: must be _exit
  micron::syscall(SYS_exit, 1);
  return 1;
}

inline int
__default_sleep_callback(void *)
{
  posix::pause();
  void (*f)(void) = nullptr;
  {
    lock_guard l(__global_callback_mtx);
    if ( !__global_callback_size ) return -1;
    f = __global_callback_ptrs.at(--__global_callback_size);
  }
  if ( f ) f();
  return 0;
}

};      // namespace micron
