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

constexpr static const bool __is_constrained = false;      // for if running on a mem constrained system
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
constexpr static const usize __class_gb_shift = 29;      // go from ~500mb
constexpr static const usize __class_precise = (1 << __class_precise_shift);
constexpr static const usize __class_small = (1 << __class_small_shift);
constexpr static const usize __class_medium = (1 << __class_medium_shift);
constexpr static const usize __class_large = (1 << __class_large_shift);
constexpr static const usize __class_huge = (1 << __class_huge_shift);
constexpr static const usize __class_1mb = (1 << __class_1mb_shift);
constexpr static const usize __class_gb = (1 << __class_gb_shift);
constexpr static const usize __alloc_limit = 0;      // forbid any allocations greater than this, active only if __is_constrained is true

// these two switches determine the number of *pages* to allocate on initialization, by default, it's 512 pages for the
// internal abcmalloc metabuffer, and a minimum of 16 per each new sheet allocation
constexpr static const usize __default_cache_size_factor = (1 << 13);      // 1 MB total (mults by class_precise)
constexpr static const usize __default_arena_page_buf = 512;               // 2MiB for now... ~81k rnd allocations
constexpr static const usize __default_magic_size = micron::numeric_limits<usize>::max();
constexpr static const usize __default_minimum_page_mul = 16;        // 65kB minimum per sheet, larger buckets will exceed this
constexpr static const f32 __default_prealloc_factor = 0.0075f;      // 0.75% of total system mem
constexpr static const usize __default_cache_step = 768;             // ~5.9MB

constexpr static const bool __default_launder
    = false;      // by default is off, laundering lets the allocators allocate same sized requests at the same address
constexpr static const bool __default_lazy_construct = true;        // should buckets be initialized lazily or all at startup
constexpr static const bool __default_single_instance = true;       // enable an allocator per thread (DEPRECATED for now)
constexpr static const bool __default_global_instance = false;      // enable a single global allocator (DEPRECATED for now)
// freestanding builds have no threading runtime
#if defined(__micron_freestanding)
constexpr static const bool __default_multithread_safe = false;
#else
constexpr static const bool __default_multithread_safe = true;      // essentially, enables locks across API calls
#endif
constexpr static const bool __default_eager_hot_tiers
    = true;      // eagerly preallocate precise/small/medium with weight-based shares even if __default_lazy_construct is true

// per-class free cache, caches recent allocations in a free list for rapid allocs
// (persistent mode forces this off: a cached block is a regrant of freed memory)
constexpr static const bool __default_per_class_free_cache = __default_persistent_mode ? false : true;

// 0 == per-dealloc tombstone ratio check (pre 1.1 behav.)
// >0 == batch only sweep a tier's sheets every N deallocations
constexpr static const u32 __default_tombstone_sweep_interval = 64;

// per-tier sheet caps (multi-word __space_mask bitmap)
// hot tiers (precise/small/medium) carry frequent small-allocation pressure;
// cold tiers (large/huge) rarely exceed 64 sheets, so keeping them narrow conserves the per-arena tier footprint
// NOTE: each tier costs sizeof(__range)=24B per __idx slot + ceil(N/64)*8B for the bitmap
constexpr static const u32 __max_sheets_precise = 512;
constexpr static const u32 __max_sheets_small = 512;
constexpr static const u32 __max_sheets_medium = 512;
constexpr static const u32 __max_sheets_large = 128;
constexpr static const u32 __max_sheets_huge = 128;      // doubled to absorb sustained huge-band pressure
constexpr static const u32 __max_sheets_arena_internal = 64;

// per-tier free-cache slot counts (LIFO depth per tier)
// 0 disables the cache for that tier
// worst-case memory pinning = sum(slots * max_block_size) ~176 KiB at the defaults below
constexpr static const u32 __cache_slots_precise = 32;      // 1-256 B blocks
constexpr static const u32 __cache_slots_small = 16;        // 257-512 B
constexpr static const u32 __cache_slots_medium = 8;        // 513 B - 4 KiB
constexpr static const u32 __cache_slots_large = 4;         // 4 K - 32 KiB
constexpr static const u32 __cache_slots_huge = 0;          // disabled (rare, large)

static_assert(__default_single_instance != __default_global_instance,
              "abcmalloc constexpr: __default_single_instance cannot be set simultaneously with __default_global_instance.");

constexpr static const byte __default_fail_result = 0;      // 0: abort 1: message 2: silent fail
constexpr static const usize __default_max_retries = 2;

