//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../linux/kmemory.hpp"
namespace abc
{

constexpr static const u64 __system_pagesize = micron::page_size;

constexpr static const u64 __class_arena_internal = 1024;
constexpr static const u64 __class_precise = (2 << 7);     // 256
constexpr static const u64 __class_small = (2 << 8);       // 512
constexpr static const u64 __class_medium = (2 << 11);     // 4096
constexpr static const u64 __class_large = (2 << 14);      // 32,768
constexpr static const u64 __class_huge = (2 << 17);       // 262,133
constexpr static const u64 __class_1mb = (2 << 19);        //  1,048,576
constexpr static const u64 __class_gb = (2 << 29);         // 1,073,741,824

// these two switches determine the number of *pages* to allocate on initialization, by default, it's 512 pages for the
// internal abcmalloc metabuffer, and a minimum of 32 per each new sheet allocation
constexpr static const u64 __default_arena_page_buf = 512;     // 2MiB for now... ~81k rnd allocations
constexpr static const u64 __default_magic_size = micron::numeric_limits<u64>::max();
constexpr static const u64 __default_minimum_page_mul
    = 32;                                                  // 131kB minimum per sheet, larger buckets will exceed this
constexpr static const u64 __default_cache_step = 768;     // ~5.9MB

constexpr static const bool __default_launder
    = false;     // by default is off, laundering lets the allocators allocate same sized requests at the same address
constexpr static const bool __default_single_instance = false;
constexpr static const bool __default_global_instance = true;
constexpr static const bool __default_multithread_safe = false;     // essentially, enables locks across API calls

static_assert(
    __default_single_instance != __default_global_instance,
    "abcmalloc constexpr: __default_single_instance cannot be set simultaneously with __default_global_instance.");

constexpr static const byte __default_fail_result = 0;     // 0: abort 1: message 2: silent fail
constexpr static const u64 __default_max_retries = 2;
// in pages (each page is 4096)
constexpr static const bool __default_saturated_mode
    = true;     // enables a saturation buffer, which checks the rate at which new requests are coming in. adjusts
                // allocation space accordingly
constexpr static const u64 __default_overcommit
    = 1;     // overcommit multiplier, multiplies all page req. by this value. MUST BE GREATER THAN ONE AND INTEGRAL.

constexpr static const bool __default_init_large_pages = false;
constexpr static const bool __default_oom_enable = false;     // NOTE: costs performance
constexpr static const bool __default_borrow_auto = true;
constexpr static const float __default_oom_limit_warn = 0.1f;
constexpr static const float __default_oom_limit_error = 0.2f;

// enforce provenance forces the allocator to verify if a req. pointer has been allocated within that session. if it
// hasn't fail out according to __default_fail_result
constexpr static const bool __default_enforce_provenance = true;
// NOTE: by enabling this the allocator will never return memory that has been freed to a new allocation
// UNLESS the page on which it resides has been unmapped
constexpr static const bool __default_tombstone = true;
constexpr static const bool __default_insert_guard_pages = false;
constexpr static const int __default_guard_page_perms = micron::prot_none;

// NOTE: all of these cost a lot of performance
constexpr static const bool __default_debug_notices = false;
constexpr static const bool __default_zero_on_alloc = false;
constexpr static const bool __default_zero_on_free = false;
constexpr static const bool __default_full_on_free = false;
constexpr static const bool __default_sanitize = false;
constexpr static const byte __default_sanitize_with_on_alloc = 0xcc;

constexpr static const bool __default_collect_stats = false;
};
