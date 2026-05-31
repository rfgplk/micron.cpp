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

#include "../kmemory.hpp"

namespace abc
{

constexpr static const bool __is_constrained = false;
constexpr static const usize __system_pagesize = micron::page_size;

// once an allocation occurs its address is NEVER freed back to the OS nor regranted to a different allocation (even on explicit free()) and
// the sheet it lives on is NEVER unmapped
constexpr static const bool __default_persistent_mode = false;

// shifts defined like this so we can easily pull them up in code
constexpr static const usize __class_arena_internal = 1024;
constexpr static const usize __class_precise_shift = 8;
constexpr static const usize __class_small_shift = 9;
constexpr static const usize __class_medium_shift = 12;
constexpr static const usize __class_large_shift = 15;
constexpr static const usize __class_huge_shift = 18;
constexpr static const usize __class_1mb_shift = 20;
constexpr static const usize __class_gb_shift = 29;
constexpr static const usize __class_precise = (1 << __class_precise_shift);
constexpr static const usize __class_small = (1 << __class_small_shift);
constexpr static const usize __class_medium = (1 << __class_medium_shift);
constexpr static const usize __class_large = (1 << __class_large_shift);
constexpr static const usize __class_huge = (1 << __class_huge_shift);
constexpr static const usize __class_1mb = (1 << __class_1mb_shift);
constexpr static const usize __class_gb = (1 << __class_gb_shift);

constexpr static const usize __alloc_limit = 0;

// these two switches determine the number of *pages* to allocate on initialization, by default, it's 512 pages for the
// internal abcmalloc metabuffer, and a minimum of 16 per each new sheet allocation

// 16 MB TLSF precise cache. Zen4 L3 is 32 MB per CCD
// a 16 MB cache sits comfortably within one CCD's L3 slice and holds ~54000 max-size precise blocks
constexpr static const usize __default_cache_size_factor = (1 << 16);

// 2048 pages = 8 MB arena metadata
// at ~80 bytes per node+sheet pair, supports ~100k sheet expansions
constexpr static const usize __default_arena_page_buf = 2048;

constexpr static const usize __default_magic_size = micron::numeric_limits<usize>::max();
constexpr static const usize __default_minimum_page_mul = 16;      // 65kB minimum per sheet, larger buckets will exceed this

// 1% of system RAM. on 256 GB this is ~2.6 GB distributed across all five size classes by weight
constexpr static const f32 __default_prealloc_factor = 0.01f;

// 8192 precise blocks per expansion = 2 MB
constexpr static const usize __default_cache_step = 8192;

constexpr static const bool __default_launder
    = false;      // by default is off, laundering lets the allocators allocate same sized requests at the same address

constexpr static const bool __default_lazy_construct = true;
constexpr static const bool __default_single_instance = true;       // enable an allocator per thread (DEPRECATED for now)
constexpr static const bool __default_global_instance = false;      // enable a single global allocator (DEPRECATED for now)
// freestanding builds have no threading runtime
#if defined(__micron_freestanding)
constexpr static const bool __default_multithread_safe = false;
#else
constexpr static const bool __default_multithread_safe = true;      // essentially, enables locks across API calls
#endif

// preallocate precise/small/medium at startup with weight-based shares.
constexpr static const bool __default_eager_hot_tiers = true;

constexpr static const bool __default_per_class_free_cache = __default_persistent_mode ? false : true;

// 128 deallocations between sweeps. server workloads sustain high throughput over long periods; sweeping too often serialises dealloc paths
// under contention
constexpr static const u32 __default_tombstone_sweep_interval = 128;

constexpr static const u32 __max_sheets_precise = 1024;
constexpr static const u32 __max_sheets_small = 1024;
constexpr static const u32 __max_sheets_medium = 1024;
constexpr static const u32 __max_sheets_large = 128;
constexpr static const u32 __max_sheets_huge = 64;
constexpr static const u32 __max_sheets_arena_internal = 64;

// free-cache slot counts. server workloads benefit from deeper caches per tier
constexpr static const u32 __cache_slots_precise = 64;
constexpr static const u32 __cache_slots_small = 32;
constexpr static const u32 __cache_slots_medium = 16;
constexpr static const u32 __cache_slots_large = 8;
constexpr static const u32 __cache_slots_huge = 0;

static_assert(__default_single_instance != __default_global_instance,
              "abcmalloc constexpr: __default_single_instance cannot be set simultaneously with __default_global_instance.");

constexpr static const byte __default_fail_result = 0;

// 3 retries. on a 256 GB system, the first expansion failure is likely transient (fragmentation, not true exhaustion).
// gives the predictor two more chances to find a viable size lets the overcommit + predictor heuristic converge.
constexpr static const usize __default_max_retries = 3;

// is a define so we can intercept at the compilation stage
#define __MICRON_ABCMALLOC_CRITICAL_EXIT 11
// in pages (each page is 4096)
constexpr static const bool __default_saturated_mode = true;      // enables a saturation buffer, which checks the rate at which new
                                                                  // requests are coming in. adjusts allocation space accordingly

constexpr static const usize __default_overcommit = 2;

constexpr static const bool __default_init_large_pages = true;

constexpr static const bool __default_oom_enable = false;
constexpr static const bool __default_borrow_auto = true;

constexpr static const float __default_oom_limit_warn = 0.05f;
constexpr static const float __default_oom_limit_error = 0.02f;
constexpr static const u32 __default_oom_check_interval = 1024;

// enforce provenance forces the allocator to verify if a req. pointer has been allocated within that session. if it
// hasn't fail out according to __default_fail_result
constexpr static const bool __default_enforce_provenance = false;
// NOTE: by enabling this the allocator will never return memory that has been freed to a new allocation
// UNLESS the page on which it resides has been unmapped

// tombstoning ON across the board
constexpr static const bool __tombstone_precise = true;
constexpr static const bool __tombstone_small = true;
constexpr static const bool __tombstone_medium = true;
constexpr static const bool __tombstone_large = true;
constexpr static const bool __tombstone_huge = true;
constexpr static const bool __default_tombstone
    = __tombstone_precise || __tombstone_small || __tombstone_medium || __tombstone_large || __tombstone_huge;

constexpr static const bool __default_insert_guard_pages = true;
constexpr static const int __default_guard_page_perms = micron::prot_none;

// NOTE: all of these cost a lot of performance

constexpr static const bool __default_self_cleanup = false;

static_assert(!(__default_persistent_mode && __default_launder),
              "abcmalloc: persistent mode is incompatible with __default_launder (it reuses addresses).");
static_assert(!(__default_persistent_mode && __default_self_cleanup),
              "abcmalloc: persistent mode requires __default_self_cleanup off (arena dtor must not unmap).");
static_assert(!(__default_persistent_mode && __default_per_class_free_cache),
              "abcmalloc: persistent mode requires __default_per_class_free_cache off (it regrants freed blocks).");

constexpr static const bool __default_debug_notices = false;
constexpr static const bool __default_zero_on_alloc = false;
constexpr static const bool __default_zero_on_free = false;
constexpr static const bool __default_full_on_free = false;
constexpr static const bool __default_sanitize = false;
constexpr static const byte __default_sanitize_with_on_alloc = 0xcc;

constexpr static const bool __default_collect_stats = false;

constexpr static const byte __default_double_free_action = 2;
// 0 == ignore silently (return false, no log)
// 1 == log diagnostic return false
// 2 == abort

// guard pages on arena metadata.
constexpr static const bool __default_guard_arena_metadata = true;
// insert guard pages at the trail of each arena metadata
// same protection flags as __default_guard_page_perms

constexpr static const bool __default_unsafe_size_recovery = false;
// when true, sz==0 scrub paths read size from addr - __hdr_offset (only for TLSF) when false sz==0 scrubs are skipped

constexpr static const bool __default_check_alignment = false;      // verify that addresses passed to pop/freeze are naturally aligned

constexpr static const bool __default_poison_on_free = false;
// fill freed regions with __default_poison_byte to detect use-after-free.

constexpr static const byte __default_poison_byte = 0x7B;

// redzones off in production. enable for staging/canary deployments.
constexpr static const bool __default_redzone = false;

constexpr static const byte __default_redzone_byte = 0xC1;
constexpr static const usize __default_redzone_size = 8;
};      // namespace abc
