//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
// Direct rigor coverage for the SIMD memory primitives in
// `simd/arch/memory_*.hpp` — the routines re-exported as
// mc::memcpy{128,256,512} and friends. These complement the
// scalar/template tests in memcpy.cpp by exercising the inline-asm
// block primitives in __bits/__asm_blocks_*.hpp directly, with
// guard zones to catch any over-write past the requested length.

#include "../../src/io/console.hpp"
#include "../../src/io/stdout.hpp"
#include "../../src/memory/memory.hpp"
#include "../../src/std.hpp"

#include "../snowball/snowball.hpp"

// ============================================================
//  Helpers
// ============================================================

static constexpr byte CANARY = 0xC3;

template<u64 N>
void
fill_seq(byte (&buf)[N])
{
  for ( u64 i = 0; i < N; i++ ) buf[i] = static_cast<byte>(i & 0xFF);
}

template<u64 N>
void
fill_value(byte (&buf)[N], byte v)
{
  for ( u64 i = 0; i < N; i++ ) buf[i] = v;
}

bool
buffers_equal(const byte *a, const byte *b, u64 n)
{
  for ( u64 i = 0; i < n; i++ )
    if ( a[i] != b[i] ) return false;
  return true;
}

bool
all_equal_byte(const byte *p, u64 n, byte v)
{
  for ( u64 i = 0; i < n; i++ )
    if ( p[i] != v ) return false;
  return true;
}

// Guarded buffer with leading & trailing canary zones.
template<u64 PAYLOAD, u64 ALIGN = 64> struct GBuf {
  alignas(ALIGN) byte data[64 + PAYLOAD + 64];

  GBuf() noexcept
  {
    for ( u64 i = 0; i < sizeof(data); i++ ) data[i] = CANARY;
  }

  byte *
  payload() noexcept
  {
    return data + 64;
  }

  bool
  guards_intact(u64 written) const noexcept
  {
    for ( u64 i = 0; i < 64; i++ )
      if ( data[i] != CANARY ) return false;
    for ( u64 i = 64 + written; i < sizeof(data); i++ )
      if ( data[i] != CANARY ) return false;
    return true;
  }
};

// ============================================================
//  main
// ============================================================

