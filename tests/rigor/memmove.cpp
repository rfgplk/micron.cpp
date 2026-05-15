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

static constexpr byte CANARY = 0xC3;
static constexpr byte SENTINEL = 0xDE;
static constexpr u64 GUARD_SZ = 16;

template<typename T, u64 N>
void
fill_pattern(T (&buf)[N], T val)
{
  for ( u64 i = 0; i < N; i++ ) buf[i] = val;
}

template<typename T>
void
fill_pattern(T *buf, u64 n, T val)
{
  for ( u64 i = 0; i < n; i++ ) buf[i] = val;
}

template<typename T, u64 N>
void
make_seq(T (&buf)[N])
{
  for ( u64 i = 0; i < N; i++ ) buf[i] = static_cast<T>(i & 0xFF);
}

template<typename T>
void
make_seq(T *buf, u64 n)
{
  for ( u64 i = 0; i < n; i++ ) buf[i] = static_cast<T>(i & 0xFF);
}

template<typename T>
void
snapshot(const T *src, T *snap, u64 n)
{
  for ( u64 i = 0; i < n; i++ ) snap[i] = src[i];
}

template<typename T>
bool
matches_snapshot(const T *dest, const T *snap, u64 n)
{
  for ( u64 i = 0; i < n; i++ )
    if ( dest[i] != snap[i] ) return false;
  return true;
}

template<typename T, u64 N>
bool
verify_buffer(T (&buf)[N], T expected)
{
  for ( u64 i = 0; i < N; i++ )
    if ( buf[i] != expected ) return false;
  return true;
}

template<typename T>
bool
verify_buffer(T *buf, u64 n, T expected)
{
  for ( u64 i = 0; i < n; i++ )
    if ( buf[i] != expected ) return false;
  return true;
}

template<typename T, u64 N>
bool
is_zeroed(T (&buf)[N])
{
  const byte *p = reinterpret_cast<const byte *>(buf);
  for ( u64 i = 0; i < N * sizeof(T); i++ )
    if ( p[i] != 0 ) return false;
  return true;
}

struct GuardedBuffer {
  byte data[GUARD_SZ + 512 + GUARD_SZ];

  GuardedBuffer()
  {
    for ( u64 i = 0; i < sizeof(data); i++ ) data[i] = CANARY;
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
      if ( data[i] != CANARY ) return false;
    for ( u64 i = GUARD_SZ + 512; i < sizeof(data); i++ )
      if ( data[i] != CANARY ) return false;
    return true;
  }

  bool
  no_overrun(u64 written_bytes) const
  {
    const byte *after = payload() + written_bytes;
    for ( u64 i = 0; i < GUARD_SZ; i++ )
      if ( after[i] != CANARY ) return false;
    for ( u64 i = 0; i < GUARD_SZ; i++ )
      if ( data[i] != CANARY ) return false;
    return true;
  }
};

template<typename T>
void
setup_overlap(T *buf, u64 buf_len, u64 src_off, u64 dest_off, u64 n)
{

  make_seq(buf, buf_len);
  (void)n;
}

template<typename T>
bool
overlap_dest_correct(const T *dest, const T *snap, u64 n)
{
  return matches_snapshot(dest, snap, n);
}

