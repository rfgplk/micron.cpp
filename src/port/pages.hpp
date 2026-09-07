//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// micron::port; page acquisition and the heap's extent
//
// page_size - large_page_size - page_alloc/page_free - page_reserve/page_commit/page_decommit - page_protect - page_discard - heap_extent() - addr_readable()

#include "__backend.hpp"

#if defined(__micron_port_linux)
#include "backends/pages_linux.hpp"
#elif defined(__micron_port_kernel)
#include "backends/pages_kernel.hpp"
#elif defined(__micron_port_metal)
#include "backends/pages_metal.hpp"
#else
#error "micron port: __backend.hpp selected no backend. This is a bug in __backend.hpp, not in your build."
#endif
