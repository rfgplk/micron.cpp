//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "bits/__arch.hpp"
#if defined(__micron_arch_wasm32)
#error "micron: Linux process interfaces are unavailable in WebAssembly"
#else

#include "linux/process/child.hpp"
#include "linux/process/exec.hpp"
#include "linux/process/process.hpp"
#include "linux/process/spawn.hpp"
#include "linux/std.hpp"

namespace micron
{
};

#endif
