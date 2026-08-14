//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// armv7-a 1-IPC calibration kernel

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
  __asm__ __volatile__("mov r3, #1\n\t"
                       ".p2align 5\n"
                       "1:\n\t"
                       "subs %0, %0, r3\n\t"
                       "subs %0, %0, r3\n\t"
                       "bge 1b"
                       : "+r"(iters)
                       :
                       : "r3", "cc");
}

inline constexpr bool calib_kernel_native = true;

};      // namespace arch
};      // namespace chrono
};      // namespace micron