int
main(void)
{
  sb::print("=== SIMD MEMORY PRIMITIVES TESTS ===");

  // -------------------------------------------------------
  //  simd::memcpy128 — sweep sizes including non-multiples-of-16
  // -------------------------------------------------------
  sb::test_case("simd::memcpy128 - exhaustive size sweep [0, 257]");
  {
    alignas(64) byte src[260];
    fill_seq(src);
    for ( u64 sz = 0; sz <= 257; sz++ ) {
      GBuf<260> gb;
      mc::memcpy128(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::memcpy256 - exhaustive size sweep [0, 257]");
  {
    alignas(64) byte src[260];
    fill_seq(src);
    for ( u64 sz = 0; sz <= 257; sz++ ) {
      GBuf<260> gb;
      mc::memcpy256(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

#if defined(__micron_x86_avx512f)
  sb::test_case("simd::memcpy512 - sweep multiples and remainders");
  {
    alignas(64) byte src[520];
    fill_seq(src);
    for ( u64 sz : { (u64)0, (u64)1, (u64)63, (u64)64, (u64)65, (u64)127, (u64)128, (u64)129, (u64)511, (u64)512 } ) {
      GBuf<520> gb;
      mc::memcpy512(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();
#endif
#endif

  // -------------------------------------------------------
  //  Aligned variants — require 16/32/64-byte alignment.
  //  Test buffer must be aligned at the SIMD width.
  // -------------------------------------------------------
  sb::test_case("simd::amemcpy128 - aligned 16-byte multiples");
  {
    alignas(16) byte src[256];
    fill_seq(src);
    for ( u64 sz : { (u64)16, (u64)32, (u64)48, (u64)128, (u64)256 } ) {
      alignas(16) GBuf<256, 16> gb;
      mc::amemcpy128(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::amemcpy256 - aligned 32-byte multiples");
  {
    alignas(32) byte src[256];
    fill_seq(src);
    for ( u64 sz : { (u64)32, (u64)64, (u64)96, (u64)256 } ) {
      alignas(32) GBuf<256, 32> gb;
      mc::amemcpy256(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();
#endif

  // -------------------------------------------------------
  //  Non-temporal variants — same correctness checks. NT
  //  store visibility is enforced by the routine's internal
  //  fence (sfence on amd64, dmb st on arm); from the testing
  //  thread's perspective the writes must be visible after
  //  the call returns.
  // -------------------------------------------------------
  sb::test_case("simd::ntmemcpy128 - aligned, fence-visible");
  {
    alignas(16) byte src[1024];
    fill_seq(src);
    for ( u64 sz : { (u64)16, (u64)64, (u64)256, (u64)1024 } ) {
      alignas(16) GBuf<1024, 16> gb;
      mc::ntmemcpy128(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::ntmemcpy256 - aligned, fence-visible");
  {
    alignas(32) byte src[1024];
    fill_seq(src);
    for ( u64 sz : { (u64)32, (u64)64, (u64)256, (u64)1024 } ) {
      alignas(32) GBuf<1024, 32> gb;
      mc::ntmemcpy256(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();
#endif

  // -------------------------------------------------------
  //  simd::memset — sweep sizes, value patterns, alignment
  // -------------------------------------------------------
  sb::test_case("simd::memset128 - exhaustive size sweep [0, 257]");
  {
    for ( byte v : { static_cast<byte>(0x00), static_cast<byte>(0xFF), static_cast<byte>(0x5A), static_cast<byte>(0xA5) } ) {
      for ( u64 sz = 0; sz <= 257; sz++ ) {
        GBuf<260> gb;
        mc::memset128(gb.payload(), v, sz);
        sb::require(all_equal_byte(gb.payload(), sz, v));
        sb::require(gb.guards_intact(sz));
      }
    }
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::memset256 - exhaustive size sweep [0, 257]");
  {
    for ( u64 sz = 0; sz <= 257; sz++ ) {
      GBuf<260> gb;
      mc::memset256(gb.payload(), static_cast<byte>(0x42), sz);
      sb::require(all_equal_byte(gb.payload(), sz, static_cast<byte>(0x42)));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();
#endif

  sb::test_case("simd::amemset128 - aligned multiples");
  {
    for ( u64 sz : { (u64)16, (u64)32, (u64)64, (u64)128, (u64)1024 } ) {
      alignas(16) GBuf<1024, 16> gb;
      mc::amemset128(gb.payload(), static_cast<byte>(0xCC), sz);
      sb::require(all_equal_byte(gb.payload(), sz, static_cast<byte>(0xCC)));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

  sb::test_case("simd::ntmemset128 - aligned, fence-visible");
  {
    for ( u64 sz : { (u64)16, (u64)64, (u64)1024, (u64)4096 } ) {
      alignas(16) GBuf<4096, 16> gb;
      mc::ntmemset128(gb.payload(), static_cast<byte>(0xBE), sz);
      sb::require(all_equal_byte(gb.payload(), sz, static_cast<byte>(0xBE)));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

  // -------------------------------------------------------
  //  simd::memcmp — equal, first-byte-differs, last-byte-differs,
  //  middle-byte-differs cases at various sizes.
  // -------------------------------------------------------
  sb::test_case("simd::memcmp128 - equal returns 0");
  {
    alignas(16) byte a[256];
    alignas(16) byte b[256];
    for ( u64 sz : { (u64)1, (u64)15, (u64)16, (u64)17, (u64)64, (u64)100, (u64)256 } ) {
      fill_seq(a);
      fill_seq(b);
      sb::require(mc::memcmp128(a, b, sz) == 0);
    }
  }
  sb::end_test_case();

  sb::test_case("simd::memcmp128 - first byte differs");
  {
    alignas(16) byte a[256];
    alignas(16) byte b[256];
    fill_seq(a);
    fill_seq(b);
    a[0] = 0x10;
    b[0] = 0x20;
    for ( u64 sz : { (u64)1, (u64)16, (u64)32, (u64)256 } ) {
      i64 r = mc::memcmp128(a, b, sz);
      sb::require(r != 0);
      sb::require(r < 0);      // 0x10 < 0x20
    }
  }
  sb::end_test_case();

  sb::test_case("simd::memcmp128 - byte i differs (sweep i)");
  {
    alignas(16) byte a[64];
    alignas(16) byte b[64];
    for ( u64 i = 0; i < 64; i++ ) {
      fill_seq(a);
      fill_seq(b);
      a[i] = 0xFF;
      b[i] = 0x00;
      i64 r = mc::memcmp128(a, b, 64);
      sb::require(r > 0);      // 0xFF > 0x00
    }
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::memcmp256 - byte i differs (sweep i)");
  {
    alignas(32) byte a[128];
    alignas(32) byte b[128];
    for ( u64 i = 0; i < 128; i++ ) {
      fill_seq(a);
      fill_seq(b);
      a[i] = 0x80;
      b[i] = 0x40;
      i64 r = mc::memcmp256(a, b, 128);
      sb::require(r > 0);
    }
  }
  sb::end_test_case();

#if defined(__micron_x86_avx512f)
  sb::test_case("simd::memcmp512 - byte i differs (sweep i)");
  {
    alignas(64) byte a[256];
    alignas(64) byte b[256];
    for ( u64 i = 0; i < 256; i++ ) {
      fill_seq(a);
      fill_seq(b);
      a[i] = 0x33;
      b[i] = 0x99;
      i64 r = mc::memcmp512(a, b, 256);
      sb::require(r < 0);
    }
  }
  sb::end_test_case();
#endif
#endif

  // -------------------------------------------------------
  //  simd::memmove — both forward (dst < src or non-overlap)
  //  and backward (dst > src with overlap) paths.
  // -------------------------------------------------------
  sb::test_case("simd::memmove128 - non-overlapping copy");
  {
    alignas(16) byte src[256];
    alignas(16) byte dst[256];
    fill_seq(src);
    for ( u64 sz : { (u64)16, (u64)32, (u64)100, (u64)256 } ) {
      fill_value(dst, 0xEE);
      mc::memmove128(dst, src, sz);
      sb::require(buffers_equal(dst, src, sz));
    }
  }
  sb::end_test_case();

  sb::test_case("simd::memmove128 - overlap dst > src (backward path)");
  {
    alignas(16) byte buf[1024];
    alignas(16) byte gold[1024];
    for ( u64 i = 0; i < 1024; i++ ) gold[i] = static_cast<byte>(i & 0xFF);
    for ( u64 shift : { (u64)16, (u64)32, (u64)48, (u64)64 } ) {
      for ( u64 sz : { (u64)64, (u64)128, (u64)256 } ) {
        for ( u64 i = 0; i < 1024; i++ ) buf[i] = gold[i];
        mc::memmove128(buf + shift, buf, sz);
        // Expect: buf[shift + i] == gold[i] for i in [0, sz)
        bool ok = true;
        for ( u64 i = 0; i < sz; i++ ) {
          if ( buf[shift + i] != gold[i] ) {
            ok = false;
            break;
          }
        }
        sb::require(ok);
      }
    }
  }
  sb::end_test_case();

  sb::test_case("simd::memmove128 - overlap dst < src (forward path)");
  {
    alignas(16) byte buf[1024];
    alignas(16) byte gold[1024];
    for ( u64 i = 0; i < 1024; i++ ) gold[i] = static_cast<byte>(i & 0xFF);
    for ( u64 shift : { (u64)16, (u64)32, (u64)64 } ) {
      for ( u64 sz : { (u64)64, (u64)128, (u64)256 } ) {
        for ( u64 i = 0; i < 1024; i++ ) buf[i] = gold[i];
        mc::memmove128(buf, buf + shift, sz);
        // Expect: buf[i] == gold[shift + i] for i in [0, sz)
        bool ok = true;
        for ( u64 i = 0; i < sz; i++ ) {
          if ( buf[i] != gold[shift + i] ) {
            ok = false;
            break;
          }
        }
        sb::require(ok);
      }
    }
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::memmove256 - overlap both directions");
  {
    alignas(32) byte buf[1024];
    alignas(32) byte gold[1024];
    for ( u64 i = 0; i < 1024; i++ ) gold[i] = static_cast<byte>(i & 0xFF);
    for ( u64 shift : { (u64)32, (u64)64, (u64)96 } ) {
      for ( u64 sz : { (u64)64, (u64)128, (u64)256 } ) {
        // Backward
        for ( u64 i = 0; i < 1024; i++ ) buf[i] = gold[i];
        mc::memmove256(buf + shift, buf, sz);
        for ( u64 i = 0; i < sz; i++ ) sb::require(buf[shift + i] == gold[i]);
        // Forward
        for ( u64 i = 0; i < 1024; i++ ) buf[i] = gold[i];
        mc::memmove256(buf, buf + shift, sz);
        for ( u64 i = 0; i < sz; i++ ) sb::require(buf[i] == gold[shift + i]);
      }
    }
  }
  sb::end_test_case();
#endif

  // -------------------------------------------------------
  //  Property-based: random size & buffer content.
  //  Uses snowball's xorshift seed so failures are reproducible.
  // -------------------------------------------------------
  sb::test_case("simd::memcpy128 - random size + content (1000 trials)");
  {
    alignas(64) byte src[513];
    u64 seed = 0xc0ffeecafebabeULL;
    auto rnd = [&seed]() {
      seed ^= seed << 13;
      seed ^= seed >> 7;
      seed ^= seed << 17;
      return seed;
    };
    for ( u64 trial = 0; trial < 1000; trial++ ) {
      u64 sz = rnd() % 513;
      for ( u64 i = 0; i < sz; i++ ) src[i] = static_cast<byte>(rnd() & 0xFF);
      GBuf<513> gb;
      mc::memcpy128(gb.payload(), src, sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

  sb::test_case("simd::memset128 - random size + value (1000 trials)");
  {
    u64 seed = 0xdeadbeefULL;
    auto rnd = [&seed]() {
      seed ^= seed << 13;
      seed ^= seed >> 7;
      seed ^= seed << 17;
      return seed;
    };
    for ( u64 trial = 0; trial < 1000; trial++ ) {
      u64 sz = rnd() % 513;
      byte v = static_cast<byte>(rnd() & 0xFF);
      GBuf<513> gb;
      mc::memset128(gb.payload(), v, sz);
      sb::require(all_equal_byte(gb.payload(), sz, v));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

  // -------------------------------------------------------
  //  simd::memchr — sweep block boundaries, hit/miss, types
  // -------------------------------------------------------
  sb::test_case("simd::memchr128 - sweep boundaries [0, 49]");
  {
    alignas(16) byte buf[64];
    for ( u64 sz : { (u64)0, (u64)1, (u64)15, (u64)16, (u64)17, (u64)31, (u64)32, (u64)33, (u64)47, (u64)48, (u64)49 } ) {
      fill_value(buf, 0xAA);
      sb::require(mc::memchr128(buf, static_cast<byte>(0xBB), sz) == nullptr);
      if ( sz == 0 ) continue;
      // hit at first byte
      buf[0] = 0xBB;
      sb::require(mc::memchr128(buf, static_cast<byte>(0xBB), sz) == &buf[0]);
      // hit at last byte
      buf[0] = 0xAA;
      buf[sz - 1] = 0xBB;
      sb::require(mc::memchr128(buf, static_cast<byte>(0xBB), sz) == &buf[sz - 1]);
    }
  }
  sb::end_test_case();

  sb::test_case("simd::memchr128 - first-hit semantics with multiple matches");
  {
    alignas(16) byte buf[128];
    fill_value(buf, 0xAA);
    buf[5] = 0xCC;
    buf[40] = 0xCC;
    buf[120] = 0xCC;
    sb::require(mc::memchr128(buf, static_cast<byte>(0xCC), 128) == &buf[5]);
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::memchr256 - sweep boundaries");
  {
    alignas(32) byte buf[128];
    for ( u64 sz : { (u64)0, (u64)31, (u64)32, (u64)33, (u64)63, (u64)64, (u64)65, (u64)128 } ) {
      fill_value(buf, 0xAA);
      sb::require(mc::memchr256(buf, static_cast<byte>(0xBB), sz) == nullptr);
      if ( sz == 0 ) continue;
      buf[sz - 1] = 0xBB;
      sb::require(mc::memchr256(buf, static_cast<byte>(0xBB), sz) == &buf[sz - 1]);
    }
  }
  sb::end_test_case();
#endif

  // -------------------------------------------------------
  //  simd::memrchr — last-hit semantics
  // -------------------------------------------------------
  sb::test_case("simd::memrchr128 - last-hit semantics");
  {
    alignas(16) byte buf[128];
    fill_value(buf, 0xAA);
    buf[5] = 0xCC;
    buf[40] = 0xCC;
    buf[120] = 0xCC;
    sb::require(mc::memrchr128(buf, static_cast<byte>(0xCC), 128) == &buf[120]);
    // limit search before last occurrence
    sb::require(mc::memrchr128(buf, static_cast<byte>(0xCC), 100) == &buf[40]);
    sb::require(mc::memrchr128(buf, static_cast<byte>(0xCC), 30) == &buf[5]);
    sb::require(mc::memrchr128(buf, static_cast<byte>(0xCC), 5) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("simd::memrchr128 - sweep boundaries");
  {
    alignas(16) byte buf[64];
    for ( u64 sz : { (u64)1, (u64)15, (u64)16, (u64)17, (u64)31, (u64)32, (u64)33, (u64)47, (u64)48, (u64)49 } ) {
      fill_value(buf, 0xAA);
      sb::require(mc::memrchr128(buf, static_cast<byte>(0xBB), sz) == nullptr);
      buf[0] = 0xBB;
      buf[sz - 1] = 0xBB;
      sb::require(mc::memrchr128(buf, static_cast<byte>(0xBB), sz) == &buf[sz - 1]);
    }
  }
  sb::end_test_case();

  // -------------------------------------------------------
  //  simd::mempcpy — return value contract: dest + n
  // -------------------------------------------------------
  sb::test_case("simd::mempcpy128 - returns d + n; copies correctly");
  {
    alignas(64) byte src[260];
    fill_seq(src);
    for ( u64 sz : { (u64)0, (u64)1, (u64)15, (u64)16, (u64)17, (u64)31, (u64)32, (u64)33, (u64)64, (u64)65, (u64)256 } ) {
      GBuf<260> gb;
      byte *r = mc::mempcpy128(gb.payload(), src, sz);
      sb::require(r == gb.payload() + sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();

#if defined(__micron_arch_x86_any)
  sb::test_case("simd::mempcpy256 - returns d + n; copies correctly");
  {
    alignas(64) byte src[260];
    fill_seq(src);
    for ( u64 sz : { (u64)0, (u64)31, (u64)32, (u64)33, (u64)63, (u64)64, (u64)65, (u64)256 } ) {
      GBuf<260> gb;
      byte *r = mc::mempcpy256(gb.payload(), src, sz);
      sb::require(r == gb.payload() + sz);
      sb::require(buffers_equal(gb.payload(), src, sz));
      sb::require(gb.guards_intact(sz));
    }
  }
  sb::end_test_case();
#endif

  // -------------------------------------------------------
  //  simd::memmem — substring search edge cases + multi-match
  // -------------------------------------------------------
  sb::test_case("simd::memmem128 - empty needle returns haystack");
  {
    byte hay[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    byte nee[1] = { 0 };
    sb::require(mc::memmem128(hay, (u64)8, nee, (u64)0) == hay);
  }
  sb::end_test_case();

  sb::test_case("simd::memmem128 - needle longer than haystack returns nullptr");
  {
    byte hay[4] = { 1, 2, 3, 4 };
    byte nee[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    sb::require(mc::memmem128(hay, (u64)4, nee, (u64)8) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("simd::memmem128 - first-match semantics");
  {
    alignas(16) byte hay[256];
    fill_value(hay, 0xAA);
    hay[10] = 'A';
    hay[11] = 'B';
    hay[12] = 'C';
    hay[100] = 'A';
    hay[101] = 'B';
    hay[102] = 'C';
    byte nee[3] = { 'A', 'B', 'C' };
    sb::require(mc::memmem128(hay, (u64)256, nee, (u64)3) == &hay[10]);
  }
  sb::end_test_case();

  sb::test_case("simd::memmem128 - needle == haystack");
  {
    alignas(16) byte buf[64];
    fill_seq(buf);
    sb::require(mc::memmem128(buf, (u64)64, buf, (u64)64) == buf);
  }
  sb::end_test_case();

  sb::test_case("simd::memmem128 - needle not found");
  {
    alignas(16) byte hay[256];
    fill_value(hay, 0xAA);
    byte nee[3] = { 'X', 'Y', 'Z' };
    sb::require(mc::memmem128(hay, (u64)256, nee, (u64)3) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("simd::memmem128 - prefix-only false match");
  {
    alignas(16) byte hay[64];
    fill_value(hay, 0xAA);
    hay[20] = 'X';
    hay[21] = 'Y';
    hay[22] = 'A';      // prefix XY but third byte wrong
    byte nee[3] = { 'X', 'Y', 'Z' };
    sb::require(mc::memmem128(hay, (u64)64, nee, (u64)3) == nullptr);
  }
  sb::end_test_case();

  sb::test_case("simd::memmem128 - long needle (>16 bytes) cross-block");
  {
    alignas(16) byte hay[256];
    fill_value(hay, 0xAA);
    byte nee[20];
    for ( u64 i = 0; i < 20; i++ ) nee[i] = static_cast<byte>(0xB0 + i);
    // place needle starting at offset 50 (mid-block + spans 16-byte boundary)
    for ( u64 i = 0; i < 20; i++ ) hay[50 + i] = nee[i];
    sb::require(mc::memmem128(hay, (u64)256, nee, (u64)20) == &hay[50]);
  }
  sb::end_test_case();

  sb::print("=== ALL TESTS PASSED ===");
  return 1;
}
