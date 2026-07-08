//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../../memory/allocation/abcmalloc/va_reserve.hpp"
#include "../../memory/mman.hpp"
#include "../../memory/stack_constants.hpp"
#include "../../types.hpp"

#include "../../linux/sys/micthread/threads.hpp"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// cactus fiber-stack backing memory
//
// raw carve/release of contiguous, double-guard-paged segments for stackful fibers
//
//   [ low guard | coroutine-frame arena --grows up--> | mid guard | <--grows down-- C exec stack | FCB ]
//     low                                                                                          high
namespace micron
{
namespace fiber
{

// geometric size classes (usable bytes, before the guard page)
inline constexpr usize __seg_size_table[] = {
  static_cast<usize>(micro_stack_size),       // 256 KiB
  static_cast<usize>(thread_stack_size),      // 512 KiB
  static_cast<usize>(small_stack_size),       // 1 MiB
  static_cast<usize>(4) * 1024 * 1024,        // 4 MiB
};
inline constexpr u32 __seg_class_count = sizeof(__seg_size_table) / sizeof(__seg_size_table[0]);

// share of the class size given to the coroutine-frame arena; the remainder is the C exec stack
inline constexpr usize __seg_frame_ratio_num = 3;
inline constexpr usize __seg_frame_ratio_den = 4;

// a carved fiber segment (two regions + two guard pages)
struct __region {
  byte *run = nullptr;              // mapping base == low guard page (PROT_NONE)
  byte *frame_base = nullptr;       // coroutine-frame arena low (grows up)
  byte *frame_limit = nullptr;      // coroutine-frame arena high (== mid guard)
  byte *mid_guard = nullptr;        // PROT_NONE page between the arena and the C stack
  byte *stack_base = nullptr;       // C exec stack low (above mid guard)
  byte *stack_top = nullptr;        // C exec stack high; FCB + initial SP live here
  usize frame_bytes = 0;            // coroutine-frame arena size
  usize stack_bytes = 0;            // C exec stack size
  usize total = 0;                  // whole mapping size
  bool from_va = false;             // carved from __va_carve (vs addrmap fallback)
  u32 cls = 0;                      // size-class index
};

[[gnu::always_inline]] inline u32
__seg_class_for(usize want) noexcept
{
  for ( u32 i = 0; i < __seg_class_count; ++i )
    if ( __seg_size_table[i] >= want ) return i;
  return __seg_class_count - 1;      // clamp to the largest class
}

// [low guard | arena (all the slack) | mid guard | stack]
[[gnu::always_inline]] inline void
__region_geometry(__region &out, byte *run, usize total, usize stack_bytes, u32 cls, bool from_va) noexcept
{
  constexpr usize page = __micron_page_size_default;
  const usize frame_bytes = total - stack_bytes - 2 * page;
  out.run = run;
  out.frame_base = run + page;
  out.frame_limit = out.frame_base + frame_bytes;
  out.mid_guard = out.frame_limit;
  out.stack_base = out.mid_guard + page;
  out.stack_top = out.stack_base + stack_bytes;
  out.frame_bytes = frame_bytes;
  out.stack_bytes = stack_bytes;
  out.total = total;
  out.from_va = from_va;
  out.cls = cls;
}

[[gnu::cold]] inline bool
__carve_region(__region &out, u32 cls) noexcept
{
  if ( cls >= __seg_class_count ) cls = __seg_class_count - 1;
  constexpr usize page = __micron_page_size_default;
  const usize usable = __seg_size_table[cls];
  const usize frame_want = (usable * __seg_frame_ratio_num / __seg_frame_ratio_den + page - 1) & ~(page - 1);
  usize stack_bytes = ((usable - frame_want) + page - 1) & ~(page - 1);
  if ( stack_bytes < page ) stack_bytes = page;

  {
    const usize want = page + frame_want + page + stack_bytes;
    const usize total = (want + abc::__sheet_align_mask) & ~abc::__sheet_align_mask;      // granule-rounded
    byte *run = reinterpret_cast<byte *>(abc::__va_carve_reserved(total));
    if ( run != nullptr ) {
      __region_geometry(out, run, total, stack_bytes, cls, true);
      if ( abc::__va_commit(reinterpret_cast<addr_t *>(out.frame_base), out.frame_bytes) != nullptr
           && abc::__va_commit(reinterpret_cast<addr_t *>(out.stack_base), out.stack_bytes) != nullptr ) [[likely]]
        return true;
      abc::__va_release(reinterpret_cast<addr_t *>(run), total);      // commit failed: hand the run back
    }
  }

  const usize total = page + frame_want + page + stack_bytes;
  byte *run = reinterpret_cast<byte *>(micron::addrmap(total));
  if ( micron::mmap_failed(run) || run == nullptr ) {
    out = __region{};
    return false;
  }
  __region_geometry(out, run, total, stack_bytes, cls, false);
  (void)micron::mprotect(reinterpret_cast<addr_t *>(out.run), page, micron::prot_none);
  (void)micron::mprotect(reinterpret_cast<addr_t *>(out.mid_guard), page, micron::prot_none);
  return true;
}

// release a carved run back to the VA reservation (or unmap the fallback mapping)
inline void
__release_region(const __region &reg) noexcept
{
  if ( reg.run == nullptr ) return;
  if ( reg.from_va )
    abc::__va_release(reinterpret_cast<addr_t *>(reg.run), reg.total);
  else
    micron::try_unmap(reg.run, reg.total);
}

[[gnu::always_inline]] inline void
__decommit_region(const __region &reg) noexcept
{
  if ( reg.frame_base != nullptr && reg.stack_top > reg.frame_base )
    (void)micron::madvise(reinterpret_cast<addr_t *>(reg.frame_base), static_cast<usize>(reg.stack_top - reg.frame_base),
                          /*MADV_DONTNEED*/ 4);
}

};      // namespace fiber
};      // namespace micron
