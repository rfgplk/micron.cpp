//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// amd64 1-IPC calibration kernel
//
// NOTE: neg on a register rather than sub $1; some cores special-case a subtraction of an
// immediate and retire it faster than one cycle, which would report a frequency that is too high

namespace micron
{
namespace chrono
{
namespace arch
{

[[gnu::noinline]] inline void
calib_kernel(i64 iters) noexcept
{
  __asm__ __volatile__("movq $-1, %%rax\n\t"
                       "negq %%rax\n\t"
                       ".p2align 5\n"
                       "1:\n\t"
                       "subq %%rax, %0\n\t"
                       "subq %%rax, %0\n\t"
                       "jge 1b"
                       : "+r"(iters)
                       :
                       : "rax", "cc");
}

inline constexpr bool calib_kernel_native = true;

};      // namespace arch
};      // namespace chrono
};      // namespace micron
