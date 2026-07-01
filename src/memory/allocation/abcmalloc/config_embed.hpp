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

constexpr static const bool __is_constrained = true;
constexpr static const usize __system_pagesize = micron::page_size;      // 4096 on ARMv7

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

// 32 MB hard ceiling
constexpr static const usize __alloc_limit = (32 << 20);

// these two switches determine the number of *pages* to allocate on initialization, by default, it's 512 pages for the
// internal abcmalloc metabuffer, and a minimum of 16 per each new sheet allocation

// 512 KB of TLSF cache roughly ~2000 max-size precise blocks
constexpr static const usize __default_cache_size_factor = (1 << 10);

// 64 pages = 256 KB of arena metadata
// 256 KB supports cca 3200 sheet expansions before needing a reload
constexpr static const usize __default_arena_page_buf = 64;

constexpr static const usize __default_magic_size = micron::numeric_limits<usize>::max();

// 65 KB minimum per sheet, larger buckets will exceed this
constexpr static const usize __default_minimum_page_mul = 16;

// 2% of total system RAM. on 128 MB that's 2.6 MB, 256MB -- 5.2MB
constexpr static const f32 __default_prealloc_factor = 0.02f;

// 384 precise blocks per expansion (384 * 256 = 96 KB)
constexpr static const usize __default_cache_step = 384;

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

constexpr static const bool __default_eager_hot_tiers = true;

// per-class free cache disabled on embedded targets
constexpr static const bool __default_per_class_free_cache = false;

constexpr static const u32 __default_tombstone_sweep_interval = 32;

// keep all tiers narrow, old behavior for amd64, default here
constexpr static const u32 __max_sheets_precise = 64;
constexpr static const u32 __max_sheets_small = 64;
constexpr static const u32 __max_sheets_medium = 64;
constexpr static const u32 __max_sheets_large = 64;
constexpr static const u32 __max_sheets_huge = 64;
constexpr static const u32 __max_sheets_arena_internal = 64;

// zero on embedded so the struct collapses to its _count field
constexpr static const u32 __cache_slots_precise = 0;
constexpr static const u32 __cache_slots_small = 0;
constexpr static const u32 __cache_slots_medium = 0;
constexpr static const u32 __cache_slots_large = 0;
constexpr static const u32 __cache_slots_huge = 0;

static_assert(__default_single_instance != __default_global_instance,
              "abcmalloc constexpr: __default_single_instance cannot be set simultaneously with __default_global_instance.");

// hard abort on failure
// embedded systems rarely have a meaningful recovery path for OOM best to crash deterministically
constexpr static const byte __default_fail_result = 0;

// only 1 retry. on a constrained system, if the first expansion fails (OOM or alloc_limit hit), a second attempt will almost certainly fail
// too
constexpr static const usize __default_max_retries = 1;

// is a define so we can intercept at the compilation stage
#define __MICRON_ABCMALLOC_CRITICAL_EXIT 11
// in pages (each page is 4096)
constexpr static const bool __default_saturated_mode = true;      // enables a saturation buffer, which checks the rate at which new
                                                                  // requests are coming in. adjusts allocation space accordingly

constexpr static const usize __default_overcommit = 1;

constexpr static const bool __default_init_large_pages = false;

// OFF assume the user handles memory fully and skillfully
// too wasteful for low cr systems
constexpr static const bool __default_oom_enable = false;
constexpr static const bool __default_borrow_auto = true;

constexpr static const float __default_oom_limit_warn = 0.15f;
constexpr static const float __default_oom_limit_error = 0.08f;
// tighter budget on constrained systems so we notice OOM faster
constexpr static const u32 __default_oom_check_interval = 256;

// enforce provenance forces the allocator to verify if a req. pointer has been allocated within that session. if it
// hasn't fail out according to __default_fail_result
constexpr static const bool __default_enforce_provenance = false;
// NOTE: by enabling this the allocator will never return memory that has been freed to a new allocation
// UNLESS the page on which it resides has been unmapped

// tombstoning off
constexpr static const bool __tombstone_precise = false;
constexpr static const bool __tombstone_small = false;
constexpr static const bool __tombstone_medium = false;
constexpr static const bool __tombstone_large = false;
constexpr static const bool __tombstone_huge = false;
constexpr static const bool __default_tombstone
    = __tombstone_precise || __tombstone_small || __tombstone_medium || __tombstone_large || __tombstone_huge;

constexpr static const bool __default_insert_guard_pages = true;
constexpr static const int __default_guard_page_perms = micron::prot_none;

// NOTE: all of these cost a lot of performance

// self-cleanup on. embedded systems may run the allocator in a subsystem that gets torn down and reconstructed
// (persistent mode pins all memory, so it must override self-cleanup off)
constexpr static const bool __default_self_cleanup = __default_persistent_mode ? false : true;

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

// abort on double free
constexpr static const byte __default_double_free_action = 2;
// 0 == ignore silently (return false, no log)
// 1 == log diagnostic return false
// 2 == abort

// guard pages on arena metadata. protects the node+sheet header region
constexpr static const bool __default_guard_arena_metadata = true;
// insert guard pages at the trail of each arena metadata
// same protection flags as __default_guard_page_perms

constexpr static const bool __default_unsafe_size_recovery = false;
// when true, sz==0 scrub paths read size from addr - __hdr_offset (only for TLSF) when false sz==0 scrubs are skipped

constexpr static const bool __default_check_alignment = false;      // verify that addresses passed to pop/freeze are naturally aligned

constexpr static const bool __default_poison_on_free = false;
// fill freed regions with __default_poison_byte to detect use-after-free.

constexpr static const byte __default_poison_byte = 0x7B;

constexpr static const bool __default_redzone = false;

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
constexpr static const bool __default_doctor_canary = true;           // snapshot the block header + lay a slack canary at alloc
constexpr static const byte __default_doctor_canary_byte = 0x5A;      // slack-canary fill byte
constexpr static const bool __default_doctor_backtrace = false;       // alloc-site backtrace
constexpr static const usize __default_doctor_max_records
    = (1ull << 24);      // soft cap: past this the ledger drops all non-live records (logged)
static_assert(!__default_doctor_rescue || __default_doctor_help, "abcmalloc: doctor rescue requires __default_doctor_help");
static_assert(!__default_doctor_harden || __default_doctor_help, "abcmalloc: doctor harden requires __default_doctor_help");
static_assert(!__default_doctor_rescue_conservative || __default_doctor_rescue,
              "abcmalloc: doctor rescue_conservative requires __default_doctor_rescue");
#endif
};      // namespace abc
