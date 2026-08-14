//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../types.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// aarch64 1-IPC calibration kernel

namespace micron
{
namespace chrono
{
namespace arch
{

[[gnu::noinline]] inline void
calib_kernel(i64 iters) noexcept
{
  __asm__ __volatile__("mov x9, #1\n\t"
                       ".p2align 5\n"
                       "1:\n\t"
                       "subs %0, %0, x9\n\t"
                       "subs %0, %0, x9\n\t"
                       "b.ge 1b"
                       : "+r"(iters)
                       :
                       : "x9", "cc");
}

inline constexpr bool calib_kernel_native = true;

};      // namespace arch
};      // namespace chrono
};      // namespace micron
