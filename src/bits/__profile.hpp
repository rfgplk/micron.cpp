//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

// NOTE: should stay pure preprocessor and include nothing, avoid ordering spaghetti
//
// WARNING: every header that reads one of these knobs ___MUST___ include this file directly

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// MICRON_PROFILE_TINY
#if defined(MICRON_PROFILE_TINY)

#ifndef MICRON_ABC_EAGER_HOT_TIERS
#define MICRON_ABC_EAGER_HOT_TIERS false      // the big one: no prealloc_share mappings, no sysinfo
#endif
#ifndef MICRON_ABC_ARENA_PAGE_BUF
#define MICRON_ABC_ARENA_PAGE_BUF 64      // 256 KiB requested, floors at the 2 MiB granule
#endif
#ifndef MICRON_ABC_CACHE_SIZE_FACTOR
#define MICRON_ABC_CACHE_SIZE_FACTOR 512      // 128 KiB requested, floors at the 2 MiB granule
#endif
#ifndef MICRON_ABC_VA_RESERVE_SIZE
#if __SIZEOF_POINTER__ == 8
#define MICRON_ABC_VA_RESERVE_SIZE (4ULL << 30)      // 4 GiB: owner table 1 MiB -> 16 KiB, free runs 2 MiB -> 32 KiB
#else
#define MICRON_ABC_VA_RESERVE_SIZE (1024U << 20)      // 1 GiB == the width-32 default; 4 GiB truncates to 0 in a 32-bit usize
#endif
#endif
#ifndef MICRON_ABC_MAX_ARENAS
#define MICRON_ABC_MAX_ARENAS 4      // ~210 KiB of pool storage, overflow list covers the rest
#endif
#ifndef MICRON_ATEXIT_CAP
#define MICRON_ATEXIT_CAP 64      // 64 KiB -> 1 KiB
#endif
#ifndef MICRON_MEM_NO_PROBE
#define MICRON_MEM_NO_PROBE 1      // drops the serializing cpuid burst in __micron_mem_init
#endif

#endif      // MICRON_PROFILE_TINY
