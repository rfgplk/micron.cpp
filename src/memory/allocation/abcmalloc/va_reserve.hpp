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
#include "../../../memory/mman.hpp"
#include "../../../memory/mmap_bits.hpp"
#include "../../../mutex/locks/guard_lock.hpp"
#include "../../../types.hpp"

namespace abc
{

constexpr static const usize __sheet_align_log2 = 21;
constexpr static const usize __sheet_align = 1ULL << __sheet_align_log2;
constexpr static const usize __sheet_align_mask = __sheet_align - 1;

#if defined(__micron_arch_width_64)
// 256 GiB
constexpr static const usize __va_reservation_size = 256ULL << 30;
#else
// 256 MiB
constexpr static const usize __va_reservation_size = 256U << 20;
#endif

constexpr static const i32 __map_noreserve_flag = 0x4000;

inline micron::atomic_token<addr_t *> __va_base{ nullptr };      // PROT_NONE base, or nullptr if not yet reserved
inline micron::atomic_token<u64> __va_offset{ 0 };               // bump cursor in bytes
inline micron::atomic_flag __va_init_lock{};                     // one-shot init guard

// released va granule free list
// WARNING: without reclamation the monotonic bump cursor (__va_offset) leaks a granule run on every sheet
// release; under sustained sheet churn the 256 GiB reservation is exhausted and __va_carve falls
// back to unregistered system memory, which a cross-thread free then misroutes
struct __va_free_run {
  u64 off;           // byte offset from __va_base
  u32 granules;      // run length in 2 MiB granules
};

constexpr static const usize __va_free_cap = __va_reservation_size >> __sheet_align_log2;
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

  u64 reuse_off = ~0ull;
  {
    micron::free_guard<> guard{ &__va_free_lock };
    for ( usize i = __va_free_count; i-- > 0; ) {
      if ( __va_free_runs[i].granules == want ) {
        reuse_off = __va_free_runs[i].off;
        __va_free_runs[i] = __va_free_runs[--__va_free_count];      // swap-remove
        break;
      }
    }
  }
  if ( reuse_off != ~0ull ) {
    addr_t *slot = reinterpret_cast<addr_t *>(reinterpret_cast<uintptr_t>(base) + reuse_off);
    if ( addr_t *got = __va_commit(slot, rounded); got ) [[likely]]
      return got;
    // remap failed: the run is now dropped from the list (effectively leaked); fall through to a fresh carve
  }

  const u64 off = __va_offset.fetch_add(rounded, micron::memory_order_acq_rel);
  if ( off + rounded > __va_reservation_size ) [[unlikely]]
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

  u64 reuse_off = ~0ull;
  {
    micron::free_guard<> guard{ &__va_free_lock };
    for ( usize i = __va_free_count; i-- > 0; ) {
      if ( __va_free_runs[i].granules == want ) {
        reuse_off = __va_free_runs[i].off;
        __va_free_runs[i] = __va_free_runs[--__va_free_count];      // swap-remove
        break;
      }
    }
  }
  if ( reuse_off != ~0ull ) return reinterpret_cast<addr_t *>(reinterpret_cast<uintptr_t>(base) + reuse_off);

  const u64 off = __va_offset.fetch_add(rounded, micron::memory_order_acq_rel);
  if ( off + rounded > __va_reservation_size ) [[unlikely]]
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
