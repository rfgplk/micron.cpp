// Copyright (c) 2025 David Lucius Severus
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "../linux/kmemory.hpp"
namespace abc
{
constexpr static const bool __is_constrained = true;     // for if running on a mem constrained system
constexpr static const u64 __system_pagesize = micron::page_size;

constexpr static const u64 __class_arena_internal = 1024;
constexpr static const u64 __class_precise = (2 << 7);     // 256
constexpr static const u64 __class_small = (2 << 8);       // 512
constexpr static const u64 __class_medium = (2 << 11);     // 4096
constexpr static const u64 __class_large = (2 << 14);      // 32,768
constexpr static const u64 __class_huge = (2 << 17);       // 262,133
constexpr static const u64 __class_1mb = (2 << 19);        //  1,048,576, disable if constained
constexpr static const u64 __class_gb = (2 << 29);         // 1,073,741,824, disable if constained
constexpr static const u64 __alloc_limit
    = (1024 << 16);     // forbid any allocations greater than this, active only if __is_constrained is true

// these two switches determine the number of *pages* to allocate on initialization, by default, it's 256 pages for the
// internal abcmalloc metabuffer, and a minimum of 32 per each new sheet allocation
constexpr static const u64 __default_arena_page_buf = 256;
constexpr static const u64 __default_magic_size = micron::numeric_limits<u64>::max();
constexpr static const u64 __default_minimum_page_mul
    = 32;                                                  // 131kB minimum per sheet, larger buckets will exceed this
constexpr static const u64 __default_cache_step = 768;     // ~5.9MB

constexpr static const bool __default_launder
    = false;     // by default is off, laundering lets the allocators allocate same sized requests at the same address
constexpr static const bool __default_construct_on_start
    = true;     // construct on start, if false, you have to construct/start it manually
constexpr static const bool __default_single_instance = false;     // enable an allocator per thread
constexpr static const bool __default_global_instance = true;      // enable a single global allocator
constexpr static const bool __default_multithread_safe
    = true;     // essentially, enables locks across API calls, only necessary if enabling global mode

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
    = 2;     // overcommit multiplier, multiplies all page req. by this value. MUST BE GREATER THAN ONE AND INTEGRAL.

constexpr static const bool __default_init_large_pages = false;
constexpr static const bool __default_oom_enable = true;     // NOTE: costs performance
constexpr static const bool __default_borrow_auto = true;
constexpr static const float __default_oom_limit_warn = 0.1f;
constexpr static const float __default_oom_limit_error = 0.2f;

// enforce provenance forces the allocator to verify if a req. pointer has been allocated within that session. if it
// hasn't fail out according to __default_fail_result
constexpr static const bool __default_enforce_provenance = false;
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
