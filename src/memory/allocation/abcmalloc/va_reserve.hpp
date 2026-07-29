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

#include "../../../atomic/atomic.hpp"
#include "../../../atomic/flag.hpp"
#include "../../../bits/__profile.hpp"
#include "../../../memory/mman.hpp"
#include "../../../memory/mmap_bits.hpp"
#include "../../../mutex/locks/guard_lock.hpp"
#include "../../../types.hpp"

namespace abc
{

// carve granule
//
// width-32 uses a 64 KiB granule, NOT 2 MiB
#if defined(__micron_arch_width_64)
constexpr static const usize __sheet_align_log2 = 21;      // 2 MiB
#else
constexpr static const usize __sheet_align_log2 = 16;      // 64 KiB == the minimum sheet size
#endif
constexpr static const usize __sheet_align = 1ULL << __sheet_align_log2;
constexpr static const usize __sheet_align_mask = __sheet_align - 1;

#ifndef MICRON_ABC_VA_RESERVE_SIZE
#if defined(__micron_arch_width_64)
// 256 GiB
#define MICRON_ABC_VA_RESERVE_SIZE (256ULL << 30)
#else
// 1 GiB. 256 MiB was not enough to hold the coroutine engine
#define MICRON_ABC_VA_RESERVE_SIZE (1024U << 20)
#endif
#endif
constexpr static const usize __va_reservation_size = MICRON_ABC_VA_RESERVE_SIZE;
static_assert(__va_reservation_size >= __sheet_align, "abcmalloc: MICRON_ABC_VA_RESERVE_SIZE must be at least one sheet granule.");
static_assert((__va_reservation_size & __sheet_align_mask) == 0,
              "abcmalloc: MICRON_ABC_VA_RESERVE_SIZE must be a whole multiple of the sheet granule.");

constexpr static const i32 __map_noreserve_flag = 0x4000;

inline micron::atomic_token<addr_t *> __va_base{ nullptr };      // PROT_NONE base, or nullptr if not yet reserved
inline micron::atomic_token<u64> __va_offset{ 0 };               // bump cursor in bytes
inline micron::atomic_flag __va_init_lock{};                     // one-shot init guard

struct __va_free_run {
  u64 off;           // byte offset from __va_base
  u32 granules;      // run length in __sheet_align granules
};

constexpr static const usize __va_free_granules = __va_reservation_size >> __sheet_align_log2;
#if defined(__micron_arch_width_64)
constexpr static const usize __va_free_cap = __va_free_granules;
#else
constexpr static const usize __va_free_cap = __va_free_granules > 1024 ? usize{ 1024 } : __va_free_granules;
#endif
inline __va_free_run __va_free_runs[__va_free_cap]{};
inline usize __va_free_count{ 0 };
inline micron::atomic_flag __va_free_lock{};

[[gnu::cold, gnu::noinline]] inline addr_t *
__va_reserve_once() noexcept
{
  addr_t *base = __va_base.get(micron::memory_order_acquire);
  if ( base ) return base;

  // serialize the actual mmap; double-checked under the lock so contenders just observe the publish
  micron::free_guard<> guard{ &__va_init_lock };
  base = __va_base.get(micron::memory_order_relaxed);
  if ( base ) return base;

  base = micron::mmap(nullptr, __va_reservation_size, micron::prot_none, micron::map_private | micron::map_anonymous | __map_noreserve_flag,
                      -1, 0);
  if ( micron::mmap_failed(base) || !base ) {
    return nullptr;
  }
  __va_base.store(base, micron::memory_order_release);
  return base;
}

// commit a carved run: replace its PROT_NONE backing with PROT_READ|WRITE anonymous pages
[[gnu::always_inline]] inline addr_t *
__va_commit(addr_t *slot, usize rounded) noexcept
{
  addr_t *got = micron::mmap(slot, rounded, micron::prot_read | micron::prot_write,
                             micron::map_private | micron::map_anonymous | micron::map_fixed, -1, 0);
  if ( micron::mmap_failed(got) || got != slot ) [[unlikely]]
    return nullptr;
  return slot;
}

constexpr static const u64 __va_bump_fail = ~static_cast<u64>(0);

[[gnu::always_inline]] inline u64
__va_bump(usize rounded) noexcept
{
  u64 off = __va_offset.get(micron::memory_order_acquire);
  for ( ;; ) {
    if ( off + rounded > __va_reservation_size ) [[unlikely]]
      return __va_bump_fail;
    if ( __va_offset.compare_exchange_weak(off, off + rounded, micron::memory_order_acq_rel, micron::memory_order_acquire) ) return off;
  }
}

inline u64
__va_reuse(u32 want) noexcept
{
  constexpr usize __none = ~static_cast<usize>(0);
  micron::free_guard<> guard{ &__va_free_lock };
  usize best = __none;
  for ( usize i = __va_free_count; i-- > 0; ) {
    const u32 g = __va_free_runs[i].granules;
    if ( g < want ) continue;
    if ( best == __none || g < __va_free_runs[best].granules ) best = i;
    if ( g == want ) break;      // exact fit, stop looking
  }
  if ( best == __none ) return __va_bump_fail;
  const u64 off = __va_free_runs[best].off;
  const u32 g = __va_free_runs[best].granules;
  if ( g > want ) {      // hand back the head, keep the shrunk tail in place
    __va_free_runs[best].off = off + (static_cast<u64>(want) << __sheet_align_log2);
    __va_free_runs[best].granules = g - want;
  } else {
    __va_free_runs[best] = __va_free_runs[--__va_free_count];      // swap-remove
  }
  return off;
}

inline addr_t *
__va_carve(usize bytes) noexcept
{
  addr_t *base = __va_base.get(micron::memory_order_acquire);
  if ( !base ) [[unlikely]] {
    base = __va_reserve_once();
    if ( !base ) return nullptr;
  }

  const usize rounded = (bytes + __sheet_align_mask) & ~__sheet_align_mask;
  const u32 want = static_cast<u32>(rounded >> __sheet_align_log2);

  const u64 reuse_off = __va_reuse(want);
  if ( reuse_off != __va_bump_fail ) {
    addr_t *slot = reinterpret_cast<addr_t *>(reinterpret_cast<uintptr_t>(base) + reuse_off);
    if ( addr_t *got = __va_commit(slot, rounded); got ) [[likely]]
      return got;
    // remap failed: the run is now dropped from the list (effectively leaked); fall through to a fresh carve
  }

  const u64 off = __va_bump(rounded);
  if ( off == __va_bump_fail ) [[unlikely]]
    return nullptr;      // reservation exhausted

  return __va_commit(reinterpret_cast<addr_t *>(reinterpret_cast<uintptr_t>(base) + off), rounded);
}

inline addr_t *
__va_carve_reserved(usize bytes) noexcept
{
  addr_t *base = __va_base.get(micron::memory_order_acquire);
  if ( !base ) [[unlikely]] {
    base = __va_reserve_once();
    if ( !base ) return nullptr;
  }

  const usize rounded = (bytes + __sheet_align_mask) & ~__sheet_align_mask;
  const u32 want = static_cast<u32>(rounded >> __sheet_align_log2);

  const u64 reuse_off = __va_reuse(want);
  if ( reuse_off != __va_bump_fail ) return reinterpret_cast<addr_t *>(reinterpret_cast<uintptr_t>(base) + reuse_off);

  const u64 off = __va_bump(rounded);
  if ( off == __va_bump_fail ) [[unlikely]]
    return nullptr;      // reservation exhausted
  return reinterpret_cast<addr_t *>(reinterpret_cast<uintptr_t>(base) + off);
}

inline void
__va_release(addr_t *slot, usize bytes) noexcept
{
  if ( !slot ) return;
  addr_t *base = __va_base.get(micron::memory_order_relaxed);
  if ( !base ) return;
  const uintptr_t si = reinterpret_cast<uintptr_t>(slot);
  const uintptr_t bi = reinterpret_cast<uintptr_t>(base);
  if ( si < bi || si >= bi + __va_reservation_size ) [[unlikely]]
    return;      // not a carved VA slot; nothing to reclaim
  const usize rounded = (bytes + __sheet_align_mask) & ~__sheet_align_mask;
  // replace the populated region with PROT_NONE again to release physical pages (keeps the VA reserved)
  (void)micron::mmap(slot, rounded, micron::prot_none,
                     micron::map_private | micron::map_anonymous | micron::map_fixed | __map_noreserve_flag, -1, 0);
  const u64 off = static_cast<u64>(si - bi);
  const u32 granules = static_cast<u32>(rounded >> __sheet_align_log2);
  micron::free_guard<> guard{ &__va_free_lock };
  if ( __va_free_count < __va_free_cap ) __va_free_runs[__va_free_count++] = __va_free_run{ off, granules };
}

[[gnu::always_inline]] inline bool
__va_contains(const void *p) noexcept
{
  addr_t *base = __va_base.get(micron::memory_order_relaxed);
  if ( !base ) return false;
  const uintptr_t pi = reinterpret_cast<uintptr_t>(p);
  const uintptr_t bi = reinterpret_cast<uintptr_t>(base);
  return pi >= bi && pi < bi + __va_reservation_size;
}

};      // namespace abc
