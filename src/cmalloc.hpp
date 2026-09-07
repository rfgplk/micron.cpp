//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "defs.hpp"

// only if we're not already including abcmalloc externally
#if defined(__micron_abcmalloc_present)
#include "memory/allocation/abcmalloc/__abc.hpp"
#include "memory/allocation/abcmalloc/__sys.hpp"
#include "memory/allocation/abcmalloc/malloc.hpp"
#elif defined(__micron_bb_alloc)
// the barebones tier: micron::bb, with namespace abc aliased onto it so that the 18 keep-set
// headers naming abc:: resolve unchanged. See defs.hpp for why abcmalloc is not an option here.
#include "memory/allocation/barebones/abc_shim.hpp"
#endif

// alias the namespaces
