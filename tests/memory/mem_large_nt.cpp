//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Large-size memcpy/memmove/memset validation across every dispatch-tier
// boundary (scalar/SIMD/ERMS/non-temporal), with PROT_NONE guard pages
// adjacent to the windows so any out-of-bounds access faults immediately.
// Windows are placed both head-adjacent and tail-adjacent to the guards.
//
// NOTE: qemu-user executes movnt*/dc zva functionally (correctness yes,
// perf no); missing-sfence ordering bugs will not reproduce under TCG.

#include "../../src/memory/memory.hpp"
#include "../../src/memory/mman.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

namespace
{

constexpr u64 PAGE = 4096;
constexpr u64 DATA = 32ull << 20;

static u8 *dst_data = nullptr;
static u8 *src_data = nullptr;

[[gnu::always_inline]] inline byte
src_pat(u64 i) noexcept
{
  return static_cast<byte>(i * 131 + 5);
}

static u8 *
map_guarded()
{
  u8 *base = micron::bytemap(PAGE + DATA + PAGE);
  if ( micron::mmap_failed(base) ) return nullptr;
  micron::mprotect(reinterpret_cast<addr_t *>(base), PAGE, micron::prot_none);
  micron::mprotect(reinterpret_cast<addr_t *>(base + PAGE + DATA), PAGE, micron::prot_none);
  return base + PAGE;
}

bool
check_one(u64 n, u8 *d, const u8 *s, u32 which) noexcept
{
  const byte fill = static_cast<byte>(which == 3 ? 0x00 : 0x5A);
  if ( which == 0 )
    mc::memcpy(d, s, n);
  else if ( which == 1 )
    mc::memmove(d, const_cast<u8 *>(s), n);
  else
    mc::memset(d, fill, n);
  for ( u64 i = 0; i < n; ++i ) {
    if ( which <= 1 ) {
      if ( d[i] != s[i] ) return false;
    } else if ( d[i] != fill )
      return false;
  }
  return true;
}

bool
run_case(u64 n, u64 off, u32 which) noexcept
{
  // head-adjacent: window starts at the leading guard edge (+off)
  u8 *dh = dst_data + off;
  const u8 *sh = src_data + off;
  for ( u64 i = 0; i < n; ++i ) const_cast<u8 *>(sh)[i] = src_pat(i + off);
  if ( !check_one(n, dh, sh, which) ) return false;
  // tail-adjacent: window ends exactly at the trailing guard page
  u8 *dt = dst_data + DATA - n;
  const u8 *st = src_data + DATA - n;
  for ( u64 i = 0; i < n; ++i ) const_cast<u8 *>(st)[i] = src_pat(i * 3 + 1);
  if ( !check_one(n, dt, st, which) ) return false;
  return true;
}

}      // namespace

int
main()
{
  dst_data = map_guarded();
  src_data = map_guarded();
  if ( !dst_data || !src_data ) {
    sb::print("mmap failed, cannot run");
    sb::require(false);
  }

  // every dispatch boundary the rewritten leaves will have, +/-1 and +/-63:
  // scalar/simd (32, 64, 128), rep windows (2048, 2112, 8192), page sizes,
  // and beyond-L3 non-temporal territory (16 MiB, 24 MiB)
  const u64 sizes[] = { 2047,
                        2048,
                        2049,
                        2111,
                        2112,
                        2113,
                        8191,
                        8192,
                        8193,
                        65535,
                        65536,
                        65537,
                        262144,
                        1ull << 20,
                        (1ull << 20) + 1,
                        (16ull << 20) - 1,
                        16ull << 20,
                        (16ull << 20) + 1,
                        24ull << 20 };
  const u64 offs[] = { 0, 1, 63 };
  const char *names[] = { "memcpy", "memmove", "memset5A", "memset0" };

  for ( u32 which = 0; which < 4; ++which ) {
    sb::test_case(names[which]);
    for ( u64 n : sizes )
      for ( u64 off : offs )
        if ( !run_case(n, off, which) ) {
          sb::print(names[which], " failed: n=", n, " off=", off);
          sb::require(false);
        }
    sb::end_test_case();
  }

#if defined(__micron_arch_x86_any)
  // force-tier: shrink the thresholds so the rep and NT tiers run
  // deterministically at small sizes (works under qemu too), then restore
  sb::test_case("forced rep/NT tiers");
  const micron::__mem_tunables saved = micron::__mem_tun;
  // rep window [2048, 16384), NT >= 16384; sizes below __mem_rep_min (2048)
  // short-circuit to the bulk loop
  micron::__mem_tun = { 16384, 16384, 2048, 2048, true };
  const u64 tier_sizes[] = { 255, 1024, 2047, 2048, 2049, 4096, 8192, 16383, 16384, 16385, 65536 };
  for ( u32 which = 0; which < 4; ++which )
    for ( u64 n : tier_sizes )
      for ( u64 off : offs )
        if ( !run_case(n, off, which) ) {
          micron::__mem_tun = saved;
          sb::print("forced-tier ", names[which], " failed: n=", n, " off=", off);
          sb::require(false);
        }
  // NT disabled entirely (tier boundary edge)
  micron::__mem_tun = { micron::__mem_tier_disabled, micron::__mem_tier_disabled, 2048, 2048, true };
  for ( u32 which = 0; which < 4; ++which )
    for ( u64 n : tier_sizes )
      if ( !run_case(n, 1, which) ) {
        micron::__mem_tun = saved;
        sb::print("rep-only ", names[which], " failed: n=", n);
        sb::require(false);
      }
  micron::__mem_tun = saved;
  sb::end_test_case();
#endif

  sb::print("mem_large_nt: all cases passed");
  return 1;
}
