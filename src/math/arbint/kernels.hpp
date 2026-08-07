//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../bits/__arch.hpp"

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// hand-written mpn kernels, per arch
//
// no terminal #error; unmatched targets fall through to the __portable constexpr implementation
//
// NOTE: --def MICRON_ARBINT_NO_ASM forces portables

#if !defined(MICRON_ARBINT_NO_ASM)

#if defined(__micron_arch_arm64)
#include "arch/kernels_arm64.hpp"
#elif defined(__micron_arch_arm32)
#include "arch/kernels_arm32.hpp"
#elif defined(__micron_arch_amd64)
#include "arch/kernels_amd64.hpp"
#elif defined(__micron_arch_x86)
#include "arch/kernels_i386.hpp"
#endif

#include "arch/simd_ops.hpp"

// put experimentals here
#include "arch/simd_mul_experiment.hpp"

#endif
