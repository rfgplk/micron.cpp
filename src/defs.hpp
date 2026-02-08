//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#if defined(__unix__) || defined(__APPLE__)
#define PAGE_SIZE_MICRON sysconf(_SC_PAGESIZE)
#else
#error "Unsupported platform."
#endif

#ifndef MICRON_ABCMALLOC_STD
#define MICRON_ABCMALLOC_STD 1
#endif
