//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::port; monotonic and wall-clock time, in nanoseconds
//
// mono_ticks() - real_ticks() - ticks_per_sec() - sleep_ns(i64)

#include "__backend.hpp"

#if defined(__micron_port_linux)
#include "backends/clock_linux.hpp"
#elif defined(__micron_port_kernel)
#include "backends/clock_kernel.hpp"
#elif defined(__micron_port_metal)
#include "backends/clock_metal.hpp"
#else
#error "micron port: __backend.hpp selected no backend. This is a bug in __backend.hpp, not in your build."
#endif
