//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../../src/slice.hpp"

#include "../snowball/snowball.hpp"

// ============================================================
//  Helper utilities
// ============================================================

static constexpr byte CANARY = 0xC3;
static constexpr byte SENTINEL = 0xDE;
static constexpr u64 GUARD_SZ = 16;

// Fill a fixed-size array with a single value
template <typename T, u64 N>
void
fill_pattern(T (&buf)[N], T val)
{
  for ( u64 i = 0; i < N; i++ )
    buf[i] = val;
}

template <typename T>
void
fill_pattern(T *buf, u64 n, T val)
{
  for ( u64 i = 0; i < n; i++ )
    buf[i] = val;
}

// Fill with sequential values 0,1,2,...(mod 256)
template <typename T, u64 N>
void
make_seq(T (&buf)[N])
{
  for ( u64 i = 0; i < N; i++ )
    buf[i] = static_cast<T>(i & 0xFF);
}

template <typename T>
void
make_seq(T *buf, u64 n)
{
  for ( u64 i = 0; i < n; i++ )
    buf[i] = static_cast<T>(i & 0xFF);
}

// Snapshot: copy n elements from src into a plain array for later comparison
template <typename T>
void
snapshot(const T *src, T *snap, u64 n)
{
  for ( u64 i = 0; i < n; i++ )
    snap[i] = src[i];
}

// Verify dest[0..n) matches snap[0..n) exactly
template <typename T>
bool
matches_snapshot(const T *dest, const T *snap, u64 n)
{
  for ( u64 i = 0; i < n; i++ )
    if ( dest[i] != snap[i] )
      return false;
  return true;
}

// Verify every element equals expected
template <typename T, u64 N>
bool
verify_buffer(T (&buf)[N], T expected)
{
  for ( u64 i = 0; i < N; i++ )
    if ( buf[i] != expected )
      return false;
  return true;
}

template <typename T>
bool
verify_buffer(T *buf, u64 n, T expected)
{
  for ( u64 i = 0; i < n; i++ )
    if ( buf[i] != expected )
      return false;
  return true;
}

// Verify all bytes of a typed array are zero
template <typename T, u64 N>
bool
is_zeroed(T (&buf)[N])
{
  const byte *p = reinterpret_cast<const byte *>(buf);
  for ( u64 i = 0; i < N * sizeof(T); i++ )
    if ( p[i] != 0 )
      return false;
  return true;
}

// ============================================================
//  Guard-zone buffer
//
//  Layout: [GUARD_SZ canary][512 bytes payload][GUARD_SZ canary]
//  Detects writes that escape the intended region in either direction.
// ============================================================

struct GuardedBuffer {
  byte data[GUARD_SZ + 512 + GUARD_SZ];

  GuardedBuffer()
  {
    for ( u64 i = 0; i < sizeof(data); i++ )
      data[i] = CANARY;
  }

  byte *
  payload()
  {
    return data + GUARD_SZ;
  }

  const byte *
  payload() const
  {
    return data + GUARD_SZ;
  }

  bool
  guards_intact() const
  {
    for ( u64 i = 0; i < GUARD_SZ; i++ )
      if ( data[i] != CANARY )
        return false;
    for ( u64 i = GUARD_SZ + 512; i < sizeof(data); i++ )
      if ( data[i] != CANARY )
        return false;
    return true;
  }

  // Trailing guard starts immediately after `written_bytes` into the payload
  bool
  no_overrun(u64 written_bytes) const
  {
    const byte *after = payload() + written_bytes;
    for ( u64 i = 0; i < GUARD_SZ; i++ )
      if ( after[i] != CANARY )
        return false;
    for ( u64 i = 0; i < GUARD_SZ; i++ )
      if ( data[i] != CANARY )
        return false;
    return true;
  }
};

// ============================================================
//  Overlap correctness helpers
//
//  For overlapping moves the invariant is:
//    dest[0..n) == original_src[0..n)
//  where original_src is snapshotted BEFORE the move call.
//
//  We also check "dangling bits" — bytes that should NOT have
//  been touched outside the destination window.
// ============================================================

// Build an overlapping scenario inside a single flat buffer.
// Returns pointers src and dest into buf such that:
//   src  starts at buf[src_off]
//   dest starts at buf[dest_off]
//   copy length = n elements
// The caller must ensure buf is large enough.
template <typename T>
void
setup_overlap(T *buf, u64 buf_len, u64 src_off, u64 dest_off, u64 n)
{
  // Fill entire buffer with sequential data so every byte is distinguishable
  make_seq(buf, buf_len);
  (void)n;     // silence unused warning
}

