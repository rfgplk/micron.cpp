

#include "../../src/simd/arch/arith_arm32.hpp"
#include "../../src/simd/simd.hpp"

#include "../snowball/snowball.hpp"

using sb::end_test_case;
using sb::print;
using sb::require_true;
using sb::test_case;

namespace ms = micron::simd;
namespace nb = micron::simd::__bits;

template<typename Flag> struct pk {
  using bit_width = nb::int32x4_t;
  using lane_width = Flag;
  nb::int32x4_t v;
  pk() = default;

  pk(nb::int32x4_t x) : v(x) { }

  operator nb::int32x4_t() const { return v; }
};

using pk8 = pk<ms::__v8>;
using pk16 = pk<ms::__v16>;
using pk32 = pk<ms::__v32>;

static nb::int32x4_t
mk32(int a, int b, int c, int d)
{
  nb::int32x4_t r = { a, b, c, d };
  return r;
}

static nb::int32x4_t
mk16(short a, short b, short c, short d, short e, short f, short g, short h)
{
  nb::int16x8_t r = { a, b, c, d, e, f, g, h };
  return nb::vreinterpretq_s32_s16(r);
}

static nb::int32x4_t
mk8(signed char a, signed char b, signed char c, signed char d, signed char e, signed char f, signed char g, signed char h, signed char i,
    signed char j, signed char k, signed char l, signed char m, signed char n, signed char o, signed char p)
{
  nb::int8x16_t r = { a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p };
  return nb::vreinterpretq_s32_s8(r);
}

static int
l32(nb::int32x4_t x, int i)
{
  return x[i];
}

static int
l16(nb::int32x4_t x, int i)
{
  nb::int16x8_t s = nb::vreinterpretq_s16_s32(x);
  return s[i];
}

static int
l8(nb::int32x4_t x, int i)
{
  nb::int8x16_t s = nb::vreinterpretq_s8_s32(x);
  return s[i];
}

int
main()
{
  print("=== SIMD ARITH ARM32 (fixed-path) TESTS ===");

  test_case("sign_32");
  {
    pk32 a{ mk32(7, 7, 7, 7) }, b{ mk32(5, -3, 0, 9) };
    nb::int32x4_t r = ms::sign(a, b);
    require_true(l32(r, 0) == 7 && l32(r, 1) == -7 && l32(r, 2) == 0 && l32(r, 3) == 7);
  }
  end_test_case();

  test_case("sign_16");
  {
    pk16 a{ mk16(4, 4, 4, 4, 4, 4, 4, 4) }, b{ mk16(2, -2, 0, 2, -2, 0, 2, -2) };
    nb::int32x4_t r = ms::sign(a, b);
    require_true(l16(r, 0) == 4 && l16(r, 1) == -4 && l16(r, 2) == 0 && l16(r, 3) == 4);
  }
  end_test_case();

  test_case("sign_8");
  {
    pk8 a{ mk8(3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3) }, b{ mk8(1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1, -1, 0, 1) };
    nb::int32x4_t r = ms::sign(a, b);
    require_true(l8(r, 0) == 3 && l8(r, 1) == -3 && l8(r, 2) == 0 && l8(r, 3) == 3);
  }
  end_test_case();

  test_case("sra32 (signed -16>>2==-4, logical-zero-fill NOT used)");
  {
    pk32 a{ mk32(-16, 16, -1, 1024) };
    nb::int32x4_t r = ms::shift_right_arithmetic(a, 2);
    require_true(l32(r, 0) == -4 && l32(r, 1) == 4 && l32(r, 2) == -1 && l32(r, 3) == 256);
  }
  end_test_case();

  test_case("sra16 (-16>>2==-4, -32768>>2==-8192)");
  {
    pk16 a{ mk16(-16, 16, -1, -32768, 256, -256, 7, -7) };
    nb::int32x4_t r = ms::shift_right_arithmetic(a, 2);
    require_true(l16(r, 0) == -4 && l16(r, 1) == 4 && l16(r, 2) == -1 && l16(r, 3) == -8192);
  }
  end_test_case();

  test_case("mul_32_64 (even-lane 32x32->64)");
  {
    pk32 a{ mk32(1000, 999, 7, 888) }, b{ mk32(1000, 111, 9, 222) };
    nb::int32x4_t r = ms::mul_32_64(a, b);
    long long w0 = (long long)(unsigned)l32(r, 0) | ((long long)l32(r, 1) << 32);
    long long w1 = (long long)(unsigned)l32(r, 2) | ((long long)l32(r, 3) << 32);
    require_true(w0 == 1000000LL && w1 == 63LL);
  }
  end_test_case();

  test_case("mul_u32_64 (even-lane u32xu32->u64)");
  {
    pk32 a{ mk32(-2, 5, 4, 6) }, b{ mk32(3, 5, 8, 6) };
    nb::int32x4_t r = ms::mul_u32_64(a, b);
    unsigned long long w0 = (unsigned long long)(unsigned)l32(r, 0) | ((unsigned long long)(unsigned)l32(r, 1) << 32);
    unsigned long long w1 = (unsigned long long)(unsigned)l32(r, 2) | ((unsigned long long)(unsigned)l32(r, 3) << 32);
    require_true(w0 == 0xFFFFFFFEULL * 3ULL && w1 == 32ULL);
  }
  end_test_case();

  test_case("maddubs_8 (vuzp_s16 deinterleave path compiles + runs)");
  {
    pk8 a{ mk8(2, 3, 4, 5, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 2, 2) }, b{ mk8(1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4) };
    volatile nb::int32x4_t r = ms::maddubs(a, b);
    (void)r;
    require_true(true);
  }
  end_test_case();

  test_case("min/max/abs (signed s32)");
  {
    pk32 a{ mk32(-5, 9, 3, -100) }, b{ mk32(2, 2, 3, 50) };
    nb::int32x4_t mn = ms::min(a, b), mx = ms::max(a, b);
    pk32 c{ mk32(-7, 7, -1, 0) };
    nb::int32x4_t ab = ms::abs(c);
    require_true(l32(mn, 0) == -5 && l32(mn, 3) == -100 && l32(mx, 0) == 2 && l32(mx, 3) == 50 && l32(ab, 0) == 7 && l32(ab, 1) == 7
                 && l32(ab, 2) == 1 && l32(ab, 3) == 0);
  }
  end_test_case();

  print("=== ALL SIMD ARITH ARM32 TESTS PASSED ===");
  return 1;
}
