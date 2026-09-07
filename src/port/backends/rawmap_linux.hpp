//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__syscall.hpp"

#include "../../types.hpp"

namespace micron
{
namespace port
{

constexpr int __rawmap_prot_rw = 0x1 | 0x2;
constexpr int __rawmap_priv_anon = 0x2 | 0x20;

inline void *
raw_map(usize sz) noexcept
{
#if defined(__micron_arch_width_32)
  const long r = micron::syscall(SYS_mmap2, 0, sz, __rawmap_prot_rw, __rawmap_priv_anon, -1, 0);
#else
  const long r = micron::syscall(SYS_mmap, 0, sz, __rawmap_prot_rw, __rawmap_priv_anon, -1, 0);
#endif
  if ( static_cast<unsigned long>(r) >= static_cast<unsigned long>(-4095) ) return nullptr;
  return reinterpret_cast<void *>(r);
}

inline void
raw_unmap(void *p, usize sz) noexcept
{
  micron::syscall(SYS_munmap, p, sz);
}

};      // namespace port
};      // namespace micron
