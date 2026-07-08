//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"
#include "config.hpp"

#ifndef MICRON_ABCMALLOC_STD
#define MICRON_ABCMALLOC_STD 1
#endif

// under a heap-owning sanitizer the abcmalloc C malloc override must not shadow the sanitizer's own allocator
#if defined(__micron_sanitizer_owns_heap) && !defined(ABCMALLOC_DISABLE)
#define ABCMALLOC_DISABLE 1
#endif

#if !defined(__micron_freestanding)
#define TWORKERS 1
#endif

#ifdef TWORKERS
#define __micron_enable_concurrency_at_startup_var
#endif

// dynamically fire up the config for abcmalloc depending on target
#if defined(__micron_arch_amd64) || defined(__micron_arch_arm64)
#define __ABC_AMD64
// technically not always true, but for our use cases it effectively is
// extending to x86 as well, the EMBED profile near perfectly matches classical x86 environments
#elif defined(__micron_arch_arm32) || defined(__micron_arch_x86)
#define __ABC_EMBED
#endif

namespace micron::except
{
// NOTE: this must be enabled for tests
// freestanding defaults to aborting unless the exception trampoline (__micron_eh) has been compiled in
#if defined(__micron_freestanding) && !defined(__micron_eh)
constexpr static const bool __use_exceptions = false;
#else
constexpr static const bool __use_exceptions = true;
#endif
};      // namespace micron::except
