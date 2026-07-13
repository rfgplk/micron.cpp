//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "intrin.hpp"

#if defined(__micron_arch_x86_any)
#include "aliases/aes.hpp"
#include "aliases/avx.hpp"
#include "aliases/avx2.hpp"
#include "aliases/avx512.hpp"
#include "aliases/bmi.hpp"
#include "aliases/fma.hpp"
#include "aliases/sse.hpp"
#endif

#if defined(__micron_arch_arm64) && defined(__micron_arm_neon)
#include "aliases/neon.hpp"
#include "aliases/neon_aes.hpp"
#endif

#if defined(__micron_arch_arm32) && defined(__micron_arm_neon)
#include "aliases/neon.hpp"
#include "aliases/neon_aes.hpp"
#endif
