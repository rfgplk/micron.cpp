//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// diagnostic and halt surface, inside a Linux kernel module
//
// WARNING: THIS FILE MAY INCLUDE __kport_abi.hpp AND types.hpp AND NOTHING ELSE.

#include "__kport_abi.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline void
write_diag(const char *__s, usize __n) noexcept
{
  ::mc_kport_write_diag(__s, static_cast<mc_kport_usize>(__n));
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
  ::mc_kport_halt(__code);
  __builtin_unreachable();
}

[[noreturn]] inline void
halt_local(int __code) noexcept
{
  ::mc_kport_halt(__code);
  __builtin_unreachable();
}

inline void
flush_diag(void) noexcept
{
  ::mc_kport_write_diag_flush();
}

};      // namespace port
};      // namespace micron
