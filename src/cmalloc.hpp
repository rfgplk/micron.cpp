//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "defs.hpp"

// only if we're not already including abcmalloc externally
#ifndef MICRON_ABCMALLOC_DISABLE_STD
#ifdef MICRON_ABCMALLOC_STD
#include "allocation/abcmalloc/__abc.hpp"
#include "allocation/abcmalloc/__sys.hpp"
#include "allocation/abcmalloc/malloc.hpp"
#endif
#endif

// alias the namespaces
