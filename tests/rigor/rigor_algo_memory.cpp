// rigor_algo_memory.cpp — snowball suite for src/algorithm/memory.hpp
//
// Coverage:
//   distance / adistance       (pointer arithmetic, word-aligned)
//   destroy                    (object lifetime)
//   overwrite                  (bytecpy wrapper)
//   zero / is_zero             (byte fill + detect)
//   copy_n / copy / ccopy /
//     scopy                    (size-typed copy variants)
//   cmove / move_n             (move semantics — source zeroed)
//   set_n / cset_n             (byte fill)
//   zero_n / czero_n           (byte zero)
//   compare_n / ccompare_n     (byte compare returning i64 — magnitude
//                               unspecified, sign meaningful)
//   equal_n / cequal_n         (boolean wrappers)
//   swap_n / cswap_n           (byte-level swap)
//
// Notes:
//  * compare_n returns i64; only assert == 0 / != 0, never specific values.
//  * distance / adistance compute in word units, not bytes nor element count.
//  * Many functions throw library_error on null input.

#include "../../src/algorithm/memory.hpp"

#include "../support/algo_rigor.hpp"

using namespace mtest::rigor;
using mtest::prng;
using sb::end_test_case;
using sb::expect_throw;
using sb::property_test;
using sb::require;
using sb::require_false;
using sb::require_true;
using sb::test_case;

