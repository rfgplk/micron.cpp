// Copyright (c) 2026 David Lucius Severus
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

#include "../../../bits/__profile.hpp"
#include "../../../types.hpp"

#if defined(BBMALLOC_CONFIG_HEADER)
#include BBMALLOC_CONFIG_HEADER
#elif defined(__BB_TINY) || defined(MICRON_PROFILE_TINY)
#include "config_tiny.hpp"
#elif defined(__BB_KERNEL)
#include "config_kernel.hpp"
#else
#include "config_small.hpp"
#endif

#ifndef MICRON_BB_FAIL_RESULT
#define MICRON_BB_FAIL_RESULT 0
#endif
#ifndef MICRON_BB_DOUBLE_FREE_ACTION
#define MICRON_BB_DOUBLE_FREE_ACTION 1
#endif
#ifndef MICRON_BB_ZERO_ON_ALLOC
#define MICRON_BB_ZERO_ON_ALLOC 0
#endif
#ifndef MICRON_BB_ZERO_ON_FREE
#define MICRON_BB_ZERO_ON_FREE 0
#endif
#ifndef MICRON_BB_POISON_ON_FREE
#define MICRON_BB_POISON_ON_FREE 0
#endif
#ifndef MICRON_BB_REDZONE
#define MICRON_BB_REDZONE 0
#endif
#ifndef MICRON_BB_REDZONE_SIZE
#define MICRON_BB_REDZONE_SIZE 16
#endif
#ifndef MICRON_BB_REDZONE_ACTION
#define MICRON_BB_REDZONE_ACTION 1
#endif
#ifndef MICRON_BB_DEBUG_NOTICES
#define MICRON_BB_DEBUG_NOTICES 0
#endif
#ifndef MICRON_BB_LINK_CHECKS
#define MICRON_BB_LINK_CHECKS 1
#endif

#if defined(BB_HOT_GATE_NEGATIVE)
#define __bb_hot [[gnu::noinline]] inline
#elif defined(__OPTIMIZE_SIZE__)
#define __bb_hot inline
#else
#define __bb_hot [[gnu::always_inline]] inline
#endif
#define __bb_cold [[gnu::cold]] [[gnu::noinline]] inline
#define __bb_slow [[gnu::noinline]] inline

