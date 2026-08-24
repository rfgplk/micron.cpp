//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once
#include "bits/__compilers.hpp"

__micron_diagnostic_push
__micron_diagnostic_ignored("-Wall")
__micron_diagnostic_ignored("-Wextra")
__micron_diagnostic_ignored("-Wpedantic")
#include "bits/__arch.hpp"

#if defined(__micron_arch_amd64)
#include "linux/sys/syscall_x86_64.hpp"

#include "bits/__syscall_codes_amd64.hpp"
#elif defined(__micron_arch_arm32)
#include "linux/sys/syscall_arm32.hpp"

#include "bits/__syscall_codes_arm32.hpp"
#elif defined(__micron_arch_x86)
#include "linux/sys/syscall_i386.hpp"

#include "bits/__syscall_codes_i386.hpp"
#elif defined(__micron_arch_arm64)
#include "linux/sys/syscall_arm64.hpp"

#include "bits/__syscall_codes_arm64.hpp"
#endif
__micron_diagnostic_pop
