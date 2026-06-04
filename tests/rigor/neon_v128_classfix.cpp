// neon_v128_classfix.cpp

#include "../../src/bits/__arch.hpp"

#if defined(__micron_arch_arm64) || defined(__micron_arch_arm32)

#include "../../src/simd/simd.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace ms = micron::simd;

static constexpr int MASK_ALL_SI = 0xF;
static constexpr int MASK_ALL_F64 = 0x3;
static constexpr int MASK_NONE = 0x0;

int
main()
{
  print("=== NEON v128 CLASS-FIX TESTS ===");

  test_case("compare i8 lanes (>= <= < > == !=)");
  {

    ms::v8 a, b;
    {
      signed char ba[16], bb[16];
      for ( int i = 0; i < 16; i++ ) {
        ba[i] = (signed char)(i - 8);
        bb[i] = (signed char)(i - 8 + 1);
      }
      a.uload(ba);
      b.uload(bb);
    }
    require_true((a < b) == MASK_ALL_SI);
    require_true((a <= b) == MASK_ALL_SI);
    require_true((b > a) == MASK_ALL_SI);
    require_true((b >= a) == MASK_ALL_SI);
    require_true((a == a) == MASK_ALL_SI);
    require_true((a != b) == MASK_ALL_SI);
    require_true((a > b) == MASK_NONE);
    require_true((a >= b) == MASK_NONE);
  }
  end_test_case();

  test_case("compare i16 lanes (>= <= < > == !=)");
  {
    ms::v16 a, b;
    {
      short ba[8], bb[8];
      for ( int i = 0; i < 8; i++ ) {
        ba[i] = (short)(i * 1000 - 4000);
        bb[i] = (short)(i * 1000 - 4000 + 1);
      }
      a.uload(ba);
      b.uload(bb);
    }
    require_true((a < b) == MASK_ALL_SI);
    require_true((a <= b) == MASK_ALL_SI);
    require_true((b > a) == MASK_ALL_SI);
    require_true((b >= a) == MASK_ALL_SI);
    require_true((a == a) == MASK_ALL_SI);
    require_true((a != b) == MASK_ALL_SI);
  }
  end_test_case();

  test_case("compare i32 lanes (>= <= < > == !=)");
  {
    ms::v32 a, b;
    {
      int ba[4] = { -100, -1, 0, 2147483646 };
      int bb[4] = { -99, 0, 1, 2147483647 };
      a.uload(ba);
      b.uload(bb);
    }
    require_true((a < b) == MASK_ALL_SI);
    require_true((a <= b) == MASK_ALL_SI);
    require_true((b > a) == MASK_ALL_SI);
    require_true((b >= a) == MASK_ALL_SI);
    require_true((a == a) == MASK_ALL_SI);
    require_true((a != b) == MASK_ALL_SI);
  }
  end_test_case();

  test_case("compare i64 lanes (>= <= < > == != — uses vcltq_s64 et al)");
  {
    ms::v64 a, b;
    {
      i64 ba[2] = { -5000000000LL, 1LL };
      i64 bb[2] = { -4999999999LL, 2LL };
      a.uload(ba);
      b.uload(bb);
    }

    require_true((a < b) == MASK_ALL_SI);
    require_true((a <= b) == MASK_ALL_SI);
    require_true((b > a) == MASK_ALL_SI);
    require_true((b >= a) == MASK_ALL_SI);
    require_true((a == a) == MASK_ALL_SI);
    require_true((a != b) == MASK_ALL_SI);
    require_true((a > b) == MASK_NONE);
  }
  end_test_case();

  test_case("compare f32 / (arm64) f64");
  {
    ms::vfloat a, b;
    {
      float ba[4] = { -1.5f, 0.0f, 3.25f, 100.0f };
      float bb[4] = { -1.0f, 0.5f, 3.50f, 100.5f };
      a.uload(ba);
      b.uload(bb);
    }
    require_true((a < b) == MASK_ALL_SI);
    require_true((a <= b) == MASK_ALL_SI);
    require_true((b > a) == MASK_ALL_SI);
    require_true((b >= a) == MASK_ALL_SI);
    require_true((a == a) == MASK_ALL_SI);
    require_true((a != b) == MASK_ALL_SI);
#if !defined(__micron_arch_arm32)
    ms::vdouble da, db;
    {
      double ba[2] = { -1.5, 3.25 };
      double bb[2] = { -1.0, 3.50 };
      da.uload(ba);
      db.uload(bb);
    }
    require_true((da < db) == MASK_ALL_F64);
    require_true((da <= db) == MASK_ALL_F64);
    require_true((db > da) == MASK_ALL_F64);
    require_true((db >= da) == MASK_ALL_F64);
    require_true((da == da) == MASK_ALL_F64);
    require_true((da != db) == MASK_ALL_F64);
#endif
  }
  end_test_case();

  test_case("signed >> arithmetic: i32 -256 >> 2 == -64");
  {
    ms::v32 v;
    {
      int b[4] = { -256, -8, 256, 7 };
      v.uload(b);
    }
    int out[4];
    (v >> 2).get(out);
    require_true(out[0] == (-256 >> 2));
    require_true(out[1] == (-8 >> 2));
    require_true(out[2] == (256 >> 2));
    require_true(out[3] == (7 >> 2));

    ms::v32 w;
    {
      int b[4] = { -256, -8, 256, 7 };
      w.uload(b);
    }
    w >>= 2;
    int out2[4];
    w.get(out2);
    require_true(out2[0] == -64 && out2[1] == -2 && out2[2] == 64 && out2[3] == 1);
  }
  end_test_case();

  test_case("signed >> arithmetic: i16 / i8 sign-extend");
  {
    ms::v16 v16;
    {
      short b[8] = { -256, -8, 256, 7, -32000, 100, -1, 1 };
      v16.uload(b);
    }
    short o16[8];
    (v16 >> 2).get(o16);
    for ( int i = 0; i < 8; i++ ) {
      short b[8] = { -256, -8, 256, 7, -32000, 100, -1, 1 };
      require_true(o16[i] == (short)(b[i] >> 2));
    }

    ms::v8 v8;
    {
      signed char b[16];
      for ( int i = 0; i < 16; i++ ) b[i] = (signed char)(i * 13 - 90);
      v8.uload(b);
    }
    signed char o8[16];
    (v8 >> 1).get(o8);
    for ( int i = 0; i < 16; i++ ) {
      signed char x = (signed char)(i * 13 - 90);
      require_true(o8[i] == (signed char)(x >> 1));
    }
  }
  end_test_case();

  test_case("signed >> arithmetic: i64 (was 0,0 before the vshlq fix)");
  {
    ms::v64 v;
    {
      i64 b[2] = { -1024LL, 4096LL };
      v.uload(b);
    }
    i64 out[2];
    (v >> 3).get(out);
    require_true(out[0] == (-1024LL >> 3));
    require_true(out[1] == (4096LL >> 3));
  }
  end_test_case();

  test_case("unsigned vshlq_u* logical right shift (the operator>> else-branch pattern)");
  {

    {
      unsigned int in[4] = { 0x80000000u, 0xFFFFFFFFu, 0x00000010u, 0x12345678u };
      uint32x4_t u = vld1q_u32(in);
      uint32x4_t r = vshlq_u32(u, vdupq_n_s32(-4));
      unsigned int out[4];
      vst1q_u32(out, r);
      for ( int i = 0; i < 4; i++ ) require_true(out[i] == (in[i] >> 4));
    }

    {
      unsigned char in[16];
      for ( int i = 0; i < 16; i++ ) in[i] = (unsigned char)(0x80 | i);
      uint8x16_t u = vld1q_u8(in);
      uint8x16_t r = vshlq_u8(u, vdupq_n_s8((signed char)-2));
      unsigned char out[16];
      vst1q_u8(out, r);
      for ( int i = 0; i < 16; i++ ) require_true(out[i] == (unsigned char)(in[i] >> 2));
    }

    {
      unsigned short in[8] = { 0x8000u, 0xFFFFu, 0x1234u, 0x0010u, 0xABCDu, 0x0001u, 0x7FFFu, 0x8001u };
      uint16x8_t u = vld1q_u16(in);
      uint16x8_t r = vshlq_u16(u, vdupq_n_s16((short)-3));
      unsigned short out[8];
      vst1q_u16(out, r);
      for ( int i = 0; i < 8; i++ ) require_true(out[i] == (unsigned short)(in[i] >> 3));
    }

    {
      u64 in[2] = { 0x8000000000000000ull, 0xFFFFFFFFFFFFFFFFull };
      uint64x2_t u = vld1q_u64(in);
      uint64x2_t r = vshlq_u64(u, vdupq_n_s64((i64)-4));
      u64 out[2];
      vst1q_u64(out, r);
      require_true(out[0] == (in[0] >> 4));
      require_true(out[1] == (in[1] >> 4));
    }
  }
  end_test_case();

  test_case("<< / <<= left shift (signed & unsigned lanes)");
  {
    ms::v32 v;
    {
      int b[4] = { 1, -1, 3, 0x40000000 };
      v.uload(b);
    }
    int out[4];
    (v << 2).get(out);
    require_true(out[0] == 4 && out[1] == -4 && out[2] == 12);
    v <<= 1;
    int out2[4];
    v.get(out2);
    require_true(out2[0] == 2 && out2[1] == -2 && out2[2] == 6);
  }
  end_test_case();

  test_case("i8 16-arg ctor: arg0 -> lane0 (natural order)");
  {
    ms::v8 v(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    signed char out[16];
    v.get(out);
    for ( int i = 0; i < 16; i++ ) require_true(out[i] == (signed char)i);

    require_true(v[0] == 0);
    require_true(v[15] == 15);
  }
  end_test_case();

  test_case("i16 8-arg ctor: arg0 -> lane0 (natural order)");
  {
    ms::v16 v(100, 200, 300, 400, 500, 600, 700, 800);
    short out[8];
    v.get(out);
    short exp[8] = { 100, 200, 300, 400, 500, 600, 700, 800 };
    for ( int i = 0; i < 8; i++ ) require_true(out[i] == exp[i]);
    require_true(v[0] == 100);
    require_true(v[7] == 800);
  }
  end_test_case();

  test_case("i32 / i64 / f32 ctors remain natural (regression guard)");
  {
    ms::v32 vi(11, 22, 33, 44);
    int oi[4];
    vi.get(oi);
    require_true(oi[0] == 11 && oi[1] == 22 && oi[2] == 33 && oi[3] == 44);

    ms::v64 vl(123456789012LL, -987654321098LL);
    i64 ol[2];
    vl.get(ol);
    require_true(ol[0] == 123456789012LL && ol[1] == -987654321098LL);

    ms::vfloat vf(1.0f, 2.0f, 3.0f, 4.0f);
    float of[4];
    vf.get(of);
    require_true(of[0] == 1.0f && of[1] == 2.0f && of[2] == 3.0f && of[3] == 4.0f);
  }
  end_test_case();

  test_case("operator=(v128&&) zeroes the moved-from source (i128)");
  {
    ms::v8 src(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    ms::v8 dst;
    dst = static_cast<ms::v8 &&>(src);

    signed char od[16];
    dst.get(od);
    for ( int i = 0; i < 16; i++ ) require_true(od[i] == (signed char)(i + 1));

    signed char os[16];
    src.get(os);
    for ( int i = 0; i < 16; i++ ) require_true(os[i] == 0);
  }
  end_test_case();

  test_case("operator=(v128&&) zeroes the moved-from source (f32)");
  {
    ms::vfloat src(9.0f, 8.0f, 7.0f, 6.0f);
    ms::vfloat dst;
    dst = static_cast<ms::vfloat &&>(src);
    float od[4];
    dst.get(od);
    require_true(od[0] == 9.0f && od[1] == 8.0f && od[2] == 7.0f && od[3] == 6.0f);
    float os[4];
    src.get(os);
    require_true(os[0] == 0.0f && os[1] == 0.0f && os[2] == 0.0f && os[3] == 0.0f);
  }
  end_test_case();

#if !defined(__micron_arch_arm32)
  test_case("operator=(v128&&) zeroes the moved-from source (f64, arm64)");
  {
    ms::vdouble src(3.5, -2.5);
    ms::vdouble dst;
    dst = static_cast<ms::vdouble &&>(src);
    double od[2];
    dst.get(od);
    require_true(od[0] == 3.5 && od[1] == -2.5);
    double os[2];
    src.get(os);
    require_true(os[0] == 0.0 && os[1] == 0.0);
  }
  end_test_case();
#endif

  print("=== ALL NEON v128 CLASS-FIX TESTS PASSED ===");
  return 1;
}

#else

int
main()
{
  return 1;
}

#endif