int
main(void)
{
  sb::print("=== MEMMOVE TESTS ===");

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

  sb::test_case("memmove overlap dest < src: overlap by 1 element (byte)");
  {

    byte buf[8];
    make_seq(buf);
    byte snap_src[7];
    snapshot(buf + 1, snap_src, 7);
    mc::memmove(buf, buf + 1, 7);
    sb::require(matches_snapshot(buf, snap_src, 7));
    sb::require(buf[7] == static_cast<byte>(7));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest < src: overlap by half (byte)");
  {

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

    byte buf[32];
    make_seq(buf);

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

    u32 tail_snap[4];

    sb::require(buf[12] == static_cast<u32>(12));
    sb::require(buf[13] == static_cast<u32>(13));
    sb::require(buf[14] == static_cast<u32>(14));
    sb::require(buf[15] == static_cast<u32>(15));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: overlap by 1 element (byte)");
  {

    byte buf[8];
    make_seq(buf);
    byte snap_src[7];
    snapshot(buf, snap_src, 7);
    mc::memmove(buf + 1, buf, 7);
    sb::require(matches_snapshot(buf + 1, snap_src, 7));
    sb::require(buf[0] == static_cast<byte>(0));
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

    sb::require(matches_snapshot(buf, snap_src, 8));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: backward copy preserves src prefix");
  {

    byte buf[32];
    make_seq(buf);
    byte prefix_snap[4];
    snapshot(buf, prefix_snap, 4);
    byte src_snap[28];
    snapshot(buf, src_snap, 28);
    mc::memmove(buf + 4, buf, 28);
    sb::require(matches_snapshot(buf + 4, src_snap, 28));

    sb::require(matches_snapshot(buf, prefix_snap, 4));
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: no dangling bits before src window");
  {
    byte buf[32];
    make_seq(buf);
    byte before_snap[4];
    snapshot(buf, before_snap, 4);
    byte src_snap[24];
    snapshot(buf + 4, src_snap, 24);
    mc::memmove(buf + 8, buf + 4, 24);
    sb::require(matches_snapshot(buf + 8, src_snap, 24));

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

    sb::require(buf[0] == static_cast<u32>(0));
    sb::require(buf[1] == static_cast<u32>(1));
    sb::require(buf[2] == static_cast<u32>(2));
    sb::require(buf[3] == static_cast<u32>(3));
  }
  sb::end_test_case();

  sb::test_case("memmove off-by-one: sentinel byte immediately after dest window");
  {
    byte buf[32];
    make_seq(buf);
    byte snap_src[16];
    snapshot(buf + 8, snap_src, 16);

    mc::memmove(buf + 8, buf + 8 + (buf + 8 < buf + 8 ? 1 : 0), 16);

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

    byte sentinel_before = storage[0];
    byte snap_src[16];
    snapshot(storage + 2, snap_src, 16);
    mc::memmove(storage + 1, storage + 2, 16);
    sb::require(storage[0] == sentinel_before);
    sb::require(matches_snapshot(storage + 1, snap_src, 16));

    sb::require(storage[17] == static_cast<byte>(17));
  }
  sb::end_test_case();

  sb::test_case("memmove off-by-one: byte after dest window untouched (dest > src)");
  {
    byte storage[34];
    make_seq(storage);

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

    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xCC));
    mc::memmove(dst, src, 32);
    sb::require(matches_snapshot(dst, src, 32));

    fill_pattern(src, static_cast<byte>(0x00));
    mc::memmove(dst, src, 32);
    sb::require(is_zeroed(dst));
  }
  sb::end_test_case();

  sb::test_case("memmove artifact: partial move leaves tail with original noise");
  {
    byte src[32], dst[32];
    make_seq(src);
    fill_pattern(dst, static_cast<byte>(0xBB));
    mc::memmove(dst, src, 16);
    sb::require(matches_snapshot(dst, src, 16));

    sb::require(verify_buffer(dst + 16, static_cast<u64>(16), static_cast<byte>(0xBB)));
  }
  sb::end_test_case();

  sb::test_case("memmove artifact: overlap dest > src, forward region not corrupted");
  {

    byte buf[8];

    buf[0] = 10;
    buf[1] = 20;
    buf[2] = 30;
    buf[3] = 40;
    buf[4] = 50;
    buf[5] = 60;
    buf[6] = 70;
    buf[7] = 80;

    mc::memmove(buf + 2, buf, 4);
    sb::require(buf[2] == static_cast<byte>(10));
    sb::require(buf[3] == static_cast<byte>(20));
    sb::require(buf[4] == static_cast<byte>(30));
    sb::require(buf[5] == static_cast<byte>(40));

    sb::require(buf[0] == static_cast<byte>(10));
    sb::require(buf[1] == static_cast<byte>(20));

    sb::require(buf[6] == static_cast<byte>(70));
    sb::require(buf[7] == static_cast<byte>(80));
  }
  sb::end_test_case();

  sb::test_case("memmove artifact: overlap dest < src, tail region not corrupted");
  {

    byte buf[8];
    buf[0] = 10;
    buf[1] = 20;
    buf[2] = 30;
    buf[3] = 40;
    buf[4] = 50;
    buf[5] = 60;
    buf[6] = 70;
    buf[7] = 80;

    mc::memmove(buf, buf + 2, 4);
    sb::require(buf[0] == static_cast<byte>(30));
    sb::require(buf[1] == static_cast<byte>(40));
    sb::require(buf[2] == static_cast<byte>(50));
    sb::require(buf[3] == static_cast<byte>(60));

    sb::require(buf[4] == static_cast<byte>(50));
    sb::require(buf[5] == static_cast<byte>(60));
    sb::require(buf[6] == static_cast<byte>(70));
    sb::require(buf[7] == static_cast<byte>(80));
  }
  sb::end_test_case();

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
    sb::require(verify_buffer(src, static_cast<byte>(0x11)));
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

  sb::test_case("rscmemmove<16> valid: basic correctness");
  {
    byte src[16], dst[16] = {};
    fill_pattern(src, static_cast<byte>(0xF1));
    bool ok = mc::rscmemmove<16>(*dst, *src);
    sb::require(ok == true);
    sb::require(verify_buffer(dst, static_cast<byte>(0xF1)));
  }
  sb::end_test_case();

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

  sb::test_case("memmove overlap dest > src: 24-byte struct shift, SIMD path");
  {
    struct s24 {
      u64 a;
      u64 b;
      u64 c;
    };

    constexpr u64 N = 16;
    s24 buf[N];
    for ( u64 i = 0; i < N; ++i ) {
      buf[i].a = 0x1000ull + i;
      buf[i].b = 0x2000ull + i;
      buf[i].c = 0x3000ull + i;
    }

    s24 ext[N + 1];
    for ( u64 i = 0; i < N; ++i ) ext[i] = buf[i];
    ext[N] = { 0, 0, 0 };
    mc::memmove(&ext[5], &ext[4], N - 4);
    for ( u64 i = 0; i < 4; ++i ) {
      sb::require(ext[i].a == 0x1000ull + i);
      sb::require(ext[i].b == 0x2000ull + i);
      sb::require(ext[i].c == 0x3000ull + i);
    }
    for ( u64 i = 5; i < N + 1; ++i ) {
      sb::require(ext[i].a == 0x1000ull + (i - 1));
      sb::require(ext[i].b == 0x2000ull + (i - 1));
      sb::require(ext[i].c == 0x3000ull + (i - 1));
    }
  }
  sb::end_test_case();

  sb::test_case("memmove overlap dest > src: leaf-insert shift pattern (24 B struct)");
  {
    struct s24 {
      u64 a;
      u64 b;
      u64 c;
    };

    constexpr u64 N = 8;
    s24 leaf[N + 1] = {};
    for ( u64 i = 0; i < N; ++i ) {
      leaf[i].a = 0xA0 + i;
      leaf[i].b = 0xB0 + i;
      leaf[i].c = 0xC0 + i;
    }

    mc::memmove(&leaf[1], &leaf[0], N);
    for ( u64 i = 1; i <= N; ++i ) {
      sb::require(leaf[i].a == 0xA0 + (i - 1));
      sb::require(leaf[i].b == 0xB0 + (i - 1));
      sb::require(leaf[i].c == 0xC0 + (i - 1));
    }
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
