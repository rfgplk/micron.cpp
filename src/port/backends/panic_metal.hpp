//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// the diagnostic and halt surface, on bare metal
//
// WARNING: THIS FILE MAY INCLUDE __mport_abi.hpp AND types.hpp AND NOTHING ELSE.

#include "__mport_abi.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

inline void
write_diag(const char *__s, usize __n) noexcept
{
  ::mc_mport_write_diag(__s, static_cast<mc_mport_usize>(__n));
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
  ::mc_mport_halt(__code);
  __builtin_unreachable();
}

[[noreturn]] inline void
halt_local(int __code) noexcept
{
  ::mc_mport_halt(__code);
  __builtin_unreachable();
}

inline void
flush_diag(void) noexcept
{
}

};      // namespace port
};      // namespace micron