// After memmove(dest, src, n) on an overlapping buffer, verify
// dest[0..n) == snap[0..n) and that bytes outside dest window inside
// the buffer are not silently corrupted unless they were legally in the src window.
template <typename T>
bool
overlap_dest_correct(const T *dest, const T *snap, u64 n)
{
  return matches_snapshot(dest, snap, n);
}

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== MEMMOVE TESTS ===");

  // ============================================================
  //  Section 1: Non-overlapping — both directions
  //  These must behave identically to memcpy.
  // ============================================================

  sb::test_case("memmove non-overlap dest < src: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xAB));
    mc::memmove(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0xAB)));
  }
  sb::end_test_case();

  sb::test_case("memmove non-overlap dest > src: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xCD));
    // dst is higher in address space when allocated separately — already verified by separate arrays
    mc::memmove(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0xCD)));
  }
  sb::end_test_case();

  sb::test_case("memmove non-overlap: sequential pattern fully reproduced");
  {
    byte src[64], dst[64];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xFF));
    mc::memmove(dst, src, 64);
    sb::require(matches_snapshot(dst, src, 64));
  }
  sb::end_test_case();

  sb::test_case("memmove non-overlap: no dangling bits from prior fill");
  {
    // Pre-fill dst with noise. After memmove every byte must match src exactly.
    byte src[32], dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xCC));
    mc::memmove(dst, src, 32);
    sb::require(matches_snapshot(dst, src, 32));
  }
  sb::end_test_case();

  sb::test_case("memmove non-overlap: count = 1");
  {
    byte src[1] = { 0x77 };
    byte dst[1] = { 0x00 };
    mc::memmove(dst, src, 1);
    sb::require(dst[0] == static_cast<byte>(0x77));
  }
  sb::end_test_case();

  sb::test_case("memmove non-overlap: count = 3 (not multiple of 4)");
  {
    byte src[3] = { 0x01, 0x02, 0x03 };
    byte dst[3] = {};
    mc::memmove(dst, src, 3);
    sb::require(dst[0] == 0x01 && dst[1] == 0x02 && dst[2] == 0x03);
  }
  sb::end_test_case();

  sb::test_case("memmove non-overlap: all-zeros source zeroes destination");
  {
    byte src[32] = {};
    byte dst[32];
    fill_pattern(dst, static_cast<byte>(0xFF));
    mc::memmove(dst, src, 32);
    sb::require(is_zeroed(dst));
  }
  sb::end_test_case();

  sb::test_case("memmove non-overlap: all-ones source fills destination");
  {
    byte src[32];
    fill_pattern(src, static_cast<byte>(0xFF));
    byte dst[32] = {};
    mc::memmove(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0xFF)));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 2: dest == src  (no-op path)
  //  The implementation returns early (neither branch fires).
  //  Content must be completely unchanged.
  // ============================================================

  sb::test_case("memmove dest == src: buffer unchanged");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[32];
    snapshot(buf, snap, 32);
    mc::memmove(buf, buf, 32);
    sb::require(matches_snapshot(buf, snap, 32));
  }
  sb::end_test_case();

  sb::test_case("memmove dest == src: u32 buffer unchanged");
  {
    u32 buf[16];
    make_seq(buf);
    u32 snap[16];
    snapshot(buf, snap, 16);
    mc::memmove(buf, buf, 16);
    sb::require(matches_snapshot(buf, snap, 16));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 3: Overlapping — dest < src  (forward copy path)
  //
  //  Scenario:  buf = [0 1 2 3 4 5 6 7 8 9 ...]
  //             src  = buf + offset
  //             dest = buf           (dest < src)
  //             copy n elements
  //  Expected:  dest[0..n) == original buf[offset..offset+n)
  //             Bytes before dest are untouched.
  //             Bytes after dest+n that were NOT in the src window are untouched.
  // ============================================================

  sb::test_case("memmove overlap dest < src: overlap by 1 element (byte)");
  {
    // buf: [0,1,2,3,4,5,6,7]
    // src = buf+1, dest = buf, n = 7   → forward copy
    // expected result: buf = [1,2,3,4,5,6,7,7]  (last byte untouched)
    byte buf[8];
    make_seq(buf);
    byte snap_src[7];
    snapshot(buf + 1, snap_src, 7);     // save what src looks like before move
    mc::memmove(buf, buf + 1, 7);
    sb::require(matches_snapshot(buf, snap_src, 7));
    sb::require(buf[7] == static_cast<byte>(7));     // byte past window untouched
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest < src: overlap by half (byte)");
  {
    // buf: [0..15], src=buf+8, dest=buf, n=8
    // expected: buf[0..8) == original buf[8..16)
    byte buf[16];
    make_seq(buf);
    byte snap_src[8];
    snapshot(buf + 8, snap_src, 8);
    mc::memmove(buf, buf + 8, 8);
    sb::require(matches_snapshot(buf, snap_src, 8));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest < src: overlap by 2 elements (byte)");
  {
    byte buf[16];
    make_seq(buf);
    byte snap_src[14];
    snapshot(buf + 2, snap_src, 14);
    mc::memmove(buf, buf + 2, 14);
    sb::require(matches_snapshot(buf, snap_src, 14));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest < src: no dangling bits past destination window");
  {
    // Bytes beyond dest+n must keep their pre-move values.
    byte buf[32];
    make_seq(buf);
    // Sentinel: we will only move 16 bytes, starting from offset 8.
    // buf[24..32) should remain as make_seq produced them.
    byte tail_snap[8];
    snapshot(buf + 24, tail_snap, 8);
    mc::memmove(buf, buf + 8, 16);
    sb::require(matches_snapshot(buf + 24, tail_snap, 8));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest < src: u32 elements");
  {
    u32 buf[16];
    make_seq(buf);
    u32 snap_src[12];
    snapshot(buf + 4, snap_src, 12);
    mc::memmove(buf, buf + 4, 12);
    sb::require(matches_snapshot(buf, snap_src, 12));
    // Tail beyond window: buf[12..16) unchanged
    u32 tail_snap[4];
    // We can only snapshot before we test; recalculate expected:
    // After make_seq, buf[12]=12, buf[13]=13, buf[14]=14, buf[15]=15 — but forward
    // copy with overlap by 4 means those positions are overwritten by src values.
    // Only buf[12..16) which was NOT part of the 12-element destination window matters.
    // dest window is buf[0..12), src window was buf[4..16) — buf[12..16) is past
    // dest window and was part of src but not destination; verify untouched.
    sb::require(buf[12] == static_cast<u32>(12));
    sb::require(buf[13] == static_cast<u32>(13));
    sb::require(buf[14] == static_cast<u32>(14));
    sb::require(buf[15] == static_cast<u32>(15));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 4: Overlapping — dest > src  (backward copy path)
  //
  //  Scenario:  buf = [0 1 2 3 4 5 6 7 8 9 ...]
  //             src  = buf
  //             dest = buf + offset  (dest > src)
  //             copy n elements
  //  Expected:  dest[0..n) == original buf[0..n)
  //             Bytes before src that are outside the dest window are untouched.
  // ============================================================

  sb::test_case("memmove overlap dest > src: overlap by 1 element (byte)");
  {
    // buf: [0,1,2,3,4,5,6,7]
    // src = buf, dest = buf+1, n = 7   → backward copy
    // expected: buf[1..8) == original buf[0..7) = [0,1,2,3,4,5,6]
    byte buf[8];
    make_seq(buf);
    byte snap_src[7];
    snapshot(buf, snap_src, 7);
    mc::memmove(buf + 1, buf, 7);
    sb::require(matches_snapshot(buf + 1, snap_src, 7));
    sb::require(buf[0] == static_cast<byte>(0));     // byte before dest untouched
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: overlap by half (byte)");
  {
    byte buf[16];
    make_seq(buf);
    byte snap_src[8];
    snapshot(buf, snap_src, 8);
    mc::memmove(buf + 8, buf, 8);
    sb::require(matches_snapshot(buf + 8, snap_src, 8));
    // buf[0..8) was src — backward copy preserves it because we copy right-to-left
    sb::require(matches_snapshot(buf, snap_src, 8));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: backward copy preserves src prefix");
  {
    // buf: [0..31], src=buf, dest=buf+4, n=28
    // src prefix buf[0..4) is NOT in the destination window and must survive intact.
    byte buf[32];
    make_seq(buf);
    byte prefix_snap[4];
    snapshot(buf, prefix_snap, 4);
    byte src_snap[28];
    snapshot(buf, src_snap, 28);
    mc::memmove(buf + 4, buf, 28);
    sb::require(matches_snapshot(buf + 4, src_snap, 28));
    // Backward copy means buf[0..4) may be partially overwritten only if
    // dest window includes it — dest window is buf[4..32), so buf[0..4) is safe.
    sb::require(matches_snapshot(buf, prefix_snap, 4));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: no dangling bits before src window");
  {
    byte buf[32];
    make_seq(buf);
    byte before_snap[4];
    snapshot(buf, before_snap, 4);     // bytes that should be left alone
    byte src_snap[24];
    snapshot(buf + 4, src_snap, 24);
    mc::memmove(buf + 8, buf + 4, 24);
    sb::require(matches_snapshot(buf + 8, src_snap, 24));
    // buf[0..4): well before src window, absolutely must be untouched
    sb::require(matches_snapshot(buf, before_snap, 4));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: u32 elements");
  {
    u32 buf[16];
    make_seq(buf);
    u32 snap_src[12];
    snapshot(buf, snap_src, 12);
    mc::memmove(buf + 4, buf, 12);
    sb::require(matches_snapshot(buf + 4, snap_src, 12));
    // buf[0..4): prefix before dest window — backward copy must preserve these
    sb::require(buf[0] == static_cast<u32>(0));
    sb::require(buf[1] == static_cast<u32>(1));
    sb::require(buf[2] == static_cast<u32>(2));
    sb::require(buf[3] == static_cast<u32>(3));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 5: Off-by-one — boundary precision
  // ============================================================

  sb::test_case("memmove off-by-one: sentinel byte immediately after dest window");
  {
    byte buf[32];
    make_seq(buf);
    byte snap_src[16];
    snapshot(buf + 8, snap_src, 16);
    // Place a sentinel at buf[24] (one past the 16-byte dest window starting at buf+8)
    // After move, buf[24] must still equal its original make_seq value (24).
    mc::memmove(buf + 8, buf + 8 + (buf + 8 < buf + 8 ? 1 : 0), 16);
    // same-pointer case — verify no-op
    byte nowrite_snap[16];
    snapshot(buf + 8, nowrite_snap, 16);
    mc::memmove(buf + 8, buf + 8, 16);
    sb::require(matches_snapshot(buf + 8, nowrite_snap, 16));
    sb::require(buf[24] == static_cast<byte>(24));
  }
  sb::end_test_case();

  sb::test_case("memmove off-by-one: byte before dest window untouched (dest < src)");
  {
    byte storage[34];
    make_seq(storage);
    // sentinel at storage[0], dest=storage+1, src=storage+2, n=16
    byte sentinel_before = storage[0];
    byte snap_src[16];
    snapshot(storage + 2, snap_src, 16);
    mc::memmove(storage + 1, storage + 2, 16);
    sb::require(storage[0] == sentinel_before);
    sb::require(matches_snapshot(storage + 1, snap_src, 16));
    // storage[17] (one past dest+n=storage[17]) must still hold original make_seq value
    sb::require(storage[17] == static_cast<byte>(17));
  }
  sb::end_test_case();

  sb::test_case("memmove off-by-one: byte after dest window untouched (dest > src)");
  {
    byte storage[34];
    make_seq(storage);
    // src=storage, dest=storage+1, n=16, sentinel=storage[17]
    byte sentinel_after = storage[17];
    byte snap_src[16];
    snapshot(storage, snap_src, 16);
    mc::memmove(storage + 1, storage, 16);
    sb::require(storage[17] == sentinel_after);
    sb::require(matches_snapshot(storage + 1, snap_src, 16));
  }
  sb::end_test_case();

  sb::test_case("memmove off-by-one: guard zone intact (non-overlap)");
  {
    GuardedBuffer gb_src, gb_dst;
    make_seq(gb_src.payload(), static_cast<u64>(64));
    mc::memmove(gb_dst.payload(), gb_src.payload(), 64);
    sb::require(gb_dst.guards_intact());
    sb::require(gb_dst.no_overrun(64));
    sb::require(matches_snapshot(gb_dst.payload(), gb_src.payload(), 64));
  }
  sb::end_test_case();

  sb::test_case("memmove off-by-one: guard zone intact (overlap dest > src)");
  {
    GuardedBuffer gb;
    make_seq(gb.payload(), static_cast<u64>(128));
    byte snap_src[64];
    snapshot(gb.payload(), snap_src, 64);
    mc::memmove(gb.payload() + 32, gb.payload(), 64);
    sb::require(gb.guards_intact());
    sb::require(matches_snapshot(gb.payload() + 32, snap_src, 64));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 6: Dangling bits / artifacting
  //  Verify no stale values from a prior content remain after move.
  // ============================================================

  sb::test_case("memmove artifact: dest pre-filled with noise, non-overlap");
  {
    byte src[32], dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xEE));
    mc::memmove(dst, src, 32);
    sb::require(matches_snapshot(dst, src, 32));
  }
  sb::end_test_case();

  sb::test_case("memmove artifact: two sequential moves, no residue from first");
  {
    byte src[32], dst[32];
    // Move 1
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xCC));
    mc::memmove(dst, src, 32);
    sb::require(matches_snapshot(dst, src, 32));
    // Move 2: completely different values
    fill_pattern(src, static_cast<byte>(0x00));
    mc::memmove(dst, src, 32);
    sb::require(is_zeroed(dst));     // no residue from move 1
  }
  sb::end_test_case();

  sb::test_case("memmove artifact: partial move leaves tail with original noise");
  {
    byte src[32], dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xBB));
    mc::memmove(dst, src, 16);     // copy only first half
    sb::require(matches_snapshot(dst, src, 16));
    // Second half must still be noise — not zeroed, not src[16..32)
    sb::require(verify_buffer(dst + 16, static_cast<u64>(16), static_cast<byte>(0xBB)));
  }
  sb::end_test_case();

  sb::test_case("memmove artifact: overlap dest > src, forward region not corrupted");
  {
    // Classic memmove corruption test: if a naive forward copy were used for dest>src
    // the data would be corrupted. Verify backward copy gives correct result.
    byte buf[8];
    // Pattern: [10, 20, 30, 40, 50, 60, 70, 80]
    buf[0] = 10;
    buf[1] = 20;
    buf[2] = 30;
    buf[3] = 40;
    buf[4] = 50;
    buf[5] = 60;
    buf[6] = 70;
    buf[7] = 80;
    // Move src=buf[0..4), dest=buf[2..6), n=4
    // Expected dest: [10,20,30,40]  i.e. buf[2..6) = {10,20,30,40}
    mc::memmove(buf + 2, buf, 4);
    sb::require(buf[2] == static_cast<byte>(10));
    sb::require(buf[3] == static_cast<byte>(20));
    sb::require(buf[4] == static_cast<byte>(30));
    sb::require(buf[5] == static_cast<byte>(40));
    // buf[0..2) must be untouched (original values 10,20)
    sb::require(buf[0] == static_cast<byte>(10));
    sb::require(buf[1] == static_cast<byte>(20));
    // buf[6..8) must be untouched (original values 70,80)
    sb::require(buf[6] == static_cast<byte>(70));
    sb::require(buf[7] == static_cast<byte>(80));
  }
  sb::end_test_case();

  sb::test_case("memmove artifact: overlap dest < src, tail region not corrupted");
  {
    // Pattern: [10, 20, 30, 40, 50, 60, 70, 80]
    byte buf[8];
    buf[0] = 10;
    buf[1] = 20;
    buf[2] = 30;
    buf[3] = 40;
    buf[4] = 50;
    buf[5] = 60;
    buf[6] = 70;
    buf[7] = 80;
    // src=buf[2..6), dest=buf[0..4), n=4
    // Expected: buf[0..4) = {30,40,50,60}
    mc::memmove(buf, buf + 2, 4);
    sb::require(buf[0] == static_cast<byte>(30));
    sb::require(buf[1] == static_cast<byte>(40));
    sb::require(buf[2] == static_cast<byte>(50));
    sb::require(buf[3] == static_cast<byte>(60));
    // buf[4..8): must be untouched
    sb::require(buf[4] == static_cast<byte>(50));
    sb::require(buf[5] == static_cast<byte>(60));
    sb::require(buf[6] == static_cast<byte>(70));
    sb::require(buf[7] == static_cast<byte>(80));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 7: rmemmove
  // ============================================================

  sb::test_case("rmemmove non-overlap: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x5C));
    mc::rmemmove(*dst, *src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0x5C)));
  }
  sb::end_test_case();

  sb::test_case("rmemmove non-overlap: sequential no dangling bits");
  {
    byte src[32], dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xAA));
    mc::rmemmove(*dst, *src, 32);
    sb::require(matches_snapshot(dst, src, 32));
  }
  sb::end_test_case();

  sb::test_case("rmemmove overlap dest > src: backward copy correctness");
  {
    byte buf[16];
    make_seq(buf);
    byte snap[8];
    snapshot(buf, snap, 8);
    mc::rmemmove(*(buf + 4), *buf, 8);
    sb::require(matches_snapshot(buf + 4, snap, 8));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 8: constexpr_memmove / constexpr_bytemove
  // ============================================================

  sb::test_case("constexpr_memmove non-overlap: basic correctness");
  {
    byte src[16], dst[16] = {};
    fill_pattern(src, static_cast<byte>(0x3F));
    mc::constexpr_memmove(dst, src, 16);
    sb::require(verify_buffer(dst, static_cast<byte>(0x3F)));
  }
  sb::end_test_case();

  sb::test_case("constexpr_memmove overlap dest > src: backward correctness");
  {
    byte buf[16];
    make_seq(buf);
    byte snap[12];
    snapshot(buf, snap, 12);
    mc::constexpr_memmove(buf + 4, buf, 12);
    sb::require(matches_snapshot(buf + 4, snap, 12));
  }
  sb::end_test_case();

  sb::test_case("constexpr_bytemove non-overlap: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x71));
    mc::constexpr_bytemove(dst, src, 32);
    sb::require(verify_buffer(dst, static_cast<byte>(0x71)));
  }
  sb::end_test_case();

  sb::test_case("constexpr_bytemove overlap dest < src: forward correctness");
  {
    byte buf[16];
    make_seq(buf);
    byte snap[14];
    snapshot(buf + 2, snap, 14);
    mc::constexpr_bytemove(buf, buf + 2, 14);
    sb::require(matches_snapshot(buf, snap, 14));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 9: cmemmove / crmemmove — compile-time count
  // ============================================================

  sb::test_case("cmemmove<32> non-overlap: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x88));
    mc::cmemmove<32>(dst, src);
    sb::require(verify_buffer(dst, static_cast<byte>(0x88)));
  }
  sb::end_test_case();

  sb::test_case("cmemmove<16> overlap dest > src: backward correctness");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[16];
    snapshot(buf, snap, 16);
    mc::cmemmove<16>(buf + 8, buf);
    sb::require(matches_snapshot(buf + 8, snap, 16));
  }
  sb::end_test_case();

  sb::test_case("cmemmove<7> non-multiple-of-4: correctness");
  {
    u16 src[7], dst[7] = {};
    fill_pattern(src, static_cast<u16>(0xBEEF));
    mc::cmemmove<7>(dst, src);
    sb::require(verify_buffer(dst, static_cast<u16>(0xBEEF)));
  }
  sb::end_test_case();

  sb::test_case("cmemmove<1> single element");
  {
    u32 src[1] = { 0xFEEDFACEu };
    u32 dst[1] = { 0 };
    mc::cmemmove<1>(dst, src);
    sb::require(dst[0] == 0xFEEDFACEu);
  }
  sb::end_test_case();

  sb::test_case("crmemmove<16> overlap dest < src: forward correctness");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[16];
    snapshot(buf + 8, snap, 16);
    mc::crmemmove<16>(*buf, *(buf + 8));
    sb::require(matches_snapshot(buf, snap, 16));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 10: smemmove — safe (nullptr / alignment) variant
  // ============================================================

  sb::test_case("smemmove valid pointers: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xA7));
    byte *result = mc::smemmove(dst, src, 32);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0xA7)));
  }
  sb::end_test_case();

  sb::test_case("smemmove nullptr dest: returns nullptr, no crash");
  {
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x11));
    byte *result = mc::smemmove<byte, byte>(nullptr, src, 16);
    sb::require(result == nullptr);
    sb::require(verify_buffer(src, static_cast<byte>(0x11)));     // src untouched
  }
  sb::end_test_case();

  sb::test_case("smemmove nullptr src: returns nullptr, dest untouched");
  {
    byte dst[16];
    fill_pattern(dst, static_cast<byte>(0x22));
    byte *result = mc::smemmove<byte, byte>(dst, static_cast<byte *>(nullptr), 16);
    sb::require(result == nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0x22)));
  }
  sb::end_test_case();

  sb::test_case("smemmove returns dest pointer on success");
  {
    byte src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    byte dst[8] = {};
    byte *result = mc::smemmove(dst, src, 8);
    sb::require(result == dst);
  }
  sb::end_test_case();

  sb::test_case("smemmove misaligned dest: returns nullptr");
  {
    alignas(16) byte storage[32] = {};
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x44));
    byte *misaligned = storage + 1;
    byte *result = mc::smemmove<byte, byte, 16>(misaligned, src, 15);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("smemmove overlap dest > src: backward correctness");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[24];
    snapshot(buf, snap, 24);
    byte *result = mc::smemmove(buf + 8, buf, 24);
    sb::require(result != nullptr);
    sb::require(matches_snapshot(buf + 8, snap, 24));
  }
  sb::end_test_case();

  sb::test_case("smemmove guard zone intact");
  {
    GuardedBuffer gb_src, gb_dst;
    make_seq(gb_src.payload(), static_cast<u64>(64));
    mc::smemmove(gb_dst.payload(), gb_src.payload(), 64);
    sb::require(gb_dst.guards_intact());
    sb::require(gb_dst.no_overrun(64));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 11: rsmemmove
  // ============================================================

  sb::test_case("rsmemmove valid: basic correctness");
  {
    byte src[16], dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xD9));
    bool ok = mc::rsmemmove(*dst, *src, 16);
    sb::require(ok == true);
    sb::require(verify_buffer(dst, static_cast<byte>(0xD9)));
  }
  sb::end_test_case();

  sb::test_case("rsmemmove overlap dest > src: correctness");
  {
    byte buf[24];
    make_seq(buf);
    byte snap[16];
    snapshot(buf, snap, 16);
    bool ok = mc::rsmemmove(*(buf + 8), *buf, 16);
    sb::require(ok == true);
    sb::require(matches_snapshot(buf + 8, snap, 16));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 12: scmemmove — safe compile-time variant
  // ============================================================

  sb::test_case("scmemmove<32> valid pointers: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xE4));
    byte *result = mc::scmemmove<32>(dst, src);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0xE4)));
  }
  sb::end_test_case();

  sb::test_case("scmemmove<16> nullptr: returns nullptr");
  {
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x33));
    byte *result = mc::scmemmove<16>(static_cast<byte *>(nullptr), src);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("scmemmove<16> overlap dest > src: backward correctness");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[16];
    snapshot(buf, snap, 16);
    byte *result = mc::scmemmove<16>(buf + 8, buf);
    sb::require(result != nullptr);
    sb::require(matches_snapshot(buf + 8, snap, 16));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 13: rscmemmove
  // ============================================================

  sb::test_case("rscmemmove<16> valid: basic correctness");
  {
    byte src[16], dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xF1));
    bool ok = mc::rscmemmove<16>(*dst, *src);
    sb::require(ok == true);
    sb::require(verify_buffer(dst, static_cast<byte>(0xF1)));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 14: bytemove / rbytemove / cbytemove — byte-reinterpret variants
  // ============================================================

  sb::test_case("bytemove non-overlap: sequential, no dangling bits");
  {
    byte src[32], dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xCC));
    mc::bytemove(dst, src, 32);
    sb::require(matches_snapshot(dst, src, 32));
  }
  sb::end_test_case();

  sb::test_case("bytemove overlap dest > src: backward copy correctness");
  {
    byte buf[16];
    make_seq(buf);
    byte snap[12];
    snapshot(buf, snap, 12);
    mc::bytemove(buf + 4, buf, 12);
    sb::require(matches_snapshot(buf + 4, snap, 12));
  }
  sb::end_test_case();

  sb::test_case("bytemove overlap dest < src: forward copy correctness");
  {
    byte buf[16];
    make_seq(buf);
    byte snap[12];
    snapshot(buf + 4, snap, 12);
    mc::bytemove(buf, buf + 4, 12);
    sb::require(matches_snapshot(buf, snap, 12));
  }
  sb::end_test_case();

  sb::test_case("bytemove dest == src: no-op, buffer unchanged");
  {
    byte buf[16];
    make_seq(buf);
    byte snap[16];
    snapshot(buf, snap, 16);
    mc::bytemove(buf, buf, 16);
    sb::require(matches_snapshot(buf, snap, 16));
  }
  sb::end_test_case();

  sb::test_case("bytemove guard zone intact");
  {
    GuardedBuffer gb_src, gb_dst;
    make_seq(gb_src.payload(), static_cast<u64>(64));
    mc::bytemove(gb_dst.payload(), gb_src.payload(), 64);
    sb::require(gb_dst.guards_intact());
    sb::require(gb_dst.no_overrun(64));
    sb::require(matches_snapshot(gb_dst.payload(), gb_src.payload(), 64));
  }
  sb::end_test_case();

  sb::test_case("rbytemove non-overlap: correctness");
  {
    byte src[16], dst[16] = {};
    fill_pattern(src, static_cast<byte>(0x4F));
    mc::rbytemove(*dst, *src, 16);
    sb::require(verify_buffer(dst, static_cast<byte>(0x4F)));
  }
  sb::end_test_case();

  sb::test_case("rbytemove overlap dest > src: backward correctness");
  {
    byte buf[16];
    make_seq(buf);
    byte snap[12];
    snapshot(buf, snap, 12);
    mc::rbytemove(*(buf + 4), *buf, 12);
    sb::require(matches_snapshot(buf + 4, snap, 12));
  }
  sb::end_test_case();

  sb::test_case("cbytemove<32> non-overlap: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x6B));
    mc::cbytemove<32>(dst, src);
    sb::require(verify_buffer(dst, static_cast<byte>(0x6B)));
  }
  sb::end_test_case();

  sb::test_case("cbytemove<16> overlap dest > src: backward correctness");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[16];
    snapshot(buf, snap, 16);
    mc::cbytemove<16>(buf + 8, buf);
    sb::require(matches_snapshot(buf + 8, snap, 16));
  }
  sb::end_test_case();

  sb::test_case("cbytemove<13> non-divisible count: no dangling bits");
  {
    byte src[13], dst[13] = {};
    make_seq(src);
    mc::cbytemove<13>(dst, src);
    sb::require(matches_snapshot(dst, src, 13));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 15: sbytemove — safe bytemove
  // ============================================================

  sb::test_case("sbytemove valid: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0xB3));
    byte *result = mc::sbytemove(dst, src, 32);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0xB3)));
  }
  sb::end_test_case();

  sb::test_case("sbytemove nullptr dest: returns nullptr");
  {
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x55));
    byte *result = mc::sbytemove<byte, byte>(static_cast<byte *>(nullptr), src, 16);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("sbytemove nullptr src: returns nullptr, dest untouched");
  {
    byte dst[16];
    fill_pattern(dst, static_cast<byte>(0x66));
    byte *result = mc::sbytemove<byte, byte>(dst, static_cast<byte *>(nullptr), 16);
    sb::require(result == nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0x66)));
  }
  sb::end_test_case();

  sb::test_case("sbytemove overlap dest > src: backward correctness");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[24];
    snapshot(buf, snap, 24);
    byte *result = mc::sbytemove(buf + 8, buf, 24);
    sb::require(result != nullptr);
    sb::require(matches_snapshot(buf + 8, snap, 24));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 16: rsbytemove
  // ============================================================

  sb::test_case("rsbytemove valid: basic correctness");
  {
    byte src[16], dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xC1));
    bool ok = mc::rsbytemove(*dst, *src, 16);
    sb::require(ok == true);
    sb::require(verify_buffer(dst, static_cast<byte>(0xC1)));
  }
  sb::end_test_case();

  sb::test_case("rsbytemove overlap dest > src: correctness");
  {
    byte buf[24];
    make_seq(buf);
    byte snap[16];
    snapshot(buf, snap, 16);
    bool ok = mc::rsbytemove(*(buf + 8), *buf, 16);
    sb::require(ok == true);
    sb::require(matches_snapshot(buf + 8, snap, 16));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 17: scbytemove / rscrbytemove
  // ============================================================

  sb::test_case("scbytemove<32> valid: basic correctness");
  {
    byte src[32], dst[32] = {};
    fill_pattern(src, static_cast<byte>(0x9D));
    byte *result = mc::scbytemove<32>(dst, src);
    sb::require(result != nullptr);
    sb::require(verify_buffer(dst, static_cast<byte>(0x9D)));
  }
  sb::end_test_case();

  sb::test_case("scbytemove<16> nullptr: returns nullptr");
  {
    byte src[16];
    fill_pattern(src, static_cast<byte>(0x44));
    byte *result = mc::scbytemove<16>(static_cast<byte *>(nullptr), src);
    sb::require(result == nullptr);
  }
  sb::end_test_case();

  sb::test_case("scbytemove<16> overlap dest > src: backward correctness");
  {
    byte buf[32];
    make_seq(buf);
    byte snap[16];
    snapshot(buf, snap, 16);
    byte *result = mc::scbytemove<16>(buf + 8, buf);
    sb::require(result != nullptr);
    sb::require(matches_snapshot(buf + 8, snap, 16));
  }
  sb::end_test_case();

  sb::test_case("rscrbytemove<16> valid: basic correctness");
  {
    byte src[16], dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xF8));
    bool ok = mc::rscrbytemove<16>(*dst, *src);
    sb::require(ok == true);
    sb::require(verify_buffer(dst, static_cast<byte>(0xF8)));
  }
  sb::end_test_case();

  // ============================================================
  //  Section 18: Stress / repeated operations
  // ============================================================

  sb::test_case("Stress: 100 non-overlap memmoves with varying values");
  {
    byte src[64], dst[64];
    for ( int i = 0; i < 100; i++ ) {
      byte val = static_cast<byte>(i);
      fill_pattern(src, val);
      fill_pattern(dst, static_cast<byte>(~i));
      mc::memmove(dst, src, 64);
      sb::require(verify_buffer(dst, val));
    }
  }
  sb::end_test_case();

  sb::test_case("Stress: 50 overlapping memmoves (dest > src) with sequential data");
  {
    byte buf[128];
    for ( int i = 0; i < 50; i++ ) {
      make_seq(buf);
      byte snap[64];
      snapshot(buf, snap, 64);
      mc::memmove(buf + 32, buf, 64);
      sb::require(matches_snapshot(buf + 32, snap, 64));
    }
  }
  sb::end_test_case();

  sb::test_case("Stress: large buffer voidcpy equivalent via memmove");
  {
    constexpr u64 SIZE = 512;
    byte src[SIZE], dst[SIZE];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0x00));
    mc::memmove(dst, src, SIZE);
    sb::require(matches_snapshot(dst, src, SIZE));
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