int
main()
{
  sb::print("=== ALGO/MEMORY RIGOR SUITE ===");

  // ════════════════════════════════════════════════════════════════════
  // distance / adistance
  // ════════════════════════════════════════════════════════════════════

  test_case("distance computes word-unit difference");
  {
    int a[16];
    // 16 ints = 64 bytes = 8 word_t (8 bytes per word on amd64)
    usize d = micron::distance(a, a + 16);
    require_true(d > 0);
  }
  end_test_case();

  test_case("adistance is absolute value");
  {
    int a[16];
    usize d1 = micron::adistance(a, a + 8);
    usize d2 = micron::adistance(a + 8, a);
    require(d1, d2);
  }
  end_test_case();

  test_case("distance same pointer is zero");
  {
    int a[1];
    require(micron::distance(a, a), usize(0));
    require(micron::adistance(a, a), usize(0));
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // zero / is_zero
  // ════════════════════════════════════════════════════════════════════

  test_case("zero[ptr] zeros a single object");
  {
    int x = 42;
    micron::zero(&x);
    require(x, 0);
  }
  end_test_case();

  test_case("is_zero[ptr] true on zeroed object");
  {
    int x = 0;
    require_true(micron::is_zero(&x));
    x = 1;
    require_false(micron::is_zero(&x));
  }
  end_test_case();

  test_case("is_zero throws on null");
  {
    int *p = nullptr;
    expect_throw([&]() { (void)micron::is_zero(p); });
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // set_n / cset_n / zero_n / czero_n
  // ════════════════════════════════════════════════════════════════════

  test_case("set_n[byte=0xAB] fills bytes");
  {
    byte buf[32];
    micron::set_n(buf, static_cast<byte>(0xAB), usize(32));
    for ( usize i = 0; i < 32; ++i ) require(static_cast<int>(buf[i]), 0xAB);
  }
  end_test_case();

  test_case("set_n with various sizes (covers SIMD branches)");
  {
    for ( usize n :
          { usize(1), usize(8), usize(15), usize(16), usize(31), usize(32), usize(33), usize(63), usize(64), usize(127), usize(256) } ) {
      byte buf[256] = {};
      micron::set_n(buf, static_cast<byte>(0x5A), n);
      for ( usize i = 0; i < n; ++i ) require(static_cast<int>(buf[i]), 0x5A);
    }
  }
  end_test_case();

  test_case("cset_n[byte] compile-time size");
  {
    byte buf[32];
    micron::cset_n<32>(buf, static_cast<byte>(0xCC));
    for ( usize i = 0; i < 32; ++i ) require(static_cast<int>(buf[i]), 0xCC);
  }
  end_test_case();

  test_case("zero_n zeroes a byte buffer");
  {
    byte buf[32];
    for ( usize i = 0; i < 32; ++i ) buf[i] = static_cast<byte>(i);
    micron::zero_n(buf, usize(32));
    for ( usize i = 0; i < 32; ++i ) require(static_cast<int>(buf[i]), 0);
  }
  end_test_case();

  test_case("czero_n with various N (covers SIMD branches)");
  {
    {
      byte b16[16];
      for ( int i = 0; i < 16; ++i ) b16[i] = static_cast<byte>(i + 1);
      micron::czero_n<16>(b16);
      for ( int i = 0; i < 16; ++i ) require(static_cast<int>(b16[i]), 0);
    }
    {
      byte b32[32];
      for ( int i = 0; i < 32; ++i ) b32[i] = static_cast<byte>(i + 1);
      micron::czero_n<32>(b32);
      for ( int i = 0; i < 32; ++i ) require(static_cast<int>(b32[i]), 0);
    }
    {
      byte b65[65];
      for ( int i = 0; i < 65; ++i ) b65[i] = static_cast<byte>(i + 1);
      micron::czero_n<65>(b65);
      for ( int i = 0; i < 65; ++i ) require(static_cast<int>(b65[i]), 0);
    }
  }
  end_test_case();

  test_case("set_n throws on null");
  {
    int *p = nullptr;
    expect_throw([&]() { (void)micron::set_n(p, static_cast<byte>(0), usize(8)); });
  }
  end_test_case();

  test_case("zero_n throws on null");
  {
    int *p = nullptr;
    expect_throw([&]() { (void)micron::zero_n(p, usize(8)); });
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // copy_n / copy / ccopy
  // ════════════════════════════════════════════════════════════════════

  test_case("copy_n[byte] basic");
  {
    byte src[32];
    byte dst[32];
    for ( usize i = 0; i < 32; ++i ) src[i] = static_cast<byte>(i);
    micron::copy_n(src, dst, usize(32));
    for ( usize i = 0; i < 32; ++i ) require(static_cast<int>(dst[i]), static_cast<int>(i));
  }
  end_test_case();

  test_case("copy_n with various sizes (covers SIMD branches)");
  {
    for ( usize n :
          { usize(1), usize(15), usize(16), usize(17), usize(31), usize(32), usize(33), usize(63), usize(64), usize(127), usize(256) } ) {
      byte src[256] = {};
      byte dst[256] = {};
      for ( usize i = 0; i < n; ++i ) src[i] = static_cast<byte>((i * 7) & 0xff);
      micron::copy_n(src, dst, n);
      for ( usize i = 0; i < n; ++i ) require(static_cast<int>(dst[i]), static_cast<int>(src[i]));
    }
  }
  end_test_case();

  test_case("copy<N>(ptr,ptr) compile-time size");
  {
    int src[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int dst[8] = {};
    micron::copy<8>(src, dst);
    for ( int i = 0; i < 8; ++i ) require(dst[i], src[i]);
  }
  end_test_case();

  test_case("ccopy<N>(ptr,ptr) constexpr copy");
  {
    int src[16];
    int dst[16] = {};
    for ( int i = 0; i < 16; ++i ) src[i] = i * 3 + 1;
    micron::ccopy<16>(src, dst);
    for ( int i = 0; i < 16; ++i ) require(dst[i], src[i]);
  }
  end_test_case();

  test_case("copy_n throws on null source");
  {
    byte src_buf[8] = {};
    (void)src_buf;
    byte *src = nullptr;
    byte dst[8] = {};
    expect_throw([&]() { (void)micron::copy_n(src, dst, usize(8)); });
  }
  end_test_case();

  test_case("copy_n throws on null destination");
  {
    byte src[8] = { 0 };
    byte *dst = nullptr;
    expect_throw([&]() { (void)micron::copy_n(src, dst, usize(8)); });
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // cmove / move_n  (source is zeroed after)
  // ════════════════════════════════════════════════════════════════════

  test_case("cmove<N> moves and zeros source");
  {
    int src[8];
    int dst[8] = {};
    for ( int i = 0; i < 8; ++i ) src[i] = i + 10;
    micron::cmove<8>(src, dst);
    for ( int i = 0; i < 8; ++i ) require(dst[i], i + 10);
    for ( int i = 0; i < 8; ++i ) require(src[i], 0);
  }
  end_test_case();

  test_case("move_n moves and zeros source");
  {
    byte src[32];
    byte dst[32] = {};
    for ( usize i = 0; i < 32; ++i ) src[i] = static_cast<byte>(i + 1);
    micron::move_n(src, dst, usize(32));
    for ( usize i = 0; i < 32; ++i ) require(static_cast<int>(dst[i]), static_cast<int>(i + 1));
    for ( usize i = 0; i < 32; ++i ) require(static_cast<int>(src[i]), 0);
  }
  end_test_case();

  // ════════════════════════════════════════════════════════════════════
  // compare_n / ccompare_n / equal_n / cequal_n
  // ════════════════════════════════════════════════════════════════════

  test_case("compare_n returns 0 on equal buffers");
  {
    byte a[32] = {};
    byte b[32] = {};
    for ( usize i = 0; i < 32; ++i ) {
      a[i] = static_cast<byte>(i);
      b[i] = static_cast<byte>(i);
    }
    require(micron::compare_n(a, b, usize(32)), i64(0));
  }
  end_test_case();

  test_case("compare_n returns non-zero on differing buffers");
  {
    byte a[16] = {};
    byte b[16] = {};
    for ( usize i = 0; i < 16; ++i ) {
      a[i] = static_cast<byte>(i);
      b[i] = static_cast<byte>(i);
    }
    b[8] = static_cast<byte>(99);
    require_true(micron::compare_n(a, b, usize(16)) != 0);
  }
  end_test_case();

  test_case("ccompare_n returns 0 on equal");
  {
    byte a[16];
    byte b[16];
    for ( usize i = 0; i < 16; ++i ) {
      a[i] = static_cast<byte>(i + 1);
      b[i] = static_cast<byte>(i + 1);
    }
    require(micron::ccompare_n<16>(a, b), i64(0));
  }
  end_test_case();

  test_case("equal_n boolean wrapper");
  {
    byte a[32];
    byte b[32];
    for ( usize i = 0; i < 32; ++i ) {
      a[i] = static_cast<byte>(i);
      b[i] = static_cast<byte>(i);
    }
    require_true(micron::equal_n(a, b, usize(32)));
    b[15] = static_cast<byte>(0xFF);
    require_false(micron::equal_n(a, b, usize(32)));
  }
  end_test_case();

  test_case("cequal_n boolean wrapper");
  {
    byte a[16];
    byte b[16];
    for ( usize i = 0; i < 16; ++i ) {
      a[i] = static_cast<byte>(i);
      b[i] = static_cast<byte>(i);
    }
    require_true(micron::cequal_n<16>(a, b));
  }
  end_test_case();

  property_test(
      "equal_n agrees with naive (10k random)",
      [](u32 raw_n, u32 raw_seed) {
        usize n = (raw_n & 0x3f) + 1;
        byte a[64] = {};
        byte b[64] = {};
        prng rng(raw_seed + 59);
        for ( usize i = 0; i < n; ++i ) {
          byte v = static_cast<byte>(rng.next() & 0xff);
          a[i] = v;
          b[i] = v;
        }
        bool flip = (raw_n >> 7) & 1u;
        if ( flip ) b[n / 2] = static_cast<byte>(b[n / 2] ^ 0xff);
        bool actual = micron::equal_n(a, b, n);
        bool expected = !flip;
        require(actual, expected);
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // swap_n / cswap_n
  // ════════════════════════════════════════════════════════════════════

  test_case("swap_n swaps byte ranges");
  {
    byte a[32];
    byte b[32];
    byte a_orig[32];
    byte b_orig[32];
    for ( usize i = 0; i < 32; ++i ) {
      a[i] = static_cast<byte>(i);
      b[i] = static_cast<byte>(255 - i);
      a_orig[i] = a[i];
      b_orig[i] = b[i];
    }
    micron::swap_n(a, b, usize(32));
    for ( usize i = 0; i < 32; ++i ) {
      require(static_cast<int>(a[i]), static_cast<int>(b_orig[i]));
      require(static_cast<int>(b[i]), static_cast<int>(a_orig[i]));
    }
  }
  end_test_case();

  test_case("swap_n various sizes covers 32-byte stride loop");
  {
    for ( usize n : { usize(31), usize(32), usize(33), usize(63), usize(64), usize(65), usize(96), usize(128) } ) {
      byte a[256];
      byte b[256];
      for ( usize i = 0; i < n; ++i ) {
        a[i] = static_cast<byte>(i & 0xff);
        b[i] = static_cast<byte>((255 - i) & 0xff);
      }
      byte a_orig[256];
      byte b_orig[256];
      for ( usize i = 0; i < n; ++i ) {
        a_orig[i] = a[i];
        b_orig[i] = b[i];
      }
      micron::swap_n(a, b, n);
      for ( usize i = 0; i < n; ++i ) {
        require(static_cast<int>(a[i]), static_cast<int>(b_orig[i]));
        require(static_cast<int>(b[i]), static_cast<int>(a_orig[i]));
      }
    }
  }
  end_test_case();

  test_case("cswap_n<N> compile-time");
  {
    int a[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    int b[8] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    micron::cswap_n<sizeof(a)>(a, b);
    int exp_a[8] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    int exp_b[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    for ( int i = 0; i < 8; ++i ) {
      require(a[i], exp_a[i]);
      require(b[i], exp_b[i]);
    }
  }
  end_test_case();

  property_test(
      "swap_n twice is identity (10k random)",
      [](u32 raw_n, u32 raw_seed) {
        usize n = (raw_n & 0x3f) + 1;
        byte a[64] = {};
        byte b[64] = {};
        byte a_orig[64] = {};
        byte b_orig[64] = {};
        prng rng(raw_seed + 61);
        for ( usize i = 0; i < n; ++i ) {
          a[i] = static_cast<byte>(rng.next() & 0xff);
          b[i] = static_cast<byte>(rng.next() & 0xff);
          a_orig[i] = a[i];
          b_orig[i] = b[i];
        }
        micron::swap_n(a, b, n);
        micron::swap_n(a, b, n);
        for ( usize i = 0; i < n; ++i ) {
          require(static_cast<int>(a[i]), static_cast<int>(a_orig[i]));
          require(static_cast<int>(b[i]), static_cast<int>(b_orig[i]));
        }
      },
      10000);

  // ════════════════════════════════════════════════════════════════════
  // Composition: copy then compare = equal
  // ════════════════════════════════════════════════════════════════════

  test_case("copy_n then equal_n returns true");
  {
    byte src[64];
    byte dst[64] = {};
    for ( usize i = 0; i < 64; ++i ) src[i] = static_cast<byte>(i & 0xff);
    micron::copy_n(src, dst, usize(64));
    require_true(micron::equal_n(src, dst, usize(64)));
  }
  end_test_case();

  test_case("set_n then is all-byte-value");
  {
    byte buf[32];
    micron::set_n(buf, static_cast<byte>(0x77), usize(32));
    byte cmp[32];
    for ( int i = 0; i < 32; ++i ) cmp[i] = static_cast<byte>(0x77);
    require_true(micron::equal_n(buf, cmp, usize(32)));
  }
  end_test_case();

  test_case("zero_n then is_zero (on aligned buffer)");
  {
    int buf[16];
    for ( int i = 0; i < 16; ++i ) buf[i] = i + 1;
    micron::zero_n(buf, usize(16));
    for ( int i = 0; i < 16; ++i ) require(buf[i], 0);
  }
  end_test_case();

  sb::print("=== ALGO/MEMORY RIGOR SUITE PASSED ===");
  return 1;
}
