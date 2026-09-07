//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// which environment micron is running in
//
//   MICRON_PORT_KERNEL   inside a Linux kernel module. no syscalls; the kernel's own symbols.
//   MICRON_PORT_METAL    bare metal / no OS. weak hooks a board port overrides.
//   (neither)            userspace Linux, the reference backend. [don't use barebones for this; prefer the userland micron branch (master)]

#include "../bits/__arch.hpp"

#if defined(MICRON_PORT_KERNEL) && defined(MICRON_PORT_METAL)
#error "micron port: MICRON_PORT_KERNEL and MICRON_PORT_METAL are mutually exclusive -- pick one target."
#endif

#if defined(MICRON_PORT_KERNEL)
#define __micron_port_kernel 1
#elif defined(MICRON_PORT_METAL)
#define __micron_port_metal 1
#else
#define __micron_port_linux 1
#endif
