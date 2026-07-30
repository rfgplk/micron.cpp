//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "__arch.hpp"

#if defined(__micron_freestanding) && !defined(MICRON_ABC_MT)
#if defined(__micron_abc_mt_resolved)
#error                                                                                                                                     \
    "micron: a threading/coroutine header was included after abcmalloc's config had already fixed the freestanding single-thread gate; cross-thread frees in this TU would skip owner routing. Include the threading header before std.hpp/cmalloc.hpp, or build with -DMICRON_ABC_MT (duck: --def MICRON_ABC_MT)."
#endif
#define MICRON_ABC_MT 1
#endif
