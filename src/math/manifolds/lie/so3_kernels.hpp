//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../../bits/__arch.hpp"

#if defined(__OPTIMIZE__) && defined(__micron_arch_amd64)
#include "arch/so3_amd64.hpp"
#elif defined(__OPTIMIZE__) && defined(__micron_arch_arm32) && defined(__micron_arm_neon)
#include "arch/so3_arm32.hpp"
#elif defined(__OPTIMIZE__) && defined(__micron_arch_arm64) && defined(__micron_arm_neon)
#include "arch/so3_arm64.hpp"
#endif
