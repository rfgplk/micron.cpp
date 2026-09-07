//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the diagnostic and halt surface, on userspace Linux
//
// WARNING: THIS FILE MAY INCLUDE syscall.hpp AND types.hpp AND NOTHING ELSE.

#include "__syscall.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline void
write_diag(const char *__s, usize __n) noexcept
{
  (void)micron::syscall(SYS_write, 2, reinterpret_cast<const void *>(__s), __n);
}

inline usize
diag_len(const char *__s) noexcept
{
  usize __n = 0;
  while ( __s[__n] ) ++__n;
  return __n;
}

inline void
write_diag(const char *__s) noexcept
{
  write_diag(__s, diag_len(__s));
}

[[noreturn]] inline void
halt(int __code) noexcept
{
  micron::syscall(SYS_exit_group, __code);
  __builtin_unreachable();
}

[[noreturn]] inline void
halt_local(int __code) noexcept
{
  micron::syscall(SYS_exit, __code);
  __builtin_unreachable();
}

inline void
flush_diag(void) noexcept
{
}

};      // namespace port
};      // namespace micron
