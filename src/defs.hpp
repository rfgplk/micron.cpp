//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "bits/__arch.hpp"
#include "bits/__profile.hpp"
#include "config.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// which allocator is compiled in
//
// abcmalloc cannot run inside a kernel module or on bare metal, and that is structural rather than
// a porting gap. Measured on this branch: tests/compiletests/barebones_core.cpp built with
// MICRON_PORT_KERNEL and the full kernel flag set emits 41 syscall instructions, every one inside
// abc:: -- __va_reserve_once, __get_kernel_memory, __vmap_freeze_at, the sheet release paths.
// abcmalloc IS a 256 GiB PROT_NONE reservation committed in place with MAP_FIXED
// (abcmalloc/va_reserve.hpp:46-105), and a module has no sparse address space to reserve into;
// port/backends/pages_kernel.hpp cannot offer reserve/commit and says so rather than faking it.
//
// So (K) and (E) select micron::bb instead, with namespace abc aliased onto it
// (memory/allocation/barebones/). MICRON_BAREBONES_ALLOC names that choice SEPARATELY from the
// port backend, deliberately: it lets the stub be built and RUN hosted, over the linux page
// source. An allocator whose only test environment is a kernel module is an allocator nobody tests.
#if defined(MICRON_PORT_KERNEL) || defined(MICRON_PORT_METAL) || defined(MICRON_BAREBONES_ALLOC)
#define __micron_bb_alloc 1
#if !defined(MICRON_ABCMALLOC_DISABLE_STD)
#define MICRON_ABCMALLOC_DISABLE_STD 1
#endif
#endif

#if !defined(MICRON_ABCMALLOC_STD) && !defined(MICRON_ABCMALLOC_DISABLE_STD)
#define MICRON_ABCMALLOC_STD 1
#endif

// under a heap-owning sanitizer the abcmalloc C malloc override must not shadow the sanitizer's own allocator
#if defined(__micron_sanitizer_owns_heap) && !defined(ABCMALLOC_DISABLE)
#define ABCMALLOC_DISABLE 1
#endif

// one predicate for every global C++ allocation path; mixing an interposed allocation with an
// inlined abc deallocation silently strands the foreign block
#if defined(MICRON_ABCMALLOC_STD) && !defined(ABCMALLOC_DISABLE) && !defined(__micron_sanitizer_owns_heap)
#define __micron_abcmalloc_std_backend 1
#endif

// ...and a SECOND, different question: is abcmalloc COMPILED IN at all. cmalloc.hpp answers it by
// skipping the whole include block, so anything that names abc:: must agree with that condition or
// it will not build -- which is exactly why -DMICRON_ABCMALLOC_DISABLE_STD did not
// (allocator_types/bits.hpp named abc::__abc_allocator with no guard whatsoever).
//
// __micron_abcmalloc_std_backend is about whether operator new routes to abc; this is about whether
// abc exists. They are not the same predicate: a sanitizer build sets ABCMALLOC_DISABLE, which
// clears the first and leaves the second true.
#if defined(MICRON_ABCMALLOC_STD) && !defined(MICRON_ABCMALLOC_DISABLE_STD)
#define __micron_abcmalloc_present 1
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
