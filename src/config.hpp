//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"

namespace micron::config
{

#if defined(__micron_freestanding)
inline constexpr bool freestanding = true;
#else
inline constexpr bool freestanding = false;
#endif

inline constexpr bool concurrency_at_startup = !freestanding;

#if defined(__micron_fast_math) && defined(__micron_arch_x86_any) && defined(__micron_x86_sse)
inline constexpr bool fast_math_x86 = true;
#else
inline constexpr bool fast_math_x86 = false;
#endif

};      // namespace micron::config
