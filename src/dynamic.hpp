//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"
#if defined(__micron_arch_wasm32)
#error "micron: native dynamic loading is unavailable in WebAssembly"
#else

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// microns dynamic loading

#include "dynamic/call.hpp"
#include "dynamic/debug.hpp"
#include "dynamic/flags.hpp"
#include "dynamic/load.hpp"
#include "dynamic/paths.hpp"
#include "dynamic/registry.hpp"
#include "dynamic/resolve.hpp"
#include "dynamic/versioning.hpp"

#endif
