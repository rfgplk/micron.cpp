//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__abc_mt.hpp"      // autofires MICRON_ABC_MT; must precede abcmalloc

// standard header file to include for when you need coroutines
//
// the io_uring reactor is on by default; define MICRON_CORO_NO_URING to disable it

#if defined(__micron_coro_reactor_seen) && !defined(MICRON_CORO_URING) && !defined(MICRON_CORO_NO_URING)
#error "stdcoro.hpp must come before any tasks/ include: the io_uring reactor was already compiled out"
#endif

#if !defined(MICRON_CORO_URING) && !defined(MICRON_CORO_NO_URING)
#define MICRON_CORO_URING
#endif

// scheduler, fibers, reactor, aio, task/generator/spawn, async sync primitives, fork/join
#include "tasks/tasks.hpp"