// is a define so we can intercept at the compilation stage
#define __MICRON_ABCMALLOC_CRITICAL_EXIT 11
// in pages (each page is 4096)
constexpr static const bool __default_saturated_mode = true;      // enables a saturation buffer, which checks the rate at which new
                                                                  // requests are coming in. adjusts allocation space accordingly
constexpr static const usize __default_overcommit
    = 1;      // overcommit multiplier, multiplies all page req. by this value. MUST BE GREATER THAN ONE AND INTEGRAL.

constexpr static const bool __default_init_large_pages = false;
constexpr static const bool __default_oom_enable = false;      // NOTE: costs performance
constexpr static const bool __default_borrow_auto = true;
constexpr static const float __default_oom_limit_warn = 0.1f;
constexpr static const float __default_oom_limit_error = 0.2f;
constexpr static const u32 __default_oom_check_interval = 1024;

// enforce provenance forces the allocator to verify if a req. pointer has been allocated within that session. if it
// hasn't fail out according to __default_fail_result
constexpr static const bool __default_enforce_provenance = false;
// NOTE: by enabling this the allocator will never return memory that has been freed to a new allocation
// UNLESS the page on which it resides has been unmapped
//
// per-tier tombstone policy; empirically pulled from benching, best results on hot reuse
constexpr static const bool __tombstone_precise = false;
constexpr static const bool __tombstone_small = false;
constexpr static const bool __tombstone_medium = false;
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

constexpr static const bool __default_guard_arena_metadata = true;
// insert guard pages at the trail of each arena metadata
// same protection flags as __default_guard_page_perms

constexpr static const bool __default_unsafe_size_recovery = false;
// when true, sz==0 scrub paths read size from addr - __hdr_offset (only for TLSF) when false sz==0 scrubs are skipped

constexpr static const bool __default_check_alignment = false;      // verify that addresses passed to pop/freeze are naturally aligned

constexpr static const bool __default_poison_on_free = false;
// fill freed regions with __default_poison_byte to detect use-after-free.

constexpr static const byte __default_poison_byte = 0x7B;

constexpr static const bool __default_redzone = false;      // insert canary bytes before/after user region on TLSF allocations
                                                            // NOTE: uses 2 * __default_redzone_size bytes per alloc

constexpr static const byte __default_redzone_byte = 0xC1;
constexpr static const usize __default_redzone_size = 8;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// doctor mode configuration
// (enabled via -DABCMALLOC_DOCTOR_HELP)
#if defined(ABCMALLOC_DOCTOR_HELP)
constexpr static const bool __default_doctor_help = true;
#if defined(ABCMALLOC_DOCTOR_RESCUE)
constexpr static const bool __default_doctor_rescue = true;      // attempt repair vs report-only
#else
constexpr static const bool __default_doctor_rescue = false;
#endif
#if defined(ABCMALLOC_DOCTOR_RESCUE_CONSERVATIVE)
constexpr static const bool __default_doctor_rescue_conservative
    = true;      // escape hatch: reroute/skip/quarantine only, no in-place repair
#else
constexpr static const bool __default_doctor_rescue_conservative = false;
#endif
constexpr static const bool __default_doctor_harden = true;      // auto-enable redzone/guard/provenance/poison for deep detection
constexpr static const bool __default_doctor_leak_report_at_exit = true;      // dump the live-set at thread/process exit
constexpr static const int __default_doctor_color = 2;      // forensic-output ANSI coloring: 0=never 1=always 2=auto (tty-detect on fd 2)
constexpr static const bool __default_doctor_crash_safe
    = true;      // install a fault-time SIGSEGV/SIGBUS handler so the sweep/forensics survive faulting on corrupt/unmapped memory
constexpr static const bool __default_doctor_canary = true;                    // snapshot the block header + lay a slack canary at alloc
constexpr static const byte __default_doctor_canary_byte = 0x5A;               // slack-canary fill byte
constexpr static const bool __default_doctor_backtrace = true;                 // capture an alloc-site backtrace by frame-pointer walking
constexpr static const usize __default_doctor_max_records = (1ull << 24);      // soft cap: past this the ledger drops all non-live records
static_assert(!__default_doctor_rescue || __default_doctor_help, "abcmalloc: doctor rescue requires __default_doctor_help");
static_assert(!__default_doctor_harden || __default_doctor_help, "abcmalloc: doctor harden requires __default_doctor_help");
static_assert(!__default_doctor_rescue_conservative || __default_doctor_rescue,
              "abcmalloc: doctor rescue_conservative requires __default_doctor_rescue");
#endif
};      // namespace abc