namespace bb
{

constexpr static const usize __default_min_block = MICRON_BB_MIN_BLOCK;
constexpr static const usize __default_max_region_bytes = MICRON_BB_MAX_REGION_BYTES;
constexpr static const u32 __max_regions = MICRON_BB_MAX_REGIONS;
constexpr static const usize __class_small = MICRON_BB_CLASS_SMALL;
constexpr static const usize __default_tlsf_sheet = MICRON_BB_SHEET;
constexpr static const u32 __max_tlsf_sheets = MICRON_BB_MAX_SHEETS;
constexpr static const i32 __sl_log2 = MICRON_BB_SL_LOG2;
constexpr static const usize __default_max_alignment = MICRON_BB_MAX_ALIGN;
constexpr static const u32 __default_order_cache_cap = MICRON_BB_CACHE_CAP;
constexpr static const i32 __default_order_cache_max_order = MICRON_BB_CACHE_MAX_ORDER;

constexpr static const int __default_fail_result = MICRON_BB_FAIL_RESULT;
constexpr static const int __default_double_free_action = MICRON_BB_DOUBLE_FREE_ACTION;
constexpr static const int __default_redzone_action = MICRON_BB_REDZONE_ACTION;

constexpr static const bool __default_zero_on_alloc = MICRON_BB_ZERO_ON_ALLOC;
constexpr static const bool __default_zero_on_free = MICRON_BB_ZERO_ON_FREE;
constexpr static const bool __default_poison_on_free = MICRON_BB_POISON_ON_FREE;
constexpr static const byte __default_poison_byte = 0x7B;
constexpr static const bool __default_redzone = MICRON_BB_REDZONE;
constexpr static const usize __default_redzone_size = MICRON_BB_REDZONE_SIZE;
constexpr static const byte __default_redzone_byte = 0xC1;
constexpr static const bool __default_debug_notices = MICRON_BB_DEBUG_NOTICES;
constexpr static const bool __default_link_checks = MICRON_BB_LINK_CHECKS != 0;

#if defined(MICRON_BB_STATS)
constexpr static const bool __default_collect_stats = true;
#else
constexpr static const bool __default_collect_stats = false;
#endif

constexpr static const i32 __max_orders = []() constexpr {
  i32 r = 0;
  usize v = __default_max_region_bytes / __default_min_block;
  while( v > 0 ) {
    v >>= 1;
    ++r;
  }
  return r;
}();

constexpr static const usize __abi_id = []() constexpr {
  usize h = static_cast<usize>(0x9E3779B97F4A7C15ull);
  const usize k[] = { __default_min_block,
                      __default_max_region_bytes,
                      static_cast<usize>(__max_regions),
                      __class_small,
                      __default_tlsf_sheet,
                      static_cast<usize>(__max_tlsf_sheets),
                      static_cast<usize>(__sl_log2),
                      __default_max_alignment,
                      static_cast<usize>(__default_order_cache_cap),
                      static_cast<usize>(__max_orders),
                      __default_redzone ? __default_redzone_size : 0 };
  for( usize i = 0; i < sizeof(k) / sizeof(k[0]); ++i ) {
    h ^= k[i] + static_cast<usize>(0x165667B19E3779F9ull) + (h << 6) + (h >> 2);
  }
  return h;
}();

static_assert((__default_min_block & (__default_min_block - 1)) == 0, "bb: min block must be a power of two");
static_assert(__default_min_block >= 2 * sizeof(void *) && __default_min_block >= 16, "bb: min block too small");
static_assert(__class_small % 16 == 0, "bb: __class_small must be a multiple of 16");
static_assert((__default_tlsf_sheet & (__default_tlsf_sheet - 1)) == 0, "bb: sheet must be a power of two");
static_assert(__default_tlsf_sheet >= 1024 && __default_tlsf_sheet % __default_min_block == 0, "bb: sheet size invalid");
static_assert(__class_small <= __default_tlsf_sheet / 16, "bb: __class_small too large for the sheet");
static_assert((__default_max_alignment & (__default_max_alignment - 1)) == 0
                  && __default_max_alignment >= __default_min_block,
              "bb: max alignment invalid");
static_assert(__default_max_region_bytes <= 0xFFFFFFFFull && __default_max_region_bytes >= 4 * __default_tlsf_sheet,
              "bb: max region bytes out of range");
static_assert(__sl_log2 >= 1 && __sl_log2 <= 5, "bb: __sl_log2 out of range");
static_assert(__max_regions >= 1 && __max_regions <= 8, "bb: __max_regions out of range");
static_assert(__max_tlsf_sheets >= 1 && __max_tlsf_sheets <= 64, "bb: __max_tlsf_sheets out of range");
static_assert(__default_order_cache_cap <= 4, "bb: cache cap out of range");
static_assert(__default_fail_result >= 0 && __default_fail_result <= 2, "bb: fail result out of range");
static_assert(__default_double_free_action >= 0 && __default_double_free_action <= 2,
              "bb: double free action out of range");
static_assert(__default_redzone_action >= 0 && __default_redzone_action <= 2, "bb: redzone action out of range");
static_assert(__default_redzone_size % 16 == 0, "bb: redzone must be a multiple of 16");
static_assert(!__default_redzone || __default_redzone_size < __class_small,
              "bb: redzone must fit under the small class");
static_assert(__max_orders >= 4 && __max_orders <= 63, "bb: derived order count out of range");
#if defined(__cpp_exceptions)
#else
static_assert(__default_fail_result != 2, "bb: exception fail policy without exceptions");
#endif

};
