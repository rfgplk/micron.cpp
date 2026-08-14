//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// i386 1-IPC calibration kernel

namespace micron
{
namespace chrono
{
namespace arch
{

[[gnu::noinline]] inline void
calib_kernel(i64 iters64) noexcept
{
  i32 iters = static_cast<i32>(iters64 > 0x3FFF'FFFF ? 0x3FFF'FFFF : iters64);
  __asm__ __volatile__("movl $-1, %%eax\n\t"
                       "negl %%eax\n\t"
                       ".p2align 5\n"
                       "1:\n\t"
                       "subl %%eax, %0\n\t"
                       "subl %%eax, %0\n\t"
                       "jge 1b"
                       : "+r"(iters)
                       :
                       : "eax", "cc");
}

inline constexpr bool calib_kernel_native = true;

};      // namespace arch
};      // namespace chrono
};      // namespace micron
