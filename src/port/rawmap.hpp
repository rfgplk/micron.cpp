//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::port; raw anonymous pages, below the allocator
//
// raw_map(usize) - raw_unmap(void *, usize)

#include "__backend.hpp"

#if defined(__micron_port_linux)
#include "backends/rawmap_linux.hpp"
#elif defined(__micron_port_kernel)
#include "backends/rawmap_kernel.hpp"
#elif defined(__micron_port_metal)
#include "backends/rawmap_metal.hpp"
#else
#error "micron port: __backend.hpp selected no backend. This is a bug in __backend.hpp, not in your build."
#endif
