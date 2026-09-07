//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::port; interrupt masking
//
// irq_state - irq_save() - irq_restore(state)

#include "__backend.hpp"

#if defined(__micron_port_linux)
#include "backends/irq_linux.hpp"
#elif defined(__micron_port_kernel)
#include "backends/irq_kernel.hpp"
#elif defined(__micron_port_metal)
#include "backends/irq_metal.hpp"
#else
#error "micron port: __backend.hpp selected no backend. This is a bug in __backend.hpp, not in your build."
#endif
