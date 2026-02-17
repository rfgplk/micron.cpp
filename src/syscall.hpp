//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"

#if defined(__micron_arch_amd64)
#include "linux/sys/syscall_x86_64.hpp"

#include "bits/__syscall_codes_amd64.hpp"
#elif defined(__micron_arch_arm32)
#include "linux/sys/syscall_arm32.hpp"

#include "bits/__syscall_codes_arm32.hpp"
#endif
#pragma GCC diagnostic pop
